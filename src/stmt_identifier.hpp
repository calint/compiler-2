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

struct identifier_elem {
    token name_tk;
    std::unique_ptr<expr_any> array_index_expr;
    token ws1;
    bool has_array_index_expr{};
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
                    tc.make_ident_info(tk, path_as_string_, false)};

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
                                    tz.next_whitespace_token(), true);

                if (not tz.is_next_char(']')) {
                    throw compiler_exception{
                        tz, "expected '[' to close array index expression"};
                }
            } else {
                elems_.emplace_back(tk, nullptr, tz.next_whitespace_token(),
                                    false);
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

            const ident_info ii{
                tc.make_ident_info(tk_prv, path_as_string_, false)};

            set_type(ii.type_ref);
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
        return elems_.at(0).name_tk;
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
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
        get_unary_ops().source_to(os);
        int sep{};
        for (const identifier_elem& e : elems_) {
            if (sep++) {
                std::print(os, ".");
            }
            e.name_tk.source_to(os);
            if (e.has_array_index_expr) {
                std::print(os, "[");
                e.array_index_expr->source_to(os);
                std::print(os, "]");
            }
            e.ws1.source_to(os);
        }
        ws2_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 std::string_view dst) const -> void override {

        tc.comment_source(*this, os, indent);

        const ident_info dst_info{tc.make_ident_info(tok(), dst, false)};

        if (not is_expression()) {
            const ident_info src_info{
                tc.make_ident_info(tok(), identifier(), false)};

            tc.asm_cmd(tok(), os, indent, "mov", dst_info.id_nasm,
                       src_info.id_nasm);

            get_unary_ops().compile(tc, os, indent, dst_info.id_nasm);
            return;
        }

        std::vector<std::string> allocated_registers;

        const std::string effective_address{
            stmt_identifier::compile_effective_address(
                tok(), tc, os, indent, elems(), allocated_registers, "")};

        const ident_info src_info{
            tc.make_ident_info(tok(), identifier(), false)};

        const std::string_view size_specifier{
            type::get_size_specifier(tok(), src_info.type_ref.size())};

        tc.asm_cmd(tok(), os, indent, "mov", dst_info.id_nasm,
                   std::format("{} [{}]", size_specifier, effective_address));

        get_unary_ops().compile(tc, os, indent, dst_info.id_nasm);

        for (const std::string& reg :
             allocated_registers | std::views::reverse) {
            tc.free_scratch_register(tok(), os, indent, reg);
        }
    }

  private:
    // helper function to emit bounds checking code
    static auto emit_bounds_check(const token& tk, toc& tc, std::ostream& os,
                                  size_t indent, std::string_view reg_to_check,
                                  size_t array_size,
                                  std::string_view comparison,
                                  std::string_view reg_size = "") -> void {
        if (not tc.is_bounds_check()) {
            return;
        }

        tc.comment_start(tk, os, indent);
        std::println(os, "bounds check");

        if (not reg_size.empty()) {
            // check with size register (reg_to_check + reg_size vs array_size)
            const std::string reg_top_idx{
                tc.alloc_scratch_register(tk, os, indent)};
            tc.asm_cmd(tk, os, indent, "mov", reg_top_idx, reg_size);
            tc.asm_cmd(tk, os, indent, "add", reg_top_idx, reg_to_check);
            tc.asm_cmd(tk, os, indent, "cmp", reg_top_idx,
                       std::to_string(array_size));

            if (tc.is_bounds_check_with_line()) {
                const std::string reg_line_num{
                    tc.alloc_scratch_register(tk, os, indent)};
                tc.comment_start(tk, os, indent);
                std::println(os, "line number");
                tc.asm_cmd(tk, os, indent, "mov", reg_line_num,
                           std::to_string(tk.at_line()));
                tc.asm_cmd(tk, os, indent, std::format("cmov{}", comparison),
                           "rbp", reg_line_num);
                tc.free_scratch_register(tk, os, indent, reg_line_num);
            }

            tc.free_scratch_register(tk, os, indent, reg_top_idx);
        } else {
            // Simple check (reg_to_check vs array_size)
            tc.asm_cmd(tk, os, indent, "cmp", reg_to_check,
                       std::to_string(array_size));

            if (tc.is_bounds_check_with_line()) {
                const std::string reg_line_num{
                    tc.alloc_scratch_register(tk, os, indent)};
                tc.comment_start(tk, os, indent);
                std::println(os, "line number");
                tc.asm_cmd(tk, os, indent, "mov", reg_line_num,
                           std::to_string(tk.at_line()));
                tc.asm_cmd(tk, os, indent, std::format("cmov{}", comparison),
                           "rbp", reg_line_num);
                tc.free_scratch_register(tk, os, indent, reg_line_num);
            }
        }

        toc::asm_jxx(tk, os, indent, comparison, "panic_bounds");
    }

  public:
    static auto
    compile_effective_address(const token& src_loc_tk, toc& tc,
                              std::ostream& os, size_t indent,
                              const std::vector<identifier_elem>& elems,
                              std::vector<std::string>& allocated_registers,
                              std::string_view reg_size) -> std::string {

        std::string path{elems.front().name_tk.text()};
        const ident_info base_info{tc.make_ident_info(src_loc_tk, path, false)};

        std::string reg_offset{"rsp"};
        int32_t accum_offset{};
        const size_t elems_size{elems.size()};

        for (size_t i{}; i < elems_size; i++) {
            const identifier_elem& curr_elem{elems[i]};
            const ident_info curr_info{
                tc.make_ident_info(src_loc_tk, path, false)};
            const size_t type_size{curr_info.type_ref.size()};
            const bool is_last{i == elems_size - 1};

            // Handle array access without indexing
            if (not curr_elem.has_array_index_expr) {
                // Bounds check for last element without indexing
                if (is_last and not reg_size.empty() and curr_info.is_array) {
                    emit_bounds_check(src_loc_tk, tc, os, indent, reg_size,
                                      curr_info.array_size, "g");
                }

                // Accumulate field offsets
                if (i + 1 < elems_size) {
                    const identifier_elem& next_elem{elems[i + 1]};
                    accum_offset +=
                        static_cast<int32_t>(toc::get_field_offset_in_type(
                            src_loc_tk, curr_info.type_ref,
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
                    curr_elem.array_index_expr->compile(tc, os, indent,
                                                        reg_idx);

                    const token& tk{curr_elem.array_index_expr->tok()};
                    const bool use_reg_size{not reg_size.empty()};
                    emit_bounds_check(tk, tc, os, indent, reg_idx,
                                      curr_info.array_size,
                                      use_reg_size ? "g" : "ge",
                                      use_reg_size ? reg_size : "");

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
            }

            // convert 'rsp' to dedicated register
            if (reg_offset == "rsp") {
                reg_offset = tc.alloc_scratch_register(src_loc_tk, os, indent);
                allocated_registers.push_back(reg_offset);
                tc.asm_cmd(src_loc_tk, os, indent, "lea", reg_offset,
                           std::format("[rsp - {}]", -base_info.stack_ix));
            }

            // calculate array index
            const std::string reg_idx{
                tc.alloc_scratch_register(src_loc_tk, os, indent)};
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
                        src_loc_tk, curr_info.type_ref,
                        next_elem.name_tk.text()));
                path.push_back('.');
                path += next_elem.name_tk.text();
            }
        }

        if (reg_offset == "rsp") {
            return std::format("rsp - {}",
                               -(base_info.stack_ix + accum_offset));
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
