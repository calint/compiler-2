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

    struct address_state {
        operand known_address;
        ident_info storage;
        operand base;
        // only the storage offset is pending; field offsets stay separate
        bool storage_offset_pending{};
        int64_t field_offset{};
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
    //
    // compile_lea
    //   Select deepest known address, otherwise root storage
    //   Load stored pointer if needed
    //   Walk remaining elements
    //     Accumulate constant field offsets
    //     No array index
    //       Check range when required
    //       Continue
    //     Final index with supported scale
    //       Compile and bounds-check index
    //       Reuse base, or compute complex address into scratch
    //       Return [base + index * scale + displacement]
    //     Other index
    //       Prepare writable base
    //         Storage offset pending -> compute storage address
    //         Known address -> copy into scratch
    //         Shared storage base -> preserve before modifying
    //         Existing private base -> reuse
    //       Compile/check index, scale, add, free temporary
    //   Build memory operand with remaining constant offsets
    //
    [[nodiscard]] auto compile_lea(toc& tc, const size_t indent,
                                   const token& src_loc_tk,
                                   std::vector<operand>& allocated_registers,
                                   const operand& reg_count,
                                   const std::span<const operand> lea_path,
                                   const operand& address_register) const
        -> operand override {

        // align the full lea path with this identifier's elements
        const std::span<const operand> known_addresses{
            lea_path.last(elems_.size())};

        // start at the deepest known address, or the root if none exists
        const size_t start_index{find_start_element(known_addresses)};

        std::string path{elems_[start_index].name_tk.text()};

        const ident_info storage{tc.make_ident_info(src_loc_tk, path)};

        const type* value_type{&storage.type_ref()};

        // a known address or a loaded pointer already accounts for storage
        // field offsets remain separate until the final operand is built

        address_state state{
            .known_address{known_addresses[start_index]},
            .storage{storage},
            .base{},
            .storage_offset_pending{known_addresses[start_index].is_empty() and
                                    not storage.is_pointer},
            .field_offset{},
        };

        load_pointer_if_needed(tc, indent, src_loc_tk, allocated_registers,
                               state);

        for (size_t elem_index{start_index}; elem_index < elems_.size();
             ++elem_index) {

            const ident_elem& cur_elem{elems_[elem_index]};

            // field offsets stay in the operand, not in the base register

            if (elem_index != start_index) {

                state.field_offset = add_address_offset(
                    state.field_offset,
                    address_offset(toc::field_offset_in_type(
                        *value_type, cur_elem.name_tk.text())));

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

                    check_array_bounds(tc, indent, src_loc_tk, reg_count,
                                       cur_info.array_len, true);
                }

                continue;
            }

            // try to do the last part of the address calculation as a scaled
            // index register

            if (is_last_elem and tc.machine().can_lower_index_scale(
                                     cur_info.type_ref().size_bytes())) {

                return compile_indexed_operand(
                    tc, indent, src_loc_tk, allocated_registers, state,
                    *cur_elem.array_index_expr, cur_info, reg_count);
            }

            // earlier indices and unsupported scales need a computed base

            state.base = prepare_writable_base(tc, indent, src_loc_tk,
                                               allocated_registers, state,
                                               address_register);

            add_index_to_base(tc, indent, src_loc_tk, state.base,
                              *cur_elem.array_index_expr, cur_info,
                              is_last_elem ? reg_count : operand{});
        }

        return make_memory_operand(tc, state, *value_type);
    }

  private:
    [[nodiscard]] static auto
    find_start_element(const std::span<const operand> known_addresses)
        -> size_t {
        size_t index{known_addresses.size()};
        while (index != 0) {
            --index;
            if (not known_addresses[index].is_empty()) {
                return index;
            }
        }

        return 0;
    }

    static auto
    load_pointer_if_needed(toc& tc, const size_t indent,
                           const token& src_loc_tk,
                           std::vector<operand>& allocated_registers,
                           address_state& state) -> void {

        // a known address needs no load, and ordinary storage is not a pointer
        if (not state.known_address.is_empty() or
            not state.storage.is_pointer) {
            return;
        }

        machine& x{tc.machine()};

        state.base =
            x.alloc_scratch_register(src_loc_tk, indent, tc.get_type_address());

        allocated_registers.push_back(state.base);

        x.copy_value(
            src_loc_tk, indent, state.base,
            operand::mem(state.storage.operand, tc.get_type_address()));
    }

    [[nodiscard]] static auto compile_indexed_operand(
        toc& tc, const size_t indent, const token& src_loc_tk,
        std::vector<operand>& allocated_registers, address_state& state,
        const expr_any& index_expr, const ident_info& array_info,
        const operand& range_count) -> operand {

        machine& x{tc.machine()};

        // the returned operand needs this index register to stay live
        const operand index_register{x.alloc_scratch_register(
            src_loc_tk, indent, tc.get_type_default())};

        allocated_registers.push_back(index_register);

        compile_checked_index(tc, indent, index_expr, index_register,
                              array_info.array_len, range_count);

        if (state.base.is_empty()) {
            state.base = starting_address(tc, state);
            if (not state.base.index_register().empty() or
                state.base.displacement() != 0) {

                state.base = copy_address_to_register(
                    tc, indent, src_loc_tk, allocated_registers, state.base);
            }
        }

        const int64_t displacement{
            state.storage_offset_pending
                ? add_address_offset(state.storage.offset, state.field_offset)
                : state.field_offset};

        return operand::mem(state.base.base_register(),
                            index_register.base_register(),
                            array_info.type_ref().size_bytes(), displacement,
                            array_info.type_ref());
    }

    static auto
    add_index_to_base(toc& tc, const size_t indent, const token& src_loc_tk,
                      const operand& base_register, const expr_any& index_expr,
                      const ident_info& array_info, const operand& range_count)
        -> void {

        machine& x{tc.machine()};

        const operand index_register{x.alloc_scratch_register(
            src_loc_tk, indent, tc.get_type_default())};

        compile_checked_index(tc, indent, index_expr, index_register,
                              array_info.array_len, range_count);

        x.scale_index(index_expr.tok(), indent, index_register,
                      array_info.type_ref().size_bytes());

        x.add_subtract(src_loc_tk, indent, '+', base_register, index_register);
        x.free_scratch_register(src_loc_tk, indent, index_register);
    }

    [[nodiscard]] static auto make_memory_operand(toc& tc,
                                                  const address_state& state,
                                                  const type& value_type)
        -> operand {

        const operand base{state.base.is_empty() ? starting_address(tc, state)
                                                 : state.base};

        operand result{operand::mem(base, value_type)};

        result.increment_offset(state.field_offset);

        // the storage offset must not be counted again after address_of
        if (not state.storage_offset_pending) {
            return result;
        }

        result.increment_offset(state.storage.offset);

        return result;
    }

    [[nodiscard]] static auto
    prepare_writable_base(toc& tc, const size_t indent, const token& src_loc_tk,
                          std::vector<operand>& allocated_registers,
                          address_state& state,
                          const operand& reserved_address_register) -> operand {

        machine& x{tc.machine()};

        operand base_register{state.base};
        const ident_info& storage{state.storage};

        // include the storage offset once, using the reserved pointer if given
        if (state.storage_offset_pending) {
            base_register = reserved_address_register;

            if (base_register.is_empty()) {

                base_register = x.alloc_scratch_register(src_loc_tk, indent,
                                                         tc.get_type_address());

                allocated_registers.push_back(base_register);
            }

            x.address_of(src_loc_tk, indent, base_register, storage.operand);

            state.storage_offset_pending = false;

            return base_register;
        }

        if (base_register.is_empty()) {
            base_register = copy_address_to_register(tc, indent, src_loc_tk,
                                                     allocated_registers,
                                                     state.known_address);
        }

        // this address does not use the shared storage base directly
        if (base_register.base_register() != storage.operand.base_register()) {
            return base_register;
        }

        // preserve the shared storage base before adding an index
        base_register =
            x.alloc_scratch_register(src_loc_tk, indent, tc.get_type_address());

        allocated_registers.push_back(base_register);
        x.address_of(src_loc_tk, indent, base_register,
                     operand::mem(storage.operand.base_register(), "", 1, 0,
                                  storage.type_ref()));

        return base_register;
    }

    static auto compile_checked_index(toc& tc, const size_t indent,
                                      const expr_any& index_expr,
                                      const operand& index_register,
                                      const size_t array_length,
                                      const operand& range_count) -> void {

        machine& x{tc.machine()};

        x.comment(index_expr.tok(), indent, "set array index");

        index_expr.compile(tc, indent,
                           toc::make_ident_info_from_register(index_register));

        check_array_bounds(tc, indent, index_expr.tok(), index_register,
                           array_length, not range_count.is_empty(),
                           range_count);
    }

    static auto
    check_array_bounds(toc& tc, const size_t indent, const token& src_loc_tk,
                       const operand& index_or_count, const size_t array_length,
                       const bool allow_end, const operand& range_count = {})
        -> void {

        machine& x{tc.machine()};

        x.check_bounds(src_loc_tk, indent, index_or_count, array_length,
                       allow_end, range_count,
                       {
                           .upper{tc.is_bounds_check_upper()},
                           .lower{tc.is_bounds_check_lower()},
                           .with_line{tc.is_bounds_check_with_line()},
                       });
    }

    [[nodiscard]] static auto starting_address(toc& tc,
                                               const address_state& state)
        -> operand {
        if (not state.known_address.is_empty()) {
            return state.known_address;
        }

        const machine& x{tc.machine()};

        return x.make_register_operand(state.storage.operand.base_register(),
                                       tc.get_type_address());
    }

    [[nodiscard]] static auto
    copy_address_to_register(toc& tc, const size_t indent,
                             const token& src_loc_tk,
                             std::vector<operand>& allocated_registers,
                             const operand& address) -> operand {

        machine& x{tc.machine()};

        const operand address_register{x.alloc_scratch_register(
            src_loc_tk, indent, tc.get_type_address())};

        allocated_registers.push_back(address_register);

        if (not address.index_register().empty() or
            address.displacement() != 0) {

            x.address_of(src_loc_tk, indent, address_register, address);

            return address_register;
        }

        x.copy_value(src_loc_tk, indent, address_register,
                     x.make_register_operand(address.base_register(),
                                             tc.get_type_address()));

        return address_register;
    }
};
