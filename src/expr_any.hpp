#pragma once
// reviewed: 2025-09-29

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <ostream>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_arith.hpp"
#include "expr_bool.hpp"
#include "expr_type.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "stmt_builtin_convert.hpp"
#include "stmt_const.hpp"
#include "token.hpp"
#include "ub_unset_var.hpp"
#include "unary_ops.hpp"

class expr_any final : public statement {
    using expr_variant = std::variant<expr_arith, expr_bool, expr_type>;

    // helper template for nicer handling of variants using overloaded lambdas
    template <class... Ts> struct overloaded : Ts... {
        using Ts::operator()...;
    };

    std::vector<expr_variant> vars_;
    std::vector<token> var_delims_tk_;
    // e.g. 'i8[3]' in 'i8[3]{1, 2}'
    token element_type_tk_;
    token open_bracket_tk_;
    stmt_const literal_count_const_;
    token close_bracket_tk_;
    token open_brace_tk_;
    token close_brace_tk_;
    token string_tk_;
    size_t array_count_{};
    bool is_array_{};
    bool is_identifier_{};
    // e.g. an array parameter 's[]' gets its size from the argument
    bool is_unsized_destination_{};

  public:
    // a type or array may be written without '{}', e.g. 'var p = point', 'p =
    // point' and 'var a = i8[4]'
    expr_any(toc& tc, tokenizer& tz, const type& tp, const bool in_args,
             const bool is_array, const size_t array_count)
        : statement{tz.next_whitespace_token()}, array_count_{array_count},
          is_array_{is_array} {

        set_type(tp);

        // the basic case, e.g. 'x + 1' or 'point{1, 2}'
        if (not is_array) {
            vars_.emplace_back(parse_variant(tc, tz, tp, in_args));
            return;
        }

        // array, e.g. 'i8[]{1, 2}' or 'a' of 'var b = a'

        is_unsized_destination_ = array_count_ == 0;

        // e.g. "hello" fills the start of an 'i8' array
        if (tz.is_peek_char('"')) {
            string_tk_ = tz.next_token();

            array_count_ = string_array_count(tc.get_type_i8(), string_tk_, tp,
                                              array_count_);

            return;
        }

        // check if it is '{ ... }' or identifier e.g. 'str.data'

        parse_element_type(tc, tz, tp);

        open_brace_tk_ = tz.is_next_char_token('{');

        if (open_brace_tk_.is_empty() and not open_bracket_tk_.is_empty()) {
            // e.g. 'var a = i8[4]' is 'i8[4]{}'
            if (not literal_count_const_.has_value()) {
                throw compiler_exception{
                    close_bracket_tk_,
                    std::format("an array needs a constant size, e.g. "
                                "'{}[4]'",
                                element_type_tk_.text())};
            }

            return;
        }

        if (open_brace_tk_.is_empty()) {
            // 'expr_type' copies the whole array as bytes, 'expr_arith' would
            // read a single scalar
            // e.g. 'var b = a' copies the array 'a'
            vars_.emplace_back(expr_type{tc, tz, tp, true});
            is_identifier_ = true;
            return;
        }

        // e.g. the '{1, 2, 3}' of 'var a = i8[]{1, 2, 3}'
        parse_braced_elements(tc, tz, tp, in_args);
    }

    // a method receiver, the first argument of the call
    expr_any(const token pos_tk, expr_type receiver) : statement{pos_tk} {
        set_type(receiver.get_type());
        vars_.emplace_back(std::move(receiver));
    }

    expr_any() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        string_tk_.source_to(os);

        if (not open_bracket_tk_.is_empty()) {
            element_type_tk_.source_to(os);
            open_bracket_tk_.source_to(os);
            literal_count_const_.source_to(os);
            close_bracket_tk_.source_to(os);
        }

        open_brace_tk_.source_to(os);

        if (not vars_.empty()) {
            vars_.front().visit([&os](const auto& expression) -> void {
                expression.source_to(os);
            });

            for (const auto [d, e] :
                 std::views::zip(var_delims_tk_, vars_ | std::views::drop(1))) {

                d.source_to(os);

                e.visit([&os](const auto& expression) -> void {
                    expression.source_to(os);
                });
            }
        }

        close_brace_tk_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        if (is_array_identifier()) {
            const ident_info src_info{tc.make_ident_info(*this)};

            if (not src_info.is_array) {
                throw compiler_exception{tok(), "source must be an array"};
            }
        }

        // the base case
        if (is_identifier_ or not is_array_) {
            compile_variant(tc, indent, dst_info, tok(), vars_.at(0));
            return;
        }

