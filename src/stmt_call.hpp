#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <format>
#include <functional>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "compile_error.hpp"
#include "expr_any.hpp"
#include "expr_bool.hpp"
#include "expr_type.hpp"
#include "generics.hpp"
#include "ident_info.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "parse_lookahead.hpp"
#include "statement.hpp"
#include "stmt_def_func.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_order.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

//
// a call, e.g. 'f(x)', 'lst.add(x)' or 'point.at(1, 2)', is the classes below,
// in reading order:
//
//   'stmt_call'       parses the call, prints its source and hands it to one
//                     of the compilers (last in the file, it uses the others)
//   'missing_argument_text', 'missing_delimiter_text',
//   'throw_not_array_argument', 'named_argument_type',
//   'parse_array_parameter_argument'
//                     free functions that 'generic_call' and 'stmt_call'
//                     use
//   'generic_call'    the instance of a generic function that the call names
//   'call_site'       what compiling the call needs of it
//   'inline_call'     compiles the body of the function in place
//   'noinline_call'   passes addresses to the body that is emitted once
//   'constructor_call_tag'
//                     selects the constructor of 'point.at(1, 2)'
//   free functions    'assert_call_in_func', 'path_argument'
//   'builtin_arguments'
//                     the arguments of a built-in that parses like a call,
//                     e.g. 'exit(0)'
//   'shared_storage_check'
//                     the result and the arguments that may share storage
//                     ('stmt_call::assert_no_shared_storage')
//
// an argument, from the parse to the body:
//
//   'stmt_call::argument_places' the place each argument names, none for a
//                                value; whether an argument goes through a
//                                temporary is the rule of 'inline_call' or
//                                'noinline_call'
//   'param_binding'              inlined: what the parameter stands for
//                                ('inline_call::make_bindings')
//   'noinline_call::noinline_arg'
//                                noinline: the variable passed, or the
//                                temporary that holds the value
//                                ('noinline_call::compile_arguments',
//                                'call_site::make_argument_temporary')
//   frame slot                   the address written to it
//                                ('noinline_call::slot_address_of')
//
// the formatter sorts the functions of a class by name, so the comment above
// each class lists them in the order they run
//

// the argument numbered 'number', counted from 1 without a receiver, is
// missing, the ')' is where it would start
[[nodiscard]] inline auto missing_argument_text(const size_t number,
                                                const std::string_view param)
    -> std::string {

    return std::format("expected argument {} ({})", number, in_quotes(param));
}

// the argument numbered 'number', counted from 1 without a receiver, lacks
// the ',' before it
[[nodiscard]] inline auto missing_delimiter_text(const size_t number,
                                                 const std::string_view param)
    -> std::string {

    return std::format("expected ',' before argument {} ({})", number,
                       in_quotes(param));
}

[[noreturn]] inline auto throw_not_array_argument(const token& src_loc_tk,
                                                  const std::string_view source,
                                                  const std::string_view param)
    -> void {

    throw compile_error{
        src_loc_tk, std::format("{} is not an array, parameter {} takes one",
                                in_quotes(source), in_quotes(param))};
}

// the type that a variable, a field, an element or a typed constant passed
// as an argument has, null for an untyped constant, e.g. 'k' of 'let k = 5',
// which like a literal takes the type of its parameter
[[nodiscard]] inline auto named_argument_type(const toc& tc,
                                              const stmt_identifier& ident)
    -> const type* {

    const ident_info info{make_ident_info(tc, ident)};

    if (info.is_const() and not info.constant().is_typed()) {
        return nullptr;
    }

    return &info.type_ref();
}

// the whole array the array parameter 'param' takes, read as the array of a
// built-in and not as a value of the element type, which would report its
// kind, e.g. 'p' of a 'point' for 's i8[]'; an element would give the
// parameter the whole array's length
[[nodiscard]] inline auto parse_array_parameter_argument(
    toc& tc, tokenizer& tz, const std::string_view param) -> stmt_identifier {

    // e.g. the '{}' of 'f({})', which has no storage that the parameter
    // refers to
    if (is_zero_value(tz)) {
        throw_not_array_argument(tz.next_token(), "{}", param);
    }

    stmt_identifier array{stmt_identifier::parse_array_argument(tc, tz)};

    if (not array.is_array()) {
        throw_not_array_argument(array.tok().start_position(),
                                 array.path_text(), param);
    }

    return array;
}

// the instance of a generic function that a call names, its type arguments
// written, e.g. 'tz.to<name>()', or deduced from the arguments, e.g. 'f(x)' of
// 'func f<T type>(s T)', see 'etc/docs/README.md' section 2.10
//
// 'parse' reads '<...>' with 'written_type_arguments' or, without '<',
// deduces the types with 'deduced_type_arguments', which looks ahead at the
// arguments with 'argument_type' and 'value_type'; then the instance is made
struct generic_call {
  private:
    // the type arguments of a call without '<...>' and the arguments they
    // were taken from
    struct deduction {
        std::vector<const type*> type_args;
        std::vector<size_t> deducing_args;
    };

  public:
    // the name of the instance
    std::string func_name;
    // e.g. '<', 'name' and '>' in 'tz.to<name>()', empty when deduced
    std::vector<token> generic_tks;
    // the arguments a type was deduced from, counted without a receiver, e.g.
    // 0 for 'take(point{3, 4})', a literal there names its type
    std::vector<size_t> deducing_args;

    //
    // statics
    //

    // 'tz' is after the name and before the '<' or the '('
    // note: 'name_tk' is the name as written and 'generic_name' the generic
    //       function, e.g. 'to' and 'tokenizer.to'
    [[nodiscard]] static auto
    parse(toc& tc, tokenizer& tz, const token& name_tk,
          const std::string_view generic_name, const type* const expected_type)
        -> generic_call {

        // e.g. not the '<=' of 'p.get <= 3', which the deduction reports
        const std::optional<token> open_tk{
            tz.is_type_args_next() ? tz.accept_char_token('<') : std::nullopt,
        };

        std::vector<const type*> type_args;
        std::vector<token> generic_tks;
        std::vector<size_t> deducing_args;

        if (not open_tk) {
            deduction deduced{
                deduced_type_arguments(tc, tz, name_tk, generic_name,
                                       expected_type),
            };

            type_args = std::move(deduced.type_args);
            deducing_args = std::move(deduced.deducing_args);
        } else {
            const generic_arg_tokens args{
                generic_arg_tokens::parse(tz, *open_tk, false),
            };

            type_args =
                written_type_arguments(tc, name_tk, generic_name, args.arg_tks);

            generic_tks = args.list_tks;
        }

        return {
            .func_name{
                func_instantiator::instantiate_func(tc, name_tk, generic_name,
                                                    type_args),
            },
            .generic_tks{std::move(generic_tks)},
            .deducing_args{std::move(deducing_args)},
        };
    }

  private:
    //
    // statics
    //

    // the type of the argument at 'index' when it is a variable, a field, an
    // element or a typed constant, or the value it makes names its type:
    // 'point{1, 2}', 'point.at(1, 2)' or 'mk(1, 2)', otherwise null; an
    // argument that no operator continues, no '.' or '[' reaches into and no
    // '{' follows is whole, e.g. 'a' of 'f(a 2)', the parse of the call
    // reports what follows it; 'point.m()' tells a type only when 'm' is a
    // constructor. 'tz' is after the '(', it ends after the argument when the
    // argument tells its type
    [[nodiscard]] static auto argument_type(toc& tc, tokenizer& tz,
                                            const size_t index) -> const type* {

        [[maybe_unused]] const size_t reached{skip_to_argument(tz, index)};

        // note: the callers saw the argument ('assert_argument_present')
        assert(reached == index);

        const token tk{tz.next_token()};
        const term_kind tm{classify_term(tc, tk, tz)};

        // e.g. 'mk()' hidden by 'let mk = 5' is still a call, its result
        // tells the type; not the 'mk()' of 'mk().x + 1', which the parse of
        // the argument reports
        if (tm != term_kind::value) {
            const type* const made_type{value_type(tc, tz, tk, tm)};

            if (continues_expression(tz) or is_reached_into(tz)) {
                return nullptr;
            }

            return made_type;
        }

        // note: read from the copy to find where the argument ends and its
        //       type, the call parses it again

        const stmt_identifier ident{tc, tk, tz};

        if (ident.is_method_receiver() or continues_expression(tz)) {
            return nullptr;
        }

        // e.g. 'a' of 'f(a)' or 'f(a{})' for 'a i8[4]', which is not of the
        // element type, as in a call without type parameters
        if (ident.is_array()) {
            throw_unindexed_array(ident.tok(), ident.path_text());
        }

        // e.g. not the 'point' of 'point{1, 2}' with 'var point = 1' or the
        // 'v' of 'v{}', which the parse of the argument reports
        if (tz.peek_char_after_whitespace() == '{') {
            return nullptr;
        }

        return named_argument_type(tc, ident);
    }

    // the argument at 'index' parsed without a type, as the initializer of a
    // 'var' is or an array argument of a call, so a mistake in it is reported
    // as in a call without type parameters; a value parses and tells no type,
    // nor does a literal that takes its type from the parameter, e.g.
    // '{1, 2}'; a token after it that is not a delimiter is reported as there
    // too. 'tz' is after the '(' and is a copy
    // note: the parse records nothing that outlives the error that follows
    static auto assert_argument_parses(
        toc& tc, tokenizer tz,
        const std::span<const generic_signature_param> params,
        const size_t index) -> void {

        std::ignore = skip_to_argument(tz, index);

        if (params.at(index).is_array) {
            std::ignore = parse_array_parameter_argument(
                tc, tz, params.at(index).name_tk.text());

            assert_delimiter_after(tz, params, index);

            return;
        }

        const char next{tz.peek_char_after_whitespace()};

        if (next == '{' or next == '[') {
            return;
        }

        // a parameter of the default type is the destination of its
        // argument, as in the call, e.g. the 'g(1)' of 'f(g(1), 1)' for 'func
        // g<T type>(a) res T'
        if (params.at(index).type_name.empty()) {
            std::ignore = expr_any{tc, tz, tc.builtins().default_type(),
                                   literal_typing::from_destination};
        } else {
            std::ignore = expr_bool{tc, tz.next_whitespace_token(), tz};
        }

        assert_delimiter_after(tz, params, index);
    }

    // the arguments up to the one at 'index' are there, reported as in a
    // call without type parameters: the first missing one, e.g. 'b' of 'f(1)'
    // for 'func f<T type>(a, b, x T)', or a token after the argument where
    // the list ends. 'tz' is after the '('
    static auto assert_argument_present(
        toc& tc, const tokenizer& tz,
        const std::span<const generic_signature_param> params,
        const size_t index) -> void {

        // e.g. 'f()'
        if (tz.peek_char_after_whitespace() == ')') {
            throw compile_error{
                tz, missing_argument_text(1, params.front().name_tk.text())};
        }

        tokenizer scan{tz};

        const size_t last{skip_to_argument(scan, index)};

        if (last == index) {
            return;
        }

        assert_argument_valid(tc, tz, params, last);

        throw compile_error{
            scan, missing_argument_text(last + 2,
                                        params.at(last + 1).name_tk.text())};
    }

