#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <sstream>
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

    // e.g. '.add' in 'lst.add(x)', the path is the receiver
    token method_dot_tk_;
    token method_name_tk_;

  public:
    stmt_identifier(toc& tc, unary_ops uops, token tk, tokenizer& tz)
        : statement{tk, std::move(uops)}, path_as_string_{tk.text()} {

        if (tk.is_empty()) {
            throw compiler_exception{tz, "expected an identifier"};
        }

        token tk_prv{tk};

        while (true) {
            assert_indexable(tc, tz, tk);
            parse_element(tc, tz, tk);

            if (not extends_path(tc, tz, tk, tk_prv)) {
                break;
            }
        }

        if (not tc.is_func(path_as_string_)) {
            resolve_type(tc, tk_prv);
        }

        resolve_access_range(tc);
    }

    stmt_identifier() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        get_unary_ops().source_to(os);
        path_source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const ident_info src_info{tc.make_scalar_ident_info(*this)};

        if (src_info.is_const()) {
            x.copy_value(tok(), indent, dst_info.operand,
                         make_constant_operand(src_info));

            return;
        }

        std::vector<operand> allocated_registers;

        const operand op{
            tc.get_lea_operand(indent, *this, src_info, allocated_registers),
        };

        x.copy_value(tok(), indent, dst_info.operand, op);

        get_unary_ops().compile(tc, indent, tok(), dst_info.operand);

        x.free_scratch_registers(tok(), indent, allocated_registers);
    }

    [[nodiscard]] auto accessed_range() const
        -> std::optional<field_coverage::range> override {

        return access_range_;
    }

    auto assert_not_narrowed(const toc& tc, const type& dst_type) const
        -> void override {

        // records are copied whole and cannot narrow
        assert(dst_type.is_builtin());

        const ident_info info{tc.make_ident_info(*this)};

        if (info.is_const()) {
            assert_constant_fits(info, dst_type);
            return;
        }

        const type& src_type{info.type_ref()};

        if (not src_type.is_builtin() or
            src_type.size_bytes() <= dst_type.size_bytes()) {

            return;
        }

        throw_narrowed(first_token(), trimmed_source(*this), src_type,
                       dst_type);
    }

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
            lea_path.last(elems_.size()),
        };

        // start at the deepest known address, or the root if none exists
        const size_t start_index{find_start_element(known_addresses)};

        std::string path{elems_.at(start_index).name_tk.text()};

        // registers appended from here on belong to this path and may be
        // overwritten, earlier entries are the caller's
        const size_t owned_from{allocated_registers.size()};

        operand address{
            start_address(tc, indent, src_loc_tk, allocated_registers,
                          known_addresses.at(start_index),
                          tc.make_ident_info(src_loc_tk, path)),
        };

        operand index_register;

        const type* parent_type{};

        for (size_t elem_index{start_index}; elem_index < elems_.size();
             ++elem_index) {

            const ident_elem& cur_elem{elems_.at(elem_index)};

            // field offsets stay in the operand, not in the base register

            if (elem_index != start_index) {

                path.push_back('.');
                path += cur_elem.name_tk.text();

                const size_t offset{
                    parent_type->field_offset(cur_elem.name_tk,
                                              cur_elem.name_tk.text()),
                };

                address.increment_offset(static_cast<int64_t>(offset));
            }

            const ident_info cur_info{tc.make_ident_info(src_loc_tk, path)};

            parent_type = &cur_info.type_ref();

            const bool is_last_elem{elem_index == elems_.size() - 1};

            // an unindexed element only needs a possible range bounds check
            if (not cur_elem.array_index_expr) {
                check_unindexed_range(tc, indent, src_loc_tk, cur_info,
                                      is_last_elem, reg_count);

                continue;
            }

            address = compile_indexed_element(
                tc, indent, src_loc_tk, allocated_registers, owned_from,
                cur_elem, cur_info, is_last_elem ? reg_count : operand{},
                address, address_register, index_register);
        }

        return operand::mem(address, *parent_type);
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        return path_as_string_;
    }

    [[nodiscard]] auto is_array_element() const -> bool override {
        return not elems_.empty() and elems_.back().array_index_expr != nullptr;
    }

    [[nodiscard]] auto is_identifier() const -> bool override { return true; }

    [[nodiscard]] auto is_indexed() const -> bool override {
        return is_indexed_;
    }

    // the low bytes are at the start address of the storage
    [[nodiscard]] auto keeps_low_bits_when_narrowed() const -> bool override {
        return true;
    }

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

        // a path such as 'p.y' reads its root variable
        if (first_token().is_text(var)) {
            reader(first_token(), path_text(), access_range_);
        }

        visit_index_reads(var, reader);
    }

    //
    // class methods
    //

    [[nodiscard]] auto access_range() const -> field_coverage::range {
        return access_range_;
    }

    [[nodiscard]] auto array_count() const -> size_t { return array_count_; }

    // 'use' runs while the scratch registers that build the address are still
    // allocated, then they are freed
    auto
    compile_address(toc& tc, const size_t indent, const token& src_loc_tk,
                    const std::span<const operand> lea_path,
                    const operand& reg_count, const operand& address_register,
                    const std::function_ref<void(const operand&)> use) const
        -> void {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        std::vector<operand> allocated_registers;

        const operand address{
            compile_lea(tc, indent, first_token(), allocated_registers,
                        reg_count, lea_path, address_register),
        };

        use(address);

        x.free_scratch_registers(src_loc_tk, indent, allocated_registers);
    }

    [[nodiscard]] auto first_token() const -> const token& {
        return elems_.at(0).name_tk;
    }

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }

    [[nodiscard]] auto is_exact_access() const -> bool {
        return is_exact_access_;
    }

    [[nodiscard]] auto is_method_receiver() const -> bool {
        return not method_name_tk_.is_empty();
    }

    [[nodiscard]] auto method_dot_token() const -> const token& {
        return method_dot_tk_;
    }

    [[nodiscard]] auto method_name_token() const -> const token& {
        return method_name_tk_;
    }

    auto path_source_to(std::ostream& os) const -> void {
        assert(not elems_.empty());

        elems_.front().source_to(os);
        for (const auto [d, e] :
             std::views::zip(elem_delims_tk_, elems_ | std::views::drop(1))) {

            d.source_to(os);
            e.source_to(os);
        }
    }

    // the path as written, with indexes but without unary operators
    [[nodiscard]] auto path_text() const -> std::string {
        std::stringstream ss;
        path_source_to(ss);

        return statement::trimmed_source(ss.view());
    }

    // a runtime index does not prove which element is written
    auto record_assignment(assignment_flow& flow) const -> void {
        if (not first_token().is_text(flow.var) or not is_exact_access_) {
            return;
        }

        flow.assigned.add(access_range_);
    }

    // index expressions are read even when the path is written
    auto visit_index_reads(const std::string_view var,
                           const read_visitor reader) const -> void {

        for (const ident_elem& e : elems_) {
            if (e.array_index_expr) {
                e.array_index_expr->visit_reads(var, reader);
            }
        }
    }

  private:
    // only signed types exist so a constant must fit the signed range
    auto assert_constant_fits(const ident_info& info,
                              const type& dst_type) const -> void {

        const int64_t value{
            get_unary_ops().evaluate_constant(info.const_value),
        };

        if (fits_size_bytes(value, dst_type.size_bytes())) {
            return;
        }

        throw compiler_exception{
            first_token(),
            std::format("constant '{}' does not fit '{}', use '{}(...)'",
                        trimmed_source(*this), dst_type.name(),
                        dst_type.name())};
    }

    // 'ps.x' does not mean 'ps[0].x'
    auto assert_element_selected(const toc& tc, const token& tk) const -> void {

        if (elems_.back().array_index_expr or tc.is_func(path_as_string_)) {
            return;
        }

        if (not tc.make_ident_info(tk, path_as_string_).is_array) {
            return;
        }

        throw compiler_exception{
            tk, std::format("array '{}' must be indexed", path_as_string_)};
    }

    // the '[' of an element needs an array
    auto assert_indexable(const toc& tc, tokenizer& tz, const token& tk) const
        -> void {

        if (tc.is_func(path_as_string_)) {
            return;
        }

        const ident_info cur_ident_info{
            tc.make_ident_info(tk, path_as_string_),
        };

        if (tz.peek_char_after_whitespace() == '[' and
            not cur_ident_info.is_array) {

            throw compiler_exception{
                tk,
                std::format("cannot index non-array '{}'", path_as_string_)};
        }
    }

    // without a method 'T.m' a following '(' would otherwise be reported as a
    // missing field
    auto assert_not_method_call(const toc& tc, tokenizer& tz,
                                const token& path_tk,
                                const token& name_tk) const -> void {

        if (tc.is_func(path_as_string_)) {
            return;
        }

        if (tz.peek_char_after_whitespace() != '(') {
            return;
        }

        const ident_info info{tc.make_ident_info(path_tk, path_as_string_)};

        throw compiler_exception{
            name_tk, std::format("method '{}' not found in type '{}'",
                                 name_tk.text(), info.type_ref().name())};
    }

    // a '.' followed by a field continues the path with that field, a method
    // name ends it with the path as the receiver; 'tk' becomes the field and
    // 'tk_prv' the token before it
    [[nodiscard]] auto extends_path(toc& tc, tokenizer& tz, token& tk,
                                    token& tk_prv) -> bool {

        const token dot_tk{tz.is_next_char_token('.')};
        if (dot_tk.is_empty()) {
            return false;
        }

        const token next_tk{tz.next_token()};
        if (is_method_name(tc, tz, tk, next_tk)) {
            // the path so far is resolved as the receiver
            method_dot_tk_ = dot_tk;
            method_name_tk_ = next_tk;

            return false;
        }

        assert_element_selected(tc, tk);
        assert_not_method_call(tc, tz, tk, next_tk);

        elem_delims_tk_.emplace_back(dot_tk);
        tk_prv = tk;
        tk = next_tk;
        path_as_string_.push_back('.');
        path_as_string_ += tk.text();

        return true;
    }

    // a field covers its trailing padding so that assigning every field
    // assigns the whole record
    [[nodiscard]] auto field_range(const type& parent_type,
                                   const ident_elem& elem) const
        -> field_coverage::range {

        return {
            .offset{
                access_range_.offset +
                    parent_type.field_offset(elem.name_tk, elem.name_tk.text()),
            },
            .size_bytes{
                parent_type.field_extent_bytes(elem.name_tk,
                                               elem.name_tk.text()),
            },
        };
    }

    // 'name_tk' after the path so far names a method of the path's type
    [[nodiscard]] auto is_method_name(const toc& tc, tokenizer& tz,
                                      const token& path_tk,
                                      const token& name_tk) const -> bool {

        if (tc.is_func(path_as_string_)) {
            return false;
        }

        const ident_info info{tc.make_ident_info(path_tk, path_as_string_)};

        const type& path_type{info.type_ref()};

        if (not tc.is_func(
                std::format("{}.{}", path_type.name(), name_tk.text()))) {

            return false;
        }

        // a method and a field may share a name, only the call has '(' or the
        // type arguments of a generic method
        if (tz.peek_char_after_whitespace() == '(' or
            (tz.peek_char_after_whitespace() == '<' and
             tc.is_generic_func(
                 std::format("{}.{}", path_type.name(), name_tk.text())))) {

            return true;
        }

        // without a field 'lst.clear = 1' reports the missing arguments
        return not path_type.has_field(name_tk.text());
    }

    // an unknown element leaves the whole array as the accessed range
    auto narrow_to_element(toc& tc, const expr_any& index_expr,
                           const ident_info& array_info) -> void {

        const std::optional<size_t> index{
            in_range_constant_index(tc, index_expr, array_info),
        };

        if (not index) {
            is_exact_access_ = false;
            return;
        }

        const size_t element_size_bytes{array_info.type_ref().size_bytes()};

        const size_t element_offset{*index * element_size_bytes};

        // the last element keeps the padding after the array
        const bool is_last_element{*index == array_info.array_len - 1};

        access_range_ = {
            .offset{access_range_.offset + element_offset},
            .size_bytes{
                is_last_element ? access_range_.size_bytes - element_offset
                                : element_size_bytes,
            },
        };
    }

    // a path element with an optional '[index]'
    auto parse_element(toc& tc, tokenizer& tz, const token& tk) -> void {
        const token open_bracket_tk{tz.is_next_char_token('[')};
        if (open_bracket_tk.is_empty()) {
            elems_.emplace_back(tk, token{}, nullptr, token{});
            return;
        }

        is_indexed_ = true;
        elems_.emplace_back(tk, token{},
                            std::make_unique<expr_any>(
                                tc, tz, tc.get_type_default(), false, false, 0),
                            token{});

        const token close_bracket_tk{tz.is_next_char_token(']')};
        if (close_bracket_tk.is_empty()) {
            throw compiler_exception{
                tz, "expected ']' to close array index expression"};
        }

        elems_.back().open_bracket_tk = open_bracket_tk;
        elems_.back().close_bracket_tk = close_bracket_tk;
    }

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
                access_range_ = is_root ? root_range(info)
                                        : field_range(*parent_type, elem);
            }

            parent_type = &info.type_ref();

            if (elem.array_index_expr and is_exact_access_) {
                narrow_to_element(tc, *elem.array_index_expr, info);
            }
        }
    }

    // the path is an array only while its last element is not indexed
    auto resolve_type(const toc& tc, const token& tk_prv) -> void {
        const ident_info ii{tc.make_ident_info(tk_prv, path_as_string_)};

        set_type(ii.type_ref());

        // an indexed element is technically no longer an array but an element
        if (elems_.back().array_index_expr != nullptr) {
            return;
        }

        is_array_ = ii.is_array;
        array_count_ = ii.array_len;
    }

    // the variable keeps the offset and covers its whole storage
    [[nodiscard]] auto root_range(const ident_info& info) const
        -> field_coverage::range {

        return {
            .offset{access_range_.offset},
            .size_bytes{storage_size_bytes(tok(), info)},
        };
    }

    //
    // statics
    //

    // adds the element index of 'cur_elem' to 'address', which has no index
    // yet, keeping its base and displacement
    [[nodiscard]] auto static add_index(
        toc& tc, const token& src_loc_tk, const size_t indent,
        std::vector<operand>& allocated_registers, const ident_elem& cur_elem,
        const ident_info& cur_info, const operand& reg_count,
        const operand& address, operand& index_register) -> operand {

        machine& x{tc.machine()};

        // a register kept by an earlier fold is reused since the fold already
        // consumed its value
        if (index_register.is_empty()) {
            // index expressions are parsed with the default type
            index_register = x.alloc_scratch_register(src_loc_tk, indent,
                                                      tc.get_type_default());

            allocated_registers.push_back(index_register);
        }

        const int64_t addend_elements{
            compile_checked_index(tc, indent, *cur_elem.array_index_expr,
                                  index_register, cur_info.array_len,
                                  reg_count),
        };

        const size_t type_size{cur_info.type_ref().size_bytes()};

        // a scale the addressing mode cannot encode is applied to the index
        // register instead, leaving scale 1 in the operand
        const bool is_encodable_scale{x.can_lower_index_scale(type_size)};
        if (not is_encodable_scale) {
            x.scale_index(src_loc_tk, indent, index_register, type_size);
        }

        return operand::mem(address.base_register(),
                            index_register.base_register(),
                            is_encodable_scale ? type_size : 1,
                            add_address_offset(address.displacement(),
                                               scaled_address_offset(
                                                   addend_elements, type_size)),
                            cur_info.type_ref());
    }

    // the array size is known at compile, also for an inlined array argument,
    // so a constant index out of bounds is always a bug
    static auto assert_index_in_bounds(const ident_elem& elem,
                                       const ident_info& array_info,
                                       const int64_t index,
                                       const bool allow_end) -> void {

        const bool is_past_end{
            allow_end ? std::cmp_greater(index, array_info.array_len)
                      : std::cmp_greater_equal(index, array_info.array_len),
        };

        if (index >= 0 and not is_past_end) {
            return;
        }

        throw compiler_exception{
            elem.array_index_expr->tok(),
            std::format("index {} is out of bounds for array '{}' of size {}",
                        index, elem.name_tk.text(), array_info.array_len)};
    }

    static auto
    check_array_bounds(toc& tc, const size_t indent, const token& src_loc_tk,
                       const operand& index_or_count, const size_t array_length,
                       const bool allow_end, const operand& range_count = {})
        -> void {

        machine& x{tc.machine()};

        x.check_bounds(src_loc_tk, indent, index_or_count, array_length,
                       allow_end, range_count, tc.bounds_check_options());
    }

    // the range of an unindexed array is checked once, at the last element
    static auto check_unindexed_range(toc& tc, const size_t indent,
                                      const token& src_loc_tk,
                                      const ident_info& cur_info,
                                      const bool is_last_elem,
                                      const operand& reg_count) -> void {

        if (not cur_info.is_array or reg_count.is_empty()) {
            return;
        }

        // note: an array before the last element is indexed
        assert(is_last_elem);

        check_array_bounds(tc, indent, src_loc_tk, reg_count,
                           cur_info.array_len, true);
    }

    // the index goes into 'index_register', returns the constant a trailing
    // '+ c' or '- c' leaves out for the displacement, e.g. 'ix + 1' compiles
    // 'ix' and returns 1
    //
    // note: only without bounds checks, they check the sum in the register
    //
    [[nodiscard]] static auto compile_checked_index(
        toc& tc, const size_t indent, const expr_any& index_expr,
        const operand& index_register, const size_t array_length,
        const operand& range_count) -> int64_t {

        machine& x{tc.machine()};

        x.comment(index_expr.tok(), indent, "set array index");

        const ident_info index_info{
            toc::make_ident_info_from_register(index_register),
        };

        const machine::bounds_check_options checks{tc.bounds_check_options()};

        if (not checks.upper and not checks.lower) {
            const std::optional<int64_t> addend{
                index_expr.compile_without_trailing_addend(tc, indent,
                                                           index_info),
            };

            if (addend) {
                x.comment(index_expr.tok(), indent,
                          "index constant {} goes into the displacement",
                          *addend);

                return *addend;
            }
        }

        index_expr.compile(tc, indent, index_info);

        check_array_bounds(tc, indent, index_expr.tok(), index_register,
                           array_length, not range_count.is_empty(),
                           range_count);

        return 0;
    }

    // the address after the element index, a constant index goes into the
    // displacement and any other one into an index register
    [[nodiscard]] static auto compile_indexed_element(
        toc& tc, const size_t indent, const token& src_loc_tk,
        std::vector<operand>& allocated_registers, const size_t owned_from,
        const ident_elem& cur_elem, const ident_info& cur_info,
        const operand& range_count, const operand& address,
        const operand& address_register, operand& index_register) -> operand {

        const std::optional<int64_t> constant_index{
            cur_elem.array_index_expr->constant_value(tc),
        };

        if (constant_index) {
            assert_index_in_bounds(cur_elem, cur_info, *constant_index,
                                   not range_count.is_empty());
        }

        // with a range count the run-time check covers 'index + count'
        if (constant_index and range_count.is_empty()) {
            operand folded{address};
            folded.increment_offset(
                address_offset(static_cast<size_t>(*constant_index) *
                               cur_info.type_ref().size_bytes()));

            return folded;
        }

        // an operand holds one index, so an earlier one is folded first
        operand base{address};
        if (not address.index_register().empty()) {
            base = operand::mem(
                fold_indexed_address(tc, indent, src_loc_tk,
                                     allocated_registers, owned_from, address,
                                     address_register, index_register),
                cur_info.type_ref());
        }

        return add_index(tc, cur_elem.array_index_expr->tok(), indent,
                         allocated_registers, cur_elem, cur_info, range_count,
                         base, index_register);
    }

    [[nodiscard]] static auto
    find_start_element(const std::span<const operand> known_addresses)
        -> size_t {

        size_t index{known_addresses.size()};
        while (index != 0) {
            --index;
            if (not known_addresses.at(index).is_empty()) {
                return index;
            }
        }

        return 0;
    }

    // computes 'address' into a register so another index can be added
    [[nodiscard]] static auto
    fold_indexed_address(toc& tc, const size_t indent, const token& src_loc_tk,
                         std::vector<operand>& allocated_registers,
                         const size_t owned_from, const operand& address,
                         const operand& address_register,
                         operand& index_register) -> operand {

        machine& x{tc.machine()};

        operand target{
            fold_register(std::span{allocated_registers}.subspan(owned_from),
                          address, address_register, index_register),
        };

        if (target.is_empty()) {
            target = x.alloc_scratch_register(src_loc_tk, indent,
                                              tc.get_type_address());

            allocated_registers.push_back(target);
        }

        x.address_of(src_loc_tk, indent, target, address);

        return target;
    }

    // picks a register that 'lea target, [address]' may overwrite, or empty
    // when a new one must be allocated
    // note: only 'address' refers to an owned register and the fold replaces
    //       'address', so an owned base is dead once 'lea' has read it
    [[nodiscard]] static auto
    fold_register(const std::span<const operand> owned, const operand& address,
                  const operand& address_register, operand& index_register)
        -> operand {

        // the caller loads the final address into it anyway, so it is free
        // until then and folding there may save the final move
        if (not address_register.is_empty()) {
            return address_register;
        }

        // an owned base is a loaded pointer or an earlier fold target, reusing
        // it keeps 'index_register' for the next index
        for (const operand& r : owned) {
            // the frame register and the caller's registers are never owned,
            // so a match proves the base is private to this path
            if (r.base_register() == address.base_register()) {
                return r;
            }
        }

        // the base must survive, so without an index register a new one is
        // needed
        if (index_register.is_empty()) {
            return {};
        }

        // 'lea' may write the index register it reads, the next index then
        // gets its own register so it never overwrites the new base
        const operand folded_into{index_register};
        index_register = {};

        return folded_into;
    }

    // empty when the index is computed at run time or is out of range
    [[nodiscard]] static auto
    in_range_constant_index(const toc& tc, const expr_any& index_expr,
                            const ident_info& array_info)
        -> std::optional<size_t> {

        const std::optional<int64_t> index{index_expr.constant_value(tc)};
        if (not index or *index < 0 or
            std::cmp_greater_equal(*index, array_info.array_len)) {

            return std::nullopt;
        }

        return static_cast<size_t>(*index);
    }

    [[nodiscard]] static auto
    load_pointer(toc& tc, const size_t indent, const token& src_loc_tk,
                 std::vector<operand>& allocated_registers,
                 const operand& pointer_slot) -> operand {

        machine& x{tc.machine()};

        const operand pointer_register{
            x.alloc_scratch_register(src_loc_tk, indent, tc.get_type_address()),
        };

        allocated_registers.push_back(pointer_register);

        x.copy_value(src_loc_tk, indent, pointer_register,
                     operand::mem(pointer_slot, tc.get_type_address()));

        return operand::mem(pointer_register, tc.get_type_address());
    }

    // an inlined argument's known address, a loaded pointer or the storage
    [[nodiscard]] static auto
    start_address(toc& tc, const size_t indent, const token& src_loc_tk,
                  std::vector<operand>& allocated_registers,
                  const operand& known_address, const ident_info& storage)
        -> operand {

        if (not known_address.is_empty()) {
            return known_address;
        }

        if (storage.is_pointer) {
            return load_pointer(tc, indent, src_loc_tk, allocated_registers,
                                storage.operand);
        }

        return storage.operand;
    }

    [[nodiscard]] static auto storage_size_bytes(const token& src_loc_tk,
                                                 const ident_info& info)
        -> size_t {

        if (not info.is_array) {
            return info.type_ref().size_bytes();
        }

        return multiply_storage_size(src_loc_tk, info.type_ref().size_bytes(),
                                     info.array_len);
    }
};
