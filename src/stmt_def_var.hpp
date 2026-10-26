#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <string_view>
#include <utility>

#include "compile_error.hpp"
#include "expr_any.hpp"
#include "expr_arith.hpp"
#include "expr_bool.hpp"
#include "ident_info.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "parse_lookahead.hpp"
#include "stmt_assign_var.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "type.hpp"
#include "ub_assigned.hpp"
#include "ub_reads.hpp"

//
// a definition with an initializer, the types below in reading order:
//
//   'name_in_initializer'    whether the name is seen in its initializer
//   'initializer_declaration'
//                            the part 'var', 'let' and 'dat' share: the type
//                            or size from the initializer
//   'stmt_def_const'         a 'let' that folds, a constant
//   'stmt_def_var'           a 'var' or a 'let' that is a variable
//   'make_stmt_def_var'      reads the keyword and picks the statement
//

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
    // of unknown length only while the initializer that gives the length is
    // parsed
    array_shape shape_;

  public:
    // 'tz' is at the initializer, after the '='
    initializer_declaration(toc& tc, tokenizer& tz, const token& name_tk,
                            const name_in_initializer name_visibility)
        : name_tk_{name_tk}, name_visibility_{name_visibility} {

        deduce_declaration(tc, tz);

        // note: both deductions reject an array whose size is not known
        assert(not shape_.is_length_unknown());
    }

    // the folded 'value', read with the name in scope like its parse, so
    // 'let k = k + 1' reads the uninitialized 'k', not a constant of an outer
    // block
    [[nodiscard]] auto fold(toc& tc, const expr_any& value) const
        -> std::optional<folded_value> {

        std::optional<folded_value> constant;

        with_placeholder(tc, [&] -> void {
            constant = value.fold_checked(tc, tc.use_at(type_ref()));
        });

        // note: untyped only at the default type, e.g. 'let k = 5' fits any
        //       integer where it is used; 'let b = true' is a 'bool'

        if (constant and not constant->is_typed() and
            not type_ref().is_same(tc.builtins().default_type())) {

            constant->own_type = &type_ref();
        }

        return constant;
    }

    // the value at the deduced type and size
    [[nodiscard]] auto parse_value(toc& tc, tokenizer& tz) const -> expr_any {
        // note: set by the parse, which throws when it fails
        std::optional<expr_any> value;

        with_placeholder(tc, [&] -> void {
            value.emplace(tc, tz, type_ref(), literal_typing::written, shape_);
        });

        assert(value);

        return std::move(*value);
    }

    [[nodiscard]] auto shape() const -> array_shape { return shape_; }

    [[nodiscard]] auto type_ref() const -> const type& { return *type_; }

    //
    // statics
    //

    // a variable, a 'var' or a 'dat', named by 'name_tk'
    [[nodiscard]] static auto
    make_var_info(const token& name_tk, const type& tp, const array_shape shape)
        -> var_info {

        return {
            .name{name_tk.text()},
            .type_ptr{&tp},
            .src_loc_tk{name_tk},
            .shape{shape},
            .placement{},
            .foo_array{},
        };
    }

    // the '=' before the initializer, 'keyword' names the definition in the
    // example
    [[nodiscard]] static auto read_equals(tokenizer& tz,
                                          const std::string_view keyword)
        -> token {

        const token equals_tk{
            tz.expect_char_token(
                '=', std::format("expected '=' followed by an initializer, "
                                 "e.g. '{} x = i32(0)'",
                                 keyword)),
        };

        return equals_tk;
    }

  private:
    // e.g. 'var s = "hi"' is an 'i8' array of 2
    auto deduce_declaration(toc& tc, tokenizer& tz) -> void {
        // note: the placeholder has the default type until the type is known
        set_type(tc.builtins().default_type());

        // the name is visible like in the parse, e.g. 'point' in
        // 'var point = point[2]' is the variable
        const type* element_type{};

        with_placeholder(tc, [&element_type, &tc, &tz] -> void {
            element_type = literal_element_type(tc, tz);
        });

        if (element_type == nullptr) {
            trial_parse(tc, tz, [this, &tc, &tz] -> void {
                deduce_from_initializer(tc, tz);
            });

            return;
        }

        shape_ = array_shape::array(std::nullopt);
        set_type(*element_type);

        std::optional<token> bracket_tk;

        trial_parse(tc, tz, [this, &tc, &tz, &bracket_tk] -> void {
            // an unsized array, the initializer gives the count
            const expr_any initializer{tc, tz, type_ref(),
                                       literal_typing::written, shape_};

            // later statements read the size while parsing, e.g. 'var b = a'
            shape_ = array_shape::array(initializer.array_count());
            bracket_tk = initializer.open_bracket_token();
        });

        if (shape_.length().value_or(0) == 0) {
            // e.g. the '[' of 'var a = i8[]{}'
            // note: an empty string is rejected by 'string_array_count'
            assert(bracket_tk);

            throw compile_error{*bracket_tk, "an empty array needs a size"};
        }
    }

    // e.g. the array 'a'
    // note: the type of a whole initializer, e.g. 'bool' for 'a < b' and the
    //       first operand for 'a + 1', so the initializer is parsed; a generic
    //       argument tells its type only as a whole term, see
    //       'generic_call::value_type'
    auto deduce_from_initializer(toc& tc, tokenizer& tz) -> void {
        // 'expr_arith' cannot parse the '{' of an instance literal
        const token tk{tz.next_token()};

        if (const term tm{classify_term(tc, tk, tz)};
            tm.is(term_kind::record_literal) or tm.is(term_kind::record_type)) {

            set_type(tc.get_type_or_throw(tk));
            return;
        }

        // e.g. 'var x = i8' is 'i8(0)'
        if (is_bare_builtin_type(tc, tk, tz)) {
            set_type(tc.get_type_or_throw(tk));
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
        set_type(arith->get_type());

        const stmt_identifier* const ident{arith->bare_path()};

        if (ident == nullptr) {
            return;
        }

        deduce_whole_array_copy(tc, tk, *ident);
    }

    // e.g. 'var c = a' copies the whole array 'a'
    auto deduce_whole_array_copy(toc& tc, const token& tk,
                                 const stmt_identifier& ident) -> void {

        const ident_info info{make_ident_info(tc, ident)};

        if (not info.is_array()) {
            return;
        }

        // e.g. an unsized array parameter
        if (info.is_length_unknown()) {
            throw compile_error{tk,
                                std::format("size of array {} is not known",
                                            in_quotes(ident.dotted_path()))};
        }

        shape_ = info.shape();
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

        const toc::scope_entry block{tc, scope::block()};

        tc.add_var(name_tk_, 0, make_var_info(name_tk_, type_ref(), shape_),
                   var_kind::var);

        parse();
    }

    //
    // statics
    //

    // the element type of a string or of an array literal, e.g. 'i8' of
    // '"hi"', 'point' of 'point[]{{1, 2}}' or the default type of '[]{1, 2}',
    // null when the initializer is neither
    [[nodiscard]] static auto literal_element_type(const toc& tc, tokenizer& tz)
        -> const type* {

        if (tz.peek_char_after_whitespace() == '"') {
            return &tc.builtins().i8();
        }

        if (is_default_array_literal(tz)) {
            return &tc.builtins().default_type();
        }

        const token tk{tz.next_token()};

        const bool is_literal{
            classify_term(tc, tk, tz).is(term_kind::array_literal),
        };

        tz.put_back_token(tk);

        if (not is_literal) {
            return nullptr;
        }

        return &tc.get_type_or_throw(tk);
    }
};

