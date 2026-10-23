#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compile_error.hpp"
#include "constant_arith.hpp"
#include "ident_info.hpp"
#include "operand.hpp"
#include "parse_lookahead.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_assigned.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

// a constant index out of bounds is always a bug; an unsized array parameter
// without a length rejects only a negative index; 'allow_end' accepts the end,
// where a range of a zero count starts
inline auto assert_constant_index(const ident_info& array_info,
                                  const token& index_tk,
                                  const std::string_view array_name,
                                  const int64_t index, const bool allow_end)
    -> void {

    const std::optional<size_t> array_len{array_info.array_len()};

    if (not array_len and index >= 0) {
        return;
    }

    if (not array_len) {
        throw compile_error{index_tk,
                            std::format("index {} is negative", index)};
    }

    const bool is_past_end{
        allow_end ? std::cmp_greater(index, *array_len)
                  : std::cmp_greater_equal(index, *array_len),
    };

    if (index >= 0 and not is_past_end) {
        return;
    }

    throw compile_error{
        index_tk,
        std::format("index {} is out of bounds for array {} of size {}", index,
                    in_quotes(array_name), *array_len)};
}

// the constant range of a bulk operation, 'count' elements from 'start', e.g.
// 'array_copy(a[1], b, 5)' or 'write(1, a, 5, 1)'
struct constant_range {
    // empty when computed at run time, then a count is known to pass the end
    // only when it is larger than the array
    std::optional<int64_t> start;
    int64_t count{};
    // the written start, empty when the range starts at the array
    std::optional<token> start_tk;
    // where an error about the count points
    token count_tk;
    // e.g. 'count 5' or 'a sector of 4 elements'
    std::string count_text;
    // the path of the array, e.g. 'a.b' of 'a.b[1]'
    std::string_view array_name;
};

// a constant range is known to be negative or to pass the end of the array,
// e.g. 5 in 'array_copy(a, b, 5)' of 'a = i8[4]'; a run-time one is left to
// the bounds check at run time
inline auto assert_constant_range(const ident_info& array_info,
                                  const constant_range& range) -> void {

    if (range.count < 0) {
        throw compile_error{range.count_tk,
                            std::format("count {} is negative", range.count)};
    }

    const bool has_constant_start{range.start and range.start_tk};

    // a start at the end is the empty range of a zero count
    if (has_constant_start) {
        assert_constant_index(array_info, *range.start_tk, range.array_name,
                              *range.start, true);
    }

    const std::optional<size_t> array_len{array_info.array_len()};

    if (not array_len) {
        return;
    }

    // note: a start computed at run time that passes its bounds check is not
    //       negative, so at most the whole array remains

    const size_t remaining{
        range.start ? *array_len - static_cast<size_t>(*range.start)
                    : *array_len,
    };

    if (std::cmp_less_equal(range.count, remaining)) {
        return;
    }

    const std::string from_index{
        has_constant_start ? std::format(" from index {}", *range.start)
                           : std::string{},
    };

    throw compile_error{
        range.count_tk,
        std::format("{}{} is past the end of array {} of size {}",
                    range.count_text, from_index, in_quotes(range.array_name),
                    *array_len)};
}

// a place: a variable, a field or an element named by a path, e.g.
// 'w.rooms[i].items[j].x', also the receiver of a method call; the parse
// resolves the type and the bytes of the root variable that the path reaches
// ('accessed_bytes_', the whole array after a run-time index), the compile
class expr_any;

// resolves the names again in the current frames and builds one memory operand
// ('compile_lea', see 'etc/docs/README.md' chapter 6); a path that names a
// function or a constant has no storage
class stmt_identifier final : public statement {
    struct ident_elem {
        token name_tk;
        token open_bracket_tk;
        std::unique_ptr<expr_any> array_index_expr;
        token close_bracket_tk;

        auto source_to(std::ostream& os) const -> void;
    };

    // the scratch registers that the address of a path is built with
    class path_registers final {
        std::reference_wrapper<std::vector<operand>> allocated_;
        // the registers from here on were allocated for this path and may be
        // overwritten, the earlier ones are the caller's
        size_t owned_from_;
        // where the caller prefers the address, empty without a preference
        operand address_register_;
        // kept between the indexes of the path while it is not in the way
        operand index_register_;