    // the argument at 'index', which is there, is reported as in a call
    // without type parameters. 'tz' is after the '('
    static auto
    assert_argument_valid(toc& tc, const tokenizer& tz,
                          const std::span<const generic_signature_param> params,
                          const size_t index) -> void {

        tokenizer after{tz};

        // e.g. 'point{1, 2}' of 'g(point{1, 2})', which parses only at a
        // type, else parsed like a 'var' initializer or, for an array
        // parameter, as in the call
        if (not params.at(index).is_array and
            argument_type(tc, after, index) != nullptr) {

            assert_delimiter_after(after, params, index);
        } else {
            assert_argument_parses(tc, tz, params, index);
        }
    }

    // the arguments up to and including the one at 'param_index', or all and
    // no surplus one when it is none, reported as in a call without type
    // parameters. 'tz' is after the '('
    static auto assert_arguments_valid_up_to(
        toc& tc, const tokenizer& tz, const std::string_view generic_name,
        const std::span<const generic_signature_param> params,
        const std::optional<size_t> param_index) -> void {

        const size_t checked_count{
            param_index ? *param_index + 1 : params.size(),
        };

        for (size_t i{}; i < checked_count; ++i) {
            assert_argument_present(tc, tz, params, i);
            assert_argument_valid(tc, tz, params, i);
        }

        if (not param_index) {
            assert_no_surplus_argument_of(tz, generic_name, params);
        }
    }

    // 'tz' is after the argument at 'index', a ',' or the ')' follows, e.g.
    // not the '2' of 'f(1 2)'
    static auto assert_delimiter_after(
        const tokenizer& tz,
        const std::span<const generic_signature_param> params,
        const size_t index) -> void {

        const char delimiter{tz.peek_char_after_whitespace()};

        if (delimiter == ',' or delimiter == ')') {
            return;
        }

        const size_t next_index{index + 1};

        if (next_index == params.size()) {
            throw compile_error{tz, std::string{missing_close_paren_text}};
        }

        throw compile_error{
            tz, missing_delimiter_text(next_index + 1,
                                       params.at(next_index).name_tk.text())};
    }

    // the arguments, which are there, are no more than 'params', reported
    // as in a call without type parameters, e.g. the '2' of 'g(1, 2)' for
    // 'func g<T type>(b) res T'. 'tz' is after the '('
    static auto assert_no_surplus_argument_of(
        const tokenizer& tz, const std::string_view name,
        const std::span<const generic_signature_param> params) -> void {

        tokenizer after{tz};

        if (not params.empty()) {
            std::ignore = skip_to_argument(after, params.size() - 1);
            after.skip_argument();
        }

        assert_no_surplus_argument(after, name, params.size());
    }

    // e.g. 'point.at(1, 2)', not the method 'point.m()' or 'point.zz(1)',
    // which the parse of the argument reports; 'tk' is the type and 'tz' is
    // after it
    [[nodiscard]] static auto constructor_call_type(toc& tc, tokenizer& tz,
                                                    const token& tk)
        -> const type* {

        std::ignore = tz.seen_char_token('.');

        // the type names the result, the constructor, e.g. 'at', does not
        const token ctor_tk{tz.next_token()};

        if (not tz.accept_char_token('(')) {
            return nullptr;
        }

        tz.skip_to_close_paren();

        const type& result_type{tc.get_type_or_throw(tk)};

        const std::string ctor_name{
            method_name_of(result_type.name(), ctor_tk.text()),
        };

        const bool is_constructor{
            tc.funcs().has(ctor_name) and
                tc.get_func_or_throw(ctor_tk, ctor_name).is_constructor(),
        };

        return is_constructor ? &result_type : nullptr;
    }

    // the type arguments of 'f(x)' for 'func f<T type>(s T)', for each type
    // parameter in this order, so the errors come as a call without type
    // parameters would report them first:
    //   1. the arguments up to the deducing one are there
    //      ('assert_argument_present')
    //   2. the type that argument tells ('argument_type'), none for a literal
    //      or an expression
    //   3. otherwise the type the call is assigned to, when the result is
    //      declared with the type parameter
    //   4. otherwise the arguments up to the deducing one, or all and a
    //      surplus one for a result, are reported as in that call
    //      ('assert_argument_valid'), then "cannot deduce"
    [[nodiscard]] static auto
    deduced_type_arguments(toc& tc, const tokenizer& tz, const token& name_tk,
                           const std::string_view generic_name,
                           const type* const expected_type) -> deduction {

        const generic_func_info& generic{tc.generics().get_func(generic_name)};

        // the arguments are looked at ahead, the call reads them later
        tokenizer scan{tz};

        // note: a function is a call here only before its '('
        //       ('classify_term'), a method at its name, e.g. 'p.get'

        if (not scan.accept_char_token('(')) {
            throw compile_error{name_tk, "expected '(' after method name"};
        }

        deduction deduced{};

        for (const token& param_tk : generic.signature.type_param_tks) {
            const generic_deduction source{
                generic.signature.deduction(param_tk.text()),
            };

            const type* arg_type{};

            if (source.param_index) {
                assert_argument_present(tc, scan, generic.signature.params,
                                        *source.param_index);

                tokenizer arg_tz{scan};

                arg_type = argument_type(tc, arg_tz, *source.param_index);

                if (arg_type != nullptr) {
                    deduced.deducing_args.push_back(*source.param_index);
                }
            }

            if (arg_type == nullptr and source.is_result) {
                arg_type = expected_type;
            }

            if (arg_type == nullptr) {
                // e.g. the 'zz' of 'f(zz, 1)' for 'func f<A type>(b, a A)' or
                // the 'point' of 'show(point)', reported as in a call without
                // type parameters
                assert_arguments_valid_up_to(tc, scan, generic_name,
                                             generic.signature.params,
                                             source.param_index);

                throw compile_error{
                    name_tk,
                    std::format("cannot deduce {} of generic function {}, "
                                "write '{}<...>'",
                                in_quotes(param_tk.text()),
                                in_quotes(generic_name), name_tk.text())};
            }

            deduced.type_args.push_back(arg_type);
        }

        return deduced;
    }

    // moves 'tz' from after the '(' to the argument at 'index' and returns
    // 'index'; when the list ends before it, 'tz' is after the argument where
    // it ends and that argument's index is returned
    [[nodiscard]] static auto skip_to_argument(tokenizer& tz,
                                               const size_t index) -> size_t {

        for (size_t i{}; i < index; ++i) {
            tz.skip_argument();

            const std::optional<token> delimiter_tk{tz.accept_char_token(',')};

            if (not delimiter_tk) {
                return i;
            }

            // e.g. 'f(1,)', reported before the argument it lacks
            tz.assert_no_trailing_delimiter(*delimiter_tk, ')', "a call");
        }

        return index;
    }

    // the type of a 'bool' literal, an instance literal, a constructor call or
    // a call of a built-in or of a function, 'tk' is its first token and 'tz'
    // is after it; 'tz' ends after the value when it names a type
    // note: an argument is parsed at the type of its parameter, so it is read
    //       here without a parse, unlike a 'var' initializer, see
    //       'initializer_declaration::deduce_from_initializer'
    [[nodiscard]] static auto value_type(toc& tc, tokenizer& tz,
                                         const token& tk, const term_kind tm)
        -> const type* {

        // e.g. 'true', the one literal whose type does not depend on where
        // it goes
        if (tm == term_kind::literal) {
            const bool is_bool{constant_parser::is_bool_literal(tk.text())};
            return is_bool ? &tc.builtins().boolean() : nullptr;
        }

        // e.g. 'point{1, 2}', a missing '}' is left to the parse of the
        // argument
        if (tm == term_kind::record_literal) {
            std::ignore = tz.skip_braced_block();
            return &tc.get_type_or_throw(tk);
        }

        if (tm == term_kind::constructor_call) {
            return constructor_call_type(tc, tz, tk);
        }

        // e.g. 'read(0, buf)', the value of a built-in
        if (const type* const builtin_type{builtin_value_type(tc, tk, tm)};
            builtin_type != nullptr) {

            if (not tz.accept_char_token('(')) {
                return nullptr;
            }

            tz.skip_to_close_paren();

            return builtin_type;
        }

        // e.g. 'mk(1, 2)', a generic function has no return type before it is
        // instantiated
        if (tm != term_kind::call or not tc.funcs().has(tk.text())) {
            return nullptr;
        }

        std::ignore = tz.seen_char_token('(');
        tz.skip_to_close_paren();

        const type& return_type{
            tc.get_func_or_throw(tk, tk.text()).result_type(),
        };

        if (return_type.is_same(tc.builtins().void_type())) {
            return nullptr;
        }

        return &return_type;
    }

    // the types between '<' and '>', one per type parameter, e.g. 'name' of
    // 'tz.to<name>()'
    [[nodiscard]] static auto
    written_type_arguments(const toc& tc, const token& name_tk,
                           const std::string_view generic_name,
                           const std::span<const token> arg_tks)
        -> std::vector<const type*> {

        const std::span<const token> param_tks{
            tc.generics().get_func(generic_name).signature.type_param_tks,
        };

        if (arg_tks.size() != param_tks.size()) {
            throw compile_error{
                name_tk, std::format("generic function {} takes {}, got {}",
                                     in_quotes(generic_name),
                                     counted(param_tks.size(), "type argument"),
                                     arg_tks.size())};
        }

        std::vector<const type*> type_args;
        type_args.reserve(arg_tks.size());

        for (const auto [param_tk, arg_tk] :
             std::views::zip(param_tks, arg_tks)) {

            type_args.emplace_back(
                &resolve_type_arg(tc, generic_name, param_tk.text(), arg_tk));
        }

        return type_args;
    }
};

// what compiling a call needs of it, 'stmt_call' makes it for each compile
struct call_site {
    // the name of a function or method, the type of a constructor call; the
    // code and the errors of the call are located at it
    token tk;
    // e.g. 'at' of 'point.at(1, 2)', else 'tk'; the limits are located at it
    token name_tk;
    // the receiver of a method is written before the name
    token begin_tk;
    token close_paren_tk;
    // e.g. 'lst.add(x)', for the comment of the call and the frame of an error
    std::string source;
    // a method receiver is the first argument
    std::span<const expr_any> args;
    // the callee, resolved when the call was parsed; never null
    const stmt_def_func* func{};
    // one per argument, the verdict that the checks of the call and its
    // compile share, see 'stmt_call::argument_places'
    std::vector<std::optional<place>> places;

    // an error found while the inlined body compiled gets the call as a
    // frame, unless it is located in the call itself, e.g. in an argument
    auto add_call_frame(compile_error& e) const -> void {
        if (is_located_in_call(e)) {
            return;
        }

        func->add_call_frame(e, begin_tk, source);
    }

