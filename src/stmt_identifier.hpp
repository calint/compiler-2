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

    // builds a memory operand for the identifier, emitting address calculations
    // as needed and adding allocated registers to the vector. its form depends
    // on the existing "lea", storage offset, pointer indirection, and whether
    // the index scale is encodable. on x86 this may be e.g. [rbp + r15 * 4 +
    // 24] or [r14 + 28] with a computed base in r14 or simply [r13]
    [[nodiscard]] auto compile_lea(
        toc& tc, const size_t indent, const token& src_loc_tk,
        std::vector<operand>& allocated_registers, const operand& reg_count,
        const std::span<const operand> lea_path) const -> operand override {

        // align the full lea path with this identifier's elements
        const std::span<const operand> leas{lea_path.last(elems_.size())};

        // start from the first known address, or the root if none exists
        size_t elem_index_with_lea{leas.size()};
        operand lea;
        while (elem_index_with_lea) {
            --elem_index_with_lea;
            if (not leas[elem_index_with_lea].is_empty()) {
                lea = leas[elem_index_with_lea];
                break;
            }
        }

        // start at an element with "lea" or 0 when no "lea" found
        std::string path{elems_[elem_index_with_lea].name_tk.text()};
        const ident_info base_info{tc.make_ident_info(src_loc_tk, path)};
        const type* value_type{&base_info.type_ref()};

        // working address base: a storage register, existing lea, or scratch
        // register holding a loaded/computed address; empty until initialized.
        // field offsets and any pending storage offset are added separately.
        operand reg_offset;

        bool offset_pending{lea.is_empty() and not base_info.is_pointer};
        // note: offset_pending means base_info.offset has not yet been
        //       included in the address. starting from rbp or rbx alone still
        //       requires this offset to reach the variable's storage.
        //
        //       while pending, add it to the final operand's displacement,
        //       e.g. [rbx + r15 * 4 + 24]. if address_of first incorporates
        //       it into reg_offset, clear the flag to avoid adding it twice.
        //
        //       an existing lea already includes the storage offset. for a
        //       pointer-backed variable, the offset locates the pointer slot;
        //       the load below uses it to obtain the object's address. neither
        //       case needs the offset added to the resulting address.
        //
        //       the flag tracks only base_info.offset. field offsets in
        //       accumulated_offset and scaled array indices still apply
        //       regardless of whether it is pending.

        int64_t accumulated_offset{};

        const size_t elem_count{elems_.size()};

        machine& x{tc.machine()};

        // load the object's address when the base is a pointer slot
        if (lea.is_empty() and base_info.is_pointer) {
            reg_offset = x.alloc_scratch_register(src_loc_tk, indent,
                                                  tc.get_type_address());

            allocated_registers.push_back(reg_offset);

            x.copy_value(
                src_loc_tk, indent, reg_offset,
                operand::mem(base_info.operand, tc.get_type_address()));
        }

        for (const auto [index, cur_elem] :
             elems_ | std::views::enumerate |
                 std::views::drop(elem_index_with_lea)) {

            const size_t elem_index{static_cast<size_t>(index)};

            // advance from the previous element into this field
            if (elem_index != elem_index_with_lea) {
                accumulated_offset = add_address_offset(
                    accumulated_offset,
                    address_offset(toc::get_field_offset_in_type(
                        *value_type, cur_elem.name_tk.text())));

                path.push_back('.');
                path += cur_elem.name_tk.text();
            }

            const ident_info cur_info{tc.make_ident_info(src_loc_tk, path)};
            const size_t type_size_bytes{cur_info.type_ref().size_bytes()};
            const bool is_last_elem{elem_index == elem_count - 1};
            value_type = &cur_info.type_ref();

            // an unindexed element only needs a possible range bounds check
            if (not cur_elem.array_index_expr) {
                if (is_last_elem and not reg_count.is_empty() and
                    cur_info.is_array) {

                    emit_bounds_check(tc, indent, src_loc_tk, reg_count,
                                      cur_info.array_len, true);
                }

                continue;
            }

            // leave the final index scaled in the operand when encodable
            if (is_last_elem and x.can_encode_index_scale(type_size_bytes)) {
                const operand reg_idx{x.alloc_scratch_register(
                    src_loc_tk, indent, tc.get_type_default())};

                allocated_registers.push_back(reg_idx);

                compile_array_index(tc, indent, *cur_elem.array_index_expr,
                                    reg_idx, cur_info.array_len, reg_count);

                if (reg_offset.is_empty()) {
                    reg_offset = init_reg_offset(
                        tc, indent, src_loc_tk, lea, allocated_registers, true,
                        true, base_info.operand.base_register());
                }

                const int64_t offset{
                    offset_pending ? add_address_offset(base_info.offset,
                                                        accumulated_offset)
                                   : accumulated_offset};

                return operand::mem(
                    reg_offset.base_register(), reg_idx.base_register(),
                    static_cast<uint8_t>(type_size_bytes), offset, *value_type);
            }

            // prepare a writable base for index arithmetic
            if (reg_offset.is_empty()) {
                reg_offset = init_reg_offset(tc, indent, src_loc_tk, lea,
                                             allocated_registers, false, true,
                                             base_info.operand.base_register());
            }

            if (offset_pending) {
                const operand offset_register{x.alloc_scratch_register(
                    src_loc_tk, indent, tc.get_type_address())};

                allocated_registers.push_back(offset_register);
                reg_offset = offset_register;
                x.address_of(src_loc_tk, indent, reg_offset, base_info.operand);
                offset_pending = false;

            } else if (not reg_offset.is_indexed() and
                       reg_offset.base_register() ==
                           base_info.operand.base_register()) {

                const operand offset_register{x.alloc_scratch_register(
                    src_loc_tk, indent, tc.get_type_address())};

                allocated_registers.push_back(offset_register);
                reg_offset = offset_register;

                // copy the shared storage base before modifying it
                x.address_of(src_loc_tk, indent, reg_offset,
                             operand::mem(base_info.operand.base_register(), "",
                                          1, 0, base_info.type_ref()));
            }

            // add the scaled index to the working address
            const operand reg_idx{x.alloc_scratch_register(
                src_loc_tk, indent, tc.get_type_default())};

            compile_array_index(tc, indent, *cur_elem.array_index_expr, reg_idx,
                                cur_info.array_len,
                                is_last_elem ? reg_count : operand{});

            x.scale_index(cur_elem.array_index_expr->tok(), indent, reg_idx,
                          type_size_bytes);

            x.add_subtract(src_loc_tk, indent, '+', reg_offset, reg_idx);
            x.free_scratch_register(src_loc_tk, indent, reg_idx);
        }

        // finish with field offsets and any storage offset not yet included
        if (reg_offset.is_empty()) {
            reg_offset = init_reg_offset(tc, indent, src_loc_tk, lea,
                                         allocated_registers, true, false,
                                         base_info.operand.base_register());
        }

        operand op{operand::mem(reg_offset.base_register(),
                                reg_offset.index_register(), reg_offset.scale(),
                                reg_offset.displacement(), *value_type)};

        op.increment_offset(accumulated_offset);

        if (offset_pending) {
            op.increment_offset(base_info.offset);
        }

        return op;
    }

  private:
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
                src_loc_tk, indent, tc.get_type_address())};

            allocated_registers.push_back(index_reg);

            // changes will be made to the register so return an allocated
            // register
            if (lea.is_indexed()) {
                x.address_of(src_loc_tk, indent, index_reg, lea);
            } else {
                x.copy_value(src_loc_tk, indent, index_reg,
                             x.make_register_operand(lea.base_register(),
                                                     tc.get_type_address()));
            }

            return index_reg;
        }

        return tc.machine().make_register_operand(base_register,
                                                  tc.get_type_address());
    }
};
