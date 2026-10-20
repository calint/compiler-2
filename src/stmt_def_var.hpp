#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <optional>
#include <ostream>
#include <string_view>
#include <utility>

#include "compile_error.hpp"
#include "expr_arith.hpp"
#include "expr_bool.hpp"
#include "ident_info.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "parse_lookahead.hpp"
#include "stmt_assign_var.hpp"
#include "stmt_builtin_convert.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "type.hpp"
#include "ub_assigned.hpp"
#include "ub_reads.hpp"

// whether the name of a definition is visible in its initializer: a 'var' that
// reads itself is rejected as uninitialized, a 'dat' may be named like its
// type, e.g. 'dat point = point{1, 2}'
enum class name_in_initializer : uint8_t { visible, hidden };

// the type and the array size that an initializer gives a definition, e.g.
// 'var a = i8[]{1, 2}' or 'dat p = point{1, 2}', and its value at them; the
// initializer is parsed once to learn them and again at them
class initializer_declaration final {
    token name_tk_;
    name_in_initializer name_visibility_;
    const type* type_{};
    // none when not an array, 0 only while the initializer that gives the
    // size is parsed
    std::optional<size_t> array_count_;

  public:
    // 'tz' is at the initializer, after the '='
    initializer_declaration(toc& tc, tokenizer& tz, const token& name_tk,
                            const name_in_initializer name_visibility)
        : name_tk_{name_tk}, name_visibility_{name_visibility} {

        deduce_declaration(tc, tz);

        // note: both deductions reject an array whose size is not known
        assert(array_count_ != 0);
    }

    [[nodiscard]] auto array_count() const -> std::optional<size_t> {
        return array_count_;
    }

    // the value at the deduced type and size
    [[nodiscard]] auto parse_value(toc& tc, tokenizer& tz) const -> expr_any {

        expr_any value;

        with_placeholder(tc, [&] -> void {
            value = {tc, tz, type_ref(), literal_typing::written, array_count_};
        });

        return value;
    }

    [[nodiscard]] auto type_ref() const -> const type& { return *type_; }

  private:
    // e.g. 'var c = a' copies the whole array 'a'
    auto deduce_array_copy(toc& tc, const token& tk, const expr_arith& arith)
        -> void {

        const ident_info info{tc.make_ident_info(arith)};

        if (not info.is_array()) {
            return;
        }

        // e.g. an unsized array parameter
        if (info.array_len() == 0) {
            throw compile_error{tk,
                                std::format("size of array '{}' is not known",
                                            arith.name_or_literal())};
        }

        array_count_ = info.array_len();
    }

    // e.g. 'var b = x < 3' is a 'bool', 'var a = i8[]{1, 2}' an 'i8' array and
    // 'var s = "hi"' an 'i8' array of 2
    auto deduce_declaration(toc& tc, tokenizer& tz) -> void {
        const bool is_string{tz.peek_char_after_whitespace() == '"'};

        if (not is_string and not starts_array_literal(tc, tz)) {
            set_type(tc.builtins().default_type());

            trial_parse(tc, tz, [this, &tc, &tz] -> void {
                deduce_from_initializer(tc, tz);
            });

            return;
        }

        array_count_ = 0;
        set_type(is_string ? tc.builtins().i8() : array_element_type(tc, tz));

        // later statements read the size while parsing, e.g. 'var b = a'
        token bracket_tk;

        trial_parse(tc, tz, [this, &tc, &tz, &bracket_tk] -> void {
            // an unsized array, the initializer gives the count
            const expr_any initializer{tc, tz, type_ref(),
                                       literal_typing::written, 0};

            array_count_ = initializer.array_count();
            bracket_tk = initializer.open_bracket_token();
        });

        if (array_count_ == 0) {
            // e.g. the '[' of 'var a = i8[]{}'
            // note: an empty string is rejected by 'string_array_count'
            assert(not bracket_tk.is_empty());

            throw compile_error{bracket_tk, "an empty array needs a size"};
        }
    }

    // e.g. 'point{1, 2}', 'x < 3', 'flag', 'p', 'f(x)', 'i32(x)', 'a + 1' or
    // the array 'a'
    auto deduce_from_initializer(toc& tc, tokenizer& tz) -> void {
        // 'expr_arith' cannot parse the '{' of an instance literal
        const token tk{tz.next_token()};

        if (is_record_literal(tc, tk, tz) or is_bare_record_type(tc, tk, tz)) {
            set_type(tc.get_type_or_throw(tk, tk.text()));
            return;
        }

        // e.g. 'var x = i8' is 'i8(0)'
        if (is_bare_builtin_type(tc, tk, tz)) {
            set_type(stmt_builtin_convert::conversion_type(tc, tk));
            return;
        }

        tz.put_back_token(tk);

        // 'expr_bool' parses any arithmetic too, as a comparison shorthand
        const expr_bool condition{tc, tz.next_whitespace_token(), tz};

        // a comparison, 'not', 'and' or 'or'
        const expr_arith* const arith{condition.arithmetic()};

        if (arith == nullptr) {
            set_type(tc.builtins().boolean());
            return;
        }

        // the first operand decides the width of an expression
        if (not arith->is_single_operand()) {
            set_type(arith->get_type());
            return;
        }

        set_type(arith->single_operand_type());

        if (not arith->is_name_or_literal() or
            not arith->get_unary_ops().is_empty()) {

            return;
        }

        deduce_array_copy(tc, tk, *arith);
    }

    // the definition is in scope while its initializer is parsed, see
    // 'with_placeholder'
    [[nodiscard]] auto placeholder_info() const -> var_info {
        return {
            .name{name_tk_.text()},
            .type_ptr{type_},
            .src_loc_tk{name_tk_},
            .is_array{array_count_.has_value()},
            .array_len{array_count_.value_or(0)},
            .pointer_register{},
            .base_register{},
            .value_register{},
            .foo_array{},
        };
    }