    // 'compile' gets the destination of the result: 'dst', or a temporary of
    // the call when 'has_result_temporary', then copied into 'dst', widened
    // when 'dst' is wider; a block holds the temporaries of the call
    // note: a block is a scope, which counts toward the nesting limit and the
    //       'max scopes in use' report, so a call without temporaries enters
    //       none
    auto compile_with_temporaries(
        toc& tc, const destination& dst, const bool has_argument_temporary,
        const bool has_result_temporary,
        const std::function_ref<void(const destination&)> compile) const
        -> void {

        std::optional<toc::scope_entry> temporaries;

        if (has_argument_temporary or has_result_temporary) {
            temporaries.emplace(tc, scope::block());
        }

        if (not has_result_temporary) {
            compile(dst);
            return;
        }

        const destination result{
            add_temporary(tc, temporary_name(tk, "call-result", 0),
                          func->result_type()),
        };

        compile(result);

        machine& x{tc.machine()};

        x.copy_value({tk, tc.indent()}, dst.operand(), result.operand());
    }

    // a variable of the current block that holds the argument at 'index',
    // compiled into it
    [[nodiscard]] auto make_argument_temporary(toc& tc,
                                               const size_t index) const
        -> destination {

        // note: the name is unique per call and argument, an inlined body
        //       names it through the binding of its parameter and a call in
        //       the body has temporaries of its own; each level of an inlined
        //       recursion declares it again, the binding resolves it in the
        //       caller's scope

        const destination temporary{
            add_temporary(tc, temporary_name(tk, "call-arg", index),
                          func->params().at(index).get_type()),
        };

        args.at(index).compile(tc, temporary);

        return temporary;
    }

    // the result is written field by field, so its padding is zeroed for
    // '==' to compare instances byte by byte; after the arguments, which may
    // read the destination, e.g. 'p = f(p.a + 1)'
    auto zero_result_padding(toc& tc, const destination& dst) const -> void {
        if (dst.is_empty()) {
            return;
        }

        machine& x{tc.machine()};

        x.zero_padding({tk, tc.indent()}, dst.operand(), dst.type_ref());
    }

    //
    // statics
    //

    // 'dst' has another type than the result, after 'assert_result_type' a
    // widening
    [[nodiscard]] static auto result_type_differs(const destination& dst,
                                                  const stmt_def_func& func)
        -> bool {

        return not dst.type_ref().is_same(func.result_type());
    }

  private:
    // a variable in the current block, a root variable of its own
    [[nodiscard]] auto add_temporary(toc& tc, const std::string& name,
                                     const type& tpe) const -> destination {

        tc.add_var(
            {
                .name{name},
                .type_ptr{&tpe},
                .src_loc_tk{tk},
                .shape{},
                .read_only_why{},
                .placement{},
                .foo_array{},
            },
            var_kind::var);

        const ident_info info{tc.resolve_declared(tk, name)};

        // note: a declared variable resolves to its storage
        assert(not info.is_address_by_call());

        return destination::of_name(
            info,
            {
                .address{info.operand()},
                .is_final_address{},
                .storage{tc.storage_of(name, access_path::whole_variable())},
                .text{name},
            });
    }

    [[nodiscard]] auto is_located_in_call(const compile_error& e) const
        -> bool {

        // note: a recursive call is checked inside its own body, so an error
        //       can be located after the call

        // note: an error in a body is at a token of its source; only a missing
        //       'main' and the registers the backend holds have no place

        assert(e.where);

        return e.where->file_ix == begin_tk.file_index() and
               e.where->start_index >= begin_tk.start_index() and
               e.where->start_index <= close_paren_tk.end_index();
    }
};

// compiles a call of an inlined function: the body is compiled in place, its
// parameters and its result are bound to the arguments and the destination
//
// the steps:
//
//   0. 'compile': 'compile_within_limits', the limits on how deep the
//      compile of calls nests, each a recursion of the compiler, or
//      'check_recursion' in the check of a body that takes from a call
//   1. 'compile_in_block': a result narrower than the operand it widens into
//      is computed in a register and widened ('compile_widened'), a block
//      holds the temporaries of the call
//   2. 'compile_with_bindings': the bindings of the result and the arguments
//      ('make_bindings'), an instance literal or a call result argument is
//      made in a temporary, an indexed argument may get its address in a
//      register, the shorter of two versions
//      ('compile_body_with_address_registers')
//   3. 'compile_body': the body in place with the bindings
//      ('stmt_def_func::compile_inlined')
class inline_call final {
    // what the body names by its result and its parameters
    struct call_bindings {
        std::optional<param_binding> result;
        std::vector<param_binding> arguments;
    };

    // the most scopes open at once: blocks, loops and function bodies
    static constexpr size_t max_scope_count_{256};

    // an inlined recursion ends at compile time when a constant argument
    // makes its condition fold, otherwise it would expand without end
    static constexpr size_t max_inlined_nesting_{64};

    call_site call_{};

  public:
    explicit inline_call(call_site call) : call_{std::move(call)} {
        assert(call_.func->is_inlined());
    }

    // a recursion in the check of a body that takes from a call goes to
    // 'check_recursion'
    auto compile(toc& tc, const destination& dst) const -> void {
        const size_t nesting{tc.scopes().inlined_nesting(call_.func->name())};

        if (nesting != 0 and tc.scopes().is_in_body_taking_from_call()) {
            check_recursion(tc, dst, nesting);
            return;
        }

        compile_within_limits(tc, dst, nesting);
    }

  private:
    // the inlined recursions of the callee, 'nesting' levels, and the scopes
    // are within their limits
    auto assert_inline_nesting(const toc& tc, const size_t nesting) const
        -> void {

        const std::string_view name{call_.func->name()};

        // e.g. a recursion that a constant argument ends too late or never
        if (nesting >= max_inlined_nesting_) {
            throw compile_error{
                call_.name_tk,
                std::format("recursion of inlined function {} is nested "
                            "deeper than {} calls, declare it 'noinline'",
                            in_quotes(name), max_inlined_nesting_),
                compile_error::error_kind::inlining_limit,
            };
        }

        if (tc.scopes().count() < max_scope_count_) {
            return;
        }

        throw compile_error{
            call_.name_tk,
            std::format("inlined calls and blocks are nested too deep, the "
                        "limit is {} scopes, declare a function 'noinline'",
                        max_scope_count_),
            compile_error::error_kind::inlining_limit,
        };
    }

    // the bindings of the arguments of the inlined body, in their order
    [[nodiscard]] auto bind_arguments(toc& tc,
                                      machine::held_registers& held) const
        -> std::vector<param_binding> {

        std::vector<param_binding> arguments;

        for (const auto [i, arg, param, arg_place] :
             std::views::zip(std::views::iota(size_t{}), call_.args,
                             call_.func->params(), call_.places)) {

            // note: a temporary is made here, in the order of the arguments,
            //       and named per call, so each level of an inlined recursion
            //       binds its own

            if (is_temporary(arg_place, param)) {
                const destination temporary{
                    call_.make_argument_temporary(tc, i),
                };

                arguments.push_back(param_binding::make_name(
                    param.name(), temporary.bound_info(), temporary.text()));

                continue;
            }

            arguments.push_back(bind_argument(tc, arg, param, arg_place, held));
        }

        return arguments;
    }

    // a recursion in the check of a body that takes from a call, in a dry run
    // with its deeper levels: a value or an array length the check does not
    // know may not let it end, so a limit ('compile_error::is_limit') ends
    // the recursion, not the check (README 1.5 item 2):
    //   1. it nests as deep at every call ('nests_alike_at_every_call'):
    //      expanded, the nesting limit rejects it, e.g. a 'g(k + 1)' that
    //      'g(2)' enters and that ends only at 'k == 0'
    //   2. otherwise only its arguments are bound, the body is checked where
    //      the recursion is entered and in its own check
    auto check_recursion(toc& tc, const destination& dst,
                         const size_t nesting) const -> void {

        const bool nests_alike{nests_alike_at_every_call(tc)};

        const auto compile_level{
            [&] -> void {
                if (nests_alike) {
                    compile_within_limits(tc, dst, nesting);
                } else {
                    compile_arguments_without_body(tc);
                }
            },
        };

        // note: a deeper level is in the dry run of the first
        if (nesting > 1) {
            compile_level();
            return;
        }

        const auto compile_first_level{
            [&] -> void {
                try {
                    compile_level();
                } catch (const compile_error& e) {
                    if (not ends_recursion(e, nests_alike)) {
                        throw;
                    }
                }
            },
        };

        tc.dry_run(compile_first_level, machine::discarded_code::here);
    }

    // the arguments bound as for the body, which is not compiled: an inlined
    // recursion that the check of a body that takes from a call does not
    // expand ('check_recursion')
    auto compile_arguments_without_body(toc& tc) const -> void {
        // a block holds the temporaries of the call
        std::optional<toc::scope_entry> temporaries;

        if (has_argument_temporary()) {
            temporaries.emplace(tc, scope::block());
        }

        machine& x{tc.machine()};

        machine::held_registers held{x, {call_.tk, tc.indent() + 1}};

        std::ignore = bind_arguments(tc, held);
    }

    // an error found in the inlined body reports where it was called from,
    // an error in the arguments is in the caller and has no call frame
    auto compile_body(toc& tc, const call_bindings& bindings) const -> void {
        try {
            call_.func->compile_inlined(tc, call_.tk, bindings.result,
                                        bindings.arguments);
        } catch (compile_error& e) {
            call_.add_call_frame(e);
            throw;
        }
    }

    // a new register per argument because the index register may belong to
    // an enclosing binding that is used after the call
    auto compile_body_with_address_registers(toc& tc,
                                             call_bindings bindings) const
        -> void {

        machine& x{tc.machine()};

        machine::held_registers held{x, {call_.tk, tc.indent() + 1}};

        for (param_binding& binding : bindings.arguments) {
            if (not is_indexed_reference(binding)) {
                continue;
            }

            ident_info& bound{binding.info};

            const operand address{
                held.alloc_scratch_register({call_.tk, tc.indent()},
                                            tc.builtins().address()),
            };

            x.comment({call_.tk, tc.indent()}, "address of parameter {}",
                      in_quotes(binding.from));

            x.address_of({call_.tk, tc.indent()}, address, bound.operand());

            bound = bound.at_address(
                operand::mem(address, bound.operand().type_ref()));
        }

        compile_body(tc, bindings);
    }

    auto compile_in_block(toc& tc, const destination& dst) const -> void {
        if (call_.func->has_result() and
            call_site::result_type_differs(dst, *call_.func)) {
            // note: a narrower integer, see 'stmt_call::assert_result_type'

            compile_widened(tc, dst);
            return;
        }

        // a result the body passes to a 'mut' parameter needs storage, a
        // register destination gets it through a temporary of the call
        const bool has_result_temporary{
            dst.is_register() and call_.func->uses().is_result_passed_as_mut(),
        };

        call_.compile_with_temporaries(tc, dst, has_argument_temporary(),
                                       has_result_temporary,
                                       [&](const destination& result) -> void {
                                           compile_with_bindings(tc, result);
                                       });
    }

    // the result is made at its type in a register that the body names, the
    // copy into the operand widens it
    auto compile_widened(toc& tc, const destination& dst) const -> void {
        statement::compile_through_scratch(
            tc, call_.tk, dst.operand(), call_.func->result_type(),
            [&](const operand& result) -> void {
                compile_in_block(tc, destination::of_register(result));
            });
    }

