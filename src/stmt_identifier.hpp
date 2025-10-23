#pragma once

#include <algorithm>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "compiler_exception.hpp"
#include "expr_any.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "unary_ops.hpp"

struct identifier_elem {
    token name_tk;
    std::unique_ptr<expr_any> array_index_expr;
    token ws1;
};

class stmt_identifier : public statement {
    std::vector<identifier_elem> elems_;
    token ws1_;
    token ws2_;
    std::string path_as_string_;

  public:
    stmt_identifier(toc& tc, unary_ops uops, token tk, tokenizer& tz)
        : statement{tk, std::move(uops)}, path_as_string_{tk.text()} {

        token tk_prv{tk};

        while (true) {
            if (not tc.is_func(path_as_string_)) {
                const ident_info curr_ident_info{
                    tc.make_ident_info(tk, path_as_string_)};

                if (tz.peek_char() == '[' and not curr_ident_info.is_array) {
                    throw compiler_exception{
                        tk,
                        std::format("'{}' is not an array", path_as_string_)};
                }
            }

            if (tz.is_next_char('[')) {
                elems_.emplace_back(tk,
                                    std::make_unique<expr_any>(
                                        tc, tz, tc.get_type_default(), false),
                                    tz.next_whitespace_token());

                if (not tz.is_next_char(']')) {
                    throw compiler_exception{
                        tz, "expected '[' to close array index expression"};
                }
            } else {
                elems_.emplace_back(tk, nullptr, tz.next_whitespace_token());
            }

            if (tz.is_next_char('.')) {
                tk_prv = tk;
                tk = tz.next_token();
                path_as_string_.push_back('.');
                path_as_string_ += tk.text();
                continue;
            }

            ws2_ = tz.next_whitespace_token();

            if (tc.is_func(path_as_string_)) {
                break;
            }

            const ident_info ii{tc.make_ident_info(tk_prv, path_as_string_)};

            set_type(*ii.type_ptr);
            break;
        }
    }

    ~stmt_identifier() override = default;

    stmt_identifier() = default;
    stmt_identifier(const stmt_identifier&) = default;
    stmt_identifier(stmt_identifier&&) = default;
    auto operator=(const stmt_identifier&) -> stmt_identifier& = default;
    auto operator=(stmt_identifier&&) -> stmt_identifier& = default;

    [[nodiscard]] auto first_token() const -> const token& {
        return elems_[0].name_tk;
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        return path_as_string_;
    }

    [[nodiscard]] auto elems() const -> std::span<const identifier_elem> {
        return elems_;
    }

    [[nodiscard]] auto is_indexed() const -> bool override {
        return std::ranges::any_of(elems_,
                                   [](const identifier_elem& e) -> bool {
                                       return e.array_index_expr != nullptr;
                                   });
    }

    [[nodiscard]] auto is_identifier() const -> bool override { return true; }

    auto source_to(std::ostream& os) const -> void override {
        get_unary_ops().source_to(os);
        int sep{};
        for (const identifier_elem& e : elems_) {
            if (sep++) {
                std::print(os, ".");
            }
            e.name_tk.source_to(os);
            if (e.array_index_expr) {
                std::print(os, "[");
                e.array_index_expr->source_to(os);
                std::print(os, "]");
            }
            e.ws1.source_to(os);
        }
        ws2_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 const std::string_view dst) const -> void override {

        tc.comment_source(*this, os, indent);

        const ident_info dst_info{tc.make_ident_info(tok(), dst)};

        // DEBUG
        // std::println(std::cerr, "[{}] identifier path: {}", tok().at_line(),
        //              dst_info.id);
        // for (size_t i = 0; i < dst_info.elem_path.size(); i++) {
        //     std::println(std::cerr, "  {} ; {} ; {}", dst_info.elem_path[i],
        //                  dst_info.type_path[i]->name(),
        //                  dst_info.lea_path[i]);
        // }

        const ident_info src_info{tc.make_ident_info(tok(), identifier())};

        if (src_info.is_const()) {
            tc.asm_cmd(tok(), os, indent, "mov", dst_info.operand,
                       std::format("{}{}", get_unary_ops().to_string(),
                                   src_info.operand));
            return;
        }

        // simple identifier or is indexing in array or relative to "lea"

        if (not is_indexed() and not src_info.has_lea()) {
            // note: contains no array indexing and is not relative a lea,
            //       e.g. world.location.link
            tc.asm_cmd(tok(), os, indent, "mov", dst_info.operand,
                       src_info.operand);
            get_unary_ops().compile(tc, os, indent, dst_info.operand);
            return;
        }

        // is indexing in array or relative to "lea"

        std::vector<std::string> allocated_registers;

        const std::string effective_address{
            stmt_identifier::compile_effective_address(
                tok(), tc, os, indent, elems(), allocated_registers, "",
                src_info.lea_path)};

        const std::string_view size_specifier{
            toc::get_size_specifier(src_info.type_ptr->size())};

        tc.asm_cmd(tok(), os, indent, "mov", dst_info.operand,
                   std::format("{} [{}]", size_specifier, effective_address));

        get_unary_ops().compile(tc, os, indent, dst_info.operand);

        for (const std::string& reg :
             allocated_registers | std::views::reverse) {
            tc.free_scratch_register(tok(), os, indent, reg);
        }
    }

