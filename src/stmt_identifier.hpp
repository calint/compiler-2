#pragma once

#include <algorithm>
#include <format>
#include <ranges>
#include <string>

#include "compiler_exception.hpp"
#include "expr_any.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "unary_ops.hpp"

class stmt_identifier : public statement {
    std::vector<identifier_elem> elems_;
    token ws1_;
    token ws2_;
    std::string path_as_string_;

  public:
    stmt_identifier(toc& tc, tokenizer& tz, token tk, unary_ops uops = {})
        : statement{tz.next_whitespace_token(), std::move(uops)},
          path_as_string_{tk.name()} {

        token tk_prv{tk};
        // note: keep track of previous token for better compilation error
        //       message

        while (true) {
            // note: for better compilation error messages
            if (not tc.is_func(path_as_string_)) {
                // try to find the identifier
                const ident_info curr_ident_info{
                    tc.make_ident_info(tk, path_as_string_, false)};
            }

            // has array index?
            if (tz.is_next_char('[')) {
                // yes, read expression

                // note: array index expression always results into an
                //       integer, thus default type
                elems_.emplace_back(tk,
                                    std::make_unique<expr_any>(
                                        tc, tz, tc.get_type_default(), false),
                                    tz.next_whitespace_token(), true);

                if (not tz.is_next_char(']')) {
                    throw compiler_exception{
                        tz, "expected '[' to close array index expression"};
                }
            } else {
                // no array indexing
                elems_.emplace_back(tk, nullptr, tz.next_whitespace_token(),
                                    false);
            }

            if (tz.is_next_char('.')) {
                tk_prv = tk;
                tk = tz.next_token();
                path_as_string_.push_back('.');
                path_as_string_ += tk.name();
                continue;
            }

            ws2_ = tz.next_whitespace_token();

            if (tc.is_func(path_as_string_)) {
                //? todo: should get the type? not necessary right now but
                // for consistency
                break;
            }

            const ident_info ii{
                tc.make_ident_info(tk_prv, path_as_string_, false)};

            set_type(ii.type_ref);

            // done
            break;
        }
    }

    stmt_identifier() = default;
    stmt_identifier(const stmt_identifier&) = default;
    stmt_identifier(stmt_identifier&&) = default;
    auto operator=(const stmt_identifier&) -> stmt_identifier& = default;
    auto operator=(stmt_identifier&&) -> stmt_identifier& = default;

    ~stmt_identifier() override = default;

    [[nodiscard]] auto first_token() const -> const token& {
        return elems_.at(0).name_tk;
    }

    [[nodiscard]] auto identifier() const -> const std::string& override {
        return path_as_string_;
    }