    auto compile_with_bindings(toc& tc, const destination& dst) const -> void {
        machine& x{tc.machine()};

        // scratch registers stay allocated until the inlined body is compiled
        machine::held_registers held{x, {call_.tk, tc.indent() + 1}};

        const call_bindings bindings{make_bindings(tc, dst, held)};

        call_.zero_result_padding(tc, dst);

        const bool has_indexed_argument{
            std::ranges::any_of(bindings.arguments, is_indexed_reference),
        };

        // an access can use the indexed address as it is, so computing it
        // into a register is not tried
        if (not has_indexed_argument or x.has_indexed_addressing()) {
            compile_body(tc, bindings);
            return;
        }

        // note: the body is compiled in two versions and the one with the
        //       least code is kept
        //
        //       1. bindings use the indexed addresses as they are, e.g.
        //          '[s0 + t0 * 4 + 28]'
        //       2. each indexed address goes once into a new register, the
        //          displacement included, e.g. '[t1]'
        //
        //       rv32i has no base + index addressing, so in version 1 every
        //       access adds base and index again; a register pays off only
        //       when the body accesses the argument more than once, which is
        //       only known after compiling the body
        //
        //       the argument registers are shared by both versions, so 'held'
        //       frees them after the choice

        tc.emit_most_efficient(
            call_.tk, [&] -> void { compile_body(tc, bindings); },
            [&] -> void { compile_body_with_address_registers(tc, bindings); });
    }

    // the call within the nesting limits, 'nesting' inlined bodies of the
    // callee around it, a recursion when not 0; a recursion that does not end
    // at compile time runs out of registers before it reaches the nesting
    // limit, the advice is the same
    auto compile_within_limits(toc& tc, const destination& dst,
                               const size_t nesting) const -> void {

        assert_inline_nesting(tc, nesting);

        try {
            compile_in_block(tc, dst);
        } catch (const compile_error& e) {
            if (nesting == 0 or
                e.kind != compile_error::error_kind::out_of_registers) {

                throw;
            }

            throw recursion_error();
        }
    }

    [[nodiscard]] auto has_argument_temporary() const -> bool {
        return std::ranges::any_of(
            std::views::zip(call_.places, call_.func->params()),
            [](const auto& pair) -> bool {
                const auto& [arg_place, param]{pair};
                return is_temporary(arg_place, param);
            });
    }

    // the bindings of the result and of the arguments of the inlined body
    [[nodiscard]] auto make_bindings(toc& tc, const destination& dst,
                                     machine::held_registers& held) const
        -> call_bindings {

        // note: the call checked the use and the type of the result before
        //       the body was entered

        assert(call_.func->has_result() != dst.is_empty());

        call_bindings bindings{};

        // the result names the destination, so the widths must agree
        if (const std::optional<func_result>& result{call_.func->result()}) {

            assert(dst.type_ref().is_same(call_.func->result_type()));

            bindings.result = bind_result(dst, *result);
        }

        bindings.arguments = bind_arguments(tc, held);

        return bindings;
    }

    // what may end a recursion at one call and not at another is known here:
    // every value argument folds and every array argument has its length; an
    // instance or a 'mut' argument folds at no call
    [[nodiscard]] auto nests_alike_at_every_call(const toc& tc) const -> bool {
        return std::ranges::all_of(
            std::views::zip(call_.args, call_.func->params(), call_.places),
            [&](const auto& triple) -> bool {
                const auto& [arg, param, arg_place]{triple};

                if (arg_place and arg_place->info.is_length_unknown()) {
                    return false;
                }

                return not is_value_param(param) or
                       arg.fold(tc, tc.use_at(param.get_type()));
            });
    }

    // an inlined recursion that runs out of registers, whether or not a
    // constant argument would end it
    [[nodiscard]] auto recursion_error() const -> compile_error {
        return {
            call_.name_tk,
            std::format("recursion of inlined function {} runs out of "
                        "registers before it ends at compile time, declare "
                        "it 'noinline'",
                        in_quotes(call_.func->name())),
            compile_error::error_kind::out_of_registers,
        };
    }

    //
    // statics
    //

    // the inlined body reaches a place or a value of a built-in type by
    // the binding it gives, see 'stmt_call::argument_places'
    [[nodiscard]] static auto
    bind_argument(toc& tc, const expr_any& arg,
                  const stmt_def_func_param& param,
                  const std::optional<place>& arg_place,
                  machine::held_registers& held) -> param_binding {

        // note: 'stmt_call::assert_argument_type' checked the type, an array
        //       argument is a place
        //       ('parse_array_parameter_argument')

        assert(arg_place or not param.is_array());

        if (arg_place) {
            const stmt_identifier& ident{*arg_place->ident};

            // an indexed argument keeps its computed address
            if (ident.has_computed_address(arg_place->info)) {
                return bind_reference(tc, *arg_place, param, held);
            }

            return param_binding::make_name(param.name(), arg_place->info,
                                            ident.path_text());
        }

        // e.g. 'x' bound to a register of the caller; a constant, which the
        // callee's own constants would shadow, is folded
        if (const stmt_identifier* const ident{arg.bare_path()}) {
            const ident_info info{make_ident_info(tc, *ident)};

            if (info.is_register()) {
                return param_binding::make_name(param.name(), info,
                                                ident->path_text());
            }
        }

        return bind_expression(tc, arg, param, held);
    }

    // a constant lets the inlined body be decided at compile time
    [[nodiscard]] static auto bind_expression(toc& tc, const expr_any& arg,
                                              const stmt_def_func_param& param,
                                              machine::held_registers& held)
        -> param_binding {

        const std::optional<folded_value> folded{
            arg.fold(tc, tc.use_at(param.get_type())),
        };

        if (folded) {
            // note: as a noinline call, which compiles every argument
            arg.check_folded_conditions(tc);

            return param_binding::make_constant(param.name(), param.get_type(),
                                                folded->value);
        }

        const operand reg{
            held.alloc_scratch_register({arg.tok(), tc.indent()},
                                        param.get_type()),
        };

        arg.compile(tc, destination::of_register(reg));

        return param_binding::make_register(param.name(), reg,
                                            reg.register_name());
    }

    // an indexed argument keeps its computed address, e.g.
    // '[rbp + r14 * 4 + 205]', '[r15 + r14]' or simply '[r15]'
    [[nodiscard]] static auto bind_reference(toc& tc, const place& arg_place,
                                             const stmt_def_func_param& param,
                                             machine::held_registers& held)
        -> param_binding {

        const stmt_identifier& ident{*arg_place.ident};

        // note: the address registers stay allocated until the inlined body is
        //       compiled

        const operand lea{
            ident.compile_lea(tc, held, {}),
        };

        return param_binding::make_name(
            param.name(), arg_place.info.at_address(lea), ident.path_text());
    }

    // the result name of the function refers to the destination
    [[nodiscard]] static auto bind_result(const destination& dst,
                                          const func_result& result)
        -> param_binding {

        const std::string_view name{result.name_tk.text()};

        // e.g. the scratch register of an expression, or a parameter that
        // stands for one
        if (dst.is_register()) {
            return param_binding::make_register(name, dst.operand(),
                                                dst.text());
        }

        return param_binding::make_name(name, dst.bound_info(), dst.text());
    }

    // a limit ends the recursion, not the check, except the inlining limit of
    // a recursion that nests alike at every call, which rejects it
    [[nodiscard]] static auto ends_recursion(const compile_error& e,
                                             const bool nests_alike) -> bool {

        const bool is_nesting_alike_limit{
            nests_alike and e.kind == compile_error::error_kind::inlining_limit,
        };

        return e.is_limit() and not is_nesting_alike_limit;
    }

    // e.g. '[s0 + t0 * 4 + 28]' but not '[t1 + 28]'
    [[nodiscard]] static auto is_indexed_reference(const param_binding& binding)
        -> bool {

        const ident_info& bound{binding.info};

        return bound.is_address_by_call() and
               not bound.operand().index_register().empty();
    }

    // an instance literal or a call result, e.g. 'point{1, 2}', is made in
    // a temporary of the call that the body names
    [[nodiscard]] static auto
    is_temporary(const std::optional<place>& arg_place,
                 const stmt_def_func_param& param) -> bool {

        return not arg_place and not param.get_type().is_builtin();
    }

    // a parameter that takes a value, not a place: read-only, built-in and
    // not an array
    [[nodiscard]] static auto is_value_param(const stmt_def_func_param& param)
        -> bool {

        return param.is_read_only() and not param.is_array() and
               param.get_type().is_builtin();
    }
};

// compiles a call of a 'noinline' function: its body is emitted once for all
// calls and reaches the result and the arguments through their addresses in
// the callee frame
//
// the steps:
//
//   1. 'compile' and 'compile_arguments_and_call': a result or an argument
//      that is not a variable in memory goes through a temporary, the result
//      is copied after the call; the arguments are compiled in their order
//      ('compile_arguments'), the address of a variable at its argument
//   2. 'compile_with_arguments': the arguments agree with the parameters,
//      checked by 'stmt_call::assert_argument_type' before ('assert_call'
//      asserts it); the body for the array lengths is asked for
//      ('toc::add_noinline_body') and compiled after 'main'
//      ('stmt_def_func::emit_noinline_body')
//   3. the addresses are written to the callee frame ('populate_frame': the
//      frame and stack checks and 'write_frame_slots', which returns the
//      receiver address the callee finds in a register) and the function is
//      called
class noinline_call final {
    // what the call passes for an argument: the variable it names or a
    // temporary of the current block that holds its value
    // note: a place is passed by its address, any other argument in a
    //       temporary
    struct noinline_arg {
        ident_info info;
        // computed at the argument's position in the list, in a register held
        // until the frame is written when the variable has a run-time index
        operand address;
    };

    call_site call_{};

  public:
    explicit noinline_call(call_site call) : call_{std::move(call)} {
        assert(not call_.func->is_inlined());
    }

    // the callee reaches the result and the arguments through their addresses,
    // a result destination or an argument that is not a variable in memory,
    // e.g. a register or an expression, goes through a temporary, as does a
    // result that widens into an operand
    auto compile(toc& tc, const destination& dst) const -> void {
        call_.compile_with_temporaries(tc, dst, has_argument_temporary(),
                                       has_result_temporary(dst, *call_.func),
                                       [&](const destination& result) -> void {
                                           compile_arguments_and_call(tc,
                                                                      result);
                                       });
    }

  private:
    auto comment_frame_slot(toc& tc, const frame_slot& slot) const -> void {
        machine& x{tc.machine()};

        if (not slot.param_index) {
            x.comment({call_.tk, tc.indent()},
                      "result address in callee frame");

            return;
        }

        const size_t index{*slot.param_index};
        const expr_any& arg{call_.args.at(index)};
        const stmt_def_func_param& param{call_.func->params().at(index)};

        x.comment(
            {call_.tk, tc.indent()}, "address of argument {} to parameter {}",
            in_quotes(statement::trimmed_source(arg)), in_quotes(param.name()));
    }