    auto compile_lea(const token& src_loc_tk, toc& tc, std::ostream& os,
                     size_t indent,
                     std::vector<std::string>& allocated_registers,
                     const std::string& reg_size,
                     std::span<const std::string> lea_path) const
        -> std::string override {

        return compile_effective_address(src_loc_tk, tc, os, indent, elems_,
                                         allocated_registers, reg_size,
                                         lea_path);
    }

    static auto compile_effective_address(
        const token& src_loc_tk, toc& tc, std::ostream& os, const size_t indent,
        const std::span<const identifier_elem> elems,
        std::vector<std::string>& allocated_registers,
        const std::string_view reg_size,
        const std::span<const std::string> lea_path) -> std::string {

        // pick the last n elements from lea_path since it is a path to root
        // while elems might not be
        //   e.g:`world.locations.links` while elems is `loc.link`
        std::vector<std::string> leas;
        leas.reserve(elems.size());
        const size_t n{lea_path.size()};
        for (size_t i{lea_path.size() - elems.size()}; i < n; i++) {
            leas.push_back(lea_path[i]);
        }

        // find the first element from the top that has a "lea" and get accessor
        // relative to that "lea"
        size_t elem_index_with_lea{leas.size()};
        std::string lea;
        while (elem_index_with_lea) {
            elem_index_with_lea--;
            if (not leas[elem_index_with_lea].empty()) {
                lea = leas[elem_index_with_lea];
                break;
            }
        }

        // start at element with "lea" or 0 when no "lea" found
        std::string path{elems[elem_index_with_lea].name_tk.text()};
        const ident_info base_info{tc.make_ident_info(src_loc_tk, path)};

        std::string reg_offset;
        int32_t accum_offset{};
        const size_t elems_size{elems.size()};

        for (size_t i{elem_index_with_lea}; i < elems_size; i++) {
            const identifier_elem& curr_elem{elems[i]};
            const ident_info curr_info{tc.make_ident_info(src_loc_tk, path)};
            const size_t type_size{curr_info.type_ptr->size()};
            const bool is_last{i == elems_size - 1};

            // handle array access without indexing
            if (not curr_elem.array_index_expr) {
                // bounds check for last element without indexing
                if (is_last and not reg_size.empty() and curr_info.is_array) {
                    emit_bounds_check(src_loc_tk, tc, os, indent, reg_size,
                                      curr_info.array_size, "g");
                }

                // accumulate field offsets
                if (i + 1 < elems_size) {
                    const identifier_elem& next_elem{elems[i + 1]};
                    accum_offset +=
                        static_cast<int32_t>(toc::get_field_offset_in_type(
                            src_loc_tk, *curr_info.type_ptr,
                            next_elem.name_tk.text()));
                    path.push_back('.');
                    path += next_elem.name_tk.text();
                }
                continue;
            }

            // array access with indexing

            // special case: last element with encodable size
            if (is_last) {
                const bool is_encodable{type_size == 1 or type_size == 2 or
                                        type_size == 4 or type_size == 8};
                if (is_encodable) {
                    const std::string reg_idx{
                        tc.alloc_scratch_register(src_loc_tk, os, indent)};
                    allocated_registers.push_back(reg_idx);

                    tc.comment_start(curr_elem.array_index_expr->tok(), os,
                                     indent);
                    std::println(os, "set array index");

                    curr_elem.array_index_expr->compile(tc, os, indent,
                                                        reg_idx);

                    const token& tk{curr_elem.array_index_expr->tok()};
                    const bool use_reg_size{not reg_size.empty()};
                    emit_bounds_check(tk, tc, os, indent, reg_idx,
                                      curr_info.array_size,
                                      use_reg_size ? "g" : "ge",
                                      use_reg_size ? reg_size : "");

                    if (reg_offset.empty()) {
                        reg_offset =
                            init_reg_offset(src_loc_tk, tc, os, indent, lea,
                                            allocated_registers, true, true);
                    }

                    const int32_t offset{
                        (reg_offset == "rsp")
                            ? -(base_info.stack_ix + accum_offset)
                            : accum_offset};
                    const char op{(reg_offset == "rsp") ? '-' : '+'};

                    // make nice output with un-necessary assembler such as * 1
                    // + 0 etc
                    if (type_size == 1) {
                        if (offset != 0) {
                            return std::format("{} + {} {} {}", reg_offset,
                                               reg_idx, op, offset);
                        }
                        return std::format("{} + {}", reg_offset, reg_idx);
                    }
                    if (offset != 0) {
                        return std::format("{} + {} * {} {} {}", reg_offset,
                                           reg_idx, type_size, op, offset);
                    }
                    return std::format("{} + {} * {}", reg_offset, reg_idx,
                                       type_size);
                }
            }

            // convert 'rsp' to dedicated register

            if (reg_offset.empty()) {
                reg_offset = init_reg_offset(src_loc_tk, tc, os, indent, lea,
                                             allocated_registers, false, true);
            }

            if (reg_offset == "rsp") {
                reg_offset = tc.alloc_scratch_register(src_loc_tk, os, indent);
                allocated_registers.push_back(reg_offset);
                toc::asm_lea(src_loc_tk, os, indent, reg_offset,
                             std::format("rsp - {}", -base_info.stack_ix));
            }

            // calculate array index
            const std::string reg_idx{
                tc.alloc_scratch_register(src_loc_tk, os, indent)};

            tc.comment_start(curr_elem.array_index_expr->tok(), os, indent);
            std::println(os, "set array index");

            curr_elem.array_index_expr->compile(tc, os, indent, reg_idx);

            // bounds check
            const token& tk{curr_elem.array_index_expr->tok()};
            const bool use_reg_size{is_last and not reg_size.empty()};
            emit_bounds_check(tk, tc, os, indent, reg_idx, curr_info.array_size,
                              use_reg_size ? "g" : "ge",
                              use_reg_size ? reg_size : "");

            // scale the index
            if (type_size > 1) {
                if (std::optional<int> shl{get_shift_amount(type_size)}; shl) {
                    tc.asm_cmd(src_loc_tk, os, indent, "shl", reg_idx,
                               std::format("{}", *shl));
                } else {
                    tc.asm_cmd(src_loc_tk, os, indent, "imul", reg_idx,
                               std::format("{}", type_size));
                }
            }

            // add index offset to base register
            tc.asm_cmd(src_loc_tk, os, indent, "add", reg_offset, reg_idx);
            tc.free_scratch_register(src_loc_tk, os, indent, reg_idx);

            // accumulate field offsets
            if (i + 1 < elems_size) {
                const identifier_elem& next_elem{elems[i + 1]};
                accum_offset +=
                    static_cast<int32_t>(toc::get_field_offset_in_type(
                        src_loc_tk, *curr_info.type_ptr,
                        next_elem.name_tk.text()));
                path.push_back('.');
                path += next_elem.name_tk.text();
            }
        }

        if (reg_offset.empty()) {
            reg_offset = init_reg_offset(src_loc_tk, tc, os, indent, lea,
                                         allocated_registers, true, false);
        }

        if (reg_offset == "rsp") {
            return std::format("rsp - {}",
                               -(base_info.stack_ix + accum_offset));
        }

        if (accum_offset != 0) {
            operand op{reg_offset};
            op.displacement += accum_offset;
            return op.to_string();
        }

        return std::string{reg_offset};
    }

