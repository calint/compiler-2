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

    struct lea_state {
        operand lea;
        ident_info base_info;
        operand reg_offset;
        // only the storage offset is pending; field offsets stay separate
        bool offset_pending{};
        int64_t accumulated_offset{};
    };

    std::vector<ident_elem> elems_;
    std::vector<token> elem_delims_tk_;
    std::string path_as_string_;
    size_t array_count_{};
    bool is_array_{};
    bool is_indexed_{};

  public:
    stmt_identifier(toc& tc, unary_ops uops, token tk, tokenizer& tz)
        : statement{tk, std::move(uops)}, path_as_string_{tk.text()} {

        if (tk.is_empty()) {
            throw compiler_exception{tz, "expected an identifier"};
        }

        token tk_prv{tk};

        while (true) {
            if (not tc.is_func(path_as_string_)) {
                const ident_info cur_ident_info{
                    tc.make_ident_info(tk, path_as_string_)};

                if (tz.peek_char() == '[' and not cur_ident_info.is_array) {
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
            array_count_ = ii.array_len;

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

        const operand op{
            tc.get_lea_operand(indent, *this, src_info, allocated_registers)};

        x.copy_value(tok(), indent, dst_info.operand, op);

        get_unary_ops().compile(tc, indent, dst_info.operand);

        x.free_scratch_registers(tok(), indent, allocated_registers);
    }

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }

    [[nodiscard]] auto is_array_element() const -> bool override {
        return not elems_.empty() and elems_.back().array_index_expr != nullptr;
    }

    [[nodiscard]] auto array_count() const -> size_t { return array_count_; }

    // walk the identifier from a known address and collect field offsets
    // keep the final index in the operand when the backend supports its scale
    // otherwise compute a base register and leave its encoding to the backend
    [[nodiscard]] auto compile_lea(toc& tc, const size_t indent,
                                   const token& src_loc_tk,
                                   std::vector<operand>& allocated_registers,
                                   const operand& reg_count,
                                   const std::span<const operand> lea_path,
                                   const operand& address_register) const
        -> operand override {

        // align the full lea path with this identifier's elements
        const std::span<const operand> leas{lea_path.last(elems_.size())};

        // start at the nearest known address from the top, or the root if none
        // exists
        const size_t elem_index_with_lea{find_lea_start(leas)};
        const operand& lea{leas[elem_index_with_lea]};

        // start at an element with "lea" or 0 when no "lea" found
        std::string path{elems_[elem_index_with_lea].name_tk.text()};

        // create the starting point where "lea" was found
        const ident_info base_info{tc.make_ident_info(src_loc_tk, path)};

        const type* value_type{&base_info.type_ref()};

        // a known address or a loaded pointer already accounts for storage
        // field offsets remain separate until the final operand is built

        lea_state state{
            .lea{lea},
            .base_info{base_info},
            .reg_offset{},
            .offset_pending{lea.is_empty() and not base_info.is_pointer},
            .accumulated_offset{},
        };
        // note: 'offset_pending' indicates whether there is an offset that has
        // yet to be applied to the base address

        load_pointer_base(tc, indent, src_loc_tk, allocated_registers, state);

        for (const auto [index, cur_elem] :
             elems_ | std::views::enumerate |
                 std::views::drop(elem_index_with_lea)) {

            const size_t elem_index{static_cast<size_t>(index)};

            // advance from the previous element to the next if this is not the
            // starting point want to find the offset from "lea" to the
            // requested element address

            if (elem_index != elem_index_with_lea) {

                state.accumulated_offset = add_address_offset(
                    state.accumulated_offset,
                    address_offset(toc::field_offset_in_type(
                        *value_type, cur_elem.name_tk.text())));

                // add another element to the path
                path.push_back('.');
                path += cur_elem.name_tk.text();
            }

            const ident_info cur_info{tc.make_ident_info(src_loc_tk, path)};

            const bool is_last_elem{elem_index == elems_.size() - 1};

            value_type = &cur_info.type_ref();

            // an unindexed element only needs a possible range bounds check
            if (not cur_elem.array_index_expr) {

                if (cur_info.is_array and is_last_elem and
                    not reg_count.is_empty()) {

                    emit_bounds_check(tc, indent, src_loc_tk, reg_count,
                                      cur_info.array_len, true);

                    // note: 'reg_count' is a register that contains a count so
                    //       bounds checking is done for a span; used in
                    //       'array_copy' and similar operations that will
                    //       iterate of a range
                }

                continue;
            }

            // try to do the last part of the address calculation as a scaled
            // index register

            if (is_last_elem and tc.machine().can_lower_index_scale(
                                     cur_info.type_ref().size_bytes())) {

                return compile_final_index(
                    tc, indent, src_loc_tk, allocated_registers, state,
                    *cur_elem.array_index_expr, cur_info, reg_count);
            }

            // earlier indices and unsupported scales need a computed base

            state.reg_offset =
                prepare_index_base(tc, indent, src_loc_tk, allocated_registers,
                                   state, address_register);

            add_array_index(tc, indent, src_loc_tk, state.reg_offset,
                            *cur_elem.array_index_expr, cur_info,
                            is_last_elem ? reg_count : operand{});
        }

        return finish_lea(tc, indent, src_loc_tk, allocated_registers, state,
                          *value_type);
    }

  private:
    [[nodiscard]] static auto
    find_lea_start(const std::span<const operand> leas) -> size_t {
        size_t index{leas.size()};
        while (index != 0) {
            --index;
            if (not leas[index].is_empty()) {
                return index;
            }
        }
        return 0;
    }

    static auto load_pointer_base(toc& tc, const size_t indent,
                                  const token& src_loc_tk,
                                  std::vector<operand>& allocated_registers,
                                  lea_state& state) -> void {

        // a known address needs no load, and ordinary storage is not a pointer
        if (not state.lea.is_empty() or not state.base_info.is_pointer) {
            return;
        }

        machine& x{tc.machine()};

        state.reg_offset =
            x.alloc_scratch_register(src_loc_tk, indent, tc.get_type_address());

        allocated_registers.push_back(state.reg_offset);

        x.copy_value(
            src_loc_tk, indent, state.reg_offset,
            operand::mem(state.base_info.operand, tc.get_type_address()));
    }

    [[nodiscard]] static auto
    compile_final_index(toc& tc, const size_t indent, const token& src_loc_tk,
                        std::vector<operand>& allocated_registers,
                        lea_state& state, const expr_any& index_expr,
                        const ident_info& cur_info, const operand& reg_count)
        -> operand {

        machine& x{tc.machine()};

        // the returned operand needs this index register to stay live
        const operand reg_idx{x.alloc_scratch_register(src_loc_tk, indent,
                                                       tc.get_type_default())};

        allocated_registers.push_back(reg_idx);

        compile_array_index(tc, indent, index_expr, reg_idx, cur_info.array_len,
                            reg_count);

        if (state.reg_offset.is_empty()) {

            state.reg_offset = init_reg_offset(
                tc, indent, src_loc_tk, state.lea, allocated_registers, true,
                true, state.base_info.operand.base_register());
        }

        const int64_t offset{state.offset_pending
                                 ? add_address_offset(state.base_info.offset,
                                                      state.accumulated_offset)
                                 : state.accumulated_offset};

        return operand::mem(
            state.reg_offset.base_register(), reg_idx.base_register(),
            cur_info.type_ref().size_bytes(), offset, cur_info.type_ref());
    }

    static auto
    add_array_index(toc& tc, const size_t indent, const token& src_loc_tk,
                    const operand& reg_offset, const expr_any& index_expr,
                    const ident_info& cur_info, const operand& reg_count)
        -> void {

        machine& x{tc.machine()};

        const operand reg_idx{x.alloc_scratch_register(src_loc_tk, indent,
                                                       tc.get_type_default())};

        compile_array_index(tc, indent, index_expr, reg_idx, cur_info.array_len,
                            reg_count);

        x.scale_index(index_expr.tok(), indent, reg_idx,
                      cur_info.type_ref().size_bytes());

        x.add_subtract(src_loc_tk, indent, '+', reg_offset, reg_idx);
        x.free_scratch_register(src_loc_tk, indent, reg_idx);
    }

    [[nodiscard]] static auto
    finish_lea(toc& tc, const size_t indent, const token& src_loc_tk,
               std::vector<operand>& allocated_registers, lea_state& state,
               const type& value_type) -> operand {

        if (state.reg_offset.is_empty()) {
            state.reg_offset = init_reg_offset(
                tc, indent, src_loc_tk, state.lea, allocated_registers, true,
                false, state.base_info.operand.base_register());
        }

        operand result{operand::mem(
            state.reg_offset.base_register(), state.reg_offset.index_register(),
            state.reg_offset.scale(), state.reg_offset.displacement(),
            value_type)};

        result.increment_offset(state.accumulated_offset);

        // the storage offset must not be counted again after address_of
        if (not state.offset_pending) {
            return result;
        }

        result.increment_offset(state.base_info.offset);

        return result;
    }

    [[nodiscard]] static auto
    prepare_index_base(toc& tc, const size_t indent, const token& src_loc_tk,
                       std::vector<operand>& allocated_registers,
                       lea_state& state, const operand& address_register)
        -> operand {

        machine& x{tc.machine()};

        operand reg_offset{state.reg_offset};
        const ident_info& base_info{state.base_info};

        if (reg_offset.is_empty()) {
            reg_offset = init_reg_offset(tc, indent, src_loc_tk, state.lea,
                                         allocated_registers, false, true,
                                         base_info.operand.base_register());
        }

        // include the storage offset once, using the reserved pointer if given
        if (state.offset_pending) {
            reg_offset = address_register;
            if (reg_offset.is_empty()) {
                reg_offset = x.alloc_scratch_register(src_loc_tk, indent,
                                                      tc.get_type_address());

                allocated_registers.push_back(reg_offset);
            }
            x.address_of(src_loc_tk, indent, reg_offset, base_info.operand);
            state.offset_pending = false;

            return reg_offset;
        }

        // this address does not use the shared storage base directly
        if (reg_offset.is_indexed() or
            reg_offset.base_register() != base_info.operand.base_register()) {
            return reg_offset;
        }

        // preserve the shared storage base before adding an index
        reg_offset =
            x.alloc_scratch_register(src_loc_tk, indent, tc.get_type_address());

        allocated_registers.push_back(reg_offset);
        x.address_of(src_loc_tk, indent, reg_offset,
                     operand::mem(base_info.operand.base_register(), "", 1, 0,
                                  base_info.type_ref()));

        return reg_offset;
    }

    static auto compile_array_index(toc& tc, const size_t indent,
                                    const expr_any& index_expr,
                                    const operand& reg_idx,
                                    const size_t array_count,
                                    const operand& reg_count) -> void {

        machine& x{tc.machine()};

        x.comment(index_expr.tok(), indent, "set array index");

        index_expr.compile(tc, indent,
                           toc::make_ident_info_from_register(reg_idx));

        emit_bounds_check(tc, indent, index_expr.tok(), reg_idx, array_count,
                          not reg_count.is_empty(), reg_count);
    }

    // helper function to emit bounds-checking code
    static auto
    emit_bounds_check(toc& tc, const size_t indent, const token& src_loc_tk,
                      const operand& reg_to_check, const size_t array_count,
                      const bool allow_end, const operand& reg_count = {})
        -> void {

        machine& x{tc.machine()};

        x.check_bounds(src_loc_tk, indent, reg_to_check, array_count, allow_end,
                       reg_count,
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
                    const bool no_changes_to_offset_after_this,
                    const bool will_be_indirect_indexed,
                    const std::string& base_register) -> operand {

        machine& x{tc.machine()};

        if (lea.is_empty()) {
            // without a known address, start from the shared storage base

            return x.make_register_operand(base_register,
                                           tc.get_type_address());
        }

        // reuse the address when it stays unchanged and needs no second index

        if (no_changes_to_offset_after_this and
            not(will_be_indirect_indexed and lea.is_indexed())) {

            return lea;
        }

        const operand index_reg{x.alloc_scratch_register(
            src_loc_tk, indent, tc.get_type_address())};

        allocated_registers.push_back(index_reg);

        // combine the existing address before modifying it or adding an index
        if (lea.is_indexed()) {
            // load 'index_reg' with address that 'lea' currently holds
            x.address_of(src_loc_tk, indent, index_reg, lea);

            return index_reg;
        }

        // 'lea' has no index register

        // preserve the original base by working on a copy

        x.copy_value(src_loc_tk, indent, index_reg,
                     x.make_register_operand(lea.base_register(),
                                             tc.get_type_address()));

        return index_reg;
    }
};
