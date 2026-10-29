#pragma once

#include <algorithm>
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
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "compile_error.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
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
//   'generic_call'    the instance of a generic function that the call names
//   'call_limits'     how deep the compile of calls nests
//   'call_site'       what compiling the call needs of it
//   'inline_call'     compiles the body of the function in place
//   'noinline_call'   passes addresses to the body that is emitted once
//   'constructor_call_tag'
//                     selects the constructor of 'point.at(1, 2)'
//   free functions    'assert_not_string_argument', 'assert_call_in_func',
//                     'path_argument'
//   'builtin_arguments'
//                     the arguments of a built-in that parses like a call,
//                     e.g. 'exit(0)'
//
// an argument, from the parse to the body:
//
//   'call_site::argument_places' the place each argument names, none for a
//                                value; whether an argument goes through a
//                                temporary is the rule of 'inline_call' or
//                                'noinline_call'
//   'param_binding'              inlined: what the parameter stands for
//                                ('inline_call::make_bindings')
//   'noinline_call::noinline_arg'
//                                noinline: the variable passed, or the
//                                temporary that holds the value
//                                ('compile_arguments_and_call',
//                                'call_site::make_argument_temporary')
//   frame slot                   the address written to it
//                                ('noinline_call::slot_address_of')
//
// the formatter sorts the functions of a class by name, so the comment above
// each class lists them in the order they run
//

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
        std::vector<size_t> argument_indexes;
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
    // note: 'name_tk' is the name as written, e.g. 'to', and 'generic_name'
    //       the generic function, e.g. 'tokenizer.to'
    [[nodiscard]] static auto
    parse(toc& tc, tokenizer& tz, const token& name_tk,
          const std::string_view generic_name, const type* const expected_type)
        -> generic_call {

        const std::optional<token> open_tk{tz.is_next_char_token('<')};

        std::vector<const type*> type_args;
        std::vector<token> generic_tks;
        std::vector<size_t> deducing_args;

        if (not open_tk) {
            deduction deduced{
                deduced_type_arguments(tc, tz, name_tk, generic_name,
                                       expected_type),
            };

            type_args = std::move(deduced.type_args);
            deducing_args = std::move(deduced.argument_indexes);
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

    // the type of the argument at 'index' when it is a variable, a field or an
    // element, or the value it makes names its type: 'point{1, 2}',
    // 'point.at(1, 2)' or 'mk(1, 2)', otherwise null. 'tz' is after the '(' and
    // is a copy, the caller does not move
    [[nodiscard]] static auto argument_type(toc& tc, tokenizer tz,
                                            const size_t index) -> const type* {

        if (not skip_to_argument(tz, index)) {
            return nullptr;
        }

        const token tk{tz.next_token()};
        const term_kind tm{classify_term(tc, tk, tz)};

        // e.g. 'show(nope)', an unknown name is reported as such, not as a
        // type that cannot be deduced
        if (tm == term_kind::unknown) {
            std::ignore = tc.resolve_declared(tk, tk.text());
        }

        // e.g. 'show(nope(1))', reported as an unknown function
        if (tm == term_kind::call and not tc.is_func(tk.text()) and
            not tc.types().has(tk.text())) {

            std::ignore = tc.get_func_or_throw(tk, tk.text());
        }

        // e.g. 'mk()' hidden by 'let mk = 5' is still a call, its result
        // tells the type
        if (tm != term_kind::value) {
            return value_type(tc, tz, tk, tm);
        }

        // note: a constant does not tell its type, like a literal
        if (tc.scopes().lookup(tk.text()).is_constant()) {
            return nullptr;
        }

        // note: read from the copy to find where the argument ends and its
        //       type, the call parses it again

        const stmt_identifier ident{tc, tk, tz};

        const char next{tz.peek_char_after_whitespace()};

        if (ident.is_method_receiver() or (next != ',' and next != ')')) {
            return nullptr;
        }

        return &make_ident_info(tc, ident).type_ref();
    }

    // e.g. 'f(1)' of 'func f<T type>(a, x T)' lacks the argument that would
    // tell 'T', which is the error; 'tz' is after the '(' and is a copy
    static auto assert_argument_present(tokenizer tz, const size_t index,
                                        const std::string_view param_name)
        -> void {

        std::ignore = skip_to_argument(tz, index);

        // a malformed list is reported by the parse of the arguments
        if (tz.peek_char_after_whitespace() != ')') {
            return;
        }

        throw compile_error{tz, std::format("expected argument {} ({})",
                                            index + 1, in_quotes(param_name))};
    }

    // the type arguments of 'f(x)' for 'func f<T type>(s T)': a type parameter
    // is the type of the argument for a parameter declared with that type, when
    // the argument tells it ('argument_type'), or the type the call is assigned
    // to when it is the result. a literal or an expression does not tell its
    // type
    [[nodiscard]] static auto
    deduced_type_arguments(toc& tc, const tokenizer& tz, const token& name_tk,
                           const std::string_view generic_name,
                           const type* const expected_type) -> deduction {

        const generic_func_info& generic{tc.generics().get_func(generic_name)};

        // the arguments are looked at ahead, the call reads them later
        tokenizer scan{tz};

        if (not scan.is_next_char_token('(')) {
            throw_generic_function_without_arguments(name_tk, generic_name,
                                                     name_tk.text());
        }

        deduction deduced{};

        for (const token& param_tk : generic.signature.type_param_tks) {
            const generic_deduction source{
                generic.signature.deduction(param_tk.text()),
            };

            const type* arg_type{};

            if (source.param_index) {
                assert_argument_present(
                    scan, *source.param_index,
                    generic.signature.params.at(*source.param_index)
                        .name_tk.text());

                arg_type = argument_type(tc, scan, *source.param_index);

                if (arg_type != nullptr) {
                    deduced.argument_indexes.push_back(*source.param_index);
                }
            }

            if (arg_type == nullptr and source.is_result) {
                arg_type = expected_type;
            }

            if (arg_type == nullptr) {
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

    // moves 'tz' from after the '(' to the argument at 'index', false when the
    // list ends before it
    [[nodiscard]] static auto skip_to_argument(tokenizer& tz,
                                               const size_t index) -> bool {

        for (size_t i{}; i < index; ++i) {
            tz.skip_argument();

            const std::optional<token> delimiter_tk{tz.is_next_char_token(',')};

            if (not delimiter_tk) {
                return false;
            }

            // e.g. 'f(1,)', reported before the argument it lacks
            tz.assert_no_trailing_delimiter(*delimiter_tk, ')', "a call");
        }

        return true;
    }

    // the type of an instance literal, a constructor call or a call of a
    // function that is the whole argument, 'tk' is its first token and 'tz' is
    // after it
    // note: an argument is parsed at the type of its parameter, so it is read
    //       here without a parse, unlike a 'var' initializer, see
    //       'initializer_declaration::deduce_from_initializer'
    [[nodiscard]] static auto value_type(toc& tc, tokenizer& tz,
                                         const token& tk, const term_kind tm)
        -> const type* {

        const auto is_whole_argument = [&] -> bool {
            const char next{tz.peek_char_after_whitespace()};
            return next == ',' or next == ')';
        };

        // e.g. 'point{1, 2}'
        if (tm == term_kind::record_literal) {
            tz.skip_braced_block();
            return is_whole_argument() ? &tc.get_type_or_throw(tk) : nullptr;
        }

        // e.g. 'point.at(1, 2)'
        if (tm == term_kind::constructor_call) {
            std::ignore = tz.is_next_char_token('.');

            // the type names the result, the constructor, e.g. 'at', does not
            std::ignore = tz.next_token();

            if (not tz.is_next_char_token('(')) {
                return nullptr;
            }

            tz.skip_to_close_paren();

            return is_whole_argument() ? &tc.get_type_or_throw(tk) : nullptr;
        }

        // e.g. 'read(0, buf)', the value of a built-in
        if (const type* const builtin_type{builtin_value_type(tc, tk, tm)};
            builtin_type != nullptr) {

            if (not tz.is_next_char_token('(')) {
                return nullptr;
            }

            tz.skip_to_close_paren();

            return is_whole_argument() ? builtin_type : nullptr;
        }

        // e.g. 'mk(1, 2)', a generic function has no return type before it is
        // instantiated
        if (tm != term_kind::call or not tc.funcs().has(tk.text())) {
            return nullptr;
        }

        std::ignore = tz.is_next_char_token('(');
        tz.skip_to_close_paren();

        const type& return_type{
            tc.get_func_or_throw(tk, tk.text()).result_type(),
        };

        return is_whole_argument() and
                       not return_type.is_same(tc.builtins().void_type())
                   ? &return_type
                   : nullptr;
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

// the limits on how deep the compile of calls nests, each a recursion of the
// compiler: the scopes being compiled and the inlined calls of one function in
// each other; the errors are located at the call 'call_tk'
class call_limits final {
  public:
    // a scope being compiled: a block, a loop or a function body
    static constexpr size_t max_scope_count{256};

    // an inlined recursion ends at compile time when a constant argument
    // makes its condition fold, otherwise it would expand without end
    static constexpr size_t max_inlined_nesting{64};

    //
    // statics
    //

    // an inlined call compiles its body in place, so nesting is limited
    static auto assert_inline_nesting(toc& tc, const token& call_tk,
                                      const stmt_def_func& func) -> void {

        // e.g. a recursion that a constant argument ends too late or never
        if (tc.scopes().inlined_nesting(func.name()) >= max_inlined_nesting) {
            throw inlining_limit_error(
                call_tk,
                std::format("recursion of inlined function {} is nested "
                            "deeper than {} calls, declare it 'noinline'",
                            in_quotes(func.name()), max_inlined_nesting));
        }

        if (tc.scopes().count() < max_scope_count) {
            return;
        }

        throw inlining_limit_error(
            call_tk,
            std::format("inlined calls and blocks are nested too deep, the "
                        "limit is {} scopes, declare a function 'noinline'",
                        max_scope_count));
    }

    // an inlined recursion that runs out of registers
    [[nodiscard]] static auto recursion_error(const token& call_tk,
                                              const stmt_def_func& func)
        -> compile_error {

        return inlining_limit_error(
            call_tk, std::format("recursion of inlined function {} cannot be "
                                 "expanded at compile time, declare it "
                                 "'noinline'",
                                 in_quotes(func.name())));
    }

  private:
    //
    // statics
    //

    // the call is too deep to inline, e.g. a recursion that does not end at
    // compile time
    [[nodiscard]] static auto inlining_limit_error(const token& call_tk,
                                                   std::string message)
        -> compile_error {

        compile_error error{call_tk, std::move(message)};
        error.kind = compile_error::error_kind::inlining_limit;
        return error;
    }
};

// what compiling a call needs of it, 'stmt_call' makes it for each compile
struct call_site {
    // the name of a function or method, the type of a constructor call; the
    // code and the errors of the call are located at it
    token tk;
    // the receiver of a method is written before the name
    token begin_tk;
    token close_paren_tk;
    // e.g. 'lst.add(x)', for the comment of the call and the frame of an error
    std::string source;
    // a method receiver is the first argument
    std::span<const expr_any> args;
    // the callee, resolved when the call was parsed; never null
    const stmt_def_func* func{};

    // the error was found inside the inlined body, unless it is in the call
    // itself where the call is already the error location
    auto add_call_frame(compile_error& e) const -> void {
        if (is_inside_call(e)) {
            return;
        }

        e.add_call_frame(begin_tk, source);
    }

    // a variable in the current block, a root variable of its own
    auto add_temporary(toc& tc, const size_t indent, const std::string& name,
                       const type& tpe) const -> destination {

        tc.add_var(indent,
                   {
                       .name{name},
                       .type_ptr{&tpe},
                       .src_loc_tk{tk},
                       .shape{},
                       .placement{},
                       .foo_array{},
                   },
                   var_kind::var);

        return destination::of_name(
            tc.resolve_declared(tk, name), name,
            tc.storage_of(name, access_path::whole_variable()));
    }

    // the place each argument names, decided at each compile because the
    // bindings of the current scope decide it; none for a value without
    // storage:
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
        places.reserve(args.size());

        for (const auto [arg, param] : std::views::zip(args, func->params())) {
            std::optional<place> arg_place{as_place(tc, arg)};

            // note: an array argument is a place, see 'assert_argument_usable'
            assert(not param.is_array() or arg_place);

            places.push_back(std::move(arg_place));
        }

        return places;
    }

    // the result is made in a temporary of the call, then copied into
    // 'dst', widened when 'dst' is wider
    auto compile_through_result_temporary(
        toc& tc, const size_t indent, const destination& dst,
        const std::function_ref<void(const destination& result)> compile) const
        -> void {

        const destination result{
            add_temporary(tc, indent, temporary_result_name(),
                          func->result_type()),
        };

        compile(result);

        machine& x{tc.machine()};

        x.copy_value(tk, indent, dst.operand(), result.operand());
    }

    // a variable of the current block that holds the argument at 'index',
    // compiled into it
    [[nodiscard]] auto make_argument_temporary(toc& tc, const size_t indent,
                                               const size_t index) const
        -> destination {

        const destination temporary{
            add_temporary(tc, indent, temporary_argument_name(index),
                          func->params().at(index).get_type()),
        };

        args.at(index).compile(tc, indent, temporary);

        return temporary;
    }

    // unique per call and argument, an inlined body names it through the
    // binding of its parameter and a call in the body has temporaries of its
    // own; each level of an inlined recursion declares it again, the binding
    // resolves it in the caller's scope
    [[nodiscard]] auto temporary_argument_name(const size_t index) const
        -> std::string {

        return temporary_name(tk, "call-arg", index);
    }

    // the result of a call made in a temporary, see
    // 'compile_through_result_temporary'
    [[nodiscard]] auto temporary_result_name() const -> std::string {
        return temporary_name(tk, "call-result", 0);
    }

    // the result is written field by field, so its padding is zeroed for
    // '==' to compare instances byte by byte; after the arguments, which may
    // read the destination, e.g. 'p = f(p.a + 1)'
    auto zero_result_padding(toc& tc, const size_t indent,
                             const destination& dst) const -> void {

        if (dst.is_empty()) {
            return;
        }

        machine& x{tc.machine()};

        x.zero_padding(tk, indent, dst.operand(), dst.type_ref());
    }

    //
    // statics
    //

    // a block holds the temporaries of a call and frees them after it
    // note: a block is a scope, which counts toward the nesting limit and the
    //       'max scopes in use' report, so a call without temporaries enters
    //       none
    static auto in_temporaries_block(toc& tc, const bool has_temporaries,
                                     const std::function_ref<void()> compile)
        -> void {

        if (not has_temporaries) {
            compile();
            return;
        }

        const toc::scope_entry block{tc, scope::block()};

        compile();
    }

    // 'dst' has another type than the result, after 'assert_result_type' a
    // widening
    [[nodiscard]] static auto result_type_differs(const destination& dst,
                                                  const stmt_def_func& func)
        -> bool {

        return not dst.type_ref().is_same(func.result_type());
    }

  private:
    [[nodiscard]] auto is_inside_call(const compile_error& e) const -> bool {

        // note: a recursive call is checked inside its own body, so an error
        //       can be located after the call

        if (not e.where) {
            return false;
        }

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
//   1. 'compile': a result narrower than the operand it widens into is
//      computed in a register and widened ('compile_widened'), a block holds
//      the temporaries of the call
//   2. 'compile_with_bindings': the bindings of the result and the arguments
//      ('make_bindings'), an instance literal or a call result argument is
//      made in a temporary, an indexed argument may get its address in a
//      register ('compile_with_address_choice',
//      'compile_body_with_address_registers')
//   3. 'compile_body': the body under labels unique to the call, with the
//      bindings added
class inline_call final {
    // what the body names by its result and its parameters
    struct call_bindings {
        std::optional<param_binding> result;
        std::vector<param_binding> arguments;
    };

    call_site call_{};
    // one per argument, see 'call_site::argument_places'
    std::vector<std::optional<place>> places_;

  public:
    inline_call(const toc& tc, call_site call)
        : call_{std::move(call)}, places_{call_.argument_places(tc)} {

        assert(call_.func->is_inlined());
    }

    // a block holds the temporaries of the call
    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void {

        if (call_.func->has_result() and
            call_site::result_type_differs(dst, *call_.func)) {
            // note: a narrower integer, see 'stmt_call::assert_result_type'

            compile_widened(tc, indent, dst);
            return;
        }

        // a result the body passes to a 'mut' parameter needs storage, a
        // register destination gets it through a temporary of the call
        const bool has_result_temporary{
            dst.is_register() and call_.func->uses().is_result_passed_as_mut(),
        };

        call_site::in_temporaries_block(
            tc, has_argument_temporary() or has_result_temporary, [&] -> void {
                if (not has_result_temporary) {
                    compile_with_bindings(tc, indent, dst);
                    return;
                }

                call_.compile_through_result_temporary(
                    tc, indent, dst, [&](const destination& result) -> void {
                        compile_with_bindings(tc, indent, result);
                    });
            });
    }

  private:
    auto compile_body(toc& tc, const size_t indent,
                      const call_bindings& bindings) const -> void {

        // the body is inlined at every call, so its labels are named by the
        // call
        const std::string new_call_path{tc.call_path_at(call_.tk)};

        const std::string call_label{
            std::format("{}..{}", call_.func->body_label(), new_call_path),
        };

        const std::string ret_jmp_label{toc::end_label(call_label)};

        machine& x{tc.machine()};

        call_.func->source_def_comment_to(x, indent);

        x.label(indent, call_label);

        const toc::scope_entry body{
            tc,
            scope::inlined_func(call_.func->name(), new_call_path,
                                ret_jmp_label),
            call_.tk,
        };

        // an error found in the inlined body reports where it was called from,
        // an error in the arguments is in the caller and has no call frame
        try {
            call_.func->add_constants(tc, indent + 1);

            const auto add_binding{
                [&](const param_binding& binding) -> void {
                    const ident_info& bound{binding.info};

                    x.comment_binding(call_.tk, indent + 1, binding.from,
                                      binding.text,
                                      bound.is_address_by_call()
                                          ? std::optional{bound.operand()}
                                          : std::nullopt);

                    tc.add_binding(binding);
                },
            };

            if (bindings.result) {
                add_binding(*bindings.result);
            }

            for (const param_binding& binding : bindings.arguments) {
                add_binding(binding);
            }

            // the result reaches the destination through the binding of 'res',
            // so the statements of the body have no destination
            call_.func->compile_statements(tc, indent);
        } catch (compile_error& e) {
            call_.add_call_frame(e);
            throw;
        }

        // provide the exit label for 'return' to jump to
        x.label(indent, ret_jmp_label);
    }

    // a new register per argument because the index register may belong to
    // an enclosing binding that is used after the call
    auto compile_body_with_address_registers(toc& tc, const size_t indent,
                                             call_bindings bindings) const
        -> void {

        machine& x{tc.machine()};

        machine::held_registers held{x, call_.tk, indent + 1};

        for (param_binding& binding : bindings.arguments) {
            if (not is_indexed_reference(binding)) {
                continue;
            }

            ident_info& bound{binding.info};

            const operand address{
                x.alloc_scratch_register(call_.tk, indent,
                                         tc.builtins().address()),
            };

            held.add(address);

            x.comment(call_.tk, indent, "address of parameter {}",
                      in_quotes(binding.from));

            x.address_of(call_.tk, indent, address, bound.operand());

            bound = bound.at_address(
                operand::mem(address, bound.operand().type_ref()));
        }

        compile_body(tc, indent, bindings);
    }

    // the result is made at its type in a register that the body names, the
    // copy into the operand widens it
    auto compile_widened(toc& tc, const size_t indent,
                         const destination& dst) const -> void {

        machine& x{tc.machine()};

        const operand result{
            x.alloc_scratch_register(call_.tk, indent,
                                     call_.func->result_type()),
        };

        compile(tc, indent, destination::of_register(result));

        x.copy_value(call_.tk, indent, dst.operand(), result);
        x.free_scratch_register(call_.tk, indent, result);
    }

    // note: the body is compiled in two versions and the one with the least
    //       code is kept
    //
    //       1. bindings use the indexed addresses as they are, e.g.
    //          '[s0 + t0 * 4 + 28]'
    //       2. each indexed address goes once into a new register, the
    //          displacement included, e.g. '[t1]'
    //
    //       rv32i has no base + index addressing, so in version 1 every
    //       access adds base and index again; a register pays off only when
    //       the body accesses the argument more than once, which is only
    //       known after compiling the body
    //
    //       the argument registers are shared by all versions, so they are
    //       freed after the choice by 'compile_with_bindings'

    auto compile_with_address_choice(toc& tc, const size_t indent,
                                     const call_bindings& bindings) const
        -> void {

        tc.emit_most_efficient(
            call_.tk, indent,
            [&] -> void { compile_body(tc, indent, bindings); },
            [&] -> void {
                compile_body_with_address_registers(tc, indent, bindings);
            });
    }

    auto compile_with_bindings(toc& tc, const size_t indent,
                               const destination& dst) const -> void {

        machine& x{tc.machine()};

        // scratch registers stay allocated until the inlined body is compiled
        machine::held_registers held{x, call_.tk, indent + 1};

        const call_bindings bindings{make_bindings(tc, indent, dst, held)};

        call_.zero_result_padding(tc, indent, dst);

        const bool has_indexed_argument{
            std::ranges::any_of(bindings.arguments, is_indexed_reference),
        };

        // an access can use the indexed address as it is, so computing it
        // into a register is not tried
        if (not has_indexed_argument or x.has_indexed_addressing()) {
            compile_body(tc, indent, bindings);
            return;
        }

        compile_with_address_choice(tc, indent, bindings);
    }

    [[nodiscard]] auto has_argument_temporary() const -> bool {
        return std::ranges::any_of(
            std::views::iota(size_t{}, places_.size()),
            [this](const size_t i) -> bool {
                return is_temporary(places_.at(i), call_.func->params().at(i));
            });
    }

    // the bindings of the result and of the arguments of the inlined body
    [[nodiscard]] auto make_bindings(toc& tc, const size_t indent,
                                     const destination& dst,
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

        for (const auto [i, arg, param, arg_place] :
             std::views::zip(std::views::iota(size_t{}), call_.args,
                             call_.func->params(), places_)) {

            // note: a temporary is made here, in the order of the arguments,
            //       and named per call, so each level of an inlined recursion
            //       binds its own

            if (is_temporary(arg_place, param)) {
                const destination temporary{
                    call_.make_argument_temporary(tc, indent, i),
                };

                bindings.arguments.push_back(param_binding::make_name(
                    param.name(), temporary.bound_info(),
                    call_.temporary_argument_name(i)));

                continue;
            }

            bindings.arguments.push_back(
                bind_argument(tc, indent, arg, param, arg_place, held));
        }

        return bindings;
    }

    //
    // statics
    //

    // the inlined body reaches a place or a value of a built-in type by
    // the binding it gives, see 'call_site::argument_places'
    [[nodiscard]] static auto
    bind_argument(toc& tc, const size_t indent, const expr_any& arg,
                  const stmt_def_func_param& param,
                  const std::optional<place>& arg_place,
                  machine::held_registers& held) -> param_binding {

        // note: 'stmt_call::assert_argument_types' checked the type

        if (arg_place) {
            const stmt_identifier& ident{*arg_place->ident};

            // an indexed argument keeps its computed address
            if (ident.has_computed_address(arg_place->info)) {
                return bind_reference(tc, indent, *arg_place, param, held);
            }

            return bind_name(param, ident, arg_place->info);
        }

        // e.g. 'x' bound to a register of the caller; a constant, which the
        // callee's own constants would shadow, is folded
        if (const stmt_identifier* const ident{arg.bare_path()}) {
            const ident_info info{make_ident_info(tc, *ident)};

            if (info.is_register()) {
                return bind_name(param, *ident, info);
            }
        }

        return bind_expression(tc, indent, arg, param, held);
    }

    // a constant lets the inlined body be decided at compile time
    [[nodiscard]] static auto bind_expression(toc& tc, const size_t indent,
                                              const expr_any& arg,
                                              const stmt_def_func_param& param,
                                              machine::held_registers& held)
        -> param_binding {

        const std::optional<folded_value> folded{
            arg.fold(tc, tc.use_at(param.get_type())),
        };

        if (folded) {
            // note: as a noinline call, which compiles every argument
            arg.check_folded_conditions(tc, indent);

            return param_binding::make_constant(param.name(), param.get_type(),
                                                folded->value);
        }

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register(arg.tok(), indent, param.get_type()),
        };

        held.add(reg);
        arg.compile(tc, indent, destination::of_register(reg));

        return param_binding::make_register(param.name(), reg,
                                            reg.register_name());
    }

    // a place of the caller that 'ident' names and 'info' resolves
    [[nodiscard]] static auto bind_name(const stmt_def_func_param& param,
                                        const stmt_identifier& ident,
                                        const ident_info& info)
        -> param_binding {

        return param_binding::make_name(param.name(), info, ident.path_text());
    }

    // an indexed argument keeps its computed address, e.g.
    // '[rbp + r14 * 4 + 205]', '[r15 + r14]' or simply '[r15]'
    [[nodiscard]] static auto bind_reference(toc& tc, const size_t indent,
                                             const place& arg_place,
                                             const stmt_def_func_param& param,
                                             machine::held_registers& held)
        -> param_binding {

        const stmt_identifier& ident{*arg_place.ident};

        // note: the address registers stay allocated until the inlined body is
        //       compiled

        const operand lea{
            ident.compile_lea(tc, indent, held, {}),
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
            // note: a register has no address to keep
            assert(not dst.is_final_address());

            return param_binding::make_register(name, dst.operand(),
                                                dst.text());
        }

        return param_binding::make_name(name, dst.bound_info(), dst.text());
    }

    // e.g. [s0 + t0 * 4 + 28] but not [t1 + 28]
    [[nodiscard]] static auto is_indexed_reference(const param_binding& binding)
        -> bool {

        const ident_info& bound{binding.info};

        return bound.is_address_by_call() and bound.operand().is_memory() and
               not bound.operand().index_register().empty();
    }

    // an instance literal or a call result, e.g. 'point{1, 2}', is made in
    // a temporary of the call that the body names
    [[nodiscard]] static auto
    is_temporary(const std::optional<place>& arg_place,
                 const stmt_def_func_param& param) -> bool {

        return not arg_place and not param.get_type().is_builtin();
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
//      is copied after the call; the arguments are compiled in their order,
//      the address of a variable at its argument
//   2. 'compile_with_arguments': the arguments agree with the parameters,
//      checked by 'stmt_call::assert_argument_types' before ('assert_call'
//      asserts it); the body is compiled once per array lengths
//      ('stmt_def_func::emit_noinline_body')
//   3. the addresses are written to the callee frame ('populate_frame': the
//      frame and stack checks and 'write_frame_slots') and the function is
//      called, an address in a register too ('register_address')
class noinline_call final {
    // what the call passes for an argument: the variable it names or a
    // temporary of the current block that holds its value
    struct noinline_arg {
        ident_info info;
        // computed at the argument's position in the list, in a register held
        // until the frame is written when the variable has a run-time index
        operand address;
    };

    call_site call_{};
    // one per argument, a place is passed by its address, any other in a
    // temporary
    std::vector<std::optional<place>> places_;

  public:
    noinline_call(const toc& tc, call_site call)
        : call_{std::move(call)}, places_{call_.argument_places(tc)} {

        assert(not call_.func->is_inlined());
    }

    // the callee reaches the result and the arguments through their addresses,
    // a result destination or an argument that is not a variable in memory,
    // e.g. a register or an expression, goes through a temporary, as does a
    // result that widens into an operand
    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void {

        const bool has_temporaries{
            has_result_temporary(dst, *call_.func) or
                std::ranges::any_of(places_,
                                    [](const std::optional<place>& p) -> bool {
                                        return not p;
                                    }),
        };

        call_site::in_temporaries_block(tc, has_temporaries, [&] -> void {
            compile_arguments_and_call(tc, indent, dst);
        });
    }

  private:
    auto comment_frame_slot(machine& x, const size_t indent,
                            const frame_slot& slot) const -> void {

        if (not slot.param_index) {
            x.comment(call_.tk, indent, "result address in callee frame");
            return;
        }

        const size_t index{*slot.param_index};

        x.comment(call_.tk, indent, "address of argument {} to parameter {}",
                  in_quotes(statement::trimmed_source(call_.args.at(index))),
                  in_quotes(call_.func->params().at(index).name()));
    }

    // an argument or a result that needs one goes through a temporary of the
    // current block; the arguments are compiled in their order, a call in the
    // index of a place runs before the later arguments as the checks assume
    auto compile_arguments_and_call(toc& tc, const size_t indent,
                                    const destination& dst) const -> void {

        machine& x{tc.machine()};

        machine::held_registers held{x, call_.tk, indent};

        std::vector<noinline_arg> arguments;

        for (const auto [i, arg_place] :
             std::views::zip(std::views::iota(size_t{}), places_)) {

            // e.g. a literal or an expression
            if (not arg_place) {
                const destination temporary{
                    call_.make_argument_temporary(tc, indent, i),
                };

                arguments.push_back({
                    .info{temporary.bound_info()},
                    .address{temporary.operand()},
                });

                continue;
            }

            const operand address{
                arg_place->ident->get_lea_operand(tc, indent, arg_place->info,
                                                  held),
            };

            arguments.push_back({
                .info{arg_place->info},
                .address{address},
            });
        }

        if (not has_result_temporary(dst, *call_.func)) {
            compile_with_arguments(tc, indent, dst, arguments, held);
            return;
        }

        call_.compile_through_result_temporary(
            tc, indent, dst, [&](const destination& result) -> void {
                compile_with_arguments(tc, indent, result, arguments, held);
            });
    }

    // 'held' holds the addresses of the arguments, freed once the frame is
    // written
    auto compile_with_arguments(toc& tc, const size_t indent,
                                const destination& dst,
                                const std::span<const noinline_arg> arguments,
                                machine::held_registers& held) const -> void {

        assert_call(dst, *call_.func, arguments);

        call_.zero_result_padding(tc, indent, dst);

        const std::vector<size_t> array_lengths{
            array_argument_lengths(*call_.func, arguments),
        };

        // a body is emitted for a function that is called, once for all calls
        // without array parameters
        // note: an argument of unknown length is in the check of a function
        //       without its calls, no body has its lengths

        if (std::ranges::none_of(arguments, [](const noinline_arg& a) -> bool {
                return a.info.is_length_unknown();
            })) {

            tc.add_noinline_body(*call_.func, array_lengths, call_.begin_tk,
                                 call_.source);
        }

        // start the callee frame after the caller's storage, not on the stack
        // pointer, the layout of README 4.1
        const operand frame_address{tc.next_frame_address()};

        populate_frame(tc, indent, dst, arguments, frame_address,
                       array_lengths);

        held.release();

        machine& x{tc.machine()};

        // the address in a register is loaded after the registers are saved,
        // a register that holds a value of the caller is not lost
        x.call_function(call_.tk, indent, call_.func->body_label(array_lengths),
                        frame_address, register_address(dst, arguments));
    }

    // the slots of the callee frame at 'frame_address' get the addresses of
    // the result and the arguments, the registers that hold them stay
    // allocated until the frame is populated
    auto populate_frame(toc& tc, const size_t indent, const destination& dst,
                        const std::span<const noinline_arg> arguments,
                        const operand& frame_address,
                        const std::span<const size_t> array_lengths) const
        -> void {

        machine& x{tc.machine()};

        if (tc.checks().frame) {
            x.check_frame_capacity(
                call_.tk, indent, frame_address,
                operand::symbol(call_.func->frame_size_label(array_lengths),
                                tc.builtins().address()));
        }

        // the registers holding the addresses of the arguments are still
        // allocated, the check must not change a register that is free but
        // used by the call
        if (tc.checks().stack) {
            x.check_stack_capacity(call_.tk, indent);
        }

        write_frame_slots(x, indent, frame_address, dst, arguments);
    }

    // the address the callee finds in a register, none when no slot is in it
    [[nodiscard]] auto
    register_address(const destination& dst,
                     const std::span<const noinline_arg> arguments) const
        -> std::optional<operand> {

        for (const frame_slot& slot : call_.func->frame_slots()) {
            if (slot.is_in_register) {
                return slot_address_of(dst, slot, arguments);
            }
        }

        return std::nullopt;
    }

    // each slot holds an address, not the value stored at that address
    auto write_frame_slots(machine& x, const size_t indent,
                           const operand& frame_address, const destination& dst,
                           const std::span<const noinline_arg> arguments) const
        -> void {

        operand slot_address{frame_address};

        for (const frame_slot& slot : call_.func->frame_slots()) {
            // the callee finds it in a register, it has no slot
            if (slot.is_in_register) {
                continue;
            }

            comment_frame_slot(x, indent, slot);

            x.address_of(call_.tk, indent, slot_address,
                         slot_address_of(dst, slot, arguments));

            slot_address.increment_offset(
                address_offset(x.address_size_bytes()));
        }
    }

    //
    // statics
    //

    // the body is compiled for the lengths of the array arguments
    [[nodiscard]] static auto
    array_argument_lengths(const stmt_def_func& func,
                           const std::span<const noinline_arg> arguments)
        -> std::vector<size_t> {

        std::vector<size_t> lengths;

        for (const auto [argument, param] :
             std::views::zip(arguments, func.params())) {

            if (param.is_array()) {
                lengths.push_back(argument.info.element_count());
            }
        }

        return lengths;
    }

    // a noinline call passes addresses, a value without storage is passed
    // in a temporary of the parameter's type
    static auto assert_argument(const ident_info& info,
                                const stmt_def_func_param& param) -> void {

        // note: a temporary is a variable
        assert(info.is_var());

        // note: parsing the call rejects an array argument for a non-array
        //       parameter and the reverse

        assert(info.is_array() == param.is_array());

        // note: 'stmt_call::assert_argument_types' checked a variable, a
        //       temporary has the type of its parameter

        assert(info.type_ref().is_same(param.get_type()));
    }

    // the callee reaches the result and arguments through their addresses
    static auto assert_call(const destination& dst, const stmt_def_func& func,
                            const std::span<const noinline_arg> arguments)
        -> void {

        // note: 'stmt_call::compile' checked the result, a result that is
        //       not in memory or is widened goes through a temporary

        if (func.has_result()) {
            assert(dst.operand().is_memory());
            assert(dst.type_ref().is_same(func.result_type()));

            // an array literal gives its call elements an element destination
            assert(not dst.is_array());
        }

        for (const auto [argument, param] :
             std::views::zip(arguments, func.params())) {

            assert_argument(argument.info, param);
        }
    }

    // the callee writes the result through its address at the type of the
    // result
    [[nodiscard]] static auto has_result_temporary(const destination& dst,
                                                   const stmt_def_func& func)
        -> bool {

        return func.has_result() and
               (not dst.operand().is_memory() or
                call_site::result_type_differs(dst, func));
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

// a string is in read-only data that a parameter would refer to
inline auto assert_not_string_argument(tokenizer& tz) -> void {
    if (tz.peek_char_after_whitespace() != '"') {
        return;
    }

    throw compile_error{tz.next_token(), "a string is not an argument, it "
                                         "would refer to read-only data"};
}

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

    return {pos_tk,
            expr_type{std::make_unique<stmt_identifier>(std::move(path))}};
}

// the arguments of a built-in that parses like a call, e.g. '(1, buf, 4)' of
// 'write(1, buf, 4)'; each is a value at the default type, those at
// 'array_indexes' array paths ('stmt_identifier::parse_array_argument'), the
// built-in checks their count and their kinds
class builtin_arguments final {
    token open_paren_tk_;
    std::vector<expr_any> args_;
    std::vector<token> delims_tk_;
    token close_paren_tk_;

  public:
    // the name of the built-in, 'name_tk', is read; a missing '(' and a
    // count outside 'min_count' to 'max_count' are reported at it, the next
    // token can be on a later line
    builtin_arguments(toc& tc, tokenizer& tz, const token& name_tk,
                      const std::span<const size_t> array_indexes,
                      const size_t min_count, const size_t max_count)
        : open_paren_tk_{
              tz.expect_char_token_after(name_tk, '(',
                                         "expected '(' after function name"),
          } {

        read_arguments(tc, tz, array_indexes);

        if (args_.size() < min_count or args_.size() > max_count) {
            throw compile_error{
                name_tk,
                argument_count_text(name_tk.text(), min_count, max_count)};
        }
    }

    [[nodiscard]] auto at(const size_t index) const -> const expr_any& {
        return args_.at(index);
    }

    [[nodiscard]] auto count() const -> size_t { return args_.size(); }

    auto source_to(std::ostream& os) const -> void {
        open_paren_tk_.source_to(os);
        statement::source_delimited_to(os, args_, delims_tk_);
        close_paren_tk_.source_to(os);
    }

    // the arguments in their order; of the array at 'written_index' only
    // the indexes are read, e.g. the buffer of 'read'
    auto visit_operands(const operand_visitor visitor,
                        const std::optional<size_t> written_index) const
        -> void {

        for (const auto [i, e] : std::views::enumerate(args_)) {
            const bool is_written{
                written_index and std::cmp_equal(i, *written_index),
            };

            visitor({
                .node{&e},
                .written_place{is_written ? e.bare_path() : nullptr},
            });
        }
    }

  private:
    // an array or an element, checked where it is read
    auto add_array_argument(stmt_identifier array) -> void {
        array.assert_array_or_element();

        args_.push_back(path_argument(std::move(array)));
    }

    // up to and including the ')'
    auto read_arguments(toc& tc, tokenizer& tz,
                        const std::span<const size_t> array_indexes) -> void {

        while (true) {
            if (const std::optional<token> close_paren_tk{
                    tz.is_next_char_token(')'),
                }) {

                close_paren_tk_ = *close_paren_tk;
                return;
            }

            assert_not_string_argument(tz);

            if (std::ranges::contains(array_indexes, args_.size())) {
                add_array_argument(
                    stmt_identifier::parse_array_argument(tc, tz));
            } else {
                args_.emplace_back(tc, tz, tc.builtins().default_type(),
                                   literal_typing::from_destination);
            }

            const std::optional<token> delim_tk{tz.is_next_char_token(',')};

            if (delim_tk) {
                tz.assert_no_trailing_delimiter(*delim_tk, ')', "a call");
                delims_tk_.emplace_back(*delim_tk);
                continue;
            }

            // e.g. 'write(1, text n)' lacks the ','
            close_paren_tk_ = tz.expect_char_token(
                ')', "expected ',' or ')' after the argument");

            return;
        }
    }
};

// a call of a function, a method or a constructor
//
// parsing, in the constructors: the callee and its '('
// ('resolve_callee', a generic function through 'generic_call', and
// 'read_open_paren'), the kind of the callee
// ('assert_method_receiver', 'constructor_func'), then the arguments
// ('parse_arguments', each checked by 'assert_not_array_literal_argument',
// 'assert_argument_usable' and 'assert_argument_writable'), then
// 'record_uses_of_call'
//
// compiling, in 'compile': the checks made before the inlined and noinline
// paths split ('assert_call_accepted'), then 'inline_call' or 'noinline_call'
// compiles the call
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

    static constexpr std::string_view use_temporary_fix_{
        "use a temporary variable for one of them",
    };

    // the result destination or an argument that names a variable, with
    // what the callee reaches of it, kept to compare with the next ones
    struct reference {
        token tk;
        // e.g. "argument 1 'a'" or "the result destination 'x'"
        std::string text;
        storage_path target{};
        std::vector<call_effect> reached;
    };

  public:
    // 'tz' is after the name, e.g. 'f' of 'f(x)' or 'show' of 'show<name>(x)'.
    // 'expected_type' is the type the call is assigned to, it tells a type
    // parameter that is the result, e.g. 'x.f = tz.to()'
    stmt_call(toc& tc, unary_ops uops, const token tk, tokenizer& tz,
              const type* const expected_type)
        : statement{tk, std::move(uops)} {

        const std::vector<size_t> deducing_args{
            resolve_callee(tc, tz, tk.text(), expected_type),
        };

        read_open_paren(tc, tz, "function name");

        // note: a call of 'main' can only be in 'main' or after it, in a
        //       function that nothing calls, since names are defined before use

        if (callee().name() == reserved_names::main) {
            throw compile_error{tok(), "'main' cannot be called"};
        }

        parse_arguments(tc, tz, deducing_args);
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

        // e.g. 'p.get<i8>()' where 'get' is not generic, the '<' is not an
        // operator after a method name
        if (not tc.generics().has_func(name) and
            tz.peek_char_after_whitespace() == '<') {

            throw compile_error{
                tok(), std::format("method {} is not generic, it takes no "
                                   "type arguments",
                                   in_quotes(name))};
        }

        const std::vector<size_t> deducing_args{
            resolve_callee(tc, tz, name, expected_type),
        };

        read_open_paren(tc, tz, "method name");

        assert_method_receiver(receiver);

        // the receiver is the first argument
        args_.push_back(path_argument(std::move(receiver)));

        parse_arguments(tc, tz, deducing_args);
    }

    // e.g. 'point.at(1, 2)' calls the constructor 'point.at', 'tz' is after
    // 'point'
    stmt_call(constructor_call_tag /*tag*/, toc& tc, unary_ops uops,
              const token type_tk, tokenizer& tz)
        : statement{type_tk, std::move(uops)},
          constructor_{read_constructor_tokens(tz)},
          func_{&constructor_func(tc)} {

        // e.g. 'point.at<i8>(1, 2)', the '<' is not an operator after a
        // constructor name
        if (tz.peek_char_after_whitespace() == '<') {
            throw compile_error{
                callee_name_token(),
                std::format("constructor {} is not generic, it takes no type "
                            "arguments",
                            in_quotes(callee().name()))};
        }

        read_open_paren(tc, tz, "constructor name");

        parse_arguments(tc, tz, {});
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

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        // note: the built-ins that parse like a call, e.g. 'exit' and the io
        //       functions, are nodes of their own ('stmt_builtin_*')

        const call_site call{site()};

        machine& x{tc.machine()};

        x.comment(tok(), indent, call.source);

        assert_call_accepted(tc, dst);

        const bool is_recursion{
            callee().is_inlined() and
                tc.scopes().inlined_nesting(callee().name()) != 0,
        };

        // a check without a call does not expand a recursion: the body is
        // checked where the recursion is entered and in its own check, a
        // constant that would end it is one of a call
        if (is_recursion and tc.is_checking_without_call()) {
            check_arguments_alone(tc, indent);
            return;
        }

        compile_call(tc, indent, dst, call, is_recursion);

        // the operators of the call apply to its result where it is used, at
        // the width and with the overflow check of the destination, e.g. the
        // '~' of '~f(x)'
        get_unary_ops().compile(x, indent, tok(), dst.operand(),
                                tc.overflow_options(dst));
    }

    [[nodiscard]] auto has_inlined_call() const -> bool override {
        return callee().is_inlined() or statement::has_inlined_call();
    }

    // the calls in the arguments run first, then the body
    auto visit_call_effects(const toc& tc,
                            const call_effect_visitor visitor) const
        -> void override {

        statement::visit_call_effects(tc, visitor);
        visit_callee_reach(tc, visitor);
    }

    // the arguments in their order
    auto visit_operands(const operand_visitor visitor) const -> void override {
        for (const expr_any& e : args_) {
            visitor({
                .node{&e},
                .written_place{},
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
    // the type of each argument, one verdict for an inlined and a noinline
    // call: a name or a typed constant has the type of its parameter, its
    // storage or typed value is passed; a value that is not a place must fit
    // its built-in parameter, e.g. 'a + 1' and '-a' widen into it
    auto assert_argument_types(const toc& tc) const -> void {
        for (const auto [arg, param] :
             std::views::zip(args_, callee().params())) {

            if (param.get_type().is_builtin() and not as_place(tc, arg)) {
                arg.assert_not_narrowed(tc, param.get_type());
            }

            const stmt_identifier* const ident{arg.bare_path()};

            if (ident == nullptr) {
                continue;
            }

            const ident_info info{make_ident_info(tc, *ident)};

            if (info.is_const() and not info.constant().is_typed()) {
                continue;
            }

            if (not info.type_ref().is_same(param.get_type())) {
                throw_parameter_type_mismatch(arg, param, info);
            }
        }
    }

    // a callee that writes its parameter writes the variable of the caller:
    // an argument that is not a writable variable is rejected here, also in
    // a function that is never called, so inlining does not change what is
    // accepted
    auto assert_argument_writable(const toc& tc, const size_t index) const
        -> void {

        // a parameter without 'mut' takes any argument
        if (callee().params().at(index).is_read_only()) {
            return;
        }

        const expr_any& arg{args_.at(index)};

        // e.g. 'a + 1', '-a' or 'point{1, 2}'
        if (arg.bare_path() == nullptr) {
            throw_not_writable(index, arg,
                               std::format("{}, which is not a variable,",
                                           in_quotes(trimmed_source(arg))),
                               "");
        }

        const std::optional<place> written{as_place(tc, arg)};

        // e.g. '2' or a 'let' constant
        // note: 'arg' is a path, see above
        if (not written) {
            throw_not_writable(
                index, arg,
                std::format("constant {}",
                            in_quotes(arg.bare_path()->dotted_path())),
                "");
        }

        if (written->info.is_read_only()) {
            throw_not_writable(
                index, arg,
                std::format("read-only {}",
                            in_quotes(written->ident->dotted_path())),
                read_only_hint(written->ident->tok().text(), written->info));
        }
    }

    // checked before the inlined and the noinline paths split, so inlining
    // does not change what is accepted; the result is the call's own, an
    // error in it has no call frame
    auto assert_call_accepted(const toc& tc, const destination& dst) const
        -> void {

        assert_no_shared_storage(tc, dst);

        // note: the parse rejected a discarded result
        //       ('parse_name_statement') and a call without one as a value
        //       ('parse_operand')

        assert(callee().has_result() != dst.is_empty());

        if (callee().has_result()) {
            assert_result_type(dst);
        }

        assert_argument_types(tc);
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

    // the function is a method and the receiver is a value
    auto assert_method_receiver(const stmt_identifier& receiver) const -> void {

        // a constructor has no receiver, it builds its value
        if (callee().is_constructor()) {
            throw compile_error{tok(),
                                std::format("{} is a constructor, call it as "
                                            "'{}(...)'",
                                            in_quotes(callee().name()),
                                            callee().name())};
        }

        // only an element of an array can be the receiver, 'a[1].f()' but not
        // 'a.f()'
        if (receiver.is_array()) {
            throw compile_error{
                receiver.tok(),
                std::format("receiver of method {} cannot be an array",
                            in_quotes(callee().name()))};
        }
    }

    // the callee reaches the arguments, the result and the globals in place,
    // so a part that one of them writes may be read or written under
    // another name; the record of the body says which parts it reaches
    // ('body_uses')
    auto assert_no_shared_storage(const toc& tc, const destination& dst) const
        -> void {

        std::vector<reference> references;

        // none for a destination without storage, e.g. a register
        if (dst.storage()) {
            references.push_back(make_reference(
                tc, std::nullopt, callee_name_token(),
                std::format("the result destination {}", in_quotes(dst.text())),
                *dst.storage()));
        }

        for (size_t i{}; i < args_.size(); ++i) {
            const expr_any& arg{args_.at(i)};

            // a value or a constant is passed as a copy
            const std::optional<place> passed{as_place(tc, arg)};

            if (not passed) {
                continue;
            }

            const std::optional<storage_path> target{
                passed->ident->storage(tc),
            };

            // note: a variable has storage
            assert(target);

            reference arg_reference{
                make_reference(
                    tc, i, arg.tok(),
                    std::format("{} {}", describe_argument(i),
                                in_quotes(statement::trimmed_source(arg))),
                    *target),
            };

            assert_not_shared_with_earlier(references, arg_reference);
            references.push_back(std::move(arg_reference));
        }

        // note: always, a body compiled once for all calls, a noinline one,
        //       cannot see that a parameter names a global it also uses

        for (const body_uses::global_use& global : callee().uses().globals()) {
            assert_not_shared_with_global(global, references);
        }
    }

    // what the callee writes through the result or an argument may be a
    // global it uses, or what it writes to the global may be what it reaches
    // through them, e.g. 'g = add_g()' where 'add_g' sets 'res' and reads 'g'
    auto assert_not_shared_with_global(
        const body_uses::global_use& global,
        const std::span<const reference> references) const -> void {

        for (const reference& ref : references) {
            for (const call_effect& reached : ref.reached) {
                if (not(reached.is_write or global.is_written) or
                    not reached.storage.may_overlap(global.storage)) {

                    continue;
                }

                throw compile_error{
                    ref.tk,
                    std::format("{} may share storage with the global "
                                "{} that {} uses, {}",
                                ref.text, in_quotes(global.storage.root.name),
                                in_quotes(callee().name()),
                                use_temporary_fix_)};
            }
        }
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

    // the value arguments of a call that is not expanded, compiled at the type
    // of their parameter for their checks; a place, an array and an instance
    // are checked where they are parsed and by 'assert_call_accepted'
    auto check_arguments_alone(toc& tc, const size_t indent) const -> void {
        machine& x{tc.machine()};

        for (const auto [arg, param] :
             std::views::zip(args_, callee().params())) {

            const type& param_type{param.get_type()};

            if (not param.is_read_only() or param.is_array() or
                not param_type.is_builtin()) {

                continue;
            }

            const operand reg{
                x.alloc_scratch_register(tok(), indent, param_type),
            };

            arg.compile(tc, indent, destination::of_register(reg));

            x.free_scratch_register(tok(), indent, reg);
        }
    }

    // the call without its operators, inlined or noinline
    auto compile_call(toc& tc, const size_t indent, const destination& dst,
                      const call_site& call, const bool is_recursion) const
        -> void {

        if (not callee().is_inlined()) {
            noinline_call{tc, call}.compile(tc, indent, dst);
            return;
        }

        call_limits::assert_inline_nesting(tc, callee_name_token(), callee());

        // a recursion that does not end with constant arguments runs out of
        // registers before it reaches the nesting limit, the advice is the same
        try {
            inline_call{tc, call}.compile(tc, indent, dst);
        } catch (const compile_error& e) {
            if (not is_recursion or
                e.kind != compile_error::error_kind::out_of_registers) {

                throw;
            }

            throw call_limits::recursion_error(callee_name_token(), callee());
        }
    }

    // the type names the constructor after the '.', e.g. 'at' of
    // 'point.at(1, 2)', a method of the type is not one
    [[nodiscard]] auto constructor_func(const toc& tc) const
        -> const stmt_def_func& {

        // note: only a constructor call asks
        assert(constructor_);

        const token& name_tk{constructor_->name_tk};

        if (not name_tk.is_word()) {
            throw compile_error{name_tk,
                                "expected a constructor name after '.'"};
        }

        const std::string name{constructor_function_name(tc, tok(), name_tk)};

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
                            in_quotes(name), in_quotes(tok().text()))};
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
        -> reference {

        reference result{
            .tk{tk},
            .text{std::move(text)},
            .target{target},
            .reached{},
        };

        // note: the parts are compared, not named

        visit_reach_of(tc, param_index, {}, target,
                       [&](const call_effect& effect) -> void {
                           result.reached.push_back(effect);
                       });

        return result;
    }

    // a method receiver is already in 'args_', the parameter gives the type of
    // a literal argument unless its type was deduced from it, see
    // 'generic_call::deducing_args'
    auto parse_arguments(toc& tc, tokenizer& tz,
                         const std::span<const size_t> deducing_args) -> void {

        const std::span<const stmt_def_func_param> params{callee().params()};
        const size_t first{args_.size()};

        args_.reserve(params.size());

        for (size_t i{first}; i < params.size(); ++i) {
            // otherwise the missing argument is reported as a parse error of
            // the parameter's type
            // e.g. 'f()' of 'func f(x)'
            if (tz.peek_char_after_whitespace() == ')') {
                throw_missing_argument(tz, params.at(i), i);
            }

            // e.g. the ',' of 'f(a, b)'
            if (i != first) {
                const std::string message{
                    std::format("expected ',' before {} ({})",
                                describe_argument(i),
                                in_quotes(params.at(i).name())),
                };

                const token delimiter_tk{tz.expect_char_token(',', message)};

                tz.assert_no_trailing_delimiter(delimiter_tk, ')', "a call");

                arg_delims_tk_.emplace_back(delimiter_tk);
            }

            assert_not_string_argument(tz);
            assert_not_array_literal_argument(tc, tz, params.at(i));

            const bool is_deducing{
                std::ranges::contains(deducing_args,
                                      i - first_argument_index()),
            };

            // e.g. 'a' and 'b + 1' of 'f(a, b + 1)'
            args_.emplace_back(tc, tz, params.at(i).get_type(),
                               is_deducing ? literal_typing::written
                                           : literal_typing::from_destination);
        }

        tz.assert_no_surplus_argument(callee().name(),
                                      params.size() - first_argument_index());

        // e.g. 'f(a, b' lacks it
        close_paren_tk_ =
            tz.expect_char_token(')', "expected ')' after the arguments");

        // e.g. the '.' of 'mk().y' or 'mk().sum()', a result has no storage
        // to read a field of or to call a method on
        if (tz.peek_char_after_whitespace() == '.') {
            throw compile_error{
                tz, std::format("store the result of {} in a variable before "
                                "reading a field or calling a method of it",
                                in_quotes(callee().name()))};
        }

        for (size_t i{}; i < args_.size(); ++i) {
            assert_argument_usable(tc, args_.at(i), params.at(i));
            assert_argument_writable(tc, i);
        }

        record_uses_of_call(tc);
    }

    // a call is in a function; the type is the result type of the callee,
    // resolved before, so an unknown callee is reported before a missing '(';
    // that error is at the name, the next token can be on a later line, and
    // 'what' names the callee
    auto read_open_paren(const toc& tc, tokenizer& tz,
                         const std::string_view what) -> void {

        assert_call_in_func(tc, callee_name_token());
        assert_callee_body_parsed(tc);

        set_type(callee().result_type());

        open_paren_tk_ = tz.expect_char_token_after(
            callee_name_token(), '(',
            std::format("expected '(' after {}", what));
    }

    // what the callee reaches of the caller is the caller's use too, see
    // 'visit_callee_reach'
    auto record_uses_of_call(toc& tc) const -> void {

        // note: a call outside a function was rejected ('read_open_paren')
        assert(tc.scopes().is_in_func());

        visit_callee_reach(tc, [&](const call_effect& effect) -> void {
            tc.record_storage_use(effect.storage, effect.is_write
                                                      ? use_kind::write
                                                      : use_kind::read);
        });

        for (size_t i{}; i < args_.size(); ++i) {
            // a value or a constant is passed as a copy
            const std::optional<place> passed{as_place(tc, args_.at(i))};

            // note: a 'mut' argument needs storage, the result of an inlined
            //       call may be bound to a register

            if (passed and not callee().params().at(i).is_read_only() and
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
    [[nodiscard]] auto site() const -> call_site {
        return {
            .tk{tok()},
            .begin_tk{call_begin_token()},
            .close_paren_tk{close_paren_tk_},
            .source{statement::trimmed_source(*this)},
            .args{args_},
            .func{func_},
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

    [[noreturn]] auto throw_missing_argument(const tokenizer& tz,
                                             const stmt_def_func_param& param,
                                             const size_t index) const -> void {

        throw compile_error{tz, std::format("expected {} ({})",
                                            describe_argument(index),
                                            in_quotes(param.name()))};
    }

    // 'what' names the argument, 'hint' says how to make it writable
    [[noreturn]] auto throw_not_writable(const size_t index,
                                         const expr_any& arg,
                                         const std::string_view what,
                                         const std::string_view hint) const
        -> void {

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

    // the globals the callee uses and what its body reaches of an argument
    // through its parameter; a call of the function being parsed has no
    // complete record yet, so its argument is reached whole, written when the
    // parameter is 'mut'
    // note: only a function that calls itself has an incomplete record
    //       ('assert_callee_body_parsed')
    auto visit_callee_reach(const toc& tc,
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

        for (size_t i{}; i < args_.size(); ++i) {
            // a value or a constant is passed as a copy
            const std::optional<place> passed{as_place(tc, args_.at(i))};

            if (not passed) {
                continue;
            }

            const stmt_identifier& ident{*passed->ident};

            const std::optional<storage_path> target{ident.storage(tc)};

            // note: a variable has storage
            assert(target);

            visit_reach_of(tc, i, ident.dotted_path(), *target, visitor);
        }
    }

    // what the body reaches of 'target', the argument of the parameter at
    // 'param_index' or the result (empty), by its record; a call of the
    // function it is in, whose record the parse is still making, reaches it
    // whole, written unless the parameter is read-only
    auto visit_reach_of(const toc& tc, const std::optional<size_t> param_index,
                        const std::string_view name, const storage_path& target,
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
                .storage{
                    .root{target.root},
                    .path{target.path.extended_by(u.path)},
                },
                .is_write{u.is_written},
            });
        }
    }

    //
    // statics
    //

    // an array parameter takes a whole array, a scalar parameter does not
    static auto assert_argument_usable(const toc& tc, const expr_any& arg,
                                       const stmt_def_func_param& param)
        -> void {

        if (param.is_array()) {
            // an expression is not an array, nor is a name with a unary
            // operator, e.g. '-a'
            const std::optional<place> array{as_place(tc, arg)};

            const bool is_array_arg{array and array->info.is_array()};

            // an element would give the parameter the whole array's
            // length, pass the array and a start index instead
            if (not is_array_arg) {
                throw compile_error{
                    arg.tok(),
                    std::format("{} is not an array, parameter {} takes "
                                "one",
                                in_quotes(trimmed_source(arg)),
                                in_quotes(param.name()))};
            }

            return;
        }

        // the binding would make the parameter name the whole array
        if (const stmt_identifier* const ident{arg.operand_path()}) {
            assert_not_whole_array(*ident, make_ident_info(tc, *ident));
        }
    }

    // an array literal has no storage that an array parameter refers to, e.g.
    // '{1, 2}', '[2]{1, 2}' or 'i8[]{1, 2}'; '{}' is reported where it is used
    static auto
    assert_not_array_literal_argument(const toc& tc, const tokenizer& tz,
                                      const stmt_def_func_param& param)
        -> void {

        if (not param.is_array() or is_zero_value(tz)) {
            return;
        }

        tokenizer probe{tz};

        std::optional<token> literal_tk{probe.is_next_char_token('{')};

        // a '[' starts an array literal of the default type
        if (not literal_tk) {
            literal_tk = probe.is_next_char_token('[');
        }

        if (not literal_tk) {
            const token tk{probe.next_token()};

            if (classify_term(tc, tk, probe) != term_kind::array_literal) {
                return;
            }

            literal_tk = tk;
        }

        throw compile_error{
            *literal_tk,
            std::format("an array literal is not an argument, parameter {} "
                        "takes an array",
                        in_quotes(param.name()))};
    }

    // an argument that reaches a part of an earlier one or of the result that
    // one of the two writes
    static auto
    assert_not_shared_with_earlier(const std::span<const reference> earlier,
                                   const reference& ref) -> void {

        for (const reference& other : earlier) {
            if (not may_share(ref, other)) {
                continue;
            }

            throw compile_error{
                ref.tk,
                std::format("{} may share storage with {}{}, {}", ref.text,
                            other.text, shared_name_note(ref.target.root.name),
                            use_temporary_fix_)};
        }
    }

    // 'point.at' for the type named as written, e.g. an alias with constructors
    // of its own; else the name of the type that a type parameter is bound to,
    // e.g. 'T.at' where 'T' is 'point'
    [[nodiscard]] static auto
    constructor_function_name(const toc& tc, const token& type_tk,
                              const token& constructor_name_tk) -> std::string {

        std::string name{
            method_name_of(type_tk.text(), constructor_name_tk.text()),
        };

        if (tc.is_func(name) or not tc.types().has(type_tk.text())) {
            return name;
        }

        name = method_name_of(tc.get_type_or_throw(type_tk).name(),
                              constructor_name_tk.text());

        return name;
    }

    [[nodiscard]] static auto is_widened_integer(const type& result_type,
                                                 const type& dst_type) -> bool {

        return result_type.is_integer() and dst_type.is_integer() and
               result_type.size_bytes() < dst_type.size_bytes();
    }

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

    // e.g. " (both name 'g')", none for a variable the compiler added, whose
    // name the source does not have
    [[nodiscard]] static auto shared_name_note(const std::string_view name)
        -> std::string {

        if (is_temporary_name(name)) {
            return {};
        }

        return std::format(" (both name {})", in_quotes(name));
    }

    [[noreturn]] static auto
    throw_parameter_type_mismatch(const expr_any& arg,
                                  const stmt_def_func_param& param,
                                  const ident_info& info) -> void {

        throw compile_error{
            arg.tok(), std::format("expected type {} for parameter {}, got {}",
                                   in_quotes(param.get_type().name()),
                                   in_quotes(param.name()),
                                   in_quotes(info.type_ref().name()))};
    }

    // the type as written, with the type it is bound to when that differs,
    // e.g. 'T (point)'
    [[nodiscard]] static auto type_description(const toc& tc,
                                               const token& type_tk)
        -> std::string {

        std::string description{type_tk.text()};

        if (tc.types().has(type_tk.text())) {
            const std::string_view bound{
                tc.get_type_or_throw(type_tk).name(),
            };

            if (bound != type_tk.text()) {
                description += std::format(" ({})", bound);
            }
        }

        return description;
    }
};