    // what is passed for each argument, in their order; 'held' holds the
    // address registers
    [[nodiscard]] auto compile_arguments(toc& tc,
                                         machine::held_registers& held) const
        -> std::vector<noinline_arg> {

        std::vector<noinline_arg> arguments;

        for (const auto [i, arg_place] :
             std::views::zip(std::views::iota(size_t{}), call_.places)) {

            // e.g. a literal or an expression
            if (not arg_place) {
                const destination temporary{
                    call_.make_argument_temporary(tc, i),
                };

                arguments.push_back({
                    .info{temporary.bound_info()},
                    .address{temporary.operand()},
                });

                continue;
            }

            const operand address{
                arg_place->ident->operand_of(tc, arg_place->info, held),
            };

            arguments.push_back({
                .info{arg_place->info},
                .address{address},
            });
        }

        return arguments;
    }

    // the arguments are compiled in their order, a call in the index of a
    // place runs before the later arguments as the checks assume
    auto compile_arguments_and_call(toc& tc, const destination& dst) const
        -> void {

        machine& x{tc.machine()};

        machine::held_registers held{x, {call_.tk, tc.indent()}};

        const std::vector<noinline_arg> arguments{
            compile_arguments(tc, held),
        };

        compile_with_arguments(tc, dst, arguments, held);
    }

    // 'held' holds the addresses of the arguments, freed once the frame is
    // written
    auto compile_with_arguments(toc& tc, const destination& dst,
                                const std::span<const noinline_arg> arguments,
                                machine::held_registers& held) const -> void {

        assert_call(dst, *call_.func, arguments);

        call_.zero_result_padding(tc, dst);

        const std::vector<std::optional<size_t>> array_lengths{
            array_argument_lengths(*call_.func, arguments),
        };

        // a body is emitted for a function that is called, once for all calls
        // without array parameters; an argument of unknown length, in the
        // check of a function without its calls, asks for a body to check
        // with that length unknown and the others known, as an inlined call
        // compiles it
        tc.add_noinline_body(*call_.func, array_lengths, call_.begin_tk,
                             call_.source);

        // start the callee frame after the caller's storage, not on the stack
        // pointer, the layout of README 4.1
        const operand frame_address{tc.next_frame_address()};

        const std::optional<operand> receiver_address{
            populate_frame(tc, dst, arguments, frame_address, array_lengths),
        };

        held.release();

        machine& x{tc.machine()};

        // the address in a register is loaded after the registers are saved,
        // a register that holds a value of the caller is not lost
        x.call_function({call_.tk, tc.indent()}, call_.func->name(),
                        call_.func->body_label(array_lengths), frame_address,
                        receiver_address);
    }

    // a value without storage, e.g. a literal or an expression, is passed in
    // a temporary
    [[nodiscard]] auto has_argument_temporary() const -> bool {
        return std::ranges::any_of(
            call_.places,
            [](const std::optional<place>& p) -> bool { return not p; });
    }

    // the slots of the callee frame at 'frame_address' get the addresses of
    // the result and the arguments, the registers that hold them stay
    // allocated until the frame is populated; returns the receiver address
    // of 'write_frame_slots'
    [[nodiscard]] auto populate_frame(
        toc& tc, const destination& dst,
        const std::span<const noinline_arg> arguments,
        const operand& frame_address,
        const std::span<const std::optional<size_t>> array_lengths) const
        -> std::optional<operand> {

        machine& x{tc.machine()};

        x.check_frame_capacity({call_.tk, tc.indent()}, frame_address,
                               call_.func->frame_size_label(array_lengths));

        // the registers holding the addresses of the arguments are still
        // allocated, the check must not change a register that is free but
        // used by the call
        x.check_stack_capacity({call_.tk, tc.indent()});

        return write_frame_slots(tc, frame_address, dst, arguments);
    }

    // each slot holds an address, not the value stored at that address;
    // returns the address the callee finds in a register, none when no slot
    // is in it
    [[nodiscard]] auto
    write_frame_slots(toc& tc, const operand& frame_address,
                      const destination& dst,
                      const std::span<const noinline_arg> arguments) const
        -> std::optional<operand> {

        machine& x{tc.machine()};

        operand slot_address{frame_address};
        std::optional<operand> receiver_address;

        for (const frame_slot& slot : call_.func->frame_slots()) {
            // the callee finds it in a register, it has no slot
            if (slot.is_in_register) {
                receiver_address = slot_address_of(dst, slot, arguments);
                continue;
            }

            comment_frame_slot(tc, slot);

            x.address_of({call_.tk, tc.indent()}, slot_address,
                         slot_address_of(dst, slot, arguments));

            slot_address.increment_offset(
                address_offset(x.address_size_bytes()));
        }

        return receiver_address;
    }

    //
    // statics
    //

    // the body is compiled for the lengths of the array arguments
    [[nodiscard]] static auto
    array_argument_lengths(const stmt_def_func& func,
                           const std::span<const noinline_arg> arguments)
        -> std::vector<std::optional<size_t>> {

        std::vector<std::optional<size_t>> lengths;

        for (const auto [argument, param] :
             std::views::zip(arguments, func.params())) {

            if (param.is_array()) {
                lengths.push_back(argument.info.shape().length());
            }
        }

        return lengths;
    }

    // the callee reaches the result and arguments through their addresses
    static auto assert_call(const destination& dst, const stmt_def_func& func,
                            const std::span<const noinline_arg> arguments)
        -> void {

        // note: 'stmt_call::compile' checked the result, a result in a
        //       register or widened goes through a temporary

        if (func.has_result()) {
            assert(dst.storage());
            assert(dst.type_ref().is_same(func.result_type()));

            // note: an array literal gives its call elements an element
            //       destination

            assert(not dst.is_array());
        }

        // a value without storage is passed in a temporary of the
        // parameter's type
        for (const auto [argument, param] :
             std::views::zip(arguments, func.params())) {

            const ident_info& info{argument.info};

            // note: a temporary is a variable
            assert(info.is_var());

            // note: parsing the call rejects an array argument for a non-array
            //       parameter and the reverse

            assert(info.is_array() == param.is_array());

            // note: 'stmt_call::assert_argument_type' checked a variable, a
            //       temporary has the type of its parameter

            assert(info.type_ref().is_same(param.get_type()));
        }
    }

    // the callee writes the result through its address at the type of the
    // result
    [[nodiscard]] static auto has_result_temporary(const destination& dst,
                                                   const stmt_def_func& func)
        -> bool {

        return func.has_result() and
               (dst.is_register() or call_site::result_type_differs(dst, func));
    }

    // the destination for the result, the argument for a parameter
    [[nodiscard]] static auto
    slot_address_of(const destination& dst, const frame_slot& slot,
                    const std::span<const noinline_arg> arguments) -> operand {

        if (not slot.param_index) {
            return dst.operand();
        }

        return arguments.at(*slot.param_index).address;
    }
};

// selects the constructor of 'stmt_call' for 'point.at(1, 2)'
struct constructor_call_tag {};

// a call outside a function would run before 'main'
inline auto assert_call_in_func(const toc& tc, const token& call_tk) -> void {
    if (not tc.scopes().is_in_func()) {
        throw compile_error{
            call_tk,
            "a call runs only inside a function, not in the initializer of a "
            "global"};
    }
}

// a path as an argument, e.g. the receiver of a method or the array of
// 'array_length'; the whitespace before it belongs to its first token
[[nodiscard]] inline auto path_argument(stmt_identifier path) -> expr_any {
    const token pos_tk{path.tok().start_position()};
    return {pos_tk, expr_path{std::move(path)}};
}

// the arguments of a built-in that parses like a call, e.g. '(1, buf, 4)' of
// 'write(1, buf, 4)'; each is a value at the default type, those at
// 'array_indexes' array paths ('stmt_identifier::parse_array_argument'); the
// built-in gives the count they may have and checks their kinds
class builtin_arguments final {
    token open_paren_tk_;
    std::vector<expr_any> args_;
    std::vector<token> delims_tk_;
    token close_paren_tk_;

  public:
    // the name of the built-in, 'name_tk', is read and 'parse_builtin' saw
    // the '('; a count below 'min_count' is reported at the name, the next
    // token can be on a later line, a ',' after 'max_count' arguments where
    // it stands, as for a call of a function
    builtin_arguments(toc& tc, tokenizer& tz, const token& name_tk,
                      const std::span<const size_t> array_indexes,
                      const size_t min_count, const size_t max_count)
        : open_paren_tk_{tz.seen_char_token('(')} {

        const std::string count_text{
            argument_count_text(name_tk.text(), min_count, max_count),
        };

        read_arguments(tc, tz, array_indexes, min_count, max_count, count_text);

        if (args_.size() < min_count) {
            throw compile_error{name_tk, count_text};
        }
    }

    [[nodiscard]] auto at(const size_t index) const -> const expr_any& {
        return args_.at(index);
    }

    // null past the last argument, e.g. the count that 'read(0, b)' leaves out
    [[nodiscard]] auto find(const size_t index) const -> const expr_any* {
        return index < args_.size() ? &args_.at(index) : nullptr;
    }

    auto source_to(std::ostream& os) const -> void {
        open_paren_tk_.source_to(os);
        statement::source_delimited_to(os, args_, delims_tk_);
        close_paren_tk_.source_to(os);
    }

    // the arguments in their order; of the array at 'index_only_position' only
    // the indexes are read, e.g. the buffer of 'read'
    auto visit_operands(const operand_visitor visitor,
                        const std::optional<size_t> index_only_position) const
        -> void {

        for (const auto [i, e] : std::views::enumerate(args_)) {
            const bool is_index_only{
                index_only_position and std::cmp_equal(i, *index_only_position),
            };

            visitor({
                .node{&e},
                .index_only_place{is_index_only ? e.bare_path() : nullptr},
            });
        }
    }

  private:
    // the next argument: an array or an element of one at the positions of
    // 'array_indexes', otherwise a value
    auto read_argument(toc& tc, tokenizer& tz,
                       const std::span<const size_t> array_indexes) -> void {

        if (not std::ranges::contains(array_indexes, args_.size())) {
            args_.emplace_back(tc, tz, tc.builtins().default_type(),
                               literal_typing::from_destination);

            return;
        }

        stmt_identifier array{stmt_identifier::parse_array_argument(tc, tz)};

        // an array or an element of one, where a bulk operation starts
        // note: 'parse_array_argument' rejected a method
        if (not array.is_array_element()) {
            array.assert_array();
        }

        args_.push_back(path_argument(std::move(array)));
    }

    // up to and including the ')', which follows the argument numbered
    // 'max_count' at the latest; a surplus argument is not parsed, e.g. the
    // 'zz' of 'exit(1, zz)'
    auto read_arguments(toc& tc, tokenizer& tz,
                        const std::span<const size_t> array_indexes,
                        const size_t min_count, const size_t max_count,
                        const std::string_view count_text) -> void {

        while (true) {
            if (const std::optional<token> close_paren_tk{
                    tz.accept_char_token(')'),
                }) {

                close_paren_tk_ = *close_paren_tk;
                return;
            }

            read_argument(tc, tz, array_indexes);

            const std::optional<token> delim_tk{tz.accept_char_token(',')};

            if (not delim_tk) {
                // e.g. 'array_copy(a b, 1)' lacks the ',' like a call of a
                // function, 'write(1, text n)' the ',' or the ')', 'exit(1 n)'
                // the ')'
                close_paren_tk_ = tz.expect_char_token(
                    ')', missing_after_argument_text(args_.size(), min_count,
                                                     max_count));

                return;
            }

            tz.assert_no_trailing_delimiter(*delim_tk, ')', "a call");

            if (args_.size() == max_count) {
                throw compile_error{*delim_tk, std::string{count_text}};
            }

            delims_tk_.emplace_back(*delim_tk);
        }
    }

