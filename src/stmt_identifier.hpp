#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
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
    std::vector<token> elem_delims_tk_;
    std::string path_as_string_;
    size_t array_size_{};
    bool is_array_{};
    bool is_indexed_{};

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
                is_indexed_ = true;
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
                elem_delims_tk_.emplace_back(t);
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

            set_type(ii.type_ref());

            if (elems_.back().array_index_expr != nullptr) {
                // if the last element has an index expression then this is
                // technically no longer an array but an element
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
        return is_indexed_;
    }

    [[nodiscard]] auto is_identifier() const -> bool override { return true; }

    auto source_to(std::ostream& os) const -> void override {
        get_unary_ops().source_to(os);
        if (not elems_.empty()) {
            elems_.front().source_to(os);
            for (const auto [d, e] : std::views::zip(
                     elem_delims_tk_, elems_ | std::views::drop(1))) {

                d.source_to(os);
                e.source_to(os);
            }
        }
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const ident_info src_info{tc.make_ident_info(*this)};

        if (src_info.is_const()) {
            x.copy_value(tok(), indent, dst_info.operand,
                         make_constant_operand(src_info));

            return;
        }

        std::vector<operand> allocated_registers;

        operand op{src_info.operand};

        if (is_indexed() or src_info.has_lea()) {
            op = compile_lea(tc, indent, tok(), allocated_registers, {},
                             src_info.lea_path);

            op.size = src_info.type_ref().size();
        }

        x.copy_value(tok(), indent, dst_info.operand, op);

        get_unary_ops().compile(tc, indent, dst_info.operand);

        x.free_scratch_registers(tok(), indent, allocated_registers);
    }

    [[nodiscard]] auto compile_lea(
        toc& tc, const size_t indent, const token& src_loc_tk,
        std::vector<operand>& allocated_registers, const operand& reg_size,
        const std::span<const operand> lea_path) const -> operand override {

        return compile_effective_address(tc, indent, src_loc_tk, elems_,
                                         allocated_registers, reg_size,
                                         lea_path);
    }

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }

    [[nodiscard]] auto array_size() const -> size_t { return array_size_; }

    [[nodiscard]] static auto compile_effective_address(
        toc& tc, const size_t indent, const token& src_loc_tk,
        const std::span<const ident_elem> elems,
        std::vector<operand>& allocated_registers, const operand& reg_size,
        const std::span<const operand> lea_path) -> operand {

        // pick the last n elements from lea_path since it is a path to root
        // while elems might not be
        //   e.g:'world.locations.links' while elems is 'loc.link'
        std::vector<operand> leas;
        leas.reserve(elems.size());
        for (const operand& address : lea_path.last(elems.size())) {
            leas.push_back(address);
        }

        // find the first element from the top that has a 'lea' and get
        // accessor relative to that 'lea'
        size_t elem_index_with_lea{leas.size()};
        operand lea;
        while (elem_index_with_lea) {
            --elem_index_with_lea;
            if (not leas[elem_index_with_lea].is_empty()) {
                lea = leas[elem_index_with_lea];
                break;
            }
        }

        // start at an element with 'lea' or 0 when no 'lea' found
        std::string path{elems[elem_index_with_lea].name_tk.text()};
        const ident_info base_info{tc.make_ident_info(src_loc_tk, path)};

        operand reg_offset;
        int32_t accum_offset{};
        const size_t elems_size{elems.size()};

        machine& x{tc.machine()};

        for (size_t i{elem_index_with_lea}; i < elems_size; ++i) {
            const ident_elem& curr_elem{elems[i]};
            const ident_info curr_info{tc.make_ident_info(src_loc_tk, path)};
            const size_t type_size{curr_info.type_ref().size()};
            const bool is_last{i == elems_size - 1};

            // handle array access without indexing
            if (not curr_elem.array_index_expr) {
                // bounds check for the last element without indexing
                if (is_last and not reg_size.is_empty() and
                    curr_info.is_array) {
                    emit_bounds_check(tc, indent, src_loc_tk, reg_size,
                                      curr_info.array_size, true);
                }

            } else {
                // array access with indexing

                // special case: last element with encodable size
                if (is_last) {
                    const bool is_encodable{
                        x.can_encode_index_scale(type_size)};

                    if (is_encodable) {
                        const operand reg_idx{x.alloc_scratch_register(
                            src_loc_tk, indent, tc.get_type_default())};

                        allocated_registers.push_back(reg_idx);

                        compile_array_index(
                            tc, indent, *curr_elem.array_index_expr, reg_idx,
                            curr_info.array_size, reg_size);

                        if (reg_offset.is_empty()) {
                            reg_offset = init_reg_offset(
                                tc, indent, src_loc_tk, lea,
                                allocated_registers, true, true,
                                base_info.operand.base_register);
                        }

                        const int32_t offset{x.is_variables_base(reg_offset)
                                                 ? base_info.stack_idx +
                                                       accum_offset
                                                 : accum_offset};

                        return operand::mem(
                            reg_offset.base_register, reg_idx.base_register,
                            static_cast<uint8_t>(type_size), offset);
                    }
                }

                // convert the variable base to a dedicated register

                if (reg_offset.is_empty()) {
                    reg_offset = init_reg_offset(
                        tc, indent, src_loc_tk, lea, allocated_registers, false,
                        true, base_info.operand.base_register);
                }

                if (x.is_variables_base(reg_offset)) {
                    const operand offset_register{x.alloc_scratch_register(
                        src_loc_tk, indent, tc.get_type_default())};

                    allocated_registers.push_back(offset_register);
                    reg_offset = offset_register;
                    x.address_of_variable(src_loc_tk, indent, reg_offset,
                                          base_info.stack_idx);
                } else if (not reg_offset.is_indexed() and
                           reg_offset.base_register ==
                               base_info.operand.base_register) {
                    const operand offset_register{x.alloc_scratch_register(
                        src_loc_tk, indent, tc.get_type_default())};

                    allocated_registers.push_back(offset_register);
                    reg_offset = offset_register;
                    x.address_of(src_loc_tk, indent, reg_offset,
                                 operand::mem(base_info.operand.base_register,
                                              "", 1, 0));
                }

                // calculate array index
                const operand reg_idx{x.alloc_scratch_register(
                    src_loc_tk, indent, tc.get_type_default())};

                compile_array_index(tc, indent, *curr_elem.array_index_expr,
                                    reg_idx, curr_info.array_size,
                                    is_last ? reg_size : operand{});

                // scale the index
                x.scale_index(curr_elem.array_index_expr->tok(), indent,
                              reg_idx, type_size);

                // add index offset to base register
                x.add_subtract(src_loc_tk, indent, '+', reg_offset, reg_idx);
                x.free_scratch_register(src_loc_tk, indent, reg_idx);
            }

            // accumulate field offsets
            if (i + 1 < elems_size) {
                const ident_elem& next_elem{elems[i + 1]};
                accum_offset +=
                    static_cast<int32_t>(toc::get_field_offset_in_type(
                        curr_info.type_ref(), next_elem.name_tk.text()));

                path.push_back('.');
                path += next_elem.name_tk.text();
            }
        }

        if (reg_offset.is_empty()) {
            reg_offset = init_reg_offset(tc, indent, src_loc_tk, lea,
                                         allocated_registers, true, false,
                                         base_info.operand.base_register);
        }

        operand op{operand::mem(reg_offset.base_register,
                                reg_offset.index_register, reg_offset.scale,
                                reg_offset.displacement)};

        op.displacement += accum_offset;

        if (x.is_variables_base(reg_offset)) {
            // register is not optimally encoded for trailing elements of size
            // 1, 2, 4, or 8
            op.displacement += base_info.stack_idx;
        }

        return op;
    }

  private:
    static auto compile_array_index(toc& tc, const size_t indent,
                                    const expr_any& index_expr,
                                    const operand& reg_idx,
                                    const size_t array_size,
                                    const operand& reg_size) -> void {

        machine& x{tc.machine()};

        x.comment(index_expr.tok(), indent, "set array index");
        index_expr.compile(tc, indent,
                           toc::make_ident_info_from_register(reg_idx));

        emit_bounds_check(tc, indent, index_expr.tok(), reg_idx, array_size,
                          not reg_size.is_empty(), reg_size);
    }

    // helper function to emit bounds-checking code
    static auto emit_bounds_check(toc& tc, const size_t indent,
                                  const token& src_loc_tk,
                                  const operand& reg_to_check,
                                  const size_t array_size, const bool allow_end,
                                  const operand& reg_size = {}) -> void {

        machine& x{tc.machine()};

        x.check_bounds(src_loc_tk, indent, reg_to_check, array_size, allow_end,
                       reg_size,
                       {
                           .upper{tc.is_bounds_check_upper()},
                           .lower{tc.is_bounds_check_lower()},
                           .with_line{tc.is_bounds_check_with_line()},
                       });
    }

    [[nodiscard]] static auto
    init_reg_offset(toc& tc, const size_t indent, const token& src_loc_tk,
                    const operand& lea,
                    std::vector<operand>& allocated_registers,
                    const bool no_changes_to_reg_offset_after_this,
                    const bool will_be_indirect_indexed,
                    const std::string& base_register) -> operand {

        if (not lea.is_empty()) {

            // if no change is done to the lea register, just return it
            if (no_changes_to_reg_offset_after_this and
                not(will_be_indirect_indexed and lea.is_indexed())) {

                return lea;
            }

            machine& x{tc.machine()};

            const operand index_reg{x.alloc_scratch_register(
                src_loc_tk, indent, tc.get_type_default())};

            allocated_registers.push_back(index_reg);

            // changes will be made to the register so return an allocated
            // register
            if (lea.is_indexed()) {
                x.address_of(src_loc_tk, indent, index_reg, lea);
            } else {
                x.copy_value(src_loc_tk, indent, index_reg,
                             x.reg(lea.base_register));
            }

            return index_reg;
        }

        return tc.machine().reg(base_register);
    }
};
