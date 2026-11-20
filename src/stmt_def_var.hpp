#pragma once

#include <cassert>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <string_view>
#include <tuple>
#include <utility>

#include "compile_error.hpp"
#include "expr_any.hpp"
#include "expr_arith.hpp"
#include "expr_bool.hpp"
#include "ident_info.hpp"
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
// reads itself is rejected as read in its own initializer, a 'dat' may be
// named like its type, e.g. 'dat point = point{1, 2}'
enum class name_in_initializer : uint8_t { visible, hidden };

// the name of a definition, its '=' and the type and the array size that its
// initializer gives, e.g. 'var a = i8[]{1, 2}' or 'dat p = point{1, 2}', and
// its value at them; the initializer is parsed once to learn them and again
// at them
class initializer_declaration final {
    token name_tk_;
    // after the name, where a destination that names it continues
    tokenizer at_name_;
    token equals_tk_;
    name_in_initializer name_visibility_{};
    const type* type_{};
    // of unknown length only while the initializer that gives the length is
    // parsed
    array_shape shape_;

  public:
    // 'tz' is at the name, after 'keyword', and is left at the initializer
    // for 'parse_value'
    initializer_declaration(parse_toc& tc, tokenizer& tz,
                            const std::string_view keyword,
                            const name_in_initializer name_visibility)
        : name_tk_{read_name(tc, tz)}, at_name_{tz},
          equals_tk_{read_equals(tz, keyword, name_tk_)},
          name_visibility_{name_visibility} {

        deduce_declaration(tc, tz);

        // note: both deductions reject an array whose size is not known
        assert(not shape_.is_length_unknown());
    }

    // the destination is read where it ends at '=', past it the '[2]' in
    // 'var a = [2]{}' would read as an index
    [[nodiscard]] auto at_name() const -> tokenizer { return at_name_; }

    // the variable the definition declares, a 'var' or a 'dat'
    [[nodiscard]] auto declared_var() const -> var_info {
        return {
            .name{name_tk_.text()},
            .type_ptr{type_},
            .src_loc_tk{name_tk_},
            .shape{shape_},
            .read_only_why{},
            .placement{},
            .foo_array{},
        };
    }

    [[nodiscard]] auto equals_tk() const -> const token& { return equals_tk_; }

    [[nodiscard]] auto name_tk() const -> const token& { return name_tk_; }

    // the value at the deduced type and size
    [[nodiscard]] auto parse_value(parse_toc& tc, tokenizer& tz) const
        -> expr_any {

        // note: set by the parse, which throws when it fails
        std::optional<expr_any> value;

        with_placeholder(tc, [&] -> void {
            value.emplace(tc, tz, type_ref(), literal_typing::written, shape_);
        });

        assert(value);

        return std::move(*value);
    }

    [[nodiscard]] auto type_ref() const -> const type& { return *type_; }

  private:
    // sets the type and size, e.g. 'var s = "hi"' is an 'i8' array of 2
    auto deduce_declaration(parse_toc& tc, tokenizer& tz) -> void {
        // note: the placeholder has the default type until the type is known
        set_type(tc.builtins().default_type());

        // the name is visible like in the parse, e.g. 'point' in
        // 'var point = point[2]' is the variable
        const type* element_type{};

        with_placeholder(tc, [&element_type, &tc, &tz] -> void {
            element_type = literal_element_type(tc, tz);
        });

        if (element_type == nullptr) {
            trial_parse(tc, tz, [this, &tc](tokenizer& probe) -> void {
                deduce_from_initializer(tc, probe);
            });

            return;
        }

        deduce_literal_length(tc, tz, *element_type);
    }

    // note: the type of a whole initializer, e.g. 'bool' for 'a < b' and the
    //       first operand for 'a + 1', so the initializer is parsed; a generic
    //       argument tells its type only as a whole term, see
    //       'generic_call::value_type'