      public:
        path_registers(std::vector<operand>& allocated,
                       operand address_register)
            : allocated_{allocated}, owned_from_{allocated.size()},
              address_register_{std::move(address_register)} {}

        auto add(const operand& reg) -> void {
            allocated_.get().push_back(reg);
        }

        [[nodiscard]] auto address_register() const -> const operand& {
            return address_register_;
        }

        auto forget_index_register() -> void { index_register_ = {}; }

        [[nodiscard]] auto index_register() const -> const operand& {
            return index_register_;
        }

        [[nodiscard]] auto owned() const -> std::span<const operand> {
            return std::span{allocated_.get()}.subspan(owned_from_);
        }

        auto set_index_register(const operand& reg) -> void {
            index_register_ = reg;
        }
    };

    std::vector<ident_elem> elems_;
    std::vector<token> elem_delims_tk_;
    std::string path_as_string_;
    // see 'var_info::array_len'
    std::optional<size_t> array_count_;
    bool is_array_{};
    bool is_indexed_{};

    // bytes of the root variable accessed by this path, after a run-time index
    // the whole array
    byte_range accessed_bytes_;

    // the fields and indexes of this path
    access_path access_path_;

    // e.g. '.add' in 'lst.add(x)', the path is the receiver
    token method_dot_tk_;
    token method_name_tk_;

  public:
    // a path of names, fields and indexes from 'tk', which is read
    stmt_identifier(toc& tc, unary_ops uops, token tk, tokenizer& tz);

    // note: 'expr_any' is incomplete here, the special members are defined in
    //       'stmt_identifier_impl.hpp', where a 'unique_ptr' of it is complete

    stmt_identifier();

    stmt_identifier(stmt_identifier&&) noexcept;

    stmt_identifier(const stmt_identifier&) = delete;
    auto operator=(const stmt_identifier&) -> stmt_identifier& = delete;

    ~stmt_identifier() override;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        get_unary_ops().source_to(os);
        path_source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const ident_info src_info{make_scalar_ident_info(tc, *this)};

        // note: a list copies a constant itself
        assert(not src_info.is_const());

        std::vector<operand> allocated_registers;

        const operand op{
            get_lea_operand(tc, indent, src_info, allocated_registers),
        };

        x.copy_value(tok(), indent, dst.operand(), op);

        get_unary_ops().compile(x, indent, tok(), dst.operand(),
                                tc.overflow_options(dst));