    //
    // statics
    //

    // what may follow the argument numbered 'count'
    [[nodiscard]] static auto
    missing_after_argument_text(const size_t count, const size_t min_count,
                                const size_t max_count) -> std::string {

        if (count < min_count) {
            return std::format("expected ',' before argument {}", count + 1);
        }

        if (count < max_count) {
            return "expected ',' or ')' after the argument";
        }

        return std::string{missing_close_paren_text};
    }
};

// the result destination and the arguments of a call that name variables,
// with what the callee reaches of each: a part that one of them writes may be
// reached through another one or be a global the callee uses, so it may be
// read or written under another name, see 'stmt_call::assert_no_shared_storage'
class shared_storage_check final {
  public:
    // the result destination or an argument that names a variable, with
    // what the callee reaches of it, kept to compare with the next ones
    struct reference {
        token tk;
        // e.g. "argument 1 'a'" or "the result destination 'x'"
        std::string text;
        storage_path target{};
        std::vector<call_effect> reached;
    };

  private:
    static constexpr std::string_view use_temporary_fix_{
        "use a temporary variable for one of them",
    };

    std::reference_wrapper<const stmt_def_func> callee_;
    std::vector<reference> references_;

  public:
    explicit shared_storage_check(const stmt_def_func& callee)
        : callee_{callee} {}

    // rejects 'ref' when it reaches a part of an earlier reference that one
    // of the two writes, else keeps it
    auto add(reference ref) -> void {
        for (const reference& other : references_) {
            if (not may_share(ref, other)) {
                continue;
            }

            // note: the root of an argument is a name of the source or a
            //       parameter, a temporary is the result or behind a binding

            assert(not is_temporary_name(ref.target.root.name));

            throw compile_error{
                ref.tk, std::format("{} may share storage with {} (both name "
                                    "{}), {}",
                                    ref.text, other.text,
                                    in_quotes(ref.target.root.name),
                                    use_temporary_fix_)};
        }

        references_.push_back(std::move(ref));
    }

    auto assert_not_shared_with_globals() const -> void {
        const stmt_def_func& callee{callee_.get()};

        for (const body_uses::global_use& global : callee.uses().globals()) {
            assert_not_shared_with_global(global);
        }
    }

  private:
    // what the callee writes through the result or an argument may be a
    // global it uses, or what it writes to the global may be what it reaches
    // through them, e.g. 'g = add_g()' where 'add_g' sets 'res' and reads 'g'
    auto
    assert_not_shared_with_global(const body_uses::global_use& global) const
        -> void {

        for (const reference& ref : references_) {
            for (const call_effect& reached : ref.reached) {
                if (not(reached.is_write or global.is_written) or
                    not reached.storage.may_overlap(global.storage)) {

                    continue;
                }

                const stmt_def_func& callee{callee_.get()};

                throw compile_error{
                    ref.tk,
                    std::format("{} may share storage with the global "
                                "{} that {} uses, {}",
                                ref.text, in_quotes(global.storage.root.name),
                                in_quotes(callee.name()), use_temporary_fix_)};
            }
        }
    }

    //
    // statics
    //

    [[nodiscard]] static auto may_share(const reference& lhs,
                                        const reference& rhs) -> bool {

        for (const call_effect& l : lhs.reached) {
            for (const call_effect& r : rhs.reached) {
                if ((l.is_write or r.is_write) and
                    l.storage.may_overlap(r.storage)) {

                    return true;
                }
            }
        }

        return false;
    }
};

// a call of a function, a method or a constructor
//
// parsing, in the constructors: the call is in a function
// ('assert_call_in_func', first as for a built-in), the kind of the callee
// ('assert_method_receiver', 'constructor_func'), the callee
// ('resolve_callee', a generic function through 'generic_call'), then from
// the '(' the arguments ('parse_arguments', an array one read by
// 'parse_array_parameter_argument'), each checked in its order by
// 'check_arguments' ('assert_argument_writable', 'assert_argument_type'), then
// 'record_uses_of_call'
//
// compiling, in 'compile': the checks made before the inlined and noinline
// paths split ('assert_call_accepted'), then 'inline_call' or 'noinline_call'
// compiles the call, an inlined recursion in the check of a body that takes
// from a call through 'inline_call::check_recursion'
class stmt_call final : public statement {
    // e.g. the '.' in 'lst.add(x)', the receiver is the first argument;
    // none when the call is not a method call
    std::optional<token> method_dot_tk_;

    // e.g. the '.' and 'at' in 'point.at(1, 2)', 'tok()' is the type
    struct constructor_tokens {
        token dot_tk;
        token name_tk;
    };

    // none when the call is not a constructor call
    std::optional<constructor_tokens> constructor_;
    // e.g. '<', 'name' and '>' in 'tz.to<name>()'
    std::vector<token> generic_tks_;
    // the callee, resolved at the parse; 'toc' keeps it at this address
    const stmt_def_func* func_{};
    token open_paren_tk_;
    std::vector<expr_any> args_;
    std::vector<token> arg_delims_tk_;
    token close_paren_tk_;

  public:
    // 'tz' is after the name, before its '<' or '('. 'expected_type' is the
    // type the call is assigned to, it tells a type parameter that is the
    // result, e.g. 'x.f = tz.to()'
    stmt_call(toc& tc, unary_ops uops, const token& tk, tokenizer& tz,
              const type* const expected_type)
        : statement{tk, std::move(uops)} {

        assert_call_in_func(tc, tok());

        const std::vector<size_t> deducing_args{
            resolve_callee(tc, tz, tk.text(), expected_type),
        };

        // note: a call of 'main' can only be in 'main' or after it, in a
        //       function that nothing calls, since names are defined before use

        if (callee().name() == reserved_names::main) {
            throw compile_error{tok(), "'main' cannot be called"};
        }

        parse_arguments(tc, tz, "function name", deducing_args);
    }

    // e.g. 'lst.add(x)' calls 'list.add' with 'lst' as 'self'
    stmt_call(toc& tc, unary_ops uops, stmt_identifier receiver, tokenizer& tz,
              const type* const expected_type)
        : statement{receiver.method_name_token(), std::move(uops)},
          method_dot_tk_{receiver.method_dot_token()} {

        const std::string name{
            method_name_of(receiver.get_type().name(),
                           receiver.method_name_token().text()),
        };

        // the receiver is written first, it is checked before the instance
        // is made and the deduction reads the arguments
        assert_call_in_func(tc, tok());
        assert_method_receiver(tc, receiver, name);

        // e.g. 'p.get<i8>()' where 'get' is not generic, the '<' is not an
        // operator after a method name
        if (not tc.generics().has_func(name) and tz.is_type_args_next()) {

            throw_not_generic(tok(), "method", name);
        }

        const std::vector<size_t> deducing_args{
            resolve_callee(tc, tz, name, expected_type),
        };

        // the receiver is the first argument
        args_.push_back(path_argument(std::move(receiver)));

        parse_arguments(tc, tz, "method name", deducing_args);
    }

    // e.g. 'point.at(1, 2)' calls the constructor 'point.at', 'tz' is after
    // 'point'
    stmt_call(constructor_call_tag /*tag*/, toc& tc, unary_ops uops,
              const token& type_tk, tokenizer& tz)
        : statement{type_tk, std::move(uops)},
          constructor_{read_constructor_tokens(tz)},
          func_{&constructor_func(tc)} {

        // e.g. 'point.at<i8>(1, 2)', the '<' is not an operator after a
        // constructor name
        if (tz.is_type_args_next()) {
            throw_not_generic(callee_name_token(), "constructor",
                              callee().name());
        }

        parse_arguments(tc, tz, "constructor name", {});
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        source_callee_to(os);
        open_paren_tk_.source_to(os);

        // the receiver is written before the name
        const size_t first{first_argument_index()};

        source_delimited_to(os, args_ | std::views::drop(first),
                            arg_delims_tk_);

        close_paren_tk_.source_to(os);
    }

    auto compile(toc& tc, const destination& dst) const -> void override {

        // note: the built-ins that parse like a call, e.g. 'exit' and the io
        //       functions, are nodes of their own ('stmt_builtin_*')

        const call_site call{site(tc)};

        machine& x{tc.machine()};

        x.comment({tok(), tc.indent()}, call.source);

        assert_call_accepted(tc, dst, call.places);

        if (callee().is_inlined()) {
            inline_call{call}.compile(tc, dst);
        } else {
            noinline_call{call}.compile(tc, dst);
        }

        // a call statement, its result is not used
        if (dst.is_empty()) {
            // note: the parse gives a call statement no operators
            assert(get_unary_ops().is_empty());

            return;
        }

        // the operators of the call apply to its result where it is used, at
        // the width and with the overflow check of the destination, e.g. the
        // '~' of '~f(x)'
        get_unary_ops().compile(x, {tok(), tc.indent()}, dst.operand(),
                                tc.overflow_check(dst));
    }

    // the calls in the arguments run first, then the body
    auto visit_call_effects(const toc& tc,
                            const call_effect_visitor visitor) const
        -> void override {

        statement::visit_call_effects(tc, visitor);
        visit_callee_effects(tc, argument_places(tc), visitor);
    }

    // the arguments in their order
    auto visit_operands(const operand_visitor visitor) const -> void override {
        for (const expr_any& e : args_) {
            visitor({
                .node{&e},
                .index_only_place{},
            });
        }
    }

    //
    // class methods
    //

    // note: set at the parse before the arguments are parsed
    [[nodiscard]] auto callee() const -> const stmt_def_func& { return *func_; }

    // e.g. 'at' of 'point.at(1, 2)', 'tok()' is the type there
    [[nodiscard]] auto callee_name_token() const -> const token& {
        return constructor_ ? constructor_->name_tk : tok();
    }

  private:
    // the place each argument names, decided again at the parse, the walk of
    // the effects and each compile because the bindings of the current scope
    // decide it; none for a value without storage:
    //
    //   argument        example         inlined body gets  noinline body
    //   place           'p', 'a[i]'     the name, or the   its address
    //                                   computed address
    //   instance        'point{1, 2}',  a temporary        a temporary
    //                   'mk(1, 2)'
    //   value           'x + 1',        a constant, a      a temporary
    //                   '-a[i]', '-x',  register, or the
    //                   '3', 'k', 'x'   register 'x' is
    //                   bound to a      bound to
    //                   register
    [[nodiscard]] auto argument_places(const toc& tc) const
        -> std::vector<std::optional<place>> {

        std::vector<std::optional<place>> places;
        places.reserve(args_.size());

        for (const expr_any& arg : args_) {
            places.push_back(as_place(tc, arg));
        }

        return places;
    }

