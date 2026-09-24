#pragma once

#include <cstddef>
#include <cstdint>
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
    std::vector<token> elem_delims_tk_;
    std::string path_as_string_;
    size_t array_count_{};
    bool is_array_{};
    bool is_indexed_{};

    // bytes of the root variable accessed by this path
    field_coverage::range access_range_;

    // false when a runtime index leaves the element unknown
    bool is_exact_access_{};

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

        resolve_access_range(tc);
    }

    stmt_identifier() = default;

    [[nodiscard]] auto first_token() const -> const token& {
        return elems_[0].name_tk;
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        return path_as_string_;
    }

    [[nodiscard]] auto is_indexed() const -> bool override {
        return is_indexed_;
    }

    [[nodiscard]] auto is_identifier() const -> bool override { return true; }

    auto assert_var_not_used(const std::string_view var,
                             const field_coverage& assigned) const
        -> void override {

        // a path such as 'p.y' reads its root variable
        if (first_token().is_text(var) and not assigned.covers(access_range_)) {
            throw_uninitialized(first_token(), var);
        }

        assert_indexes_not_used(var, assigned);
    }

    // index expressions are read even when the path is written
    auto assert_indexes_not_used(const std::string_view var,
                                 const field_coverage& assigned) const -> void {

        for (const ident_elem& e : elems_) {
            if (e.array_index_expr) {
                e.array_index_expr->assert_var_not_used(var, assigned);
            }
        }
    }

    // a runtime index does not prove which element is written
    auto record_assignment(assignment_flow& flow) const -> void {
        if (not first_token().is_text(flow.var) or not is_exact_access_) {
            return;
        }

        flow.assigned.add(access_range_);
    }

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

    // * using 'lea_path' which depends on the call-stack builds and
    //    accessor operand to this identifier
    // * scratch registers used to build the indexing are added to
    //   'allocated_registers'
    // * preferred starting point to operand building is specified in
    //   'address_register'
    // * if identifier is a reference to an array and a span operation is
    //   constructed then the range is specified in 'reg_count'
    // * the operand has the form: e.g. rbp + 4 * r15 + 248
    // * the 'machine' interface describes what scalings are supported
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

        const operand& known_address{known_addresses[start_index]};

        operand writable_base{address_register};
        operand index_register;
        operand address;

        if (not known_address.is_empty()) {
            address = known_address;
        } else if (storage.is_pointer) {

            address = load_pointer(tc, indent, src_loc_tk, allocated_registers,
                                   storage.operand);

            if (writable_base.is_empty()) {
                // 'load_pointer' just appended its scratch register; nothing
                // has been appended since; reuse that private pointer copy
                // as a writable base; it stays allocated until caller cleanup
                writable_base = allocated_registers.back();
            }
        } else {
            address = storage.operand;
        }

        const type* parent_type{};

        for (size_t elem_index{start_index}; elem_index < elems_.size();
             ++elem_index) {

            const ident_elem& cur_elem{elems_[elem_index]};

            // field offsets stay in the operand, not in the base register

            if (elem_index != start_index) {

                path.push_back('.');
                path += cur_elem.name_tk.text();

                const size_t offset{parent_type->field_offset(
                    cur_elem.name_tk, cur_elem.name_tk.text())};

                address.increment_offset(static_cast<int64_t>(offset));
            }

            const ident_info cur_info{tc.make_ident_info(src_loc_tk, path)};

            parent_type = &cur_info.type_ref();

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

            if (not address.index_register().empty()) {

                if (writable_base.is_empty() and
                    not index_register.is_empty()) {
                    writable_base = index_register;
                    index_register = {};
                }

                writable_base =
                    fold_address(tc, indent, src_loc_tk, allocated_registers,
                                 address, writable_base);

                address = operand::mem(writable_base, cur_info.type_ref());
            }

            address = add_index(tc, cur_elem.array_index_expr->tok(), indent,
                                allocated_registers, cur_elem, cur_info,
                                is_last_elem ? reg_count : operand{}, address,
                                index_register);
        }

        return operand::mem(address, *parent_type);
    }

  private:
    auto resolve_access_range(toc& tc) -> void {
        if (tc.is_func(path_as_string_)) {
            return;
        }

        is_exact_access_ = true;

        std::string path;
        const type* parent_type{};

        for (const ident_elem& elem : elems_) {
            const bool is_root{path.empty()};
            if (not is_root) {
                path.push_back('.');
            }
            path += elem.name_tk.text();

            const ident_info info{tc.make_ident_info(elem.name_tk, path)};

            // inside an unknown element only the types are followed
            if (is_exact_access_) {
                const size_t field_offset{
                    is_root ? 0
                            : parent_type->field_offset(elem.name_tk,
                                                        elem.name_tk.text())};

                access_range_ = {
                    .offset{access_range_.offset + field_offset},
                    .size_bytes{storage_size_bytes(info)},
                };
            }

            parent_type = &info.type_ref();

            if (elem.array_index_expr and is_exact_access_) {
                narrow_to_element(tc, *elem.array_index_expr, info);
            }
        }
    }

    // an unknown element leaves the whole array as the accessed range
    auto narrow_to_element(toc& tc, const expr_any& index_expr,
                           const ident_info& array_info) -> void {

        const std::optional<int64_t> index{constant_index(tc, index_expr)};
        if (not index or *index < 0 or
            std::cmp_greater_equal(*index, array_info.array_len)) {

            is_exact_access_ = false;

            return;
        }

        const size_t element_size_bytes{array_info.type_ref().size_bytes()};

        access_range_ = {
            .offset{access_range_.offset +
                    (static_cast<size_t>(*index) * element_size_bytes)},
            .size_bytes{element_size_bytes},
        };
    }

    [[nodiscard]] static auto constant_index(toc& tc,
                                             const expr_any& index_expr)
        -> std::optional<int64_t> {

        if (index_expr.is_expression()) {
            return std::nullopt;
        }

        const ident_info info{tc.make_ident_info(index_expr)};
        if (not info.is_const()) {
            return std::nullopt;
        }

        return index_expr.get_unary_ops().evaluate_constant(info.const_value);
    }

    [[nodiscard]] static auto storage_size_bytes(const ident_info& info)
        -> size_t {

        if (not info.is_array) {
            return info.type_ref().size_bytes();
        }

        return multiply_storage_size(info.type_ref().size_bytes(),
                                     info.array_len);
    }

    [[nodiscard]] auto static add_index(
        toc& tc, const token& src_loc_tk, const size_t indent,
        std::vector<operand>& allocated_registers, const ident_elem& cur_elem,
        const ident_info& cur_info, const operand& reg_count,
        const operand& address, operand& index_register) -> operand {

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

        return operand::mem(address.base_register(),
                            checked_index.base_register(), scale,
                            address.displacement(), cur_info.type_ref());
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

    [[nodiscard]] static auto
    fold_address(toc& tc, const size_t indent, const token& src_loc_tk,
                 std::vector<operand>& allocated_registers,
                 const operand& address, operand destination_register)
        -> operand {

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