// a 'let' whose initializer folds to a constant, e.g. 'let n = 2 * 3' or
// 'let k = i8(3)': nothing is stored, the name stands for the value
class stmt_def_const final : public statement {
    token name_tk_;
    token equals_tk_;
    expr_any initializer_;
    // typed when the initializer is, e.g. 'let v = i8(5)' is checked by type
    // like a variable of 'i8'
    folded_value constant_;

  public:
    // the constant is declared during the parse so later statements can name
    // it, e.g. as an array size
    stmt_def_const(toc& tc, const token tk, const token name_tk,
                   const token equals_tk, const type& tp, expr_any initializer,
                   const folded_value& constant)
        : statement{tk}, name_tk_{name_tk}, equals_tk_{equals_tk},
          initializer_{std::move(initializer)}, constant_{constant} {

        set_type(tp);

        tc.add_const(name_tk_, 0, name_tk_.text(), constant_);
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        equals_tk_.source_to(os);
        initializer_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        // note: the names it folds from are constants wherever it is
        //       compiled, a name that is a constant only with a call is a
        //       variable in the parse, e.g. the 'n' of a 'foo'

        assert(initializer_.constant_value(tc) == constant_.value);

        tc.add_const(name_tk_, indent, name_tk_.text(), constant_);
    }