    // the type of an argument, one verdict for an inlined and a noinline
    // call: a name or a typed constant has the type of its parameter, its
    // storage or typed value is passed; a value that is not a place must fit
    // its built-in parameter, e.g. 'a + 1' and '-a' widen into it
    // note: checked at the parse, a binding has the type of its parameter,
    //       so no call changes the verdict
    auto assert_argument_type(const toc& tc, const size_t index,
                              const std::optional<place>& arg_place) const
        -> void {

        const expr_any& arg{args_.at(index)};
        const stmt_def_func_param& param{callee().params().at(index)};

        if (param.get_type().is_builtin() and not arg_place) {
            arg.assert_not_narrowed(tc, param.get_type());
        }

        const stmt_identifier* const ident{arg.bare_path()};

        if (ident == nullptr) {
            return;
        }

        const type* const arg_type{named_argument_type(tc, *ident)};

        if (arg_type != nullptr and not arg_type->is_same(param.get_type())) {
            throw_parameter_type_mismatch(arg, param, *arg_type);
        }
    }

    // a callee that writes its parameter writes the variable of the caller:
    // an argument that is not a writable variable is rejected here, also in
    // a function that is never called, so inlining does not change what is
    // accepted
    auto assert_argument_writable(const size_t index,
                                  const std::optional<place>& written) const
        -> void {

        // a parameter without 'mut' takes any argument
        if (callee().params().at(index).is_read_only()) {
            return;
        }

        const expr_any& arg{args_.at(index)};
        const stmt_identifier* const path{arg.bare_path()};

        // e.g. 'a + 1', '-a' or 'point{1, 2}'
        if (path == nullptr) {
            throw_not_writable(index,
                               std::format("{}, which is not a variable,",
                                           in_quotes(trimmed_source(arg))),
                               "");
        }

        // e.g. '2' or a 'let' constant
        if (not written) {
            throw_not_writable(
                index, std::format("constant {}", in_quotes(path->path_text())),
                "");
        }

        if (written->info.is_read_only()) {
            throw_not_writable(
                index,
                std::format("read-only {}",
                            in_quotes(written->ident->path_text())),
                read_only_hint(written->ident->tok().text(), written->info));
        }
    }

    // what the destination and the bindings of the current scope decide,
    // checked before the inlined and the noinline paths split, so inlining
    // does not change what is accepted; the result is the call's own, an
    // error in it has no call frame
    auto assert_call_accepted(
        const toc& tc, const destination& dst,
        const std::span<const std::optional<place>> places) const -> void {

        assert_no_shared_storage(tc, dst, places);

        // note: the parse rejected a discarded result
        //       ('parse_name_statement') and a call without one as a value
        //       ('parse_operand')

        assert(callee().has_result() != dst.is_empty());

        if (callee().has_result()) {
            assert_result_type(dst);
        }

        // e.g. the 'i' of 'f(arr[i], h(i))' with 'h' writing 'i'
        assert_not_written_later(tc, *this, "argument");
    }

    // a function is called after its body or from inside it, so the record of
    // a callee is complete except for a call of itself; a generic instance
    // parsed inside a body may name that body's function, e.g. 'gen<W>'
    // calling back 'W.run' that instantiates it (README 7.2)
    auto assert_callee_body_parsed(const toc& tc) const -> void {
        if (callee().is_body_parsed() or is_call_in_own_body(tc)) {
            return;
        }

        throw compile_error{
            callee_name_token(),
            std::format("{} is called before its body ends, two "
                        "functions cannot call each other",
                        in_quotes(callee().name()))};
    }

    // the function 'name' is a method and the receiver is a value
    auto assert_method_receiver(const toc& tc, const stmt_identifier& receiver,
                                const std::string_view name) const -> void {

        // a constructor has no receiver, it builds its value; a generic
        // function of a type is a method
        if (not tc.generics().has_func(name) and
            tc.get_func_or_throw(tok(), name).is_constructor()) {

            const std::string path{receiver.path_text()};

            // e.g. 'point.at(1)' after 'dat point = point{}', the value hides
            // the type, 'point.at(...)' is what was written
            if (tc.types().has(path)) {
                throw_value_hides_type(receiver.tok(), path);
            }

            throw compile_error{
                tok(), std::format("{} is a constructor, call it as '{}(...)'",
                                   in_quotes(name), name)};
        }

        // only an element of an array can be the receiver, 'a[1].f()' but not
        // 'a.f()'
        if (receiver.is_array()) {
            throw compile_error{
                receiver.tok(),
                std::format("receiver of method {} cannot be an array",
                            in_quotes(name))};
        }
    }

    // the callee reaches the arguments, the result and the globals in place,
    // so a part that one of them writes may be read or written under
    // another name; the record of the body says which parts it reaches
    // ('body_uses')
    auto assert_no_shared_storage(
        const toc& tc, const destination& dst,
        const std::span<const std::optional<place>> places) const -> void {

        shared_storage_check check{callee()};

        // none for a destination without storage, e.g. a register
        const std::optional<storage_path> dst_storage{dst.storage()};

        if (dst_storage) {
            check.add(make_reference(
                tc, std::nullopt, callee_name_token(),
                std::format("the result destination {}", in_quotes(dst.text())),
                *dst_storage));
        }

        for (size_t i{}; i < args_.size(); ++i) {
            // a value or a constant is passed as a copy
            const std::optional<place>& passed{places.at(i)};

            if (not passed) {
                continue;
            }

            const expr_any& arg{args_.at(i)};
            const storage_path target{place_storage(tc, *passed)};

            check.add(
                make_reference(tc, i, arg.tok(),
                               std::format("{} {}", describe_argument(i),
                                           in_quotes(trimmed_source(arg))),
                               target));
        }

        // note: always, a body compiled once for all calls, a noinline one,
        //       cannot see that a parameter names a global it also uses

        check.assert_not_shared_with_globals();
    }

    // e.g. the '.' of 'mk().y' or 'mk().sum()', a result has no storage to
    // read a field of or to call a method on; the '[' of 'mk()[0]', a result
    // is never an array
    auto assert_result_not_accessed(const tokenizer& tz) const -> void {
        const char next{tz.peek_char_after_whitespace()};

        if (next != '.' and next != '[') {
            return;
        }

        // e.g. 'g().y' of a 'g' without a result, as 'x = g()'
        if (not callee().has_result()) {
            throw_no_value(callee_name_token(), callee().name());
        }

        if (next == '[') {
            throw compile_error{
                callee_name_token(),
                std::format("a call of {} is not an array, it cannot be "
                            "indexed",
                            in_quotes(callee().name()))};
        }

        throw compile_error{tz, std::format("store the result of {} in a "
                                            "variable before reading a field "
                                            "or calling a method of it",
                                            in_quotes(callee().name()))};
    }

    // the result has the type of the destination or is a narrower integer,
    // which widens into it like a name, e.g. 'exit(f8())'
    //
    // note: the parse rejected another kind ('expr_any::assert_number_kind')
    //       or instance type, 'assert_not_narrowed' a wider integer
    auto assert_result_type([[maybe_unused]] const destination& dst) const
        -> void {

        assert(not call_site::result_type_differs(dst, callee()) or
               is_widened_integer(callee().result_type(), dst.type_ref()));
    }

    // the receiver of a method is written before the name
    [[nodiscard]] auto call_begin_token() const -> const token& {
        return is_method() ? args_.at(0).tok() : tok();
    }

    // each argument in its order, then what the callee reaches of the
    // caller is recorded
    auto check_arguments(toc& tc) const -> void {
        const std::vector<std::optional<place>> places{argument_places(tc)};

        for (size_t i{}; i < args_.size(); ++i) {
            const std::optional<place>& arg_place{places.at(i)};

            // note: an array argument is the path of a whole array
            //       ('parse_array_parameter_argument'), the parse of a value
            //       rejected one ('assert_not_whole_array')

            assert(callee().params().at(i).is_array() ==
                   (arg_place and arg_place->info.is_array()));

            assert_argument_writable(i, arg_place);
            assert_argument_type(tc, i, arg_place);
        }

        record_uses_of_call(tc, places);
    }

    // the type names the constructor after the '.', e.g. 'at' of
    // 'point.at(1, 2)', a method of the type is not one; the call is in a
    // function, checked first as for a function call
    [[nodiscard]] auto constructor_func(const toc& tc) const
        -> const stmt_def_func& {

        // note: only a constructor call asks
        assert(constructor_);

        const token& name_tk{constructor_->name_tk};

        assert_call_in_func(tc, name_tk);

        if (not name_tk.is_word()) {
            throw compile_error{name_tk,
                                "expected a constructor name after '.'"};
        }

        // e.g. the '1' of 'point.1(2)' or the 'var' of 'point.var(2)', as
        // where a constructor is defined
        toc::assert_valid_name(name_tk);

        // note: the parse took 'tok()' as a type ('classify_term'), the type
        //       a type parameter is bound to names the constructor, e.g.
        //       'point.at' for 'T.at' where 'T' is 'point'

        const std::string name{
            method_name_of(tc.get_type_or_throw(tok()).name(), name_tk.text()),
        };

        if (not tc.is_func(name)) {
            throw compile_error{
                name_tk, std::format("type {} has no constructor {}",
                                     in_quotes(type_description(tc, tok())),
                                     in_quotes(name_tk.text()))};
        }

        // note: a constructor is not generic, a generic function of the type
        //       is a method ('stmt_def_func_generic::assert_generic_signature')

        const bool names_method{
            tc.generics().has_func(name) or
                not tc.get_func_or_throw(name_tk, name).is_constructor(),
        };

        if (names_method) {
            throw compile_error{
                name_tk,
                std::format("{} is a method, call it on a value of type {}",
                            in_quotes(name),
                            in_quotes(type_description(tc, tok())))};
        }

        return tc.get_func_or_throw(name_tk, name);
    }

    [[nodiscard]] auto describe_argument(const size_t index) const
        -> std::string {

        if (index < first_argument_index()) {
            return "receiver";
        }

        return std::format("argument {}", index + 1 - first_argument_index());
    }

    // the receiver is not counted as an argument
    [[nodiscard]] auto first_argument_index() const -> size_t {
        return is_method() ? 1 : 0;
    }

    // the call is in the body of its callee, a recursion
    [[nodiscard]] auto is_call_in_own_body(const toc& tc) const -> bool {
        return tc.scopes().current_func_scope().is_name(callee().name());
    }

    [[nodiscard]] auto is_method() const -> bool {
        return method_dot_tk_.has_value();
    }

    [[nodiscard]] auto make_reference(const toc& tc,
                                      const std::optional<size_t> param_index,
                                      const token& tk, std::string text,
                                      const storage_path& target) const
        -> shared_storage_check::reference {

        shared_storage_check::reference result{
            .tk{tk},
            .text{std::move(text)},
            .target{target},
            .reached{},
        };

        // note: the parts are compared, not named

        visit_effects_of(tc, param_index, {}, target,
                         [&](const call_effect& effect) -> void {
                             result.reached.push_back(effect);
                         });

        return result;
    }