        if (is_string()) {
            compile_string(tc, indent, dst_info);
            return;
        }

        // assign array elements

        const size_t array_count{destination_array_count(dst_info)};

        // the parser could not check the size of an unsized destination
        if (vars_.size() > array_count) {
            throw compiler_exception{
                vars_.at(array_count)
                    .visit([](const auto& expression) -> const token& {
                        return expression.tok();
                    }),
                std::format("too many elements specified for array of size {}",
                            array_count)};
        }

        ident_info cur_dst_info{dst_info};

        // a call element writes at the element, not at the array start the
        // identifier names, and an element is not an array
        cur_dst_info.use_operand = true;
        cur_dst_info.is_array = false;
        cur_dst_info.array_len = 0;

        compile_elements(tc, indent, cur_dst_info);

        expr_type::zero_remaining_elements(
            tc, indent, tok(), cur_dst_info.operand, cur_dst_info.type_ref(),
            array_count - vars_.size());
    }

    [[nodiscard]] auto accessed_range() const
        -> std::optional<field_coverage::range> override {

        // note: only identifier arguments ask, and those are never arrays
        assert(not is_array_);
        assert(vars_.size() == 1);

        return vars_.at(0).visit(
            [](const auto& expression) -> std::optional<field_coverage::range> {
                return expression.accessed_range();
            });
    }

    auto assert_not_narrowed(const toc& tc, const type& dst_type) const
        -> void override {

        if (is_array_) {
            return;
        }

        vars_.at(0).visit([&](const auto& expression) -> void {
            expression.assert_not_narrowed(tc, dst_type);
        });
    }

    [[nodiscard]] auto compile_lea(toc& tc, const size_t indent,
                                   const token& src_loc_tk,
                                   std::vector<operand>& allocated_registers,
                                   const lea_request& request) const
        -> operand override {

        return vars_.at(0).visit([&](const auto& expression) -> operand {
            return expression.compile_lea(tc, indent, src_loc_tk,
                                          allocated_registers, request);
        });
    }

    [[nodiscard]] auto get_unary_ops() const -> const unary_ops& override {
        assert(not is_array_);

        return vars_.at(0).visit(
            [](const auto& expression) -> const unary_ops& {
                return expression.get_unary_ops();
            });

        // note: 'expr_type' does not have 'unary_ops' and cannot be
        //       an argument in call
    }

    // only string and '{}' initializers leave 'vars_' empty, callers ask
    // arrays only when 'is_array_identifier' and arguments are never arrays
    [[nodiscard]] auto identifier() const -> std::string_view override {
        assert(not vars_.empty());

        return vars_.at(0).visit(
            [](const auto& expression) -> std::string_view {
                return expression.identifier();
            });
    }

    [[nodiscard]] auto is_array_element() const -> bool override {
        if (is_array_ or vars_.size() != 1) {
            return false;
        }

        return vars_.at(0).visit([](const auto& expression) -> bool {
            return expression.is_array_element();
        });
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        if (is_array_) {
            return true;
        }

        return vars_.at(0).visit([](const auto& expression) -> bool {
            return expression.is_expression();
        });
    }

    [[nodiscard]] auto is_identifier() const -> bool override {
        return vars_.at(0).visit([](const auto& expression) -> bool {
            return expression.is_identifier();
        });
    }

    [[nodiscard]] auto is_indexed() const -> bool override {
        assert(not is_array_);

        return vars_.at(0).visit([](const auto& expression) -> bool {
            return expression.is_indexed();
        });
    }

    [[nodiscard]] auto tok() const -> const token& override {
        if (is_string()) {
            return string_tk_;
        }

        if (vars_.empty()) {
            return statement::tok();
        }

        return vars_.at(0).visit([](const auto& expression) -> const token& {
            return expression.tok();
        });
    }

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

        // a non-array expression is the single element
        for (const expr_variant& e : vars_) {
            e.visit([&var, &reader](const auto& expression) -> void {
                expression.visit_reads(var, reader);
            });
        }
    }

    //
    // class methods
    //

    [[nodiscard]] auto array_count() const -> size_t { return array_count_; }

    [[nodiscard]] auto as_expr_type(const size_t index = 0) const
        -> const expr_type& {

        return std::get<expr_type>(vars_.at(index));
    }

    auto assert_record_value_not_reading(
        const expr_type::record_destination& dst) const -> void {

        if (is_array_ or not std::holds_alternative<expr_type>(vars_.at(0))) {
            return;
        }

        as_expr_type().assert_not_reading(dst);
    }

    // compiles all but the trailing added constant of an arithmetic
    // expression and returns the constant, e.g. 'ix + 1' compiles 'ix' and
    // returns 1, empty when nothing was compiled
    [[nodiscard]] auto
    compile_without_trailing_addend(toc& tc, const size_t indent,
                                    const ident_info& dst_info) const
        -> std::optional<int64_t> {

        // note: only an index expression asks, it is never an array and has
        //       the default type
        assert(not is_array_);

        const expr_arith& arith{std::get<expr_arith>(vars_.front())};

        arith.assert_not_narrowed(tc, dst_info.type_ref());

        return arith.compile_without_trailing_addend(tc, indent, dst_info);
    }

    // e.g. '-2' or a named constant, empty when computed at run time
    [[nodiscard]] auto constant_value(const toc& tc) const
        -> std::optional<int64_t> {

        return constant_value(tc, tc.get_type_default());
    }

    // folded at the width of 'width_type'
    [[nodiscard]] auto constant_value(const toc& tc,
                                      const type& width_type) const
        -> std::optional<int64_t> {

        assert(not is_array_);

        const expr_variant& e{vars_.front()};

        // e.g. 'maybe == 33' with 'maybe' a constant
        const expr_bool* const bol{std::get_if<expr_bool>(&e)};

        if (bol != nullptr) {
            const std::optional<bool> value{bol->constant_value(tc)};

            if (not value) {
                return std::nullopt;
            }

            return *value ? 1 : 0;
        }

        return constant_element_value(tc, e, width_type);
    }

    [[nodiscard]] auto element_count() const -> size_t { return vars_.size(); }

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }

    [[nodiscard]] auto is_array_identifier() const -> bool {
        return is_array_ and is_identifier_;
    }

    [[nodiscard]] auto is_empty() const -> bool {
        return vars_.empty() and not is_string();
    }

    [[nodiscard]] auto is_string() const -> bool {
        return string_tk_.is_string();
    }

    [[nodiscard]] auto open_bracket_token() const -> const token& {
        return open_bracket_tk_;
    }

  private:
    // the remaining count would wrap around past the array size
    auto assert_room_for_element(const tokenizer& tz) const -> void {
        if (array_count_ == 0 or vars_.size() != array_count_) {
            return;
        }

        throw compiler_exception{
            tz, std::format("too many elements specified for array of "
                            "size {}",
                            array_count_)};
    }

    // constant elements are stored like a string so the backend can pack them
    // into wider immediates or copy them from read-only data, 'dst_info'
    // advances past the listed elements
    auto compile_elements(toc& tc, const size_t indent,
                          ident_info& dst_info) const -> void {

        machine& x{tc.machine()};

        const std::optional<std::string> bytes{
            constant_bytes(tc, dst_info.type_ref()),
        };

        if (bytes) {
            x.copy_bytes(open_brace_tk_, indent, *bytes, dst_info.operand,
                         dst_info.type_ref().alignment(), [&] -> std::string {
                             return tc.add_bytes_constant(open_brace_tk_,
                                                          *bytes);
                         });

            dst_info.operand.increment_offset(address_offset(bytes->size()));

            return;
        }

        for (const auto [i, e] : std::views::enumerate(vars_)) {
            x.comment(tok(), indent, "[{}]", i);
            compile_variant(tc, indent, dst_info, tok(), e);

            dst_info.operand.increment_offset(
                address_offset(dst_info.type_ref().size_bytes()));
        }
    }

    // the string is stored and the rest of the array is zeroed like unlisted
    // elements
    auto compile_string(toc& tc, const size_t indent,
                        const ident_info& dst_info) const -> void {

        machine& x{tc.machine()};

        const std::string bytes{token::decode_string(string_tk_.string_text())};

        const size_t size_bytes{bytes.size()};

        const size_t array_count{destination_array_count(dst_info)};

        if (size_bytes > array_count) {
            throw compiler_exception{
                string_tk_,
                std::format("string size {} overflows array size {}",
                            size_bytes, array_count)};
        }

        operand dst{dst_info.operand};

        // an empty string has nothing to store
        if (size_bytes != 0) {
            x.copy_bytes(string_tk_, indent, bytes, dst,
                         dst_info.type_ref().alignment(), [&] -> std::string {
                             return tc.add_string_constant(string_tk_);
                         });

            dst.increment_offset(address_offset(size_bytes));
        }

        const size_t remaining_size_bytes{array_count - size_bytes};

        if (remaining_size_bytes == 0) {
            return;
        }

        x.comment(string_tk_, indent, "zero remaining elements: {} B",
                  remaining_size_bytes);

        x.zero(string_tk_, indent, dst, remaining_size_bytes,
               dst_info.type_ref().alignment());
    }

    // the little endian bytes of the elements, empty when an element is not a
    // constant or there are no elements to store
    [[nodiscard]] auto constant_bytes(const toc& tc,
                                      const type& element_type) const
        -> std::optional<std::string> {

        if (vars_.empty()) {
            return std::nullopt;
        }

        std::string bytes;
        for (const expr_variant& e : vars_) {
            const std::optional<int64_t> value{
                constant_element_value(tc, e, tc.get_type_default()),
            };

            if (not value) {
                return std::nullopt;
            }

            // e.g. '{300}' for 'i8' is rejected as when stored alone
            std::get<expr_arith>(e).assert_not_narrowed(tc, element_type);

            const uint64_t bits{static_cast<uint64_t>(*value)};
            for (size_t i{}; i < element_type.size_bytes(); ++i) {
                bytes +=
                    static_cast<char>(bits >> (machine::bits_per_byte * i));
            }
        }

        return bytes;
    }

    // an unsized destination e.g. a parameter 's[]' has the size of the
    // argument which is known only at compile
    [[nodiscard]] auto destination_array_count(const ident_info& dst_info) const
        -> size_t {

        if (is_unsized_destination_) {
            return dst_info.array_len;
        }

        return array_count_;
    }

    // the elements between the braces, e.g. '1, 2, 3' of '{1, 2, 3}'
    auto parse_braced_elements(toc& tc, tokenizer& tz, const type& tp,
                               const bool in_args) -> void {

        close_brace_tk_ = tz.is_next_char_token('}');

        while (close_brace_tk_.is_empty()) {
            if (not vars_.empty()) {
                parse_element_delimiter(tz, tp);
            }

            assert_room_for_element(tz);

            vars_.emplace_back(parse_variant(tc, tz, tp, in_args));

            close_brace_tk_ = tz.is_next_char_token('}');
        }

        if (array_count_ == 0) {
            array_count_ = vars_.size();
        }
    }

    auto parse_element_delimiter(tokenizer& tz, const type& tp) -> void {
        const token delimiter_tk{
            tz.expect_char_token(
                ',', std::format("expected ',' followed by initializer "
                                 "for type '{}'",
                                 tp.name())),
        };

        var_delims_tk_.emplace_back(delimiter_tk);
    }

    // e.g. 'i8[3]' in 'i8[3]{1, 2}' names the element type that '{1, 2}' takes
    // from the destination, and the size unless it is 'i8[]'; '[3]{1, 2}' has
    // the default type
    auto parse_element_type(toc& tc, tokenizer& tz, const type& tp) -> void {
        const token tk{tz.next_token()};
        const bool is_typed{is_array_literal(tc, tk, tz)};

        if (not is_typed) {
            tz.put_back_token(tk);
        }

        if (not is_typed and not is_default_array_literal(tz)) {
            return;
        }

        open_bracket_tk_ = tz.is_next_char_token('[');

        const type& literal_type{
            is_typed ? tc.get_type_or_throw(tk, tk.text())
                     : tc.get_type_default(),
        };

        if (not literal_type.is_same(tp)) {
            throw compiler_exception{is_typed ? tk : open_bracket_tk_,
                                     std::format("expected type '{}', got '{}'",
                                                 tp.name(),
                                                 literal_type.name())};
        }

        if (is_typed) {
            element_type_tk_ = tk;
        }

        // e.g. the '4' of 'i8[4]', none for 'i8[]'
        literal_count_const_ = {tc, tz, 0};

        close_bracket_tk_ =
            tz.expect_char_token(']', "expected ']' after array size");

        if (not literal_count_const_.has_value()) {
            return;
        }

        if (literal_count_const_.value() <= 0) {
            throw compiler_exception{
                literal_count_const_.tok(),
                "expected a constant array size greater than 0"};
        }

        const size_t count{static_cast<size_t>(literal_count_const_.value())};

        // e.g. 'a = i8[3]{1, 2}' for 'var a = i8[2]{}'
        if (array_count_ != 0 and array_count_ != count) {
            throw compiler_exception{
                literal_count_const_.tok(),
                std::format("array size {} does not match destination size {}",
                            count, array_count_)};
        }

        array_count_ = count;
    }

    //
    // statics
    //

    static auto compile_bool(toc& tc, const size_t indent,
                             const ident_info& dst_info,
                             const token& src_loc_tk,
                             const expr_bool& condition) -> void {

        machine& x{tc.machine()};

        // e.g. 'true' or a named constant
        if (not condition.is_expression()) {
            const ident_info src_info{tc.make_ident_info(condition)};

            assert(src_info.is_const());

            x.copy_value(src_loc_tk, indent, dst_info.operand,
                         operand::imm(std::format("{}", src_info.const_value),
                                      src_info.type_ref()));

            return;
        }

        // stored comparisons would change what later elements read
        if (dst_info.is_register() or
            not condition.reads_var(dst_info.root_id())) {

            compile_bool_list(tc, indent, src_loc_tk, condition,
                              dst_info.operand);

            return;
        }

        const operand reg{
            x.alloc_scratch_register(src_loc_tk, indent, dst_info.type_ref()),
        };

        compile_bool_list(tc, indent, src_loc_tk, condition, reg);
        x.copy_value(src_loc_tk, indent, dst_info.operand, reg);
        x.free_scratch_register(src_loc_tk, indent, reg);
    }

    static auto compile_bool_list(toc& tc, const size_t indent,
                                  const token& src_loc_tk,
                                  const expr_bool& condition,
                                  const operand& dst) -> void {

        // labels to jump to depending on the evaluation
        const std::string jmp_to_end{
            toc::end_label(tc.create_unique_label(src_loc_tk, "bool")),
        };

        // compile and possibly evaluate constant expression
        const condition_result const_eval{
            condition.compile(tc, indent, jmp_to_end, jmp_to_end, dst),
        };

        machine& x{tc.machine()};

        // not constant evaluation
        x.label(indent, jmp_to_end);

        // a constant evaluation stores its value
        if (is_known(const_eval)) {
            x.store_boolean(src_loc_tk, indent, dst, result_value(const_eval));
        }
    }

    static auto compile_variant(toc& tc, const size_t indent,
                                const ident_info& dst_info,
                                const token src_loc_tk, const expr_variant& exp)
        -> void {

        exp.visit(overloaded{
            [&](const expr_arith& e) -> void {
                // the value boundary is where a wider source
                // loses bits
                e.assert_not_narrowed(tc, dst_info.type_ref());
                e.compile(tc, indent, dst_info);
            },
            [&](const expr_type& e) -> void {
                e.compile(tc, indent, dst_info);
            },
            [&](const expr_bool& e) -> void {
                compile_bool(tc, indent, dst_info, src_loc_tk, e);
            },
        });
    }

    // 'bool' and instance elements are not packed
    [[nodiscard]] static auto constant_element_value(const toc& tc,
                                                     const expr_variant& e,
                                                     const type& width_type)
        -> std::optional<int64_t> {

        const expr_arith* const arith{std::get_if<expr_arith>(&e)};

        if (arith == nullptr) {
            return std::nullopt;
        }

        // e.g. 'array_length(a)'
        if (arith->is_expression()) {
            return arith->folded_constant(tc, width_type);
        }

        const ident_info info{tc.make_ident_info(*arith)};

        if (not info.is_const()) {
            return std::nullopt;
        }

        return arith->get_unary_ops().evaluate_constant(info.const_value);
    }

    [[nodiscard]] static auto parse_variant(toc& tc, tokenizer& tz,
                                            const type& tp, const bool in_args)
        -> expr_variant {

        if (not tp.is_builtin()) {
            // destination is not a built-in (register) value
            // assume assign type value
            return expr_type{tc, tz, tp, false};
        }

        if (tp.is_bool()) {
            // destination is boolean

            // e.g. 'var b = bool' is 'var b = false'
            const token pos_tk{tz.cur_position_token()};
            const token tk{tz.next_token()};

            if (is_bare_builtin_type(tc, tk, tz)) {
                return expr_bool{
                    tc,
                    pos_tk,
                    tz,
                    false,
                    {},
                    {},
                    std::make_unique<stmt_builtin_convert>(tc, tk),
                };
            }

            // note: rewound by position, a string token cannot be put back
            tz.rewind_to_position(pos_tk);

            return expr_bool{tc, tz.next_whitespace_token(), tz};
        }

        // destination is a built-in (register) value

        // e.g. 'var x = i8' is 'var x = i8(0)'
        const token pos_tk{tz.cur_position_token()};
        const token tk{tz.next_token()};

        if (is_bare_builtin_type(tc, tk, tz)) {
            return expr_arith{
                tc,
                tz,
                expr_arith::options{
                    .in_args{in_args},
                    .uops{},
                    .open_paren_tk{},
                    .is_implied_subexpression{},
                    .first_op_precedence{expr_arith::initial_precedence},
                    .first_expression{
                        std::make_unique<stmt_builtin_convert>(tc, tk),
                    },
                },
            };
        }

        tz.rewind_to_position(pos_tk);

        return expr_arith{tc, tz, in_args};
    }
};