        x.free_scratch_registers(tok(), indent, allocated_registers);
    }

    [[nodiscard]] auto accessed_path() const -> access_path override {
        return access_path_;
    }

    auto assert_not_narrowed(const toc& tc, const type& dst_type) const
        -> void override {

        // instances are copied whole and cannot narrow
        assert(dst_type.is_builtin());

        const ident_info info{make_ident_info(tc, *this)};

        if (info.is_const() and not info.constant().is_typed()) {
            assert_constant_fits(info, dst_type);
            return;
        }

        const type& src_type{info.type_ref()};

        // a built-in destination takes a number, e.g. not the instance 'a'
        // of 'y = a'
        if (not src_type.is_builtin()) {
            throw compile_error{first_token(),
                                std::format("expected type {}, got {}",
                                            in_quotes(dst_type.name()),
                                            in_quotes(src_type.name()))};
        }

        if (src_type.size_bytes() <= dst_type.size_bytes()) {
            return;
        }

        throw_narrowed(first_token(), statement::trimmed_source(*this),
                       src_type, dst_type);
    }

    // the memory operand of this identifier, e.g. 'rbp + 4 * r15 + 248',
    // 'allocated_registers' receives the scratch registers of the indexing and
    // 'machine' says which scalings an operand can have
    [[nodiscard]] auto compile_lea(toc& tc, const size_t indent,
                                   const token& src_loc_tk,
                                   std::vector<operand>& allocated_registers,
                                   const lea_request& request) const
        -> operand override {

        const known_address* const known{known_in_path(request.known)};

        std::string path{elems_.front().name_tk.text()};

        path_registers registers{allocated_registers, request.address_register};

        operand address{
            start_address(tc, indent, src_loc_tk, registers,
                          known == nullptr ? operand{} : known->address,
                          tc.make_ident_info(src_loc_tk, path)),
        };

        const type* parent_type{};

        for (size_t elem_index{}; elem_index < elems_.size(); ++elem_index) {
            const ident_elem& cur_elem{elems_.at(elem_index)};

            // field offsets stay in the operand, not in the base register
            if (elem_index != 0) {
                path.push_back('.');
                path += cur_elem.name_tk.text();

                const size_t offset{
                    parent_type->field_offset(cur_elem.name_tk,
                                              cur_elem.name_tk.text()),
                };

                address.increment_offset(address_offset(offset));
            }

            const ident_info cur_info{tc.make_ident_info(src_loc_tk, path)};

            parent_type = &cur_info.type_ref();

            const bool is_last_elem{elem_index == elems_.size() - 1};

            // an unindexed element only needs a possible range bounds check
            if (not cur_elem.array_index_expr) {
                check_unindexed_range(tc, indent, src_loc_tk, cur_info,
                                      is_last_elem, request.reg_count);

                continue;
            }

            address = compile_indexed_element(
                tc, indent, src_loc_tk, registers, cur_elem, path, cur_info,
                is_last_elem ? request.reg_count : operand{}, address);
        }

        return operand::mem(address, *parent_type);
    }

    // a typed constant is a value of its type, so it wraps at the width of
    // its use like a variable, e.g. '-p' of an 'i8' 'p' bound to -128 is -128
    // at 'i8'
    [[nodiscard]] auto fold(const toc& tc, const value_use& use) const
        -> std::optional<folded_value> override {

        const ident_info info{make_ident_info(tc, *this)};

        if (not info.is_const()) {
            return std::nullopt;
        }

        const folded_value& constant{info.constant()};

        const int64_t value{
            get_unary_ops().evaluate_at(constant.value, use.width_ref(),
                                        use.checks_overflow),
        };

        return folded_value{
            .value{constant_arith::wrap_to_width(value, use.width_ref())},
            .own_type{constant.own_type},
        };
    }

    [[nodiscard]] auto is_array_element() const -> bool override {
        return not elems_.empty() and elems_.back().array_index_expr != nullptr;
    }

    [[nodiscard]] auto is_indexed() const -> bool override {
        return is_indexed_;
    }

    [[nodiscard]] auto is_name_or_literal() const -> bool override {
        return true;
    }

    // the low bytes are at the start address of the storage
    [[nodiscard]] auto
    keeps_low_bits([[maybe_unused]] const width_change change) const
        -> bool override {

        return true;
    }

    [[nodiscard]] auto name_or_literal() const -> std::string_view override {
        return path_as_string_;
    }

    [[nodiscard]] auto name_token() const -> const token& override {
        return last_token();
    }

    [[nodiscard]] auto operand_path() const -> const stmt_identifier* override {
        return this;
    }

    // a call in an index, e.g. the 'h(i)' of 'a[h(i)]'
    auto visit_call_effects(const toc& tc,
                            const call_effect_visitor visitor) const
        -> void override;

    // index expressions are read even when the path is written
    auto visit_index_reads(const read_filter var,
                           const read_visitor reader) const -> void override;

    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        // a path such as 'p.y' reads its root variable
        if (not var or first_token().is_text(*var)) {
            reader(first_token(), path_text(), accessed_bytes_, access_path_);
        }

        visit_index_reads(var, reader);
    }

    //
    // class methods
    //

    [[nodiscard]] auto array_count() const -> std::optional<size_t> {
        return array_count_;
    }

    // an array or an element of one, where a bulk operation starts
    auto assert_array_or_element() const -> void {
        assert_not_method();

        if (is_array_ or is_array_element()) {
            return;
        }

        throw compile_error{
            first_token(),
            std::format("{} is not an array", in_quotes(name_or_literal()))};
    }

    // a constant count of a bulk operation that starts at this path, see
    // 'assert_constant_range'
    auto assert_count_in_bounds(const toc& tc, const token& count_tk,
                                const int64_t count) const -> void;

    // where only an array is expected, the path does not end at a method,
    // e.g. 'array_length(a.m)'
    auto assert_not_method() const -> void {
        if (not is_method_receiver()) {
            return;
        }

        throw compile_error{
            method_name_tk_,
            std::format("'{}.{}' is a method, expected an array",
                        path_as_string_, method_name_tk_.text())};
    }

    // the index where a bulk operation starts, 0 for a whole array, empty when
    // computed at run time
    [[nodiscard]] auto constant_start_index(const toc& tc) const
        -> std::optional<int64_t>;

    [[nodiscard]] auto first_token() const -> const token& {
        return elems_.at(0).name_tk;
    }

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }

    // false when a run-time index leaves the element unknown
    [[nodiscard]] auto is_exact_access() const -> bool {
        return not access_path_.has_run_time_index();
    }

    [[nodiscard]] auto is_method_receiver() const -> bool {
        return not method_name_tk_.is_empty();
    }

    // the name at the end of the path, e.g. 'data' of 'q1.data'
    [[nodiscard]] auto last_token() const -> const token& {
        return elems_.back().name_tk;
    }

    [[nodiscard]] auto method_dot_token() const -> const token& {
        return method_dot_tk_;
    }

    [[nodiscard]] auto method_name_token() const -> const token& {
        return method_name_tk_;
    }

    // the path names a function, unless a variable, a parameter or a constant
    // of the same root is in scope, it shadows the function
    [[nodiscard]] auto names_function(const toc& tc) const -> bool {
        const std::string_view root{ident_info::root_of(path_as_string_)};
        return tc.is_func(path_as_string_) and not is_hidden_by_value(tc, root);
    }

    auto operator=(stmt_identifier&&) noexcept -> stmt_identifier&;

    auto path_source_to(std::ostream& os) const -> void;

    // the path as written, with indexes but without unary operators
    [[nodiscard]] auto path_text() const -> std::string {
        std::stringstream ss;
        path_source_to(ss);
        return statement::trimmed_source(ss.view());
    }

    // a run-time index does not prove which element is written
    auto record_assignment(assignment_flow& flow) const -> void {
        const std::optional<std::string_view> var{flow.traced_var()};

        if (not var or not first_token().is_text(*var) or
            not is_exact_access()) {

            return;
        }

        flow.assign(accessed_bytes_);
    }

    //
    // statics
    //

    // the array of a built-in, e.g. the 'a' of 'array_length(a)' or of
    // 'foo a', is a variable, a field or an element; a call, a type or a
    // function has none
    [[nodiscard]] static auto parse_array_argument(toc& tc, tokenizer& tz)
        -> stmt_identifier {

        const token tk{tz.next_token()};
        const term tm{classify_term(tc, tk, tz)};

        if (tm.is_bare_type() or tm.is(term_kind::array_literal) or
            tm.is(term_kind::record_literal)) {

            throw compile_error{
                tk, std::format("{} is a type, not an array", in_quotes(tk))};
        }

        if (tm.is(term_kind::call) or tm.is(term_kind::generic_call) or
            tm.is(term_kind::conversion) or
            tm.is(term_kind::builtin_function) or
            tm.is(term_kind::constructor_call)) {

            throw compile_error{
                tk, std::format("a call of {} is not an array, store its "
                                "result in a variable first",
                                in_quotes(tk))};
        }

        if (tm.is(term_kind::function_name)) {
            throw compile_error{tk, std::format("{} is a function, not an "
                                                "array",
                                                in_quotes(tk))};
        }

        stmt_identifier array{tc, {}, tk, tz};

        array.assert_not_method();

        return array;
    }

  private:
    // only signed types exist so a constant must fit the signed range
    auto assert_constant_fits(const ident_info& info,
                              const type& dst_type) const -> void {

        const int64_t value{
            get_unary_ops().evaluate_constant(info.constant().value),
        };

        if (constant_arith::fits_width(value, dst_type)) {
            return;
        }

        const std::string source{statement::trimmed_source(*this)};

        throw compile_error{
            first_token(),
            std::format("constant {} does not fit {}, use '{}(...)'",
                        in_quotes(source), in_quotes(dst_type.name()),
                        dst_type.name())};
    }

    // 'ps.x' does not mean 'ps[0].x'
    auto assert_element_selected(const toc& tc, const token& src_loc_tk) const
        -> void {

        if (elems_.back().array_index_expr or names_function(tc)) {
            return;
        }

        if (not tc.make_ident_info(src_loc_tk, path_as_string_).is_array()) {
            return;
        }

        throw compile_error{src_loc_tk,
                            std::format("array {} must be indexed",
                                        in_quotes(path_as_string_))};
    }

    // the '[' of an element needs an array
    auto assert_indexable(const toc& tc, tokenizer& tz,
                          const token& src_loc_tk) const -> void {

        if (names_function(tc)) {
            return;
        }

        const ident_info cur_ident_info{
            tc.make_ident_info(src_loc_tk, path_as_string_),
        };

        if (tz.peek_char_after_whitespace() == '[' and
            not cur_ident_info.is_array()) {

            throw compile_error{src_loc_tk,
                                std::format("cannot index non-array {}",
                                            in_quotes(path_as_string_))};
        }
    }

    // without a method 'T.m' a following '(' would otherwise be reported as a
    // missing field
    auto assert_not_method_call(const toc& tc, tokenizer& tz,
                                const token& src_loc_tk,
                                const token& name_tk) const -> void {

        if (names_function(tc)) {
            return;
        }

        if (tz.peek_char_after_whitespace() != '(') {
            return;
        }

        const ident_info info{tc.make_ident_info(src_loc_tk, path_as_string_)};

        throw compile_error{name_tk,
                            std::format("method {} not found in type {}",
                                        in_quotes(name_tk.text()),
                                        in_quotes(info.type_ref().name()))};
    }

    // a '.' followed by a field continues the path with that field, a method
    // name ends it with the path as the receiver; 'tk' becomes the field and
    // 'src_loc_tk' the token before it
    [[nodiscard]] auto extends_path(toc& tc, tokenizer& tz, token& tk,
                                    token& src_loc_tk) -> bool {

        // e.g. the '.entities' of 'rooms[i].entities[j]'
        const token dot_tk{tz.is_next_char_token('.')};

        if (dot_tk.is_empty()) {
            return false;
        }

        const token next_tk{tz.next_token()};

        // e.g. 'p..x', a path may go on after whitespace and line ends
        if (not next_tk.is_word()) {
            throw compile_error{tz.position_of(next_tk),
                                "expected a field name after '.'"};
        }

        // e.g. the '.add' of 'lst.add(1)'
        if (is_method_name(tc, tz, tk, next_tk)) {
            // the path so far is resolved as the receiver
            method_dot_tk_ = dot_tk;
            method_name_tk_ = next_tk;
            return false;
        }

        assert_element_selected(tc, tk);
        assert_not_method_call(tc, tz, tk, next_tk);

        elem_delims_tk_.emplace_back(dot_tk);
        src_loc_tk = tk;
        tk = next_tk;
        path_as_string_.push_back('.');
        path_as_string_ += tk.text();

        return true;
    }

    // 'name_tk' after the path so far names a method of the path's type
    [[nodiscard]] auto is_method_name(const toc& tc, tokenizer& tz,
                                      const token& src_loc_tk,
                                      const token& name_tk) const -> bool {

        if (names_function(tc)) {
            return false;
        }

        const ident_info info{tc.make_ident_info(src_loc_tk, path_as_string_)};

        const type& path_type{info.type_ref()};

        const std::string method_name{
            std::format("{}.{}", path_type.name(), name_tk.text()),
        };

        if (not tc.is_func(method_name)) {
            return false;
        }

        // a method and a field may share a name, only the call has '(' or the
        // type arguments of a generic method
        const char next{tz.peek_char_after_whitespace()};

        if (next == '(' or
            (next == '<' and tc.generics().has_func(method_name))) {

            return true;
        }

        // without a field 'lst.clear = 1' reports the missing arguments
        return not path_type.has_field(name_tk.text());
    }

    // the known address when it is of the root of this path, null when none
    // is known or it is before the path, e.g. in the argument that a
    // parameter names
    [[nodiscard]] auto
    known_in_path(const std::optional<known_address>& known) const
        -> const known_address* {

        // note: the first binding met while resolving a path is its root, so a
        //       known address is of the root or before the path

        if (not known or known->trailing_count != elems_.size() - 1) {
            return nullptr;
        }

        return &*known;
    }

    // note: after a run-time index the range stays the whole array
    auto narrow_accessed_bytes(const size_t offset, const size_t size_bytes_in)
        -> void {

        if (not is_exact_access()) {
            return;
        }

        accessed_bytes_ = {
            .offset{accessed_bytes_.offset + offset},
            .size_bytes{size_bytes_in},
        };
    }

    // an unknown element leaves the whole array as the accessed range and
    // adds a run-time index to the path
    auto narrow_to_element(toc& tc, const expr_any& index_expr,
                           const ident_info& array_info) -> void;

    // a path element with an optional '[index]'
    auto parse_element(toc& tc, tokenizer& tz, const token& tk) -> void;

    auto resolve_accessed_bytes(toc& tc) -> void;

    // the path is an array only while its last element is not indexed
    auto resolve_type(const toc& tc, const token& src_loc_tk) -> void {
        const ident_info info{tc.make_ident_info(src_loc_tk, path_as_string_)};

        set_type(info.type_ref());

        // an indexed element is technically no longer an array but an element
        if (elems_.back().array_index_expr != nullptr) {
            return;
        }

        is_array_ = info.is_array();
        array_count_ = info.array_len();
    }

    // the variable keeps the offset and covers its whole storage
    [[nodiscard]] auto root_range(const ident_info& info) const -> byte_range {
        return {
            .offset{},
            .size_bytes{storage_size_bytes(tok(), info)},
        };
    }

    //
    // statics
    //

    // adds the element index of 'cur_elem' to 'address', which has no index
    // yet, keeping its base and displacement
    [[nodiscard]] static auto
    add_index(toc& tc, const size_t indent, const token& src_loc_tk,
              path_registers& registers, const ident_elem& cur_elem,
              const ident_info& cur_info, const operand& reg_count,
              const operand& address) -> operand;

    static auto check_array_bounds(toc& tc, const size_t indent,
                                   const token& src_loc_tk,
                                   const machine::bounds_range& range) -> void {

        machine& x{tc.machine()};

        x.check_bounds(src_loc_tk, indent, range, tc.checks().bounds_options());
    }

    // the range of an unindexed array is checked once, at the last element
    static auto check_unindexed_range(toc& tc, const size_t indent,
                                      const token& src_loc_tk,
                                      const ident_info& cur_info,
                                      const bool is_last_elem,
                                      const operand& reg_count) -> void {

        if (not cur_info.is_array() or reg_count.is_empty()) {
            return;
        }

        // note: an array before the last element is indexed
        assert(is_last_elem);

        check_array_bounds(tc, indent, src_loc_tk,
                           machine::bounds_range::count_alone(
                               reg_count, cur_info.element_count()));
    }

    // the index goes into 'index_register', returns the constant a trailing
    // '+ c' or '- c' leaves out for the displacement, e.g. 'ix + 1' compiles
    // 'ix' and returns 1, only without bounds checks since they check the sum
    // in the register
    [[nodiscard]] static auto compile_checked_index(
        toc& tc, const size_t indent, const expr_any& index_expr,
        const operand& index_register, const size_t array_length,
        const operand& range_count) -> int64_t;

    // the address after the element index, a constant index goes into the
    // displacement and any other one into an index register
    [[nodiscard]] static auto compile_indexed_element(
        toc& tc, const size_t indent, const token& src_loc_tk,
        path_registers& registers, const ident_elem& cur_elem,
        const std::string_view path, const ident_info& cur_info,
        const operand& range_count, const operand& address) -> operand;

    // computes 'address' into a register so another index can be added
    [[nodiscard]] static auto
    fold_indexed_address(toc& tc, const size_t indent, const token& src_loc_tk,
                         path_registers& registers, const operand& address)
        -> operand {

        machine& x{tc.machine()};

        operand target{
            fold_register(registers, address),
        };

        if (target.is_empty()) {
            target = x.alloc_scratch_register(src_loc_tk, indent,
                                              tc.builtins().address());

            registers.add(target);
        }

        x.address_of(src_loc_tk, indent, target, address);

        return target;
    }

    // picks a register that 'lea target, [address]' may overwrite, or empty
    // when a new one must be allocated
    // note: only 'address' refers to an owned register and the fold replaces
    //       'address', so an owned base is dead once 'lea' has read it
    [[nodiscard]] static auto fold_register(path_registers& registers,
                                            const operand& address) -> operand {

        // the caller loads the final address into it anyway, so it is free
        // until then and folding there may save the final move
        if (not registers.address_register().is_empty()) {
            return registers.address_register();
        }

        // an owned base is a loaded pointer or an earlier fold target, reusing
        // it keeps 'index_register' for the next index
        for (const operand& r : registers.owned()) {
            // the frame register and the caller's registers are never owned,
            // so a match proves the base is private to this path
            if (r.base_register() == address.base_register()) {
                return r;
            }
        }

        // the base must survive, so without an index register a new one is
        // needed
        if (registers.index_register().is_empty()) {
            return {};
        }

        // 'lea' may write the index register it reads, the next index then
        // gets its own register so it never overwrites the new base
        const operand folded_into{registers.index_register()};
        registers.forget_index_register();

        return folded_into;
    }

    // empty when the index is computed at run time or is out of range
    [[nodiscard]] static auto
    in_range_constant_index(const toc& tc, const expr_any& index_expr,
                            const ident_info& array_info)
        -> std::optional<size_t>;

    [[nodiscard]] static auto
    load_pointer(toc& tc, const size_t indent, const token& src_loc_tk,
                 path_registers& registers, const operand& pointer_slot)
        -> operand {

        machine& x{tc.machine()};

        const operand pointer_register{
            x.alloc_scratch_register(src_loc_tk, indent,
                                     tc.builtins().address()),
        };

        registers.add(pointer_register);

        x.copy_value(src_loc_tk, indent, pointer_register,
                     operand::mem(pointer_slot, tc.builtins().address()));

        return operand::mem(pointer_register, tc.builtins().address());
    }

    // an inlined argument's known address, a loaded pointer or the storage
    [[nodiscard]] static auto
    start_address(toc& tc, const size_t indent, const token& src_loc_tk,
                  path_registers& registers, const operand& known_operand,
                  const ident_info& storage) -> operand {

        if (not known_operand.is_empty()) {
            return known_operand;
        }

        if (storage.is_pointer()) {
            return load_pointer(tc, indent, src_loc_tk, registers,
                                storage.operand());
        }

        return storage.operand();
    }

    [[nodiscard]] static auto storage_size_bytes(const token& src_loc_tk,
                                                 const ident_info& info)
        -> size_t {

        return multiply_storage_size(src_loc_tk, info.type_ref().size_bytes(),
                                     info.element_count());
    }
};

// see the declaration in 'statement.hpp'
inline auto statement::place_path() const -> const stmt_identifier* {
    const stmt_identifier* const path{operand_path()};

    if (path == nullptr or not path->get_unary_ops().is_empty()) {
        return nullptr;
    }

    return path;
}

// see the declaration in 'statement.hpp'
inline auto as_place(const toc& tc, const statement& st)
    -> std::optional<place> {

    const stmt_identifier* const path{st.place_path()};

    if (path == nullptr or path->names_function(tc)) {
        return std::nullopt;
    }

    ident_info info{make_ident_info(tc, *path)};

    if (not info.is_var()) {
        return std::nullopt;
    }

    return place{
        .path{path},
        .info{std::move(info)},
    };
}