    [[nodiscard]] auto elems() const -> const std::vector<identifier_elem>& {
        return elems_;
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        return std::ranges::any_of(elems_, [](const identifier_elem& e) {
            return e.has_array_index_expr;
        });
    }

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        int sep{0};
        for (const identifier_elem& e : elems_) {
            if (sep++) {
                os << '.';
            }
            e.name_tk.source_to(os);
            if (e.has_array_index_expr) {
                os << '[';
                e.array_index_expr->source_to(os);
                os << ']';
            }
            e.ws1.source_to(os);
        }
        ws2_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 const std::string& dst = "") const -> void override {

        tc.comment_source(*this, os, indent);

        const ident_info dst_info{tc.make_ident_info(tok(), dst, false)};

        // does identifier contain array indexing?
        if (not is_expression()) {
            // no, compile to 'dst_info'
            const ident_info src_info{
                tc.make_ident_info(tok(), identifier(), false)};

            tc.asm_cmd(tok(), os, indent, "mov", dst_info.id_nasm,
                       src_info.id_nasm);

            get_unary_ops().compile(tc, os, indent, dst_info.id_nasm);

            return;
        }

        // identifier contains array indexing
        // calculate effective address to built-in type

        std::vector<std::string> allocated_registers;

        const std::string effective_address{
            stmt_identifier::compile_effective_address(
                tok(), tc, os, indent, elems(), allocated_registers)};

        const ident_info src_info{
            tc.make_ident_info(tok(), identifier(), false)};

        const std::string& size_specifier{
            type::get_size_specifier(tok(), src_info.type_ref.size())};

        tc.asm_cmd(tok(), os, indent, "mov", dst_info.id_nasm,
                   std::format("{} [{}]", size_specifier, effective_address));

        get_unary_ops().compile(tc, os, indent, dst_info.id_nasm);

        for (const std::string& reg :
             allocated_registers | std::ranges::views::reverse) {
            tc.free_scratch_register(os, indent, reg);
        }
    }

    static auto compile_effective_address(
        const token& src_loc_tk, toc& tc, std::ostream& os, size_t indent,
        const std::vector<identifier_elem>& elems,
        std::vector<std::string>& allocated_registers) -> std::string {

        // initialize base element information
        std::string path{elems.front().name_tk.name()};
        const ident_info base_info{tc.make_ident_info(src_loc_tk, path, false)};

        // start with offset register being the default 'rsp'
        std::string reg_offset{"rsp"};

        // the accumulated offset to compensate for type members displacement
        // from base address on stack of instance
        int32_t accum_offset{};
        const size_t elems_size{elems.size()};

        for (size_t i{}; i < elems_size; ++i) {
            // get current element information
            const identifier_elem& curr_elem{elems[i]};
            const ident_info curr_info{
                tc.make_ident_info(src_loc_tk, path, false)};
            const size_t type_size{curr_info.type_ref.size()};
            const bool is_last{i == elems_size - 1};

            if (curr_elem.has_array_index_expr) {
                // is encodable in instruction of type:
                // [base + index * size + offset]
                const bool is_encodable{type_size == 1 or type_size == 2 or
                                        type_size == 4 or type_size == 8};

                // special case: last element with encodable size
                if (is_last and is_encodable) {
                    // calculate offset in array and store in 'reg_idx'
                    const std::string& reg_idx{
                        tc.alloc_scratch_register(src_loc_tk, os, indent)};
                    allocated_registers.push_back(reg_idx);

                    curr_elem.array_index_expr->compile(tc, os, indent,
                                                        reg_idx);

                    // is it offset from base register 'rsp' are has it been
                    // changed to dedicated register for offset calculation?
                    const int32_t offset{
                        (reg_offset == "rsp")
                            ? -(base_info.stack_ix + accum_offset)
                            : accum_offset};
                    const char op{(reg_offset == "rsp") ? '-' : '+'};

                    if (type_size == 1) {
                        return std::format("{} + {} {} {}", reg_offset, reg_idx,
                                           op, offset);
                    }
                    return std::format("{} + {} * {} {} {}", reg_offset,
                                       reg_idx, type_size, op, offset);
                }

                // convert 'rsp' to dedicated register to calculate offset of
                // target
                if (reg_offset == "rsp") {
                    reg_offset =
                        tc.alloc_scratch_register(src_loc_tk, os, indent);

                    allocated_registers.push_back(reg_offset);

                    tc.asm_cmd(src_loc_tk, os, indent, "lea", reg_offset,
                               std::format("[rsp - {}]", -base_info.stack_ix));
                }

                // compile and scale the array index
                const std::string& reg_idx{
                    tc.alloc_scratch_register(src_loc_tk, os, indent)};

                curr_elem.array_index_expr->compile(tc, os, indent, reg_idx);

                if (type_size > 1) {
                    // check whether it is possible to shift left instead of
                    // multiplication
                    if (std::optional<int> shl{get_shift_amount(type_size)};
                        shl) {
                        tc.asm_cmd(src_loc_tk, os, indent, "shl", reg_idx,
                                   std::format("{}", *shl));
                    } else {
                        tc.asm_cmd(src_loc_tk, os, indent, "imul", reg_idx,
                                   std::format("{}", type_size));
                    }
                }

                // add the index offset to the base register
                tc.asm_cmd(src_loc_tk, os, indent, "add", reg_offset, reg_idx);
                tc.free_scratch_register(os, indent, reg_idx);

                if (is_last) {
                    // last element was an array index

                    // apply any accumulated offset
                    if (accum_offset != 0) {
                        tc.asm_cmd(src_loc_tk, os, indent, "add", reg_offset,
                                   std::format("{}", accum_offset));
                    }

                    // note: constructing new string to avoid clang warning "Not
                    //       eliding copy on return clang (-Wnrvo)""
                    return std::string{reg_offset};
                }
            }

            // accumulate field offsets for nested types
            if (i + 1 < elems.size()) {
                const identifier_elem& next_elem{elems[i + 1]};
                accum_offset +=
                    static_cast<int32_t>(toc::get_field_offset_in_type(
                        src_loc_tk, curr_info.type_ref,
                        next_elem.name_tk.name()));

                // continue in the path of identifiers
                path.push_back('.');
                path += next_elem.name_tk.name();
            }
        }

        // last element was not an array index

        // return an expression, possibly with format [base + index * scale +
        // offset] that is the target address

        if (reg_offset == "rsp") {
            // still in base case
            return std::format("rsp - {}", -base_info.stack_ix - accum_offset);
        }

        // apply any accumulated offset
        if (accum_offset != 0) {
            tc.asm_cmd(src_loc_tk, os, indent, "add", reg_offset,
                       std::format("{}", accum_offset));
        }

        // dedicated register
        // note: constructing new string to avoid clang warning "Not eliding
        //       copy on return clang (-Wnrvo)""
        return std::string{reg_offset};
    }

    static auto get_shift_amount(uint64_t value) -> std::optional<int> {
        if (value == 0 or not std::has_single_bit(value)) {
            return std::nullopt;
        }
        return std::countr_zero(value);
    }
};
