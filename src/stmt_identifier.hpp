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
        : statement{tz.next_whitespace_token(), std::move(uops)} {

        while (true) {
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
                tk = tz.next_token();
                continue;
            }

            ws2_ = tz.next_whitespace_token();

            // build a path of element identifiers later used to get the type of
            // the target element
            for (const identifier_elem& e : elems_) {
                if (not path_as_string_.empty()) {
                    path_as_string_.push_back('.');
                }
                path_as_string_ += e.name_tk.name();
            }

            if (tc.is_func(path_as_string_)) {
                //? todo: should get the type? not necessary right now but
                // for
                // consistency
                break;
            }

            const ident_info ii{
                tc.make_ident_info(tok(), path_as_string_, false)};

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

    static auto compile_effective_address_to_register(
        const token& tok, toc& tc, std::ostream& os, size_t indent,
        const std::vector<identifier_elem>& elems,
        const std::string& reg_offset) -> void {

        const identifier_elem& base_elem{elems.front()};
        std::string path{base_elem.name_tk.name()};
        const ident_info base_info{tc.make_ident_info(tok, path, false)};

        tc.asm_cmd(tok, os, indent, "lea", reg_offset,
                   std::format("[rsp - {}]", -base_info.stack_ix));

        size_t accum_offset = 0;

        for (size_t i{}; i < elems.size(); ++i) {
            const identifier_elem& elem{elems.at(i)};
            const ident_info curr_info{tc.make_ident_info(tok, path, false)};

            if (elem.has_array_index_expr) {
                const std::string& reg_index{
                    tc.alloc_scratch_register(tok, os, indent)};
                elem.array_index_expr->compile(tc, os, indent, reg_index);
                const size_t curr_type_size{curr_info.type_ref.size()};
                if (curr_type_size > 1) {
                    // is the size a 2^n number?
                    if (std::optional<int> shl{
                            get_shift_amount(curr_type_size)};
                        shl) {
                        // yes, shift left
                        tc.asm_cmd(tok, os, indent, "shl", reg_index,
                                   std::format("{}", *shl));
                    } else {
                        // no, use multiplication
                        tc.asm_cmd(tok, os, indent, "imul", reg_index,
                                   std::format("{}", curr_type_size));
                    }
                }
                tc.asm_cmd(tok, os, indent, "add", reg_offset, reg_index);
                tc.free_scratch_register(os, indent, reg_index);
            }

            if (i + 1 < elems.size()) {
                const identifier_elem& next_elem{elems[i + 1]};
                accum_offset += toc::get_field_offset_in_type(
                    tok, curr_info.type_ref, next_elem.name_tk.name());
                path.push_back('.');
                path += next_elem.name_tk.name();
            }
        }

        if (accum_offset != 0) {
            tc.asm_cmd(tok, os, indent, "add", reg_offset,
                       std::format("{}", accum_offset));
        }
    }

    static auto compile_effective_address(
        const token& src_loc_tk, toc& tc, std::ostream& os, size_t indent,
        const std::vector<identifier_elem>& elems,
        std::vector<std::string>& allocated_registers) -> std::string {

        const identifier_elem& base_elem{elems.front()};
        std::string path{base_elem.name_tk.name()};
        const ident_info base_info{tc.make_ident_info(src_loc_tk, path, false)};
        std::string reg_offset{"rsp"};
        size_t accum_offset{};
        const size_t elems_size{elems.size()};

        for (size_t i{}; i < elems_size; ++i) {
            const identifier_elem& curr_elem{elems[i]};
            const ident_info curr_info{
                tc.make_ident_info(src_loc_tk, path, false)};
            const size_t curr_type_size{curr_info.type_ref.size()};
            const bool is_last{i == elems_size - 1};
            const bool is_encodable{curr_type_size == 1 or
                                    curr_type_size == 2 or
                                    curr_type_size == 4 or curr_type_size == 8};

            if (curr_elem.has_array_index_expr) {
                if (is_last and is_encodable) {
                    const std::string& reg_idx{
                        tc.alloc_scratch_register(src_loc_tk, os, indent)};
                    allocated_registers.push_back(reg_idx);
                    base_elem.array_index_expr->compile(tc, os, indent,
                                                        reg_idx);

                    if (curr_type_size == 1) {
                        return std::format(
                            "{} + {} - {}", reg_offset, reg_idx,
                            -base_info.stack_ix +
                                static_cast<int32_t>(accum_offset));
                    }
                    return std::format("{} + {} * {} - {}", reg_offset, reg_idx,
                                       curr_type_size,
                                       -base_info.stack_ix +
                                           static_cast<int32_t>(accum_offset));
                }

                // is it sill assuming that base register is 'rsp'?
                if (reg_offset == "rsp") {
                    // yes, since offset calculations need to be done allocate
                    // and initiate a register to hold the offset regarding
                    // arrays
                    reg_offset =
                        tc.alloc_scratch_register(src_loc_tk, os, indent);
                    allocated_registers.push_back(reg_offset);
                    tc.asm_cmd(src_loc_tk, os, indent, "lea", reg_offset,
                               std::format("[rsp - {}]", -base_info.stack_ix));
                }

                // calculate index
                const std::string& reg_idx{
                    tc.alloc_scratch_register(src_loc_tk, os, indent)};
                curr_elem.array_index_expr->compile(tc, os, indent, reg_idx);

                if (curr_type_size > 1) {
                    if (std::optional<int> shl{
                            get_shift_amount(curr_type_size)};
                        shl) {
                        tc.asm_cmd(src_loc_tk, os, indent, "shl", reg_idx,
                                   std::format("{}", *shl));
                    } else {
                        tc.asm_cmd(src_loc_tk, os, indent, "imul", reg_idx,
                                   std::format("{}", curr_type_size));
                    }
                }

                tc.asm_cmd(src_loc_tk, os, indent, "add", reg_offset, reg_idx);
                tc.free_scratch_register(os, indent, reg_idx);

                if (is_last) {
                    return std::string{reg_offset};
                }
            }

            if (i + 1 < elems.size()) {
                const identifier_elem& next_elem{elems[i + 1]};
                accum_offset += toc::get_field_offset_in_type(
                    src_loc_tk, curr_info.type_ref, next_elem.name_tk.name());
                path += '.' + std::string{next_elem.name_tk.name()};
            }
        }

        if (accum_offset != 0) {
            tc.asm_cmd(src_loc_tk, os, indent, "add", reg_offset,
                       std::format("{}", accum_offset));
        }

        return std::string{reg_offset};
    }

    static auto get_shift_amount(uint64_t value) -> std::optional<int> {
        if (value == 0 or not std::has_single_bit(value)) {
            return std::nullopt;
        }
        return std::countr_zero(value);
    }
};