    static auto get_shift_amount(const uint64_t value) -> std::optional<int> {
        if (value == 0 or not std::has_single_bit(value)) {
            return std::nullopt;
        }
        return std::countr_zero(value);
    }

  private:
    // helper function to emit bounds checking code
    static auto emit_bounds_check(const token& tk, toc& tc, std::ostream& os,
                                  const size_t indent,
                                  const std::string_view reg_to_check,
                                  const size_t array_size,
                                  const std::string_view comparison,
                                  const std::string_view reg_size = "")
        -> void {

        if (not tc.is_bounds_check_upper() and not tc.is_bounds_check_lower()) {
            return;
        }

        tc.comment_start(tk, os, indent);
        std::println(os, "bounds check");

        // Allocate line number register once if needed
        std::string reg_line_num;
        if (tc.is_bounds_check_with_line()) {
            reg_line_num = tc.alloc_scratch_register(tk, os, indent);
            tc.comment_start(tk, os, indent);
            std::println(os, "line number");
            tc.asm_cmd(tk, os, indent, "mov", reg_line_num,
                       std::to_string(tk.at_line()));
        }

        // check for negative index (optional lower bounds check)
        if (tc.is_bounds_check_lower()) {
            tc.asm_cmd(tk, os, indent, "test", reg_to_check, reg_to_check);
            if (tc.is_bounds_check_with_line()) {
                tc.asm_cmd(tk, os, indent, "cmovs", "rbp", reg_line_num);
            }
            toc::asm_jxx(tk, os, indent, "s", "panic_bounds");
        }

        if (tc.is_bounds_check_upper()) {
            if (not reg_size.empty()) {
                const std::string reg_top_idx =
                    tc.alloc_scratch_register(tk, os, indent);
                tc.asm_cmd(tk, os, indent, "mov", reg_top_idx, reg_size);
                tc.asm_cmd(tk, os, indent, "add", reg_top_idx, reg_to_check);
                tc.asm_cmd(tk, os, indent, "cmp", reg_top_idx,
                           std::to_string(array_size));
                tc.free_scratch_register(tk, os, indent, reg_top_idx);
            } else {
                tc.asm_cmd(tk, os, indent, "cmp", reg_to_check,
                           std::to_string(array_size));
            }
            if (tc.is_bounds_check_with_line()) {
                tc.asm_cmd(tk, os, indent, std::format("cmov{}", comparison),
                           "rbp", reg_line_num);
            }
            if (tc.is_bounds_check_upper()) {
                toc::asm_jxx(tk, os, indent, comparison, "panic_bounds");
            }
        }

        // free line number register if allocated
        if (tc.is_bounds_check_with_line()) {
            tc.free_scratch_register(tk, os, indent, reg_line_num);
        }
    }

    static auto init_reg_offset(const token& src_loc_tk, toc& tc,
                                std::ostream& os, const size_t indent,
                                const std::string& lea,
                                std::vector<std::string>& allocated_registers,
                                const bool no_changes_to_reg_offset_after_this,
                                const bool will_be_indirect_indexed)
        -> std::string {

        if (not lea.empty()) {
            // if no change will be done to the lea register just return it
            if (no_changes_to_reg_offset_after_this and
                not(will_be_indirect_indexed and operand{lea}.is_indexed())) {
                return lea;
            }

            const std::string index_reg{
                tc.alloc_scratch_register(src_loc_tk, os, indent)};
            allocated_registers.push_back(index_reg);
            const operand op{lea};

            // changes will be made to the register so return an allocated
            // register
            if (not op.index_register.empty() or op.displacement != 0) {
                toc::asm_lea(src_loc_tk, os, indent, index_reg, lea);
            } else {
                tc.asm_cmd(src_loc_tk, os, indent, "mov", index_reg, lea);
            }
            return index_reg;
        }

        return "rsp";
    }
};
