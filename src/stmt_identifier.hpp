#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <memory>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "unary_ops.hpp"

class stmt_identifier : public statement {
    struct ident_elem {
        token name_tk;
        token open_bracket_tk;
        std::unique_ptr<expr_any> array_index_expr;
        token close_bracket_tk;

        auto source_to(std::ostream& os) const -> void {
            name_tk.source_to(os);
            if (array_index_expr) {
                open_bracket_tk.source_to(os);
                array_index_expr->source_to(os);
                close_bracket_tk.source_to(os);
            }
        }
    };

    std::vector<ident_elem> elems_;
    std::vector<token> elems_delim_tk_;
    std::string path_as_string_;
    size_t array_size_{};
    bool is_array_{};

  public:
    stmt_identifier(toc& tc, unary_ops uops, token tk, tokenizer& tz)
        : statement{tk, std::move(uops)}, path_as_string_{tk.text()} {

        if (tk.is_empty()) {
            throw compiler_exception(tz, "expected an identifier");
        }

        token tk_prv{tk};

        while (true) {
            if (not tc.is_func(path_as_string_)) {
                const ident_info curr_ident_info{
                    tc.make_ident_info(tk, path_as_string_)};

                if (tz.peek_char() == '[' and not curr_ident_info.is_array) {
                    throw compiler_exception{
                        tk, std::format("cannot index non-array '{}'",
                                        path_as_string_)};
                }
            }

            if (const token t{tz.is_next_char_token('[')}; not t.is_empty()) {
                elems_.emplace_back(
                    tk, token{},
                    std::make_unique<expr_any>(tc, tz, tc.get_type_default(),
                                               false, false, 0),
                    token{});

                const token tt{tz.is_next_char_token(']')};
                if (tt.is_empty()) {
                    throw compiler_exception{
                        tz, "expected ']' to close array index expression"};
                }
                elems_.back().open_bracket_tk = t;
                elems_.back().close_bracket_tk = tt;
            } else {
                elems_.emplace_back(tk, token{}, nullptr, token{});
            }

            if (const token t{tz.is_next_char_token('.')}; not t.is_empty()) {
                elems_delim_tk_.emplace_back(t);
                tk_prv = tk;
                tk = tz.next_token();
                path_as_string_.push_back('.');
                path_as_string_ += tk.text();
                continue;
            }

            if (tc.is_func(path_as_string_)) {
                break;
            }

            const ident_info ii{tc.make_ident_info(tk_prv, path_as_string_)};

            set_type(ii.type());

            if (elems_.back().array_index_expr != nullptr) {
                // if last element has index expression then this is
                // technically no longer an array but a element
                break;
            }

            is_array_ = ii.is_array;
            array_size_ = ii.array_size;

            break;
        }
    }

    stmt_identifier() = default;

    [[nodiscard]] auto first_token() const -> const token& {
        return elems_[0].name_tk;
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        return path_as_string_;
    }

    [[nodiscard]] auto elems() const -> std::span<const ident_elem> {
        return elems_;
    }

    [[nodiscard]] auto is_indexed() const -> bool override {
        return std::ranges::any_of(elems_, [](const ident_elem& e) -> bool {
            return e.array_index_expr != nullptr;
        });
    }

    [[nodiscard]] auto is_identifier() const -> bool override { return true; }

    auto source_to(std::ostream& os) const -> void override {
        get_unary_ops().source_to(os);
        if (not elems_.empty()) {
            elems_.front().source_to(os);
            for (const auto [d, e] : std::views::zip(
                     elems_delim_tk_, elems_ | std::views::drop(1))) {
                d.source_to(os);
                e.source_to(os);
            }
        }
    }

