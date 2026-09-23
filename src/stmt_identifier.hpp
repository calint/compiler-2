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

    // compile_lea
    //   find_start_element -> deepest known address, or root
    //   initialize base from that address, a loaded pointer, or storage
    //   for each element from start_index
    //     append field name and add its offset to the operand displacement
    //     tc.make_ident_info -> current element type and array length
    //     no index expression
    //       check the range count for a final array, when supplied
    //       continue to next element
    //     base already has an index
    //       use the writable base or promote the owned index register
    //       compute_address_in_register -> free the operand's index slot
    //     extend_with_index
    //       compile_checked_index -> evaluate into a reusable index register
    //       attach index with a supported operand scale
    //         unsupported scale -> multiply index explicitly, use scale 1
    //   return memory operand with the final element type
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

        // view the last n elements of lea_path
        const std::span<const operand> known_addresses{
            lea_path.last(elems_.size())};

        // start at the deepest known address, or the root if none exists
        const size_t start_index{find_start_element(known_addresses)};

        std::string path{elems_[start_index].name_tk.text()};

        const ident_info storage{tc.make_ident_info(src_loc_tk, path)};

        const operand& lea{known_addresses[start_index]};

        operand working_base{address_register};
        operand index_register;
        operand base;
        if (not lea.is_empty()) {
            base = lea;
        } else if (storage.is_pointer) {
            base = load_pointer(tc, indent, src_loc_tk, allocated_registers,
                                storage.operand);
            if (working_base.is_empty()) {
                working_base = allocated_registers.back();
            }
        } else {
            base = storage.operand;
        }

        const type* prev_type{};

        for (size_t elem_index{start_index}; elem_index < elems_.size();
             ++elem_index) {

            const ident_elem& cur_elem{elems_[elem_index]};

            // field offsets stay in the operand, not in the base register

            if (elem_index != start_index) {

                path.push_back('.');
                path += cur_elem.name_tk.text();

                const size_t offset{prev_type->field_offset(
                    cur_elem.name_tk, cur_elem.name_tk.text())};

                base.increment_offset(static_cast<int64_t>(offset));
            }

            const ident_info cur_info{tc.make_ident_info(src_loc_tk, path)};

            prev_type = &cur_info.type_ref();

            const bool is_last_elem{elem_index == elems_.size() - 1};

            // an unindexed element only needs a possible range bounds check
            if (not cur_elem.array_index_expr) {

                if (cur_info.is_array and is_last_elem and
                    not reg_count.is_empty()) {

                    check_array_bounds(tc, indent, src_loc_tk, reg_count,
                                       cur_info.array_len, true);
                }

                continue;
            }

            // indexed element

            if (not base.index_register().empty()) {

                if (working_base.is_empty() and not index_register.is_empty()) {
                    working_base = index_register;
                    index_register = {};
                }

                working_base = compute_address_in_register(
                    tc, indent, src_loc_tk, allocated_registers, base,
                    working_base);

                base = operand::mem(working_base, cur_info.type_ref());
            }

            base = extend_with_index(
                tc, cur_elem.array_index_expr->tok(), indent,
                allocated_registers, cur_elem, cur_info,
                is_last_elem ? reg_count : operand{}, base, index_register);
        }

        return operand::mem(base, *prev_type);
    }

  private:
    [[nodiscard]] auto static extend_with_index(
        toc& tc, const token& src_loc_tk, const size_t indent,
        std::vector<operand>& allocated_registers, const ident_elem& cur_elem,
        const ident_info& cur_info, const operand& reg_count,
        const operand& base, operand& index_register) -> operand {

        machine& x{tc.machine()};

        if (index_register.is_empty()) {
            index_register = x.alloc_scratch_register(src_loc_tk, indent,
                                                      tc.get_type_default());

            allocated_registers.push_back(index_register);
        }

        const operand checked_index{compile_checked_index(
            tc, indent, *cur_elem.array_index_expr, index_register,
            cur_info.array_len, reg_count)};

        const size_t type_size{cur_info.type_ref().size_bytes()};

        uint64_t scale{1};

        if (x.can_lower_index_scale(type_size)) {
            scale = type_size;
        } else {
            x.scale_index(src_loc_tk, indent, checked_index, type_size);
        }

        return operand::mem(base.base_register(), checked_index.base_register(),
                            scale, base.displacement(), cur_info.type_ref());
    }

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

    [[nodiscard]] static auto
    load_pointer(toc& tc, const size_t indent, const token& src_loc_tk,
                 std::vector<operand>& allocated_registers,
                 const operand& pointer_slot) -> operand {

        machine& x{tc.machine()};

        const operand pointer_register{x.alloc_scratch_register(
            src_loc_tk, indent, tc.get_type_address())};

        allocated_registers.push_back(pointer_register);

        x.copy_value(src_loc_tk, indent, pointer_register,
                     operand::mem(pointer_slot, tc.get_type_address()));

        return operand::mem(pointer_register, tc.get_type_address());
    }

    [[nodiscard]] static auto compute_address_in_register(
        toc& tc, const size_t indent, const token& src_loc_tk,
        std::vector<operand>& allocated_registers, const operand& address,
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
                          const size_t array_length, const operand& range_count)
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
};