    auto deduce_from_initializer(parse_toc& tc, tokenizer& tz) -> void {
        // 'expr_arith' cannot parse the '{' of an instance literal
        const token tk{tz.next_token()};

        // e.g. 'var p = {1, 2}', a destination gives the type of '{...}'
        if (tk.is_none() and tz.peek_char_after_whitespace() == '{') {
            throw compile_error{
                tk, std::format("the type of {} is not known, write it before "
                                "the '{{'",
                                is_zero_value(tz) ? "'{}'" : "'{...}'")};
        }

        const term_kind tm{classify_term(tc, tk, tz)};

        // the type named first is the type, e.g. 'var p = point{1, 2}' or
        // 'var x = i8', which is 'i8(0)'; an instance literal that a
        // comparison or a connective continues is an operand, rejected below,
        // e.g. 'var b = point{1, 2} == p'
        if (is_instance_value(tm, tz) or is_bare_builtin_type(tm, tz)) {
            set_type(tc.get_type_or_throw(tk));
            return;
        }

        tz.put_back_token(tk);

        // 'expr_bool' parses any arithmetic too, as a comparison shorthand;
        // only the parse tells '(a + 1)' from '(a == 1)'
        expr_bool condition{tc, tz.next_whitespace_token(), tz};

        // none for a comparison, 'not', 'and' or 'or'
        const std::optional<expr_arith> arith{condition.release_lone_operand()};

        if (not arith) {
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

    // e.g. 'var a = i8[]{1, 2}' or 'var s = "hi"'
    auto deduce_literal_length(parse_toc& tc, const tokenizer& tz,
                               const type& element_type) -> void {

        shape_ = array_shape::array(std::nullopt);
        set_type(element_type);

        std::optional<token> size_tk;

        trial_parse(tc, tz, [this, &tc, &size_tk](tokenizer& probe) -> void {
            // an unsized array, the initializer gives the count
            const expr_any initializer{tc, probe, type_ref(),
                                       literal_typing::written, shape_};

            // later statements read the size while parsing, e.g. 'var b = a'
            shape_ = array_shape::array(initializer.array_count());
            size_tk = initializer.size_token();
        });

        if (shape_.length().value_or(0) == 0) {
            // e.g. the '[' of 'var a = i8[]{}' or the '""' of 'var s = ""'
            assert(size_tk);

            throw compile_error{
                *size_tk,
                std::format("an empty {} needs a size",
                            size_tk->is_string() ? "string" : "array")};
        }
    }

    // e.g. 'var c = a' copies the whole array 'a'
    auto deduce_whole_array_copy(const toc& tc, const token& tk,
                                 const stmt_identifier& ident) -> void {

        const ident_info info{make_ident_info(tc, ident)};

        if (not info.is_array()) {
            return;
        }

        // e.g. an unsized array parameter
        if (info.is_length_unknown()) {
            throw compile_error{tk,
                                std::format("the size of array {} is not known",
                                            in_quotes(ident.path_text()))};
        }

        shape_ = info.shape();
    }

    auto set_type(const type& tp) -> void { type_ = &tp; }

    // a value cannot be parsed at its type before the type is known, so the
    // initializer is parsed once to learn the type or size and that tree is
    // dropped
    auto trial_parse(parse_toc& tc, const tokenizer& tz,
                     const std::function_ref<void(tokenizer&)> parse) const
        -> void {

        tokenizer probe{tz};

        with_placeholder(tc, [&parse, &probe] -> void { parse(probe); });
    }

    // the variable is in scope while its initializer is parsed, so a read of
    // it, e.g. 'var x = x + 1', resolves to it and is rejected
    // ('scope::initializer'), not to an outer variable of that name; a
    // hidden name is not added
    auto with_placeholder(parse_toc& tc,
                          const std::function_ref<void()> parse) const -> void {

        if (name_visibility_ == name_in_initializer::hidden) {
            parse();
            return;
        }

        const parse_toc::scope_entry block{tc, scope::initializer()};

        tc.add_var(declared_var(), var_kind::var);

        parse();
    }

    //
    // statics
    //

    // the element type of a string, 'i8', or of an array literal; null when
    // the initializer is neither or the literal is an operand of a longer
    // expression, e.g. 'var b = [2]{1, 2} == a', which the parse rejects
    [[nodiscard]] static auto literal_element_type(const parse_toc& tc,
                                                   tokenizer& tz)
        -> const type* {

        if (tz.peek_char_after_whitespace() == '"') {
            tokenizer after_string{tz};
            std::ignore = after_string.next_token();

            return continues_expression(after_string) ? nullptr
                                                      : &tc.builtins().i8();
        }

        const token tk{tz.next_token()};

        const bool is_literal{
            classify_term(tc, tk, tz) == term_kind::array_literal and
                is_literal_value(tz),
        };

        tz.put_back_token(tk);

        if (not is_literal) {
            return nullptr;
        }

        // e.g. '[2]{1, 2}' has the default type
        if (tk.is_none()) {
            return &tc.builtins().default_type();
        }

        return &tc.get_type_or_throw(tk);
    }

    // the '=' after 'name_tk', 'keyword' starts the example of the error;
    // the error is at the end of the name, the next token can be on another
    // line
    [[nodiscard]] static auto read_equals(tokenizer& tz,
                                          const std::string_view keyword,
                                          const token& name_tk) -> token {

        // e.g. 'var x == 1', a comparison is not the '=' of an initializer
        if (not tz.is_lone_equals_next()) {
            throw compile_error{
                name_tk.end_position(),
                std::format("expected '=' followed by an initializer, "
                            "e.g. '{} x = i32'",
                            keyword)};
        }

        return tz.seen_char_token('=');
    }

    // the name of a definition, e.g. the 'x' of 'var x = 1'
    [[nodiscard]] static auto read_name(const parse_toc& tc, tokenizer& tz)
        -> token {

        const token name_tk{tz.next_token()};

        tc.assert_valid_unqualified_name(name_tk);

        return name_tk;
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
    folded_value constant_{};

  public:
    // the constant is declared during the parse so later statements can name
    // it, e.g. as an array size
    stmt_def_const(parse_toc& tc, const token& keyword_tk,
                   const initializer_declaration& declaration,
                   expr_any initializer, const folded_value& constant)
        : statement{keyword_tk}, name_tk_{declaration.name_tk()},
          equals_tk_{declaration.equals_tk()},
          initializer_{std::move(initializer)}, constant_{constant} {

        set_type(declaration.type_ref());

        tc.add_const(name_tk_, name_tk_.text(), constant_);
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

    auto compile(compile_toc& tc, [[maybe_unused]] const destination& dst) const
        -> void override {

        comment_source(tc);

        // note: the names it folds from are constants wherever it is
        //       compiled, a name that is a constant only with a call is a
        //       variable in the parse, e.g. the 'n' of a 'foo'

        // note: folded at the declared type like the parse, where it wraps
        //       without the overflow check, e.g. 'x + 1' of an 'i8' 127

        assert(folded_value::value_of(fold_let(tc, tok(), initializer_,
                                               get_type())) == constant_.value);

        // note: the compile of a 'var' checks the elements of a condition
        //       after the one that decides it, so does this

        initializer_.check_folded_conditions(tc);

        tc.add_const(name_tk_, name_tk_.text(), constant_);
    }

    // a constant reads and writes nothing at run time, the elements of a
    // condition after the one that decides it hold the rule like a 'var'
    auto trace_assignment(assignment_flow& flow) const -> void override {
        initializer_.assert_result_set_before_read(flow);
    }

    //
    // statics
    //

    // the value of a 'let' declared as 'tp' when it folds, none for a 'var'
    // or an array, which is not a constant; 'value' does not read the
    // defined name, its parse rejected that ('scope::initializer')
    [[nodiscard]] static auto fold_let(const toc& tc, const token& keyword_tk,
                                       const expr_any& value, const type& tp)
        -> std::optional<folded_value> {

        if (not keyword_tk.is_text("let") or value.is_array()) {
            return std::nullopt;
        }

        const bool is_default_type{tp.is_same(tc.builtins().default_type())};

        // note: one untyped constant at the default type keeps its value
        //       whole and is checked where it is used, as the literal it
        //       names, e.g. 'let k = 0x80000000' on a target whose default
        //       type is 'i32'; it is folded whole first, so '-m' of an 'm'
        //       of -0x80000000 is 0x80000000 there; a list is computed and
        //       checked at 'tp'

        if (is_default_type and value.is_single_operand()) {
            const std::optional<folded_value> whole{
                value.fold(tc, tc.use_at(tc.builtins().i64())),
            };

            if (whole and not whole->is_typed()) {
                return *whole;
            }
        }

        const std::optional<folded_value> folded{
            value.fold_checked(tc, tc.use_at(tp)),
        };

        if (not folded) {
            return std::nullopt;
        }

        folded_value constant{*folded};

        // note: untyped only at the default type, e.g. 'let k = 5' fits any
        //       integer where it is used; 'let b = true' is a 'bool'

        if (not constant.is_typed() and not is_default_type) {
            constant.own_type = &tp;
        }

        return constant;
    }
};

// e.g. 'var x = i32(0)', the initializer gives the type; a 'let' that does not
// fold is a 'var' that is read-only once initialized
class stmt_def_var final : public statement {
    // added to 'toc' at the parse and again at the compile
    var_info declared_;
    stmt_assign_var assign_var_;

  public:
    stmt_def_var(parse_toc& tc, const token& keyword_tk,
                 const initializer_declaration& declaration, expr_any value)
        : statement{keyword_tk},
          declared_{declared_var(keyword_tk, declaration)},
          assign_var_{
              declare_and_assign(tc, declared_, declaration.at_name(),
                                 declaration.equals_tk(), std::move(value)),
          } {

        set_type(declaration.type_ref());

        // note: a read of the variable in its initializer was rejected
        //       where it was parsed ('scope::initializer')
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        assign_var_.source_to(os);
    }

    auto compile(compile_toc& tc, [[maybe_unused]] const destination& dst) const
        -> void override {

        comment_source(tc);

        // a 'let' whose value folds only in this compile is a constant here,
        // e.g. 'let m = n' in a 'foo' over an unsized array parameter at a
        // call that gives the length
        if (const std::optional<folded_value> constant{fold_let(tc)}) {
            // note: the elements of a condition after the one that decides it
            //       are not checked as in a 'var': only constants of a call
            //       fold here, where 'compile_toc::check_skipped' checks
            //       nothing

            tc.add_const(declared_.src_loc_tk, declared_.name, *constant);
            return;
        }

        tc.add_var(declared_, var_kind::var);

        // the assignment names its destination itself
        assign_var_.compile(tc, destination{});
    }

    auto visit_operands(const operand_visitor visitor) const -> void override {
        assign_var_.visit_operands(visitor);
    }

  private:
    // the fold of the parse ('make_stmt_def_var'), typed as the variable the
    // parse made, also at the default type, e.g. 'let m = 0 + k' compares as
    // an 'int'
    [[nodiscard]] auto fold_let(const toc& tc) const
        -> std::optional<folded_value> {

        std::optional<folded_value> constant{
            stmt_def_const::fold_let(tc, tok(), assign_var_.value(),
                                     get_type()),
        };

        if (constant and not constant->is_typed()) {
            constant->own_type = &get_type();
        }

        return constant;
    }

    //
    // statics
    //

    // the variable is declared during the parse so later statements can name
    // it, its initializer names it as the destination
    [[nodiscard]] static auto
    declare_and_assign(parse_toc& tc, const var_info& var, tokenizer at_name,
                       const token& equals_tk, expr_any value)
        -> stmt_assign_var {

        tc.add_var(var, var_kind::var);

        return {
            stmt_identifier{tc, var.src_loc_tk, at_name},
            equals_tk,
            std::move(value),
        };
    }

    // a 'let' is read-only from its declaration on, its initializer is not
    // an assignment and is not checked for it
    [[nodiscard]] static auto
    declared_var(const token& keyword_tk,
                 const initializer_declaration& declaration) -> var_info {

        var_info var{declaration.declared_var()};

        if (keyword_tk.is_text("let")) {
            var.read_only_why = read_only_cause::let;
        }

        return var;
    }
};

// reads the name and the initializer and makes the statement of the kind they
// give: a constant for a 'let' that folds, e.g. 'let n = 2 * 3', else a
// variable
inline auto make_stmt_def_var(parse_toc& tc, const token& keyword_tk,
                              tokenizer& tz) -> std::unique_ptr<statement> {

    const initializer_declaration declaration{
        tc,
        tz,
        keyword_tk.text(),
        name_in_initializer::visible,
    };

    expr_any value{declaration.parse_value(tc, tz)};

    const std::optional<folded_value> constant{
        stmt_def_const::fold_let(tc, keyword_tk, value, declaration.type_ref()),
    };

    if (constant) {
        return std::make_unique<stmt_def_const>(tc, keyword_tk, declaration,
                                                std::move(value), *constant);
    }

    return std::make_unique<stmt_def_var>(tc, keyword_tk, declaration,
                                          std::move(value));
}