    auto compile(toc& tc, x86& x, const size_t indent,
                 const ident_info& dst_info) const -> void override {

        x86::comment_source(tc, *this, x.os, indent);

        const ident_info src_info{tc.make_ident_info(*this)};

        if (src_info.is_const()) {
            x.mov(tc, tok(), indent, dst_info.operand.str(),
                     std::format("{}{}", get_unary_ops().to_string(),
                                 src_info.const_value));
            return;
        }

        // simple identifier or is indexing in an array or relative to "lea"

        if (not is_indexed() and not src_info.has_lea()) {
            // note: contains no array indexing and is not relative a lea,
            //       e.g. world.location.link
            x.mov(tc, tok(), indent, dst_info.operand.str(),
                     src_info.operand.str());
            get_unary_ops().compile(tc, x, indent, dst_info.operand.str());
            return;
        }

        // is indexing in an array or relative to "lea"

        std::vector<std::string> allocated_registers;

        const operand op{stmt_identifier::compile_effective_address(
            tok(), tc, x, indent, elems(), allocated_registers, "",
            src_info.lea_path)};

        x.mov(tc, tok(), indent, dst_info.operand.str(),
                 op.str(src_info.type().size()));

        get_unary_ops().compile(tc, x, indent, dst_info.operand.str());

        for (const std::string& reg :
             allocated_registers | std::views::reverse) {
            tc.free_scratch_register(tok(), x.os, indent, reg);
        }
    }

