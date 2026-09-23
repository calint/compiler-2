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
    //   find_start_element -> start_index
    //   tc.make_ident_info -> starting storage and type
    //   initialize base, base_is_writable, field_offset, storage_offset_pending
    //   no known address and storage holds a pointer
    //     load_pointer -> base
    //     mark base writable
    //   base still empty
    //     x.make_register_operand -> shared storage base
    //
    //   for each element from start_index
    //     not the starting element
    //       toc::field_offset_in_type -> add to field_offset
    //       append field name to path
    //     tc.make_ident_info -> cur_info; update value_type
    //
    //     no index expression
    //       final element is an array and reg_count is present
    //         check_array_bounds(count, allow_end=true) -> back here
    //       continue to next element
    //
    //     final element and x.can_lower_index_scale accepts its element size
    //       allocate index register; keep it in allocated_registers
    //       compile_checked_index -> checked_index
    //       base has an index or displacement
    //         copy_address_to_register -> plain base register
    //       combine field_offset with storage offset if still pending
    //       RETURN from compile_lea: [base + checked_index * size + offset]
    //
    //     earlier index or unsupported final scale
    //       storage offset is NOT pending
    //         base is not writable
    //           copy_address_to_register -> base
    //         resulting base is still the shared storage register
    //           compute_address_in_register(no destination) -> new base
    //       storage offset IS pending
    //         compute_address_in_register(storage, address_register) -> base
    //         clear storage_offset_pending
    //       mark base writable
    //       add_index_to_base -> base with scaled index added
    //         pass reg_count only for the final element
    //       loop back to next element
    //
    //   after the loop
    //     operand::mem -> memory view of base, preserving index/displacement
    //     add field_offset and any pending storage offset to that view
    //     RETURN from compile_lea: completed memory operand
    //
    // helper calls below return to their caller, not out of compile_lea
    //
    // find_start_element
    //   scan known addresses backwards
    //     nonempty address found -> RETURN its element index
    //   none found -> RETURN 0 (root)
    //
    // load_pointer
    //   allocate and retain pointer register
    //   x.copy_value -> load pointer from its memory slot
    //   RETURN pointer register
    //
    // copy_address_to_register
    //   allocate and retain address register
    //   source has an index or displacement
    //     x.address_of -> compute full address, without loading memory
    //     RETURN address register
    //   x.copy_value -> copy the plain base register
    //   RETURN address register
    //
    // compute_address_in_register
    //   no destination supplied -> allocate and retain one
    //   x.address_of -> compute address into destination, without loading
    //   memory RETURN destination register
    //
    // add_index_to_base
    //   allocate temporary index register
    //   compile_checked_index -> checked_index
    //   x.scale_index -> multiply index by element size
    //   x.add_subtract -> add scaled index to the runtime base register
    //   free temporary index register
    //   RETURN base register
    //
    // compile_checked_index
    //   index_expr.compile -> evaluate expression into supplied index register
    //   check_array_bounds -> check index, or range when range_count is present
    //   RETURN index register
    //
    // check_array_bounds
    //   x.check_bounds -> emit checks selected by compiler options
    //   return to caller (void)
    //
    // retained registers stay live after compile_lea returns
    // its caller frees allocated_registers after consuming the memory operand
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

        machine& x{tc.machine()};

        operand base{known_addresses[start_index]};

        const bool has_known_address{not base.is_empty()};

        bool base_is_writable{};

        int64_t field_offset{};

        bool storage_offset_pending{not has_known_address and
                                    not storage.is_pointer};

        if (not has_known_address and storage.is_pointer) {

            base = load_pointer(tc, indent, src_loc_tk, allocated_registers,
                                storage.operand);

            base_is_writable = true;
        }

        if (base.is_empty()) {
            base = x.make_register_operand(storage.operand.base_register(),
                                           tc.get_type_address());
        }

        for (size_t elem_index{start_index}; elem_index < elems_.size();
             ++elem_index) {

            const ident_elem& cur_elem{elems_[elem_index]};

            // field offsets stay in the operand, not in the base register

            if (elem_index != start_index) {

                field_offset = add_address_offset(
                    field_offset, address_offset(toc::field_offset_in_type(
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

            if (is_last_elem and
                x.can_lower_index_scale(cur_info.type_ref().size_bytes())) {

                const operand index_register{x.alloc_scratch_register(
                    src_loc_tk, indent, tc.get_type_default())};

                allocated_registers.push_back(index_register);

                const operand checked_index{compile_checked_index(
                    tc, indent, *cur_elem.array_index_expr, index_register,
                    cur_info.array_len, reg_count)};

                if (not base.index_register().empty() or
                    base.displacement() != 0) {
                    base = copy_address_to_register(tc, indent, src_loc_tk,
                                                    allocated_registers, base);
                }

                const int64_t displacement{
                    storage_offset_pending
                        ? add_address_offset(storage.offset, field_offset)
                        : field_offset};

                return operand::mem(base.base_register(),
                                    checked_index.base_register(),
                                    cur_info.type_ref().size_bytes(),
                                    displacement, cur_info.type_ref());
            }

            // earlier indices and unsupported scales need a computed base

            if (not storage_offset_pending) {
                if (not base_is_writable) {
                    base = copy_address_to_register(tc, indent, src_loc_tk,
                                                    allocated_registers, base);
                }

                if (base.base_register() == storage.operand.base_register()) {

                    base = compute_address_in_register(
                        tc, indent, src_loc_tk, allocated_registers,
                        operand::mem(storage.operand.base_register(), "", 1, 0,
                                     storage.type_ref()),
                        {});
                }
            }

            if (storage_offset_pending) {

                base = compute_address_in_register(
                    tc, indent, src_loc_tk, allocated_registers,
                    storage.operand, address_register);

                storage_offset_pending = false;
            }

            base_is_writable = true;

            base = add_index_to_base(tc, indent, src_loc_tk, base,
                                     *cur_elem.array_index_expr, cur_info,
                                     is_last_elem ? reg_count : operand{});
        }

        operand result{operand::mem(base, *value_type)};

        result.increment_offset(field_offset);

        if (storage_offset_pending) {
            result.increment_offset(storage.offset);
        }

        return result;
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

    // NOLINTBEGIN(performance-unnecessary-value-param)
    [[nodiscard]] static auto
    load_pointer(toc& tc, const size_t indent, const token& src_loc_tk,
                 std::vector<operand>& allocated_registers,
                 const operand pointer_slot) -> operand {

        machine& x{tc.machine()};

        const operand pointer_register{x.alloc_scratch_register(
            src_loc_tk, indent, tc.get_type_address())};

        allocated_registers.push_back(pointer_register);

        x.copy_value(src_loc_tk, indent, pointer_register,
                     operand::mem(pointer_slot, tc.get_type_address()));

        return pointer_register;
    }

    [[nodiscard]] static auto
    add_index_to_base(toc& tc, const size_t indent, const token& src_loc_tk,
                      operand base_register, const expr_any& index_expr,
                      const ident_info& array_info, const operand range_count)
        -> operand {

        machine& x{tc.machine()};

        const operand index_register{x.alloc_scratch_register(
            src_loc_tk, indent, tc.get_type_default())};

        const operand checked_index{
            compile_checked_index(tc, indent, index_expr, index_register,
                                  array_info.array_len, range_count)};

        x.scale_index(index_expr.tok(), indent, checked_index,
                      array_info.type_ref().size_bytes());

        x.add_subtract(src_loc_tk, indent, '+', base_register, checked_index);

        x.free_scratch_register(src_loc_tk, indent, index_register);

        return base_register;
    }

    [[nodiscard]] static auto compute_address_in_register(
        toc& tc, const size_t indent, const token& src_loc_tk,
        std::vector<operand>& allocated_registers, const operand address,
        operand destination_register) -> operand {

        machine& x{tc.machine()};

        if (destination_register.is_empty()) {

            destination_register = x.alloc_scratch_register(
                src_loc_tk, indent, tc.get_type_address());

            allocated_registers.push_back(destination_register);
        }

        x.address_of(src_loc_tk, indent, destination_register, address);

        return destination_register;
    }

    [[nodiscard]] static auto
    compile_checked_index(toc& tc, const size_t indent,
                          const expr_any& index_expr, operand index_register,
                          const size_t array_length, const operand range_count)
        -> operand {

        machine& x{tc.machine()};

        x.comment(index_expr.tok(), indent, "set array index");

        index_expr.compile(tc, indent,
                           toc::make_ident_info_from_register(index_register));

        check_array_bounds(tc, indent, index_expr.tok(), index_register,
                           array_length, not range_count.is_empty(),
                           range_count);

        return index_register;
    }

    static auto
    check_array_bounds(toc& tc, const size_t indent, const token& src_loc_tk,
                       const operand index_or_count, const size_t array_length,
                       const bool allow_end, const operand range_count = {})
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

    [[nodiscard]] static auto
    copy_address_to_register(toc& tc, const size_t indent,
                             const token& src_loc_tk,
                             std::vector<operand>& allocated_registers,
                             const operand address) -> operand {

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
    // NOLINTEND(performance-unnecessary-value-param)
};