    auto trace_assignment(assignment_flow& flow) const -> void override {
        flow.declare(name_tk_.text());
    }
};

// e.g. 'var x = i32(0)', the initializer gives the type; a 'let' that does not
// fold is a 'var' that is read-only once initialized
class stmt_def_var final : public statement {
    token name_tk_;
    array_shape shape_;
    token equals_tk_;
    stmt_assign_var assign_var_;

  public:
    // 'at_name' is the tokenizer after the name, where the destination
    // continues
    stmt_def_var(toc& tc, const token tk, const token name_tk,
                 tokenizer at_name, const token equals_tk,
                 const initializer_declaration& declaration, expr_any value)
        : statement{tk}, name_tk_{name_tk}, shape_{declaration.shape()},
          equals_tk_{equals_tk},
          assign_var_{
              declare_and_assign(tc, name_tk, at_name, equals_tk, declaration,
                                 std::move(value)),
          } {

        set_type(declaration.type_ref());

        // the newly defined variable is not yet assigned in its initialization
        assert_var_not_used(
            name_tk_.text(),
            assigned_bytes{shape_.value_size_bytes(name_tk_, get_type())});

        // marked after the initializer, which writes the variable; statements
        // parsed from here on cannot assign it
        if (tk.is_text("let")) {
            tc.make_var_read_only(name_tk_.text(), read_only_cause::let);
        }
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        equals_tk_.source_to(os);
        assign_var_.expression().source_to(os);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        tc.add_var(name_tk_, indent, make_var_info(), var_kind::var);

        // the assignment names its destination itself
        assign_var_.compile(tc, indent, destination{});
    }

    // the initializer is read before the name is declared
    auto trace_assignment(assignment_flow& flow) const -> void override {
        assert_var_not_used(flow);
        flow.declare(name_tk_.text());
    }

    auto visit_operands(const operand_visitor visitor) const -> void override {
        assign_var_.visit_operands(visitor);
    }

  private:
    [[nodiscard]] auto make_var_info() const -> var_info {
        return initializer_declaration::make_var_info(name_tk_, get_type(),
                                                      shape_);
    }

    //
    // statics
    //

    // the variable is declared during the parse so later statements can name
    // it, its initializer names it as the destination
    [[nodiscard]] static auto
    declare_and_assign(toc& tc, const token& name_tk, tokenizer at_name,
                       const token& equals_tk,
                       const initializer_declaration& declaration,
                       expr_any value) -> stmt_assign_var {

        tc.add_var(name_tk, 0,
                   initializer_declaration::make_var_info(
                       name_tk, declaration.type_ref(), declaration.shape()),
                   var_kind::var);

        return {
            tc,
            stmt_identifier{tc, name_tk, at_name},
            equals_tk,
            std::move(value),
        };
    }
};

// reads the name and the initializer and makes the statement of the kind they
// give: a constant for a 'let' that folds, e.g. 'let n = 2 * 3', else a
// variable
inline auto make_stmt_def_var(toc& tc, const token tk, tokenizer& tz)
    -> std::unique_ptr<statement> {

    const token name_tk{tz.next_token()};

    tc.assert_valid_unqualified_name(name_tk);

    // the destination is read where it ends at '=', past it the '[2]' in
    // 'var a = [2]{}' would read as an index
    const tokenizer at_name{tz};

    const token equals_tk{initializer_declaration::read_equals(tz, tk.text())};

    const initializer_declaration declaration{
        tc,
        tz,
        name_tk,
        name_in_initializer::visible,
    };

    expr_any value{declaration.parse_value(tc, tz)};

    // a constant is not an array
    const bool may_fold{
        tk.is_text("let") and not declaration.shape().is_array(),
    };

    const std::optional<folded_value> constant{
        may_fold ? declaration.fold(tc, value) : std::nullopt,
    };

    if (constant) {
        return std::make_unique<stmt_def_const>(tc, tk, name_tk, equals_tk,
                                                declaration.type_ref(),
                                                std::move(value), *constant);
    }

    return std::make_unique<stmt_def_var>(tc, tk, name_tk, at_name, equals_tk,
                                          declaration, std::move(value));
}