    [[nodiscard]] auto compile_lea(
        const token& src_loc_tk, toc& tc, x86& x, const size_t indent,
        std::vector<std::string>& allocated_registers,
        const std::string& reg_size,
        const std::span<const std::string> lea_path) const -> operand override {

        return compile_effective_address(src_loc_tk, tc, x, indent, elems_,
                                         allocated_registers, reg_size,
                                         lea_path);
    }

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }

    [[nodiscard]] auto array_size() const -> size_t { return array_size_; }

    [[nodiscard]] static auto compile_effective_address(
        const token& src_loc_tk, toc& tc, x86& x, const size_t indent,
        const std::span<const ident_elem> elems,
        std::vector<std::string>& allocated_registers,
        const std::string_view reg_size,
        const std::span<const std::string> lea_path) -> operand {

        // pick the last n elements from lea_path since it is a path to root
        // while elems might not be
        //   e.g:'world.locations.links' while elems is 'loc.link'
        std::vector<std::string> leas;
        leas.reserve(elems.size());
        const size_t n{lea_path.size()};
        for (size_t i{lea_path.size() - elems.size()}; i < n; ++i) {
            leas.push_back(lea_path[i]);
        }

        // find the first element from the top that has a 'lea' and get
        // accessor relative to that 'lea'
        size_t elem_index_with_lea{leas.size()};
        std::string lea;
        while (elem_index_with_lea) {
            --elem_index_with_lea;
            if (not leas[elem_index_with_lea].empty()) {
                lea = leas[elem_index_with_lea];
                break;
            }
        }

        // start at an element with 'lea' or 0 when no 'lea' found
        std::string path{elems[elem_index_with_lea].name_tk.text()};
        const ident_info base_info{tc.make_ident_info(src_loc_tk, path)};

        std::string reg_offset;
        int32_t accum_offset{};
        const size_t elems_size{elems.size()};

        for (size_t i{elem_index_with_lea}; i < elems_size; ++i) {
            const ident_elem& curr_elem{elems[i]};
            const ident_info curr_info{tc.make_ident_info(src_loc_tk, path)};
            const size_t type_size{curr_info.type().size()};
            const bool is_last{i == elems_size - 1};

            // handle array access without indexing
            if (not curr_elem.array_index_expr) {
                // bounds check for the last element without indexing
                if (is_last and not reg_size.empty() and curr_info.is_array) {
                    emit_bounds_check(src_loc_tk, tc, x, indent, reg_size,
                                      curr_info.array_size, "g");
                }

                // accumulate field offsets
                if (i + 1 < elems_size) {
                    const ident_elem& next_elem{elems[i + 1]};
                    accum_offset +=
                        static_cast<int32_t>(toc::get_field_offset_in_type(
                            curr_info.type(), next_elem.name_tk.text()));
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
                    const std::string reg_idx{tc.alloc_scratch_register(
                        src_loc_tk, x.os, indent, tc.get_type_default())};
                    allocated_registers.push_back(reg_idx);

                    x.comment_line(tc, curr_elem.array_index_expr->tok(),
                                      indent, "set array index");

                    curr_elem.array_index_expr->compile(
                        tc, x, indent,
                        tc.make_ident_info_for_register(reg_idx));

                    const token& tk{curr_elem.array_index_expr->tok()};
                    const bool use_reg_size{not reg_size.empty()};
                    emit_bounds_check(tk, tc, x, indent, reg_idx,
                                      curr_info.array_size,
                                      use_reg_size ? "g" : "ge",
                                      use_reg_size ? reg_size : "");

                    if (reg_offset.empty()) {
                        reg_offset =
                            init_reg_offset(src_loc_tk, tc, x, indent, lea,
                                            allocated_registers, true, true,
                                            base_info.operand.base_register);
                    }

                    const bool is_rsp{reg_offset == "rsp"};

                    // offsets depends on if base register is rsp
                    const int32_t offset{
                        is_rsp ? -(base_info.stack_ix + accum_offset)
                               : accum_offset};

                    operand oper;
                    oper.base_register = reg_offset;
                    oper.index_register = reg_idx;
                    if (type_size != 1) {
                        oper.scale = static_cast<uint8_t>(type_size);
                    }
                    oper.displacement = is_rsp ? -offset : offset;
                    return oper;
                }
            }

            // convert 'rsp' to dedicated register

            if (reg_offset.empty()) {
                reg_offset = init_reg_offset(src_loc_tk, tc, x, indent, lea,
                                             allocated_registers, false, true,
                                             base_info.operand.base_register);
            }

            if (reg_offset == "rsp") {
                reg_offset = tc.alloc_scratch_register(src_loc_tk, x.os, indent,
                                                       tc.get_type_default());
                allocated_registers.push_back(reg_offset);
                x.lea( indent, reg_offset,
                         std::format("rsp - {}", -base_info.stack_ix));
            } else if (reg_offset == base_info.operand.base_register) {
                reg_offset = tc.alloc_scratch_register(src_loc_tk, x.os, indent,
                                                       tc.get_type_default());
                allocated_registers.push_back(reg_offset);
                x.lea( indent, reg_offset,
                         std::format("{}", base_info.operand.base_register));
            }

            // calculate array index
            const std::string reg_idx{tc.alloc_scratch_register(
                src_loc_tk, x.os, indent, tc.get_type_default())};

            x.comment_line(tc, curr_elem.array_index_expr->tok(), indent,
                              "set array index");

            curr_elem.array_index_expr->compile(
                tc, x, indent, tc.make_ident_info_for_register(reg_idx));

            // bounds check
            const token& tk{curr_elem.array_index_expr->tok()};
            const bool use_reg_size{is_last and not reg_size.empty()};
            emit_bounds_check(tk, tc, x, indent, reg_idx, curr_info.array_size,
                              use_reg_size ? "g" : "ge",
                              use_reg_size ? reg_size : "");

            // scale the index
            if (type_size > 1) {
                if (std::optional<int> shl{get_shift_amount(type_size)}; shl) {
                    x.shl( indent, reg_idx, std::format("{}", *shl));
                } else {
                    x.imul(tc, tk, indent, reg_idx,
                              std::format("{}", type_size));
                }
            }

            // add index offset to base register
            x.op(tc, src_loc_tk, indent, "add", reg_offset, reg_idx);
            tc.free_scratch_register(src_loc_tk, x.os, indent, reg_idx);

            // accumulate field offsets
            if (i + 1 < elems_size) {
                const ident_elem& next_elem{elems[i + 1]};
                accum_offset +=
                    static_cast<int32_t>(toc::get_field_offset_in_type(
                        curr_info.type(), next_elem.name_tk.text()));
                path.push_back('.');
                path += next_elem.name_tk.text();
            }
        }

        if (reg_offset.empty()) {
            reg_offset = init_reg_offset(src_loc_tk, tc, x, indent, lea,
                                         allocated_registers, true, false,
                                         base_info.operand.base_register);
        }

        operand op{reg_offset};
        // note: 'reg_offset' might be e.g. "rsp + r15 * 8 + 16" so it needs to
        // be parsed

        op.displacement += accum_offset;

        if (reg_offset == "rsp") {
            // register is not optimally encoded for trailing elements of size
            // 1, 2, 4, or 8
            op.displacement += base_info.stack_ix;
        }

        return op;
    }

    [[nodiscard]] static auto get_shift_amount(const uint64_t value)
        -> std::optional<int> {
        if (value == 0 or not std::has_single_bit(value)) {
            return std::nullopt;
        }
        return std::countr_zero(value);
    }

  private:
    // helper function to emit bound checking code
    static auto emit_bounds_check(const token& tk, toc& tc, x86& x,
                                  const size_t indent,
                                  const std::string_view reg_to_check,
                                  const size_t array_size,
                                  const std::string_view comparison,
                                  const std::string_view reg_size = "")
        -> void {

        if (not tc.is_bounds_check_upper() and not tc.is_bounds_check_lower()) {
            return;
        }

        x.comment_line(tc, tk, indent, "bounds check");

        // Allocate line number register once if needed
        std::string reg_line_num;
        if (tc.is_bounds_check_with_line()) {
            reg_line_num = tc.alloc_scratch_register(tk, x.os, indent,
                                                     tc.get_type_default());
            x.comment_line(tc, tk, indent, "line number");
            x.mov(tc, tk, indent, reg_line_num,
                     std::to_string(tk.at_line()));
        }

        // check for negative index (optional lower bounds check)
        if (tc.is_bounds_check_lower()) {
            x.test( indent, reg_to_check, reg_to_check);
            if (tc.is_bounds_check_with_line()) {
                x.cmovs( indent, "rbp", reg_line_num);
            }
            x.jcc( indent, "s", "panic_bounds");
        }

        if (tc.is_bounds_check_upper()) {
            if (not reg_size.empty()) {
                const std::string reg_top_idx = tc.alloc_scratch_register(
                    tk, x.os, indent, tc.get_type_default());
                x.mov(tc, tk, indent, reg_top_idx, reg_size);
                x.add( indent, reg_top_idx, reg_to_check);
                x.cmp(indent, reg_top_idx,
                         std::to_string(array_size));
                tc.free_scratch_register(tk, x.os, indent, reg_top_idx);
            } else {
                x.cmp(indent, reg_to_check,
                         std::to_string(array_size));
            }
            if (tc.is_bounds_check_with_line()) {
                x.op(tc, tk, indent, std::format("cmov{}", comparison),
                        "rbp", reg_line_num);
            }
            if (tc.is_bounds_check_upper()) {
                x.jcc( indent, comparison, "panic_bounds");
            }
        }

        // free line number register if allocated
        if (tc.is_bounds_check_with_line()) {
            tc.free_scratch_register(tk, x.os, indent, reg_line_num);
        }
    }

    [[nodiscard]] static auto
    init_reg_offset(const token& src_loc_tk, toc& tc, x86& x,
                    const size_t indent, const std::string& lea,
                    std::vector<std::string>& allocated_registers,
                    const bool no_changes_to_reg_offset_after_this,
                    const bool will_be_indirect_indexed,
                    const std::string& base_register) -> std::string {

        if (not lea.empty()) {
            // if no change is done to the lea register, just return it
            if (no_changes_to_reg_offset_after_this and
                not(will_be_indirect_indexed and operand{lea}.is_indexed())) {

                return lea;
            }

            const std::string index_reg{tc.alloc_scratch_register(
                src_loc_tk, x.os, indent, tc.get_type_default())};
            allocated_registers.push_back(index_reg);

            const operand op{lea};

            // changes will be made to the register so return an allocated
            // register
            if (not op.index_register.empty() or op.displacement != 0) {
                x.lea( indent, index_reg, lea);
            } else {
                x.mov(tc, src_loc_tk, indent, index_reg, lea);
            }

            return index_reg;
        }

        return base_register;
    }
};