    auto set_type(const type& tp) -> void { type_ = &tp; }

    // a value cannot be parsed at its type before the type is known, so the
    // initializer is parsed once to learn the type or size and that tree is
    // dropped
    auto trial_parse(toc& tc, tokenizer& tz,
                     const std::function_ref<void()> parse) const -> void {

        const token start_tk{tz.cur_position_token()};

        with_placeholder(tc, parse);

        tz.rewind_to_position(start_tk);
    }

    // the variable is in scope while its initializer is parsed, so a read of
    // it, e.g. 'var x = x + 1', resolves and is rejected as uninitialized
    auto with_placeholder(toc& tc, const std::function_ref<void()> parse) const
        -> void {

        if (name_visibility_ == name_in_initializer::hidden) {
            parse();
            return;
        }

        tc.enter_block();
        tc.add_var(name_tk_, 0, placeholder_info(), var_kind::var);

        parse();

        tc.exit_block();
    }

    //
    // statics
    //

    // e.g. 'i8' in 'i8[]{1, 2}' or the default type in '[]{1, 2}'
    [[nodiscard]] static auto array_element_type(const toc& tc, tokenizer& tz)
        -> const type& {

        if (is_default_array_literal(tz)) {
            return tc.builtins().default_type();
        }

        const token tk{tz.next_token()};
        tz.put_back_token(tk);

        return tc.get_type_or_throw(tk, tk.text());
    }

    // e.g. 'i8[3]{1, 2}', '[]{1, 2}' or 'point[]{{1, 2}}'
    [[nodiscard]] static auto starts_array_literal(const toc& tc, tokenizer& tz)
        -> bool {

        if (is_default_array_literal(tz)) {
            return true;
        }

        const token tk{tz.next_token()};
        const bool is_literal{is_array_literal(tc, tk, tz)};
        tz.put_back_token(tk);

        return is_literal;
    }
};

// e.g. 'var x = i32(0)', the initializer gives the type; 'let' with a
// non-constant initializer is a 'var' that is read-only once initialized
class stmt_def_var final : public statement {
    token name_tk_;
    // none when not an array
    std::optional<size_t> array_count_;
    token equals_tk_;
    stmt_assign_var assign_var_;
    bool is_let_{};
    // a 'let' of an expression that folds is a constant, e.g. 'let x = 2 * 3'
    expr_any constant_expr_;
    std::optional<int64_t> constant_value_;

  public:
    stmt_def_var(toc& tc, const token tk, tokenizer& tz)
        : statement{tk}, name_tk_{tz.next_token()}, is_let_{tk.is_text("let")} {

        toc::assert_valid_unqualified_name(name_tk_);

        // the destination is read where it ends at '=', past it the '[2]' in
        // 'var a = [2]{}' would read as an index
        tokenizer at_name{tz};

        equals_tk_ = parse_initializer_equals(tz, tk.text());

        const initializer_declaration declaration{
            tc,
            tz,
            name_tk_,
            name_in_initializer::visible,
        };

        set_type(declaration.type_ref());
        array_count_ = declaration.array_count();

        expr_any value{declaration.parse_value(tc, tz)};

        constant_value_ = folded_let(tc, value);

        // nothing is stored for a constant
        if (constant_value_.has_value()) {
            constant_expr_ = std::move(value);
            tc.add_const(name_tk_, 0, name_tk_.text(), *constant_value_);
            return;
        }

        // the variable is declared during the parse so later statements can
        // name it
        tc.add_var(name_tk_, 0, make_var_info(), var_kind::var);

        assign_var_ = {
            tc,
            stmt_identifier{tc, {}, name_tk_, at_name},
            equals_tk_,
            std::move(value),
        };

        // the newly defined variable is not yet assigned in its initialization
        assert_var_not_used(
            name_tk_.text(),
            assigned_bytes{multiply_storage_size(
                name_tk_, get_type().size_bytes(), array_count_.value_or(1))});

        // marked after the initializer, which writes the variable; statements
        // parsed from here on cannot assign it
        if (is_let_) {
            tc.make_var_read_only(name_tk_.text(), read_only_cause::let);
        }
    }

    stmt_def_var() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        equals_tk_.source_to(os);

        if (constant_value_.has_value()) {
            constant_expr_.source_to(os);
            return;
        }

        assign_var_.expression().source_to(os);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        if (constant_value_.has_value()) {
            tc.add_const(name_tk_, indent, name_tk_.text(), *constant_value_);
            return;
        }

        // an unsized array has its size from the initializer by now
        tc.add_var(name_tk_, indent, make_var_info(), var_kind::var);

        // the assignment names its destination itself
        assign_var_.compile(tc, indent, destination{});
    }

    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        if (constant_value_.has_value()) {
            return;
        }

        assign_var_.visit_reads(var, reader);
    }

  private:
    // the value of the initializer of a 'let' of the default type when it folds
    // to a constant, e.g. '2 * 3 + k', otherwise none
    [[nodiscard]] auto folded_let(const toc& tc, const expr_any& value) const
        -> std::optional<int64_t> {

        if (not is_let_ or array_count_.has_value() or
            not get_type().is_same(tc.builtins().default_type())) {

            return std::nullopt;
        }

        return value.constant_value(tc);
    }

    [[nodiscard]] auto make_var_info() const -> var_info {
        return {
            .name{name_tk_.text()},
            .type_ptr{&get_type()},
            .src_loc_tk{name_tk_},
            .is_array{array_count_.has_value()},
            .array_len{array_count_.value_or(0)},
            .pointer_register{},
            .base_register{},
            .value_register{},
            .foo_array{},
        };
    }
};