    // e.g. 'a' and 'b + 1' of 'f(a, b + 1)'
    auto parse_argument(toc& tc, tokenizer& tz, const size_t arg_index,
                        const stmt_def_func_param& param,
                        const std::span<const size_t> deducing_args) -> void {

        // e.g. the 'bs' of 'f(bs)' for 'gs bool[]', a path as the array of a
        // built-in, not a value of the element type, which for a 'bool' is a
        // condition that rejects a whole array
        if (param.is_array()) {
            stmt_identifier array{
                parse_array_parameter_argument(tc, tz, param.name()),
            };

            args_.push_back(path_argument(std::move(array)));

            return;
        }

        // a 'mut' argument names its type, so 'f(pt{1, 2})' is rejected as
        // not a variable rather than for the type it writes
        const bool is_written_type{
            std::ranges::contains(deducing_args, arg_index) or
                not param.is_read_only(),
        };

        args_.emplace_back(tc, tz, param.get_type(),
                           is_written_type ? literal_typing::written
                                           : literal_typing::from_destination);
    }

    // from the '(' on, 'what' names the callee; a method receiver is already
    // in 'args_', the parameter gives the type of a literal argument unless
    // its type was deduced from it, see 'generic_call::deducing_args'
    auto parse_arguments(toc& tc, tokenizer& tz, const std::string_view what,
                         const std::span<const size_t> deducing_args) -> void {

        // the type is the result type of the callee, resolved before, so an
        // unknown callee is reported before a missing '('
        assert_callee_body_parsed(tc);

        set_type(callee().result_type());

        read_open_paren(tz, what);

        const std::span<const stmt_def_func_param> params{callee().params()};
        const size_t first{args_.size()};

        args_.reserve(params.size());

        for (size_t i{first}; i < params.size(); ++i) {
            const stmt_def_func_param& param{params.at(i)};

            // counted without a receiver, like 'deducing_args'
            const size_t arg_index{i - first_argument_index()};

            // otherwise the missing argument is reported as a parse error of
            // the parameter's type
            // e.g. 'f()' of 'func f(x)'
            if (tz.peek_char_after_whitespace() == ')') {
                throw compile_error{
                    tz, missing_argument_text(arg_index + 1, param.name())};
            }

            if (i != first) {
                read_argument_delimiter(tz, arg_index, param);
            }

            parse_argument(tc, tz, arg_index, param, deducing_args);
        }

        assert_no_surplus_argument(tz, callee().name(),
                                   params.size() - first_argument_index());

        // e.g. 'f(a, b' lacks it
        close_paren_tk_ = tz.expect_char_token(')', missing_close_paren_text);

        assert_result_not_accessed(tz);

        check_arguments(tc);
    }

    // e.g. the ',' of 'f(a, b)'
    auto read_argument_delimiter(tokenizer& tz, const size_t arg_index,
                                 const stmt_def_func_param& param) -> void {

        const std::string message{
            missing_delimiter_text(arg_index + 1, param.name()),
        };

        const token delimiter_tk{tz.expect_char_token(',', message)};

        tz.assert_no_trailing_delimiter(delimiter_tk, ')', "a call");

        arg_delims_tk_.emplace_back(delimiter_tk);
    }

    // the '(' after the callee, 'what' names the callee
    auto read_open_paren(tokenizer& tz, const std::string_view what) -> void {
        const std::optional<token> paren_tk{tz.accept_char_token('(')};

        // at the name, the next token can be on a later line
        if (not paren_tk) {
            throw compile_error{callee_name_token(),
                                std::format("expected '(' after {}", what)};
        }

        open_paren_tk_ = *paren_tk;
    }

    // what the callee reaches of the caller is the caller's use too, see
    // 'visit_callee_effects'
    auto record_uses_of_call(
        toc& tc, const std::span<const std::optional<place>> places) const
        -> void {

        // note: a call outside a function was rejected (the constructors)
        assert(tc.scopes().is_in_func());

        visit_callee_effects(
            tc, places, [&](const call_effect& effect) -> void {
                tc.record_use(effect.storage, effect.is_write ? use_kind::write
                                                              : use_kind::read);
            });

        for (const auto [passed, param] :
             std::views::zip(places, callee().params())) {

            // note: a 'mut' argument needs storage, the result of an inlined
            //       call may be bound to a register

            if (passed and not param.is_read_only() and
                tc.scopes().names_result(passed->ident->tok().text())) {

                tc.record_result_passed_as_mut();
            }
        }
    }

    // the name is read, a generic function has its type arguments before the
    // '(', e.g. 'convert<i8>(x)', and the callee is the instance they name;
    // an unknown callee is an error here; returns
    // 'generic_call::deducing_args'
    [[nodiscard]] auto resolve_callee(toc& tc, tokenizer& tz,
                                      const std::string_view name,
                                      const type* const expected_type)
        -> std::vector<size_t> {

        if (not tc.generics().has_func(name)) {
            func_ = &tc.get_func_or_throw(tok(), name);
            return {};
        }

        generic_call generic{
            generic_call::parse(tc, tz, tok(), name, expected_type),
        };

        generic_tks_ = std::move(generic.generic_tks);

        func_ = &tc.get_func_or_throw(tok(), generic.func_name);

        return std::move(generic.deducing_args);
    }

    // what 'inline_call' and 'noinline_call' need of this call
    [[nodiscard]] auto site(const toc& tc) const -> call_site {
        return {
            .tk{tok()},
            .name_tk{callee_name_token()},
            .begin_tk{call_begin_token()},
            .close_paren_tk{close_paren_tk_},
            .source{trimmed_source(*this)},
            .args{args_},
            .func{func_},
            .places{argument_places(tc)},
        };
    }

    // e.g. 'f', 'lst.add' or 'point.at'
    auto source_callee_to(std::ostream& os) const -> void {
        if (constructor_) {
            statement::source_to(os);
            constructor_->dot_tk.source_to(os);
            constructor_->name_tk.source_to(os);
            return;
        }

        if (not is_method()) {
            statement::source_to(os);
            source_generic_to(os);
            return;
        }

        get_unary_ops().source_to(os);
        args_.front().source_to(os);
        token::source_to(os, method_dot_tk_);
        tok().source_to(os);
        source_generic_to(os);
    }

    auto source_generic_to(std::ostream& os) const -> void {
        for (const token& t : generic_tks_) {
            t.source_to(os);
        }
    }

    // 'what' names the argument at 'index', 'hint' says how to make it
    // writable
    [[noreturn]] auto throw_not_writable(const size_t index,
                                         const std::string_view what,
                                         const std::string_view hint) const
        -> void {

        const expr_any& arg{args_.at(index)};

        if (is_method() and index == 0) {
            throw compile_error{
                arg.tok(),
                std::format("{} cannot be the receiver of 'mut' method {}{}",
                            what, in_quotes(callee().name()), hint)};
        }

        throw compile_error{
            arg.tok(), std::format("{} cannot be passed to a 'mut' parameter{}",
                                   what, hint)};
    }

    // the globals the callee uses and what its body reaches of each argument
    // through its parameter ('visit_effects_of')
    auto
    visit_callee_effects(const toc& tc,
                         const std::span<const std::optional<place>> places,
                         const call_effect_visitor visitor) const -> void {

        const body_uses& callee_uses{callee().uses()};

        for (const body_uses::global_use& u : callee_uses.globals()) {
            visitor({
                .call_tk{tok()},
                .name{u.storage.root.name},
                .storage{u.storage},
                .is_write{u.is_written},
            });
        }

        for (const auto [i, passed] :
             std::views::zip(std::views::iota(size_t{}), places)) {

            // a value or a constant is passed as a copy
            if (not passed) {
                continue;
            }

            const stmt_identifier& ident{*passed->ident};
            const storage_path target{place_storage(tc, *passed)};
            const std::string name{ident.path_text()};

            visit_effects_of(tc, i, name, target, visitor);
        }
    }

    // what the body reaches of 'target', the argument of the parameter at
    // 'param_index' or the result (empty), by its record; a call of the
    // function it is in, whose record the parse is still making, reaches it
    // whole, written unless the parameter is read-only
    // note: only a function that calls itself has an incomplete record
    //       ('assert_callee_body_parsed')
    auto visit_effects_of(const toc& tc,
                          const std::optional<size_t> param_index,
                          const std::string_view name,
                          const storage_path& target,
                          const call_effect_visitor visitor) const -> void {

        if (is_call_in_own_body(tc)) {
            visitor({
                .call_tk{tok()},
                .name{name},
                .storage{target},
                .is_write{
                    not param_index or
                        not callee().params().at(*param_index).is_read_only(),
                },
            });

            return;
        }

        for (const body_uses::binding_use& u : callee().uses().bindings()) {
            if (u.param_index != param_index) {
                continue;
            }

            visitor({
                .call_tk{tok()},
                .name{name},
                .storage{target.extended_by(u.path)},
                .is_write{u.is_written},
            });
        }
    }

    //
    // statics
    //

    [[nodiscard]] static auto is_widened_integer(const type& result_type,
                                                 const type& dst_type) -> bool {

        return result_type.is_integer() and dst_type.is_integer() and
               result_type.check_width_bytes() < dst_type.check_width_bytes();
    }

    // the storage of the variable an argument names
    [[nodiscard]] static auto place_storage(const toc& tc, const place& passed)
        -> storage_path {

        std::optional<storage_path> target{passed.ident->storage(tc)};

        // note: a variable has storage
        assert(target);

        return std::move(*target);
    }

    // the '.' and the name after a type, e.g. '.at' of 'point.at(1, 2)'
    [[nodiscard]] static auto read_constructor_tokens(tokenizer& tz)
        -> constructor_tokens {

        // note: the parse saw the '.' after the type
        const token dot_tk{tz.seen_char_token('.')};

        return {
            .dot_tk{dot_tk},
            .name_tk{tz.next_token()},
        };
    }

    [[noreturn]] static auto
    throw_parameter_type_mismatch(const expr_any& arg,
                                  const stmt_def_func_param& param,
                                  const type& arg_type) -> void {

        const type& param_type{param.get_type()};

        // e.g. an 'i64' variable for an 'i32' parameter, a converted value is
        // passed like 'a + 1'; widths compared as the narrowing checks see
        // them, so the default type of a 32-bit target gets the hint too
        const bool is_narrowing{
            param.is_read_only() and not param.is_array() and
                param_type.is_integer() and arg_type.is_integer() and
                arg_type.check_width_bytes() > param_type.check_width_bytes(),
        };

        const std::string hint{
            is_narrowing ? std::format(", use '{}(...)'", param_type.name())
                         : "",
        };

        throw compile_error{
            arg.tok(),
            std::format("expected type {} for parameter {}, got {}{}",
                        in_quotes(param_type.name()), in_quotes(param.name()),
                        in_quotes(arg_type.name()), hint)};
    }

    // the type as written, with the type it is bound to when that differs,
    // e.g. 'T (point)'
    [[nodiscard]] static auto type_description(const toc& tc,
                                               const token& type_tk)
        -> std::string {

        const std::string_view bound{tc.get_type_or_throw(type_tk).name()};

        if (bound == type_tk.text()) {
            return std::string{bound};
        }

        return std::format("{} ({})", type_tk.text(), bound);
    }
};
