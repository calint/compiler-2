#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
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
#include <variant>
#include <vector>

#include "compile_error.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "expression.hpp"
#include "generics.hpp"
#include "ident_info.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "stmt_def_func.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

//
// a call, e.g. 'f(x)', 'lst.add(x)' or 'point.at(1, 2)', is the classes below,
// in reading order:
//
//   'stmt_call'       parses the call, prints its source and hands it to one
//                     of the compilers (last in the file, it uses the others)
//   'generic_call'    the instance of a generic function that the call names
//   'noinline_arg'    what a noinline call passes for an argument
//   'call_site'       what compiling the call needs of it
//   'inline_call'     compiles the body of the function in place
//   'noinline_call'   passes addresses to the body that is emitted once
//   'constructor_call_tag'
//                     selects the constructor of 'point.at(1, 2)'
//   'builtin_call_tag'
//                     selects the constructor of 'exit(0)'
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

        const token open_tk{tz.is_next_char_token('<')};

        std::vector<const type*> type_args;
        std::vector<token> generic_tks;
        std::vector<size_t> deducing_args;

        if (open_tk.is_empty()) {
            deduction deduced{
                deduced_type_arguments(tc, tz, name_tk, generic_name,
                                       expected_type),
            };

            type_args = std::move(deduced.type_args);
            deducing_args = std::move(deduced.argument_indexes);
        } else {
            const generic_arg_tokens args{
                generic_arg_tokens::parse(tz, open_tk),
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
        const name_meaning meaning{tc.scopes().lookup(tk.text())};

        // note: a constant does not tell its type, like a literal
        if (meaning.is_constant()) {
            return nullptr;
        }

        // e.g. 'show(nope)', an unknown name is reported as such, not as a
        // type that cannot be deduced, and 'show(nope(1))' as a function
        if (is_unknown_name(tc, tk)) {
            if (tz.peek_char_after_whitespace() == '(') {
                std::ignore = tc.get_func_return_type_or_throw(tk, tk.text());
            }

            std::ignore = tc.make_ident_info(tk, tk.text());
        }

        // e.g. 'point{1, 2}', a variable or a parameter hides the type
        if (not meaning.is_var_or_binding()) {
            return value_type(tc, tz, tk);
        }

        // note: read from the copy to find where the argument ends and its
        //       type, the call parses it again

        const stmt_identifier si{tc, {}, tk, tz};

        const char next{tz.peek_char_after_whitespace()};

        if (si.is_method_receiver() or (next != ',' and next != ')')) {
            return nullptr;
        }

        return &make_ident_info(tc, si).type_ref();
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

        throw compile_error{tz, std::format("expected argument {} ('{}')",
                                            index + 1, param_name)};
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

        if (scan.is_next_char_token('(').is_empty()) {
            throw compile_error{name_tk,
                                std::format("generic function '{}' needs type "
                                            "arguments, e.g. '{}<...>'",
                                            generic_name, name_tk.text())};
        }

        deduction deduced;

        for (const auto [param_tk, source] :
             std::views::zip(generic.param_tks, generic.deductions)) {

            const type* arg_type{};

            if (source.param_index) {
                assert_argument_present(scan, *source.param_index,
                                        source.param_name);

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
                    std::format("cannot deduce '{}' of generic function '{}', "
                                "write '{}<...>'",
                                param_tk.text(), generic_name, name_tk.text())};
            }

            deduced.type_args.push_back(arg_type);
        }

        return deduced;
    }

    // a name that names nothing: no value, type or function, no keyword or
    // built-in; a number or a character literal is not a name
    [[nodiscard]] static auto is_unknown_name(const toc& tc, const token& tk)
        -> bool {

        const std::string_view text{tk.text()};

        if (not tk.is_word()) {
            return false;
        }

        const char first{text.front()};

        if (std::isalpha(static_cast<unsigned char>(first)) == 0 and
            first != '_') {

            return false;
        }

        return not tc.scopes().lookup(text).is_value() and
               not tc.types().has(text) and not tc.is_func(text) and
               not std::ranges::contains(reserved_names::keywords, text) and
               not std::ranges::contains(reserved_names::builtin_functions,
                                         text);
    }

    // moves 'tz' from after the '(' to the argument at 'index', false when the
    // list ends before it
    [[nodiscard]] static auto skip_to_argument(tokenizer& tz,
                                               const size_t index) -> bool {

        for (size_t i{}; i < index; ++i) {
            tz.skip_argument();

            if (tz.is_next_char_token(',').is_empty()) {
                return false;
            }
        }

        return true;
    }

    // the type of an instance literal, a constructor call or a call of a
    // function that is the whole argument, 'tk' is its first token and 'tz' is
    // after it
    [[nodiscard]] static auto value_type(toc& tc, tokenizer& tz,
                                         const token& tk) -> const type* {

        const auto is_whole_argument = [&] -> bool {
            const char next{tz.peek_char_after_whitespace()};
            return next == ',' or next == ')';
        };

        const bool is_type{tc.types().has(tk.text())};

        // e.g. 'point{1, 2}'
        if (is_type and tz.peek_char_after_whitespace() == '{') {
            tz.skip_braced_block();
            return is_whole_argument() ? &tc.get_type_or_throw(tk) : nullptr;
        }

        // e.g. 'point.at(1, 2)'
        if (is_type and not tz.is_next_char_token('.').is_empty()) {
            // the type names the result, the constructor, e.g. 'at', does not
            std::ignore = tz.next_token();

            if (tz.is_next_char_token('(').is_empty()) {
                return nullptr;
            }

            tz.skip_to_close_paren();

            return is_whole_argument() ? &tc.get_type_or_throw(tk) : nullptr;
        }

        // e.g. 'read(0, buf)', the value of a built-in
        if (const type* const builtin_type{builtin_value_type(tc, tk, tz)};
            builtin_type != nullptr) {

            if (tz.is_next_char_token('(').is_empty()) {
                return nullptr;
            }

            tz.skip_to_close_paren();

            return is_whole_argument() ? builtin_type : nullptr;
        }

        // e.g. 'mk(1, 2)', a generic function has no return type before it is
        // instantiated
        if (not tc.is_func(tk.text()) or tc.generics().has_func(tk.text()) or
            tz.is_next_char_token('(').is_empty()) {

            return nullptr;
        }

        tz.skip_to_close_paren();

        const type& return_type{
            tc.get_func_return_type_or_throw(tk, tk.text()),
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
            tc.generics().get_func(generic_name).param_tks,
        };

        if (arg_tks.size() != param_tks.size()) {
            throw compile_error{
                name_tk, std::format("generic function '{}' takes {}, got {}",
                                     generic_name,
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

// what a noinline call passes for an argument: the variable it names or
// a temporary that holds its value
struct noinline_arg {
    ident_info info;
    // the variable and the bytes of it that the callee reaches
    std::optional<storage_path> target;
    bool is_temporary;
};

// what compiling a call needs of it, 'stmt_call' makes it for each compile
struct call_site {
    // the function name, the code and the errors of the call are located at it
    token tk;
    // the receiver of a method is written before the name
    token begin_tk;
    token close_paren_tk;
    // e.g. 'lst.add(x)', for the comment of the call and the frame of an error
    std::string source;
    // a method receiver is the first argument
    std::span<const expr_any> args;
    // e.g. the '~' of '~f(x)', applied to the result
    unary_ops uops;
    // the overflow check of 'uops', that of the destination of the call,
    // e.g. none in the argument of a conversion
    machine::runtime_check uops_check;

    // a frame is a scope being compiled: a block, a loop or a function body,
    // inlined or the dry run of a noinline call; each recurses in the compiler
    static constexpr size_t max_frame_count{256};

    // a level is the dry run of a noinline call nested in the dry run of the
    // call that reaches it, see 'noinline_call::dry_run_body'
    static constexpr size_t max_noinline_dry_run_level{100};

    // an inlined recursion ends at compile time when a constant argument
    // makes its condition fold, otherwise it would expand without end
    static constexpr size_t max_inlined_nesting{64};

    // the error was found inside the inlined body, unless it is in the call
    // itself where the call is already the error location
    auto add_call_frame(compile_error& e) const -> void {
        if (is_inside_call(e)) {
            return;
        }

        e.add_call_frame(begin_tk, source);
    }

    // a variable in the current block
    auto add_temporary(toc& tc, const size_t indent, const std::string& name,
                       const type& tpe) const -> ident_info {

        tc.add_var(tk, indent,
                   {
                       .name{name},
                       .type_ptr{&tpe},
                       .src_loc_tk{tk},
                       .is_temporary{true},
                       .array_len{},
                       .placement{},
                       .foo_array{},
                   },
                   var_kind::var);

        return tc.make_ident_info(tk, name);
    }

    // the result has the type of the destination, an operand widens a
    // narrower integer, see 'destination::as_operand'
    auto assert_result_type(const destination& dst,
                            const stmt_def_func& func) const -> void {

        if (not result_type_differs(dst, func)) {
            return;
        }

        if (dst.is_operand() and
            is_widened_integer(func.get_type(), dst.type_ref())) {

            return;
        }

        throw compile_error{
            dst.error_token(tk),
            std::format("expected type '{}', got '{}' from '{}', use '{}(...)'",
                        dst.type_ref().name(), func.get_type().name(),
                        func.name(), dst.type_ref().name()),
        };
    }

    // a result must be stored and a call without one cannot provide it
    auto assert_result_use(const destination& dst,
                           const stmt_def_func& func) const -> void {

        if (func.returns() and dst.is_empty()) {
            throw compile_error{tk, "return value is discarded"};
        }

        if (not func.returns() and not dst.is_empty()) {
            throw compile_error{tk, "function does not return a value"};
        }
    }

    // unique per call and argument, an inlined body names it through the
    // binding of its parameter and a call in the body has temporaries of its
    // own; each level of an inlined recursion declares it again, the binding
    // resolves it in the caller's scope
    [[nodiscard]] auto temporary_argument_name(const size_t index) const
        -> std::string {

        return toc::temporary_name(tk, "call-arg", index);
    }

    //
    // statics
    //

    // the root variable and the bytes of it that the argument names, any
    // element of the array when it is the element of a 'foo'
    [[nodiscard]] static auto argument_target(const toc& tc,
                                              const expr_any& arg)
        -> std::optional<storage_path> {

        return tc.storage_of(ident_info::root_of(arg.name_or_literal()),
                             arg.accessed_path());
    }

    // computed from the argument, e.g. 'x + 1' or '-x', so it has no storage
    // the callee could reach
    [[nodiscard]] static auto is_computed_value(const expr_any& arg) -> bool {
        return arg.is_expression() or not arg.get_unary_ops().is_empty();
    }

    // the callee gets a copy, not the storage of a variable: an expression,
    // an instance literal or call, or a name with unary operators
    [[nodiscard]] static auto is_copied_value(const expr_any& arg) -> bool {
        return not arg.is_name_or_literal() or is_computed_value(arg);
    }

    // an instance literal or a call result, e.g. 'point{1, 2}' or 'mk(1, 2)'
    [[nodiscard]] static auto
    is_instance_temporary(const expr_any& arg, const stmt_def_func_param& param)
        -> bool {

        return not param.get_type().is_builtin() and
               not arg.is_name_or_literal();
    }

    [[nodiscard]] static auto is_widened_integer(const type& result_type,
                                                 const type& dst_type) -> bool {

        return result_type.is_builtin() and not result_type.is_bool() and
               dst_type.is_builtin() and not dst_type.is_bool() and
               result_type.size_bytes() < dst_type.size_bytes();
    }

    // 'dst' has another type than the result, after 'assert_result_type' a
    // widening of an operand
    [[nodiscard]] static auto result_type_differs(const destination& dst,
                                                  const stmt_def_func& func)
        -> bool {

        return not dst.type_ref().is_same(func.get_type());
    }

    // a temporary of the call is a root variable of its own
    [[nodiscard]] static auto temporary_storage(const toc& tc,
                                                const std::string_view name)
        -> std::optional<storage_path> {

        return tc.storage_of(name, access_path::whole_variable());
    }

    // the frames ran out in the dry run of a noinline call, see
    // 'noinline_call::dry_run_body'
    [[noreturn]] static auto throw_dry_run_frame_limit(const token& src_loc_tk)
        -> void {

        throw compile_error{
            src_loc_tk,
            std::format("inlined calls and blocks are nested too deep to check "
                        "the arguments of a noinline call, the limit is {} "
                        "scopes",
                        max_frame_count),
        };
    }

    [[noreturn]] static auto throw_noinline_nesting(const token& src_loc_tk)
        -> void {

        throw compile_error{
            src_loc_tk,
            std::format("noinline calls are nested too deep to check their "
                        "arguments, the limit is {} levels",
                        max_noinline_dry_run_level),
        };
    }

    [[noreturn]] static auto
    throw_parameter_type_mismatch(const expr_any& arg,
                                  const stmt_def_func_param& param,
                                  const ident_info& info) -> void {

        throw compile_error{
            arg.tok(),
            std::format("expected type '{}' for parameter '{}', got '{}'",
                        param.get_type().name(), param.name(),
                        info.type_ref().name())};
    }

  private:
    [[nodiscard]] auto is_inside_call(const compile_error& e) const -> bool {

        // note: a recursive call is checked inside its own body, so an error
        //       can be located after the call

        return e.file_ix == begin_tk.file_index() and
               e.start_index >= begin_tk.start_index() and
               e.start_index <= close_paren_tk.end_index();
    }
};

// compiles a call of an inlined function: the body is compiled in place, its
// parameters and its result are bound to the arguments and the destination
//
// the steps:
//
//   1. 'compile': an instance literal or a call result argument is made in a
//      temporary
//   2. 'compile_to_destination': a memory destination may get its address in
//      a register ('compile_with_result_address_register'), or unary
//      operators may work on a scratch register
//   3. 'compile_with_bindings': the bindings of the result and the arguments
//      ('make_bindings'), an indexed argument may get its address in a
//      register ('compile_with_address_choice',
//      'compile_body_with_address_registers')
//   4. 'compile_body': the body under labels unique to the call, with the
//      bindings added
//
// a 'noinline_call' runs step 3 as a dry run for the errors of its call
class inline_call {
    call_site call_;

  public:
    explicit inline_call(call_site call) : call_{std::move(call)} {}

    // an instance literal or a call result is made in a variable of the
    // parameter's type that the body reaches through the binding of its
    // parameter
    auto compile(toc& tc, const size_t indent, const destination& dst,
                 const stmt_def_func& func) const -> void {

        if (func.returns() and call_site::result_type_differs(dst, func)) {
            // note: 'call_site::assert_result_type' allows a type mismatch
            //       only for an operand

            assert(dst.is_operand());

            compile_widened(tc, indent, dst, func);

            return;
        }

        const bool has_instance_temporary{
            std::ranges::any_of(std::views::zip(call_.args, func.params()),
                                [](const auto& arg_and_param) -> bool {
                                    return call_site::is_instance_temporary(
                                        std::get<0>(arg_and_param),
                                        std::get<1>(arg_and_param));
                                }),
        };

        // note: a block is a frame, which counts toward the nesting limit and
        //       the 'max frames in use' report, so a call without temporaries
        //       enters none

        if (not has_instance_temporary) {
            compile_to_destination(tc, indent, dst, func);
            return;
        }

        // the block frees the temporaries after the call
        tc.enter_block();

        for (const auto [i, arg, param] : std::views::zip(
                 std::views::iota(size_t{}), call_.args, func.params())) {

            if (not call_site::is_instance_temporary(arg, param)) {
                continue;
            }

            const ident_info temporary_info{
                call_.add_temporary(tc, indent,
                                    call_.temporary_argument_name(i),
                                    param.get_type()),
            };

            arg.compile(tc, indent,
                        destination::of_name(
                            temporary_info, call_site::temporary_storage(
                                                tc, temporary_info.root_id())));
        }

        compile_to_destination(tc, indent, dst, func);

        tc.exit_block();
    }

    // 'call_arguments' are those of a 'noinline' call that is only checked,
    // the body sees an argument that went through a temporary as that
    // temporary
    auto compile_with_bindings(
        toc& tc, const size_t indent, const destination& dst,
        const stmt_def_func& func,
        const std::span<const noinline_arg> call_arguments = {}) const -> void {

        // scratch registers stay allocated until the inlined body is compiled
        std::vector<operand> allocated_registers;

        const std::vector<param_binding> bindings{
            make_bindings(tc, indent, dst, func, call_arguments,
                          allocated_registers),
        };

        // the result binding has its own address choice in
        // 'compile_to_destination'
        // note: the result binding is made before those of the arguments,
        //       which are the last 'call_.args.size()'
        const size_t first_argument_binding{
            bindings.size() - call_.args.size(),
        };

        const bool has_indexed_argument{
            std::ranges::any_of(bindings |
                                    std::views::drop(first_argument_binding),
                                is_indexed_reference),
        };

        const machine& x{tc.machine()};

        // an access can use the indexed address as it is, so computing it
        // into a register is not tried
        if (not has_indexed_argument or x.has_indexed_addressing()) {
            compile_body(tc, indent, func, bindings, allocated_registers);
            return;
        }

        compile_with_address_choice(tc, indent, func, bindings,
                                    first_argument_binding,
                                    allocated_registers);
    }

  private:
    auto apply_unary_ops_to_result(toc& tc, const size_t indent,
                                   const stmt_def_func& func) const -> void {

        if (call_.uops.is_empty()) {
            return;
        }

        assert(func.returns());

        const func_return_info& return_info{*func.returns()};

        const ident_info ret_info{
            tc.make_ident_info(call_.tk, return_info.ident_tk.text()),
        };

        machine& x{tc.machine()};

        call_.uops.compile(x, indent, call_.tk, ret_info.operand(),
                           call_.uops_check);
    }

    // the id has no indexes, so an id naming an array means the result is
    // one of its elements
    auto comment_result_address(toc& tc, const size_t indent,
                                const destination& dst) const -> void {

        machine& x{tc.machine()};

        if (tc.make_ident_info(call_.tk, dst.name()).is_array()) {
            x.comment(call_.tk, indent,
                      "address of result element in array '{}'", dst.name());

            return;
        }

        x.comment(call_.tk, indent, "address of indexed result '{}'",
                  dst.name());
    }

    // the registers are freed before the exit label
    auto compile_body(toc& tc, const size_t indent, const stmt_def_func& func,
                      const std::span<const param_binding> bindings_to_add,
                      const std::span<const operand> registers_to_free) const
        -> void {

        machine& x{tc.machine()};

        // the body is inlined at every call, so its labels are named by the
        // call
        const std::string_view call_path{tc.scopes().call_path()};

        const std::string src_loc{
            tc.source_location_for_use_in_label(call_.tk),
        };

        const std::string new_call_path{
            call_path.empty() ? src_loc
                              : std::format("{}.{}", src_loc, call_path),
        };

        const std::string call_label{
            std::format("{}.{}", func.body_label(), new_call_path),
        };

        const std::string ret_jmp_label{toc::end_label(call_label)};

        func.source_def_comment_to(x, indent);

        x.label(indent, call_label);

        // note: a noinline function is compiled in place only in its dry run
        if (func.is_inlined()) {
            tc.enter_inlined_func(func.name(), new_call_path, ret_jmp_label);
        } else {
            tc.enter_noinline_dry_run(func.name(), new_call_path,
                                      ret_jmp_label);
        }

        const machine::call_frame_scope call_frame{
            x,
            call_.tk,
            std::string{func.name()},
            machine::frame_kind::inlined,
        };

        // an error found in the inlined body reports where it was called from,
        // an error in the arguments is in the caller and has no call frame
        try {
            func.add_constants(tc, indent + 1);

            // the body names the arguments and the result by the parameters
            for (const param_binding& e : bindings_to_add) {
                const bound_storage* const storage{
                    std::get_if<bound_storage>(&e.target),
                };

                x.comment_binding(call_.tk, indent + 1, e.from, e.target_text(),
                                  storage == nullptr ? operand{}
                                                     : storage->lea);

                tc.add_binding(e);
            }

            // the result reaches the destination through the binding of 'res',
            // so the statements of the body have no destination
            func.compile_statements(tc, indent);
        } catch (compile_error& e) {
            call_.add_call_frame(e);
            throw;
        }

        x.free_scratch_registers(call_.tk, indent + 1, registers_to_free);

        // provide the exit label for 'return' to jump to
        x.label(indent, ret_jmp_label);

        apply_unary_ops_to_result(tc, indent, func);

        tc.exit_func(func.name());
    }

    // a new register per argument because the index register may belong to
    // an enclosing binding that is used after the call
    auto compile_body_with_address_registers(
        toc& tc, const size_t indent, const stmt_def_func& func,
        std::vector<param_binding> bindings_to_add,
        const size_t first_argument_binding,
        const bool keeps_displacement) const -> void {

        machine& x{tc.machine()};

        std::vector<operand> address_registers;

        for (param_binding& binding :
             bindings_to_add | std::views::drop(first_argument_binding)) {

            if (not is_indexed_reference(binding)) {
                continue;
            }

            bound_storage& storage{std::get<bound_storage>(binding.target)};

            const operand address{
                x.alloc_scratch_register(call_.tk, indent,
                                         tc.builtins().address()),
            };

            address_registers.push_back(address);

            x.comment(call_.tk, indent, "address of parameter '{}'",
                      binding.from);

            storage.lea = load_indexed_address(x, indent, address, storage.lea,
                                               keeps_displacement);
        }

        compile_body(tc, indent, func, bindings_to_add, address_registers);
    }

    auto compile_to_destination(toc& tc, const size_t indent,
                                const destination& dst,
                                const stmt_def_func& func) const -> void {

        if (not dst.operand().is_memory()) {
            compile_with_bindings(tc, indent, dst, func);
            return;
        }

        machine& x{tc.machine()};

        const bool has_unary_ops{not call_.uops.is_empty()};

        const bool has_indexed_result{
            not dst.operand().index_register().empty(),
        };

        // without base + index addressing each result access adds base and
        // index again, computing the address once can be shorter
        if (not has_unary_ops and has_indexed_result and
            not x.has_indexed_addressing()) {

            tc.emit_most_efficient(
                call_.tk, indent,
                [&] -> void { compile_with_bindings(tc, indent, dst, func); },
                [&] -> void {
                    emit_most_efficient_address(
                        tc, indent, [&](const bool keeps) -> void {
                            compile_with_result_address_register(
                                tc, indent, dst, func, keeps);
                        });
                });

            return;
        }

        if (not has_unary_ops) {
            compile_with_bindings(tc, indent, dst, func);
            return;
        }

        // unary ops on a memory result are a load, modify and store on a
        // load/store machine, a scratch register can be shorter
        tc.emit_most_efficient(
            call_.tk, indent,
            [&] -> void { compile_with_bindings(tc, indent, dst, func); },
            [&] -> void {
                const operand reg{
                    x.alloc_scratch_register(call_.tk, indent, dst.type_ref()),
                };

                compile_with_bindings(tc, indent, destination::of_register(reg),
                                      func);

                x.copy_value(call_.tk, indent, dst.operand(), reg);
                x.free_scratch_register(call_.tk, indent, reg);
            });
    }

    // the result is made at its type in a register that the body names, the
    // copy into the operand widens it and the unary ops of the call apply at
    // the width of the operand, as on a variable
    auto compile_widened(toc& tc, const size_t indent, const destination& dst,
                         const stmt_def_func& func) const -> void {

        machine& x{tc.machine()};

        const operand result{
            x.alloc_scratch_register(call_.tk, indent, func.get_type()),
        };

        call_site narrow_call{call_};
        narrow_call.uops = {};

        inline_call{narrow_call}.compile(
            tc, indent, destination::of_register(result), func);

        x.copy_value(call_.tk, indent, dst.operand(), result);
        x.free_scratch_register(call_.tk, indent, result);

        call_.uops.compile(x, indent, call_.tk, dst.operand(),
                           call_.uops_check);
    }

    // note: the body is compiled in three versions and the one with the least
    //       code is kept
    //
    //       1. bindings use the indexed addresses as they are, e.g.
    //          '[s0 + t0 * 4 + 28]'
    //       2. each indexed address goes once into a new register, the
    //          displacement included, e.g. '[t1]'
    //       3. as 2 but the displacement stays in each access, e.g.
    //          '[t1 + 28]'
    //
    //       rv32i has no base + index addressing, so in version 1 every
    //       access adds base and index again; a register pays off only when
    //       the body accesses the argument more than once, and a kept
    //       displacement saves an 'addi' unless the field offsets then exceed
    //       the 12-bit immediate range; which is shorter is only known after
    //       compiling the body
    //
    //       the argument registers are shared by all versions, so they are
    //       freed after the choice instead of inside the body

    auto compile_with_address_choice(
        toc& tc, const size_t indent, const stmt_def_func& func,
        const std::vector<param_binding>& bindings,
        const size_t first_argument_binding,
        const std::span<const operand> allocated_registers) const -> void {

        machine& x{tc.machine()};

        tc.emit_most_efficient(
            call_.tk, indent,
            [&] -> void { compile_body(tc, indent, func, bindings, {}); },
            [&] -> void {
                emit_most_efficient_address(
                    tc, indent, [&](const bool keeps) -> void {
                        compile_body_with_address_registers(
                            tc, indent, func, bindings, first_argument_binding,
                            keeps);
                    });
            });

        x.free_scratch_registers(call_.tk, indent + 1, allocated_registers);
    }

    // a new register because the index register may belong to an enclosing
    // binding that is used after the call
    auto compile_with_result_address_register(
        toc& tc, const size_t indent, const destination& dst,
        const stmt_def_func& func, const bool keeps_displacement) const
        -> void {

        machine& x{tc.machine()};

        const operand address{
            x.alloc_scratch_register(call_.tk, indent, tc.builtins().address()),
        };

        comment_result_address(tc, indent, dst);

        const destination address_info{
            dst.with_operand(load_indexed_address(
                x, indent, address, dst.operand(), keeps_displacement)),
        };

        compile_with_bindings(tc, indent, address_info, func);

        x.free_scratch_register(call_.tk, indent, address);
    }

    // the address register holds the address with or without the
    // displacement, the first is shorter when the accesses exceed the load
    // and store immediate range
    auto emit_most_efficient_address(
        toc& tc, const size_t indent,
        const std::function_ref<void(bool keeps_displacement)> emit) const
        -> void {

        tc.emit_most_efficient(
            call_.tk, indent, [&] -> void { emit(false); },
            [&] -> void { emit(true); });
    }

    // a displacement kept in the operand saves adding it to the address, e.g.
    // 'add t1, s0, t0' and '28(t1)' without 'addi t1, t1, 28', but offsets
    // beyond the load and store immediate range then cost more per access
    [[nodiscard]] auto load_indexed_address(machine& x, const size_t indent,
                                            const operand& dst,
                                            const operand& location,
                                            const bool keeps_displacement) const
        -> operand {

        if (not keeps_displacement) {
            x.address_of(call_.tk, indent, dst, location);
            return operand::mem(dst, location.type_ref());
        }

        x.address_of(call_.tk, indent, dst,
                     operand::mem(location.base_register(),
                                  location.index_register(), location.scale(),
                                  0, location.type_ref()));

        return operand::mem(dst.base_register(), {}, 1, location.displacement(),
                            location.type_ref());
    }

    // the bindings of the result and of the arguments of the inlined body
    [[nodiscard]] auto
    make_bindings(toc& tc, const size_t indent, const destination& dst,
                  const stmt_def_func& func,
                  const std::span<const noinline_arg> call_arguments,
                  std::vector<operand>& allocated_registers) const
        -> std::vector<param_binding> {

        assert(call_arguments.empty() or
               call_arguments.size() == call_.args.size());

        // note: the call checked the use and the type of the result before
        //       the body was entered

        assert(func.returns().has_value() != dst.is_empty());

        std::vector<param_binding> bindings;

        // the result names the destination, so the widths must agree
        if (const std::optional<func_return_info> ret{func.returns()}; ret) {
            assert(dst.type_ref().is_same(func.get_type()));

            bindings.push_back(bind_result(dst, *ret));
        }

        for (const auto [i, arg, param] : std::views::zip(
                 std::views::iota(size_t{}), call_.args, func.params())) {

            // 'compile' of a 'noinline' call made the temporary, the dry run
            // must not name the register of the caller that the body does not
            // see
            const bool is_call_temporary{
                not call_arguments.empty() and
                    call_arguments.at(i).is_temporary,
            };

            if (is_call_temporary or
                call_site::is_instance_temporary(arg, param)) {

                const std::string temporary{call_.temporary_argument_name(i)};

                bindings.push_back(
                    bind_storage(param, temporary,
                                 call_site::temporary_storage(tc, temporary)));

                continue;
            }

            bindings.push_back(
                bind_argument(tc, indent, arg, param, allocated_registers));
        }

        return bindings;
    }

    //
    // statics
    //

    static auto assert_binding_type(const toc& tc, const expr_any& arg,
                                    const stmt_def_func_param& param) -> void {

        const ident_info info{make_ident_info(tc, arg)};

        // an untyped constant, e.g. a literal, is checked by its value; a
        // typed one has the rules of a variable of its type
        if (info.is_const() and not info.constant().is_typed()) {
            return;
        }

        if (info.type_ref().is_same(param.get_type())) {
            return;
        }

        call_site::throw_parameter_type_mismatch(arg, param, info);
    }

    // the inlined body reaches an argument through its storage, its constant
    // value or a scratch register holding its value
    [[nodiscard]] static auto
    bind_argument(toc& tc, const size_t indent, const expr_any& arg,
                  const stmt_def_func_param& param,
                  std::vector<operand>& allocated_registers) -> param_binding {

        const bool is_reference{
            not arg.is_expression() and arg.is_name_or_literal() and
                arg.has_computed_address(
                    tc.make_ident_info(arg.tok(), arg.name_or_literal())),
        };

        // a binding uses the argument's storage or typed value, so its type
        // must match
        if (not call_site::is_computed_value(arg)) {
            assert_binding_type(tc, arg, param);
        }

        if (is_reference and arg.get_unary_ops().is_empty()) {
            return bind_reference(tc, indent, arg, param, allocated_registers);
        }

        // e.g. '-a[i]', a value computed like an expression that must fit
        // the parameter like a name with unary operators
        if (is_reference) {
            arg.assert_not_narrowed(tc, param.get_type());
            return bind_expression(tc, indent, arg, param, allocated_registers);
        }

        if (arg.is_expression()) {
            return bind_expression(tc, indent, arg, param, allocated_registers);
        }

        return bind_plain_argument(tc, indent, arg, param, allocated_registers);
    }

    // a constant lets the inlined body be decided at compile time
    [[nodiscard]] static auto
    bind_expression(toc& tc, const size_t indent, const expr_any& arg,
                    const stmt_def_func_param& param,
                    std::vector<operand>& allocated_registers)
        -> param_binding {

        const std::optional<folded_value> folded{
            arg.fold(tc, tc.use_at(param.get_type())),
        };

        if (folded) {
            return param_binding::make_constant(
                param.name_or_literal(), param.get_type(), folded->value);
        }

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register(arg.tok(), indent, param.get_type()),
        };

        allocated_registers.push_back(reg);
        arg.compile(tc, indent, destination::of_register(reg));

        return param_binding::make_register(param.name_or_literal(),
                                            param.get_type(), reg);
    }

    // an identifier or a constant, with or without unary operators
    [[nodiscard]] static auto
    bind_plain_argument(toc& tc, const size_t indent, const expr_any& arg,
                        const stmt_def_func_param& param,
                        std::vector<operand>& allocated_registers)
        -> param_binding {

        // constants and unary ops pass a value, not storage, so the value
        // must fit the parameter
        arg.assert_not_narrowed(tc, param.get_type());

        const ident_info arg_info{make_ident_info(tc, arg)};

        // a name is resolved in the callee where its own constants would
        // shadow the caller's, and the bound constant must be a plain integer,
        // e.g. '~1' is not
        if (arg_info.is_const()) {
            const std::optional<folded_value> folded{
                arg.fold(tc, tc.use_at(param.get_type())),
            };

            // note: a constant name or literal folds
            assert(folded);

            return param_binding::make_constant(
                param.name_or_literal(), param.get_type(), folded->value);
        }

        if (arg.get_unary_ops().is_empty()) {
            return bind_storage(param, arg.name_or_literal(),
                                call_site::argument_target(tc, arg));
        }

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register(arg.tok(), indent, param.get_type()),
        };

        allocated_registers.push_back(reg);
        x.copy_value(param.tok(), indent, reg, arg_info.operand());

        arg.get_unary_ops().compile(x, indent, arg.tok(), reg,
                                    tc.checks().overflow_options());

        return param_binding::make_register(param.name_or_literal(),
                                            param.get_type(), reg);
    }

    // an indexed argument keeps its computed address, e.g.
    // '[rbp + r14 * 4 + 205]', '[r15 + r14]' or simply '[r15]'
    [[nodiscard]] static auto
    bind_reference(toc& tc, const size_t indent, const expr_any& arg,
                   const stmt_def_func_param& param,
                   std::vector<operand>& allocated_registers) -> param_binding {

        const ident_info arg_info{make_ident_info(tc, arg)};

        std::vector<operand> regs_lea;

        const operand lea{
            arg.compile_lea(tc, indent, arg.tok(), regs_lea,
                            {
                                .reg_count{},
                                .known{arg_info.known()},
                                .address_register{},
                            }),
        };

        // the address registers stay allocated until the inlined body is
        // compiled
        allocated_registers.append_range(regs_lea);

        return param_binding::make_storage(
            param.name_or_literal(), param.get_type(),
            {
                .path{std::string{arg.name_or_literal()}},
                .lea{lea},
                .is_element{arg.is_array_element()},
                .bound{call_site::argument_target(tc, arg)},
            });
    }

    // the result name of the function refers to the destination
    [[nodiscard]] static auto bind_result(const destination& dst,
                                          const func_return_info& ret)
        -> param_binding {

        // e.g. the scratch register of an expression, or a parameter that
        // stands for one
        if (dst.is_register()) {
            // note: a register has no address to keep
            assert(not dst.is_address_known());

            return param_binding::make_register(
                ret.ident_tk.text(), *ret.type_ptr, dst.operand(), dst.name());
        }

        operand dst_lea{
            dst.is_address_known() ? dst.operand() : operand{},
        };

        // a destination such as 'arr[1]' is not an array
        return param_binding::make_storage(ret.ident_tk.text(), *ret.type_ptr,
                                           {
                                               .path{dst.name()},
                                               .lea{std::move(dst_lea)},
                                               .is_element{not dst.is_array()},
                                               .bound{dst.storage()},
                                           });
    }

    // a variable of the caller or a temporary of the call, named by 'path'
    [[nodiscard]] static auto
    bind_storage(const stmt_def_func_param& param, const std::string_view path,
                 const std::optional<storage_path>& bound) -> param_binding {

        return param_binding::make_storage(param.name_or_literal(),
                                           param.get_type(),
                                           {
                                               .path{std::string{path}},
                                               .lea{},
                                               .is_element{},
                                               .bound{bound},
                                           });
    }

    // e.g. [s0 + t0 * 4 + 28] but not [t1 + 28]
    [[nodiscard]] static auto is_indexed_reference(const param_binding& binding)
        -> bool {

        const bound_storage* const storage{
            std::get_if<bound_storage>(&binding.target),
        };

        return storage != nullptr and storage->lea.is_memory() and
               not storage->lea.index_register().empty();
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
//      is copied and the unary ops applied after the call
//   2. 'compile_with_arguments': the types are checked ('assert_call'), the
//      errors that depend on this call, e.g. shared storage, are found by a
//      dry run of the body as an inlined call ('dry_run_body', see
//      'etc/docs/README.md' chapter 4), the addresses are written to
//      the callee frame ('frame_slot_addresses', 'write_frame_slots') and the
//      function is called
class noinline_call {
    // what the call kind reads of the result or of an argument
    struct call_kind_item {
        // the variable and the bytes of it, e.g. 'v' of the frame 2 and the
        // field 'x' of 'v.x'
        std::optional<storage_path> target;
        bool is_pointer{};
        bool is_array{};
        std::optional<size_t> array_len;
    };

    call_site call_;

  public:
    explicit noinline_call(call_site call) : call_{std::move(call)} {}

    // the callee reaches the result and the arguments through their addresses,
    // a result destination or an argument that is not a variable in memory,
    // e.g. a register or an expression, goes through a temporary, as does a
    // result that widens into an operand
    auto compile(toc& tc, const size_t indent, const destination& dst,
                 const stmt_def_func& func) const -> void {

        const bool has_argument_temporary{
            std::ranges::any_of(std::views::zip(call_.args, func.params()),
                                [&](const auto& arg_and_param) -> bool {
                                    return needs_temporary(
                                        tc, std::get<0>(arg_and_param),
                                        std::get<1>(arg_and_param));
                                }),
        };

        // note: a block is a frame, which counts toward the nesting limit and
        //       the 'max frames in use' report, so a call without temporaries
        //       enters none

        if (not has_result_temporary(dst, func) and
            not has_argument_temporary) {

            compile_arguments_and_call(tc, indent, dst, func);
            return;
        }

        // the block frees the temporaries after the call
        tc.enter_block();

        compile_arguments_and_call(tc, indent, dst, func);

        tc.exit_block();
    }

  private:
    // the callee has written the result, the operators of the call follow
    auto apply_unary_ops_to_destination(toc& tc, const size_t indent,
                                        const destination& dst) const -> void {

        machine& x{tc.machine()};

        call_.uops.compile(x, indent, call_.tk, dst.operand(),
                           call_.uops_check);
    }

    // the callee reaches the result and arguments through their addresses
    auto assert_call(const destination& dst, const stmt_def_func& func,
                     const std::span<const noinline_arg> arguments) const
        -> void {

        // note: 'stmt_call::compile' checked the result, a result that is
        //       not in memory or is widened goes through a temporary

        if (func.returns()) {
            assert(dst.operand().is_memory());
            assert(dst.type_ref().is_same(func.get_type()));

            // an array literal gives its call elements an element destination
            assert(not dst.is_array());
        }

        for (const auto [arg, argument, param] :
             std::views::zip(call_.args, arguments, func.params())) {

            assert_argument(arg, argument.info, param);
        }
    }

    // the result address comes first, then one address per argument
    auto comment_frame_slot(machine& x, const size_t indent,
                            const stmt_def_func& func,
                            const size_t slot_index) const -> void {

        if (func.returns() and slot_index == 0) {
            x.comment(call_.tk, indent, "result address in callee frame");
            return;
        }

        const size_t arg_idx{slot_index - func.first_param_slot()};

        x.comment(call_.tk, indent,
                  "address of argument '{}' to parameter '{}'",
                  statement::trimmed_source(call_.args.at(arg_idx)),
                  func.params().at(arg_idx).name());
    }

    // an argument or a result that needs one goes through a temporary of the
    // current block
    auto compile_arguments_and_call(toc& tc, const size_t indent,
                                    const destination& dst,
                                    const stmt_def_func& func) const -> void {

        std::vector<noinline_arg> arguments;

        for (const auto [i, arg, param] : std::views::zip(
                 std::views::iota(size_t{}), call_.args, func.params())) {

            if (not needs_temporary(tc, arg, param)) {
                arguments.push_back({
                    .info{make_ident_info(tc, arg)},
                    .target{call_site::argument_target(tc, arg)},
                    .is_temporary{},
                });

                continue;
            }

            const ident_info temporary{
                call_.add_temporary(tc, indent,
                                    call_.temporary_argument_name(i),
                                    param.get_type()),
            };

            arguments.push_back({
                .info{temporary},
                .target{
                    call_site::temporary_storage(tc, temporary.root_id()),
                },
                .is_temporary{true},
            });

            arg.compile(tc, indent,
                        destination::of_name(arguments.back().info,
                                             arguments.back().target));
        }

        if (has_result_temporary(dst, func)) {
            // the temporary has the type of the result, the copy into the
            // destination widens it
            const ident_info result_info{
                call_.add_temporary(
                    tc, indent, toc::temporary_name(call_.tk, "call-result", 0),
                    func.get_type()),
            };

            compile_with_arguments(
                tc, indent,
                destination::of_name(
                    result_info,
                    call_site::temporary_storage(tc, result_info.root_id())),
                func, arguments);

            machine& x{tc.machine()};

            x.copy_value(call_.tk, indent, dst.operand(),
                         result_info.operand());
        } else {
            compile_with_arguments(tc, indent, dst, func, arguments);
        }

        apply_unary_ops_to_destination(tc, indent, dst);
    }

    auto
    compile_with_arguments(toc& tc, const size_t indent, const destination& dst,
                           const stmt_def_func& func,
                           const std::span<const noinline_arg> arguments) const
        -> void {

        assert_call(dst, func, arguments);
        dry_run_body(tc, indent, dst, func, arguments);

        machine& x{tc.machine()};

        const std::vector<size_t> array_lengths{
            array_argument_lengths(func, arguments),
        };

        // a body is emitted for a function that is called, once for all calls
        // without array parameters
        tc.add_noinline_body(func, array_lengths);

        // the registers stay allocated until the callee frame is populated
        std::vector<operand> address_registers;

        const std::vector<operand> addresses{
            frame_slot_addresses(tc, indent, dst, func, arguments,
                                 address_registers),
        };

        // start the callee frame after the caller's storage, not on the stack
        // pointer
        // example: caller uses 24 bytes; callee returns a value and takes one
        // argument
        //
        // base      +------------------------+
        //           | caller's storage       | 24 bytes
        // base + 24 +------------------------+ <- frame_address; callee's base
        //           | result address         | 1 address
        //           +------------------------+
        //           | argument address       | 1 address
        //           +------------------------+
        //           | callee's locals        |
        //           +------------------------+
        //
        // the base is the frame base register, x86_64 'rbx' and rv32i 's1';
        // root calls use the variables base register instead, x86_64 'rbp' and
        // rv32i 's0'
        const operand frame_address{tc.next_frame_address()};

        if (tc.checks().frame) {
            x.check_frame_capacity(
                call_.tk, indent, frame_address,
                operand::symbol(func.frame_size_label(array_lengths),
                                tc.builtins().address()));
        }

        // the registers holding the addresses of the arguments are still
        // allocated, the check must not change a register that is free but
        // used by the call
        if (tc.checks().stack) {
            x.check_stack_capacity(call_.tk, indent);
        }

        // the address of that slot is loaded after the registers are saved, a
        // register that holds a value of the caller is not lost
        const std::optional<size_t> register_slot{
            func.has_slot_register(tc) ? func.register_slot_index()
                                       : std::nullopt,
        };

        write_frame_slots(x, indent, func, frame_address, addresses,
                          register_slot);

        x.free_scratch_registers(call_.tk, indent, address_registers);

        x.call_function(
            call_.tk, indent, func.body_label(array_lengths), frame_address,
            register_slot ? addresses.at(*register_slot) : operand{});
    }

    // the body is compiled once for all callers, so the errors that depend on
    // this call are found by compiling the body as an inline call and
    // dropping the code
    auto dry_run_body(toc& tc, const size_t indent, const destination& dst,
                      const stmt_def_func& func,
                      const std::span<const noinline_arg> arguments) const
        -> void {

        const std::optional<std::string> kind{
            call_kind(dst, func, arguments),
        };

        // note: what the body can reject depends only on which arguments are
        //       the same variable, e.g. 'f(v, v, n)' but not 'f(v, w, n)',
        //       on which are a global, since the body may name that global
        //       too, e.g. 'f(v, g, n)', and on the bytes of each that are
        //       passed, e.g. 'f(g.a)' and 'f(g.b)'; so 'f(v, w, n)' and
        //       'f(x, y, z)' give the same result and are checked once
        //
        //       the call is added before its dry run, so the same kind of
        //       call reached inside it, e.g. a recursive call, returns here;
        //       the kinds are finitely many, so the nested dry runs end

        if (not kind) {
            return;
        }

        // e.g. 1 for a call in 'main'
        const size_t level{tc.scopes().noinline_dry_run_level() + 1};

        const std::optional<dry_run_height> checked_height{
            tc.dry_runs().height_of(*kind),
        };

        const dry_run_height below{checked_height.value_or(dry_run_height{})};

        // e.g. a chain of noinline calls nests a dry run in each; a checked
        // call counts the levels and the frames below it, so the verdict does
        // not depend on which calls were checked first
        if (level + below.levels > call_site::max_noinline_dry_run_level) {
            call_site::throw_noinline_nesting(call_.tk);
        }

        const size_t frame_check{tc.scopes().frames().size() + below.frames};

        if (frame_check >= call_site::max_frame_count) {
            call_site::throw_dry_run_frame_limit(call_.tk);
        }

        tc.reach_frame_check(frame_check);

        // note: a call reached again inside its own dry run, e.g. 'f(a, b)'
        //       calling 'f(b, a)' calling 'f(a, b)', finds the height 0 its
        //       dry run has until it ends, so the height of a cycle depends on
        //       where it was entered, by at most the number of its kinds;
        //       a known limitation: near the limit the verdict of a chain
        //       through a cycle depends on the order of the calls (docs 4.3)

        if (checked_height) {
            tc.reach_dry_run_level(level + below.levels);
            return;
        }

        const inline_call dry_run{call_};

        tc.dry_run_call_kind(*kind, level, [&] -> void {
            dry_run.compile_with_bindings(tc, indent, dst, func, arguments);
        });
    }

    // the result address precedes the argument addresses
    [[nodiscard]] auto
    frame_slot_addresses(toc& tc, const size_t indent, const destination& dst,
                         const stmt_def_func& func,
                         const std::span<const noinline_arg> arguments,
                         std::vector<operand>& address_registers) const
        -> std::vector<operand> {

        std::vector<operand> addresses;

        if (func.returns()) {
            addresses.push_back(dst.operand());
        }

        for (const auto [arg, argument] :
             std::views::zip(call_.args, arguments)) {

            // a temporary is a plain variable, not indexed like its argument
            if (argument.is_temporary) {
                addresses.push_back(argument.info.operand());
                continue;
            }

            addresses.push_back(arg.get_lea_operand(tc, indent, argument.info,
                                                    address_registers));
        }

        return addresses;
    }

    // each slot holds an address, not the value stored at that address
    auto write_frame_slots(machine& x, const size_t indent,
                           const stmt_def_func& func,
                           const operand& frame_address,
                           const std::vector<operand>& addresses,
                           const std::optional<size_t> register_slot) const
        -> void {

        operand slot{frame_address};

        for (const auto [i, addr] : std::views::enumerate(addresses)) {
            // the callee finds it in a register, it has no slot
            if (register_slot and std::cmp_equal(i, *register_slot)) {
                continue;
            }

            comment_frame_slot(x, indent, func, static_cast<size_t>(i));
            x.address_of(call_.tk, indent, slot, addr);

            slot.increment_offset(address_offset(x.address_size_bytes()));
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
    static auto assert_argument(const expr_any& arg, const ident_info& info,
                                const stmt_def_func_param& param) -> void {

        // note: a temporary is a variable
        assert(info.is_var());

        // note: parsing the call rejects an array argument for a non-array
        //       parameter and the reverse

        assert(info.is_array() == param.is_array());

        if (not info.type_ref().is_same(param.get_type())) {
            call_site::throw_parameter_type_mismatch(arg, param, info);
        }
    }

    // what the checks of the body depend on: the callee, the bytes each
    // argument passes (a global or which local, and the access path), the
    // array lengths and whether each argument is a temporary or a variable.
    // empty when an argument is a pointer, e.g. a parameter of the enclosing
    // noinline function, whose calls are checked with their own arguments
    [[nodiscard]] static auto
    call_kind(const destination& dst, const stmt_def_func& func,
              const std::span<const noinline_arg> arguments)
        -> std::optional<std::string> {

        std::vector<call_kind_item> items;

        // note: a pointer destination is resolved to its address before, and
        //       the result is not an array, see 'assert_call'

        if (func.returns()) {
            items.push_back({
                .target{dst.storage()},
                .is_pointer{},
                .is_array{},
                .array_len{},
            });
        }

        for (const noinline_arg& argument : arguments) {
            items.push_back({
                .target{argument.target},
                .is_pointer{argument.info.is_pointer()},
                .is_array{argument.info.is_array()},
                .array_len{argument.info.array_len()},
            });
        }

        std::string kind{func.name()};
        std::vector<root_variable> locals;

        for (const call_kind_item& item : items) {
            const std::optional<std::string> part{
                call_kind_part(item, locals),
            };

            if (not part) {
                return std::nullopt;
            }

            kind += *part;
        }

        // a temporary aliases no variable, a variable may be named by the body
        for (const noinline_arg& argument : arguments) {
            kind += argument.is_temporary ? " temporary" : " variable";
        }

        return kind;
    }

    // what an argument adds to the call kind, e.g. ' local 0.8' or
    // ' global g[?]', none for a pointer, which the call kind cannot tell.
    // 'locals' are the variables met so far
    [[nodiscard]] static auto call_kind_part(const call_kind_item& item,
                                             std::vector<root_variable>& locals)
        -> std::optional<std::string> {

        if (item.is_pointer) {
            return std::nullopt;
        }

        std::string part;

        // the body can reject an index by the length of its array
        if (item.is_array) {
            // note: an unsized array without a call is a parameter of a
            //       function checked as a noinline body, a pointer

            assert(item.array_len);

            part += std::format(" array {}", *item.array_len);
        }

        // note: the result and the arguments are variables in memory
        assert(item.target);

        const storage_path& target{*item.target};

        const std::string path{target.path.text()};

        if (target.root.is_global()) {
            return part + std::format(" global {}{}", target.root.name, path);
        }

        return part + std::format(" local {}{}",
                                  local_index(locals, target.root), path);
    }

    // the callee writes the result through its address at the type of the
    // result
    [[nodiscard]] static auto has_result_temporary(const destination& dst,
                                                   const stmt_def_func& func)
        -> bool {

        return func.returns() and (not dst.operand().is_memory() or
                                   call_site::result_type_differs(dst, func));
    }

    // the index of 'root' among the locals met so far, added when it is new
    [[nodiscard]] static auto local_index(std::vector<root_variable>& locals,
                                          const root_variable& root) -> size_t {

        for (size_t i{}; i < locals.size(); ++i) {
            if (locals.at(i).is_same(root)) {
                return i;
            }
        }

        locals.push_back(root);

        return locals.size() - 1;
    }

    // a value without storage, e.g. a literal or an expression, an array is
    // only passed by name
    [[nodiscard]] static auto needs_temporary(const toc& tc,
                                              const expr_any& arg,
                                              const stmt_def_func_param& param)
        -> bool {

        if (param.is_array()) {
            return false;
        }

        if (call_site::is_instance_temporary(arg, param)) {
            return true;
        }

        if (call_site::is_computed_value(arg)) {
            return true;
        }

        return not make_ident_info(tc, arg).is_var();
    }
};

// selects the constructor of 'stmt_call' for 'point.at(1, 2)'
struct constructor_call_tag {};

// selects the constructor of 'stmt_call' for a built-in that parses like a
// call, e.g. 'exit(0)'
struct builtin_call_tag {};

// a call of a function, a method or a constructor
//
// parsing, in the constructors: the callee and its '('
// ('read_generic_callee', a generic function through 'generic_call', and
// 'read_open_paren'), the kind of the callee
// ('assert_method_receiver', 'constructor_func'), then the arguments
// ('parse_arguments', each checked by 'assert_argument_usable' and
// 'assert_argument_writable', or 'parse_builtin_arguments')
//
// compiling, in 'compile': no two arguments may share storage
// ('assert_no_shared_storage'), then 'inline_call' or 'noinline_call'
// compiles the call
class stmt_call : public expression {
    // e.g. the '.' in 'lst.add(x)', the receiver is the first argument
    token method_dot_tk_;
    // e.g. the '.' and 'at' in 'point.at(1, 2)', 'tok()' is the type
    token constructor_dot_tk_;
    token constructor_name_tk_;
    // e.g. '<', 'name' and '>' in 'tz.to<name>()'
    std::vector<token> generic_tks_;
    std::string func_name_;
    token open_paren_tk_;
    std::vector<expr_any> args_;
    std::vector<token> arg_delims_tk_;
    token close_paren_tk_;

    static constexpr std::string_view use_temporary_fix_{
        "use a temporary variable for one of them",
    };

    // an argument that names a variable, kept to compare with the next ones
    struct reference {
        size_t index;
        storage_path target;
        bool is_read_only;
    };

  public:
    // 'tz' is after the name, e.g. 'f' of 'f(x)' or 'show' of 'show<name>(x)'.
    // 'expected_type' is the type the call is assigned to, it tells a type
    // parameter that is the result, e.g. 'x.f = tz.to()'
    stmt_call(toc& tc, unary_ops uops, const token tk, tokenizer& tz,
              const type* const expected_type)
        : expression{tk, std::move(uops)}, func_name_{tk.text()} {

        const std::vector<size_t> deducing_args{
            read_generic_callee(tc, tz, expected_type),
        };

        read_open_paren(tc, tz, "function name");

        // note: a call of 'main' can only be in 'main' or after it, in a
        //       function that nothing calls, since names are defined before use

        if (func_name_ == reserved_names::main) {
            throw compile_error{tok(), "'main' cannot be called"};
        }

        parse_arguments(tc, tz, tc.get_func_or_throw(tok(), func_name_),
                        deducing_args);
    }

    // e.g. 'lst.add(x)' calls 'list.add' with 'lst' as 'self'
    stmt_call(toc& tc, unary_ops uops, stmt_identifier receiver, tokenizer& tz,
              const type* const expected_type)
        : expression{receiver.method_name_token(), std::move(uops)},
          method_dot_tk_{receiver.method_dot_token()},
          func_name_{
              std::format("{}.{}", receiver.get_type().name(),
                          receiver.method_name_token().text()),
          } {

        const std::vector<size_t> deducing_args{
            read_generic_callee(tc, tz, expected_type),
        };

        read_open_paren(tc, tz, "method name");

        const stmt_def_func& func{tc.get_func_or_throw(tok(), func_name_)};

        assert_method_receiver(func, receiver);

        add_receiver_argument(std::move(receiver));

        parse_arguments(tc, tz, func, deducing_args);
    }

    // e.g. 'point.at(1, 2)' calls the constructor 'point.at', 'tz' is after
    // 'point'
    stmt_call(constructor_call_tag /*tag*/, toc& tc, unary_ops uops,
              const token type_tk, tokenizer& tz)
        : expression{type_tk, std::move(uops)},
          constructor_dot_tk_{tz.is_next_char_token('.')},
          constructor_name_tk_{tz.next_token()},
          func_name_{
              constructor_function_name(tc, type_tk, constructor_name_tk_),
          } {

        assert(not constructor_dot_tk_.is_empty());

        const stmt_def_func& func{constructor_func(tc, tz)};

        // note: 'func_instantiator::define_func' rejects a generic constructor
        assert(not tc.generics().has_func(func_name_));

        read_open_paren(tc, tz, "constructor name");

        parse_arguments(tc, tz, func, {});
    }

    stmt_call() = default;

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

        assert_in_func(tc);

        // note: the built-ins that parse like a call, e.g. 'exit' and the io
        //       functions, are subclasses that override 'compile'

        const call_site call{site(tc.overflow_options(dst))};

        machine& x{tc.machine()};

        x.comment(tok(), indent, call.source);

        const stmt_def_func& func{tc.get_func_or_throw(tok(), func_name_)};

        assert_no_shared_storage(tc, dst, func);

        // the result is the call's own, an error in it has no call frame;
        // checked here so that inlining does not change what is accepted
        call.assert_result_use(dst, func);

        if (func.returns()) {
            call.assert_result_type(dst, func);
        }

        if (not func.is_inlined()) {
            noinline_call{call}.compile(tc, indent, dst, func);
            return;
        }

        assert_inline_nesting(tc, func);

        // note: read before the compile, a throw leaves its frames entered
        const bool is_recursion{tc.scopes().inlined_nesting(func.name()) != 0};

        // a recursion that does not end with constant arguments runs out of
        // registers before it reaches the nesting limit, the advice is the same
        try {
            inline_call{call}.compile(tc, indent, dst, func);
        } catch (const compile_error& e) {
            if (not is_recursion or
                e.kind != compile_error::error_kind::out_of_registers) {

                throw;
            }

            throw inlining_limit_error(recursion_message(func));
        }
    }

    // reported at the call because an inlined result aliases the destination
    auto assert_not_narrowed([[maybe_unused]] const toc& tc,
                             const type& dst_type) const -> void override {

        assert_own_type_not_narrowed(dst_type);
    }

    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        for (const expr_any& e : args_) {
            e.visit_reads(var, reader);
        }
    }

  protected:
    // a built-in has no definition, 'result_type' is the type of its value,
    // 'void' if it has none
    stmt_call(builtin_call_tag /*tag*/, toc& tc, unary_ops uops, const token tk,
              tokenizer& tz, const type& result_type)
        : expression{tk, std::move(uops)}, func_name_{tk.text()} {

        open_paren_tk_ = tz.is_next_char_token('(');

        set_type(result_type);

        assert_open_paren_found(tz, "function name");

        parse_builtin_arguments(tc, tz);
    }

    [[nodiscard]] auto argument(const size_t arg_index) const
        -> const statement& {

        return args_.at(arg_index);
    }

    // e.g. '5' or 'n - 1' of a constant 'n', empty when computed at run time
    [[nodiscard]] auto argument_constant(const toc& tc,
                                         const size_t arg_index) const
        -> std::optional<int64_t> {

        return args_.at(arg_index).constant_value(tc);
    }

    [[nodiscard]] auto argument_count() const -> size_t { return args_.size(); }

    // a call outside a function would run before 'main'
    auto assert_in_func(const toc& tc) const -> void {
        if (not tc.scopes().is_in_func()) {
            throw compile_error{
                tok(), "a global variable cannot be initialized with a call"};
        }
    }

  private:
    // the receiver is the first argument
    auto add_receiver_argument(stmt_identifier receiver) -> void {
        // the whitespace before the receiver belongs to its first token
        const token& first_tk{receiver.first_token()};

        const token receiver_pos_tk{
            token::position(first_tk.file_index(), first_tk.start_index(),
                            first_tk.at_line()),
        };

        args_.emplace_back(
            receiver_pos_tk,
            expr_type{std::make_unique<stmt_identifier>(std::move(receiver))});
    }

    // a callee that writes its parameter writes the variable of the caller:
    // an argument that is not a writable variable is rejected here, also in
    // a function that is never called, so inlining does not change what is
    // accepted
    auto assert_argument_writable(const toc& tc, const size_t index,
                                  const stmt_def_func& func) const -> void {

        // a parameter without 'mut' takes any argument
        if (func.param(index).is_read_only()) {
            return;
        }

        const expr_any& arg{args_.at(index)};

        // e.g. 'a + 1', '-a' or 'point{1, 2}'
        if (call_site::is_copied_value(arg)) {
            throw_not_writable(index, arg,
                               std::format("'{}', which is not a variable,",
                                           trimmed_source(arg)),
                               "");
        }

        const ident_info info{make_ident_info(tc, arg)};

        // e.g. '2' or a 'let' constant
        if (not info.is_var()) {
            throw_not_writable(
                index, arg, std::format("constant '{}'", arg.name_or_literal()),
                "");
        }

        if (info.is_read_only()) {
            throw_not_writable(
                index, arg,
                std::format("read-only '{}'", arg.name_or_literal()),
                read_only_hint(info));
        }
    }

    // an inlined call compiles its body in place, so nesting is limited
    auto assert_inline_nesting(toc& tc, const stmt_def_func& func) const
        -> void {

        if (tc.scopes().inlined_nesting(func.name()) >=
            call_site::max_inlined_nesting) {

            throw inlining_limit_error(recursion_message(func));
        }

        const size_t frame_count{tc.scopes().frames().size()};

        tc.reach_frame_check(frame_count);

        if (frame_count < call_site::max_frame_count) {
            return;
        }

        // note: the chain does not depend on the arguments, see
        //       'noinline_call::dry_run_body'

        if (tc.scopes().noinline_dry_run_level() != 0) {
            call_site::throw_dry_run_frame_limit(tok());
        }

        throw inlining_limit_error(
            std::format("inlined calls and blocks are nested too deep, the "
                        "limit is {} scopes, declare a function 'noinline'",
                        call_site::max_frame_count));
    }

    // the function is a method and the receiver is a value
    auto assert_method_receiver(const stmt_def_func& func,
                                const stmt_identifier& receiver) const -> void {

        // a constructor has no receiver, it builds its value
        if (func.is_constructor()) {
            throw compile_error{tok(),
                                std::format("'{}' is a constructor, call it as "
                                            "'{}(...)'",
                                            func_name_, func_name_)};
        }

        // only an element of an array can be the receiver, 'a[1].f()' but not
        // 'a.f()'
        if (receiver.is_array()) {
            throw compile_error{
                receiver.first_token(),
                std::format("receiver of method '{}' cannot be an array",
                            func_name_)};
        }
    }

    // the callee writes a 'mut' parameter in place, so storage shared with
    // another argument could be read after it changed, unless both
    // parameters are read-only. the result destination is only compared with
    // '--checks=alias'. with it, a global that the callee uses counts as an
    // argument, see 'body_uses'
    auto assert_no_shared_storage(const toc& tc, const destination& dst,
                                  const stmt_def_func& func) const -> void {

        const bool is_result_checked{
            tc.checks().alias and dst.is_storage(),
        };

        std::optional<storage_path> result;

        if (is_result_checked) {
            result = dst.storage();
        }

        std::vector<reference> references;

        for (size_t i{}; i < args_.size(); ++i) {
            const expr_any& arg{args_.at(i)};

            if (call_site::is_copied_value(arg)) {
                continue;
            }

            // constants pass their value
            if (not make_ident_info(tc, arg).is_var()) {
                continue;
            }

            std::optional<storage_path> target{
                call_site::argument_target(tc, arg),
            };

            // note: a variable has storage
            assert(target);

            if (is_result_checked) {
                assert_not_shared_with_result(i, *target, dst, result);
            }

            const bool is_read_only{func.param(i).is_read_only()};

            assert_not_shared_with_earlier(references, i, *target,
                                           is_read_only);

            references.push_back({
                .index{i},
                .target{std::move(*target)},
                .is_read_only{is_read_only},
            });
        }

        if (not tc.checks().alias) {
            return;
        }

        for (const body_uses::global_use& global : func.uses().globals()) {
            assert_not_shared_with_global(func, global, references, dst,
                                          result);
        }
    }

    // a ',' after the last parameter, or any argument of a function without
    // parameters, is one argument too many, e.g. 'f(1, 2)' of 'func f(x)'
    auto assert_no_surplus_argument(tokenizer& tz,
                                    const size_t param_count) const -> void {

        const char next{tz.peek_char_after_whitespace()};

        if (next == ')' or (param_count != 0 and next != ',')) {
            return;
        }

        // e.g. 'f(1, 2,)' has no surplus argument
        tokenizer probe{tz};
        const token delimiter_tk{probe.is_next_char_token(',')};

        if (not delimiter_tk.is_empty()) {
            probe.assert_no_trailing_delimiter(delimiter_tk, ')', "a call");
        }

        throw compile_error{tz, std::format("'{}' takes {}", func_name_,
                                            counted(param_count, "argument"))};
    }

    // an argument that names the storage of an earlier one, unless both
    // parameters are read-only or the bytes they reach are disjoint
    auto assert_not_shared_with_earlier(
        const std::span<const reference> earlier, const size_t index,
        const storage_path& target, const bool is_read_only) const -> void {

        const expr_any& arg{args_.at(index)};

        for (const reference& other : earlier) {
            if (is_read_only and other.is_read_only) {
                continue;
            }

            if (not target.may_overlap(other.target)) {
                continue;
            }

            throw compile_error{
                arg.tok(),
                std::format("{} '{}' may share storage with {} '{}' "
                            "(both name '{}'), {}",
                            describe_argument(index),
                            statement::trimmed_source(arg),
                            describe_argument(other.index),
                            statement::trimmed_source(args_.at(other.index)),
                            target.root.name, use_temporary_fix_)};
        }
    }

    // what the callee writes through the result or an argument may be a
    // global it uses, or what it writes to the global may be what it reaches
    // through them, e.g. 'g = add_g()' where 'add_g' sets 'res' and reads 'g'
    auto assert_not_shared_with_global(
        const stmt_def_func& func, const body_uses::global_use& global,
        const std::span<const reference> args, const destination& dst,
        const std::optional<storage_path>& result) const -> void {

        // the part of 'target' that the callee reaches through it as 'u'
        const auto overlaps = [&](const storage_path& target,
                                  const body_uses::binding_use& u) -> bool {
            const storage_path reached{
                .root{target.root},
                .path{target.path.extended_by(u.path)},
            };

            return (u.is_written or global.is_written) and
                   reached.may_overlap(global.storage);
        };

        for (const body_uses::binding_use& u : func.uses().bindings()) {
            if (u.param_index or not result or not overlaps(*result, u)) {
                continue;
            }

            throw compile_error{
                tok(), std::format("the result destination '{}' may share "
                                   "storage with the global '{}' that '{}' "
                                   "uses, {}",
                                   dst.text(), global.storage.root.name,
                                   func_name_, use_temporary_fix_)};
        }

        for (const reference& arg : args) {
            for (const body_uses::binding_use& u : func.uses().bindings()) {
                if (u.param_index != arg.index or not overlaps(arg.target, u)) {
                    continue;
                }

                throw compile_error{
                    args_.at(arg.index).tok(),
                    std::format("{} '{}' may share storage with the global "
                                "'{}' that '{}' uses, {}",
                                describe_argument(arg.index),
                                statement::trimmed_source(args_.at(arg.index)),
                                global.storage.root.name, func_name_,
                                use_temporary_fix_)};
            }
        }
    }

    // an argument that names the result destination, only for '--checks=alias'
    auto assert_not_shared_with_result(
        const size_t index, const storage_path& target, const destination& dst,
        const std::optional<storage_path>& result) const -> void {

        if (not result or not target.may_overlap(*result)) {
            return;
        }

        throw compile_error{
            args_.at(index).tok(),
            std::format("{} '{}' may share storage with the "
                        "result destination '{}' (both name '{}'), {}",
                        describe_argument(index),
                        statement::trimmed_source(args_.at(index)), dst.text(),
                        target.root.name, use_temporary_fix_)};
    }

    // 'what' names what precedes the '(' in the error
    auto assert_open_paren_found(const tokenizer& tz,
                                 const std::string_view what) const -> void {

        if (not open_paren_tk_.is_empty()) {
            return;
        }

        throw compile_error{tz, std::format("expected '(' after {}", what)};
    }

    // the receiver of a method is written before the name
    [[nodiscard]] auto call_begin_token() const -> const token& {
        return is_method() ? args_.at(0).tok() : tok();
    }

    // the type names the constructor after the '.', e.g. 'at' of
    // 'point.at(1, 2)', a method of the type is not one
    [[nodiscard]] auto constructor_func(const toc& tc,
                                        const tokenizer& tz) const
        -> const stmt_def_func& {

        if (not constructor_name_tk_.is_word()) {
            throw compile_error{tz.position_of(constructor_name_tk_),
                                "expected constructor name after '.'"};
        }

        if (not tc.is_func(func_name_)) {
            throw compile_error{constructor_name_tk_,
                                std::format("type '{}' has no constructor '{}'",
                                            type_description(tc, tok()),
                                            constructor_name_tk_.text())};
        }

        const stmt_def_func& func{
            tc.get_func_or_throw(constructor_name_tk_, func_name_),
        };

        if (not func.is_constructor()) {
            throw compile_error{
                constructor_name_tk_,
                std::format("'{}' is a method, call it on a value of type '{}'",
                            func_name_, tok().text())};
        }

        return func;
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

    [[nodiscard]] auto inlining_limit_error(std::string message) const
        -> compile_error {

        compile_error error{tok(), std::move(message)};
        error.kind = compile_error::error_kind::inlining_limit;
        return error;
    }

    [[nodiscard]] auto is_constructor() const -> bool {
        return not constructor_dot_tk_.is_empty();
    }

    [[nodiscard]] auto is_method() const -> bool {
        return not method_dot_tk_.is_empty();
    }

    // a method receiver is already in 'args_', the parameter gives the type of
    // a literal argument unless its type was deduced from it, see
    // 'generic_call::deducing_args'
    auto parse_arguments(toc& tc, tokenizer& tz, const stmt_def_func& func,
                         const std::span<const size_t> deducing_args) -> void {

        const std::span<const stmt_def_func_param> params{func.params()};
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
                    std::format("expected ',' before {} ('{}')",
                                describe_argument(i), params.at(i).name()),
                };

                const token delimiter_tk{tz.expect_char_token(',', message)};

                tz.assert_no_trailing_delimiter(delimiter_tk, ')', "a call");

                arg_delims_tk_.emplace_back(delimiter_tk);
            }

            assert_not_string_argument(tz);

            const bool is_deducing{
                std::ranges::contains(deducing_args,
                                      i - first_argument_index()),
            };

            // e.g. 'a' and 'b + 1' of 'f(a, b + 1)'
            args_.emplace_back(tc, tz, params.at(i).get_type(),
                               is_deducing ? literal_typing::written
                                           : literal_typing::from_destination);
        }

        assert_no_surplus_argument(tz, params.size() - first_argument_index());

        // e.g. 'f(a, b' lacks it
        close_paren_tk_ =
            tz.expect_char_token(')', "expected ')' after arguments");

        // e.g. the '.' of 'mk().y', a result has no storage to read a field of
        if (tz.peek_char() == '.') {
            throw compile_error{
                tz, std::format("store the result of '{}' in a variable before "
                                "reading its field",
                                func_name_)};
        }

        for (size_t i{}; i < args_.size(); ++i) {
            assert_argument_usable(tc, args_.at(i), params.at(i));
            assert_argument_writable(tc, i, func);
        }

        record_uses_of_call(tc, func);
    }

    // a built-in has no parameter list, so any count of default type
    // arguments is parsed and the built-in checks the count
    auto parse_builtin_arguments(toc& tc, tokenizer& tz) -> void {
        while (true) {
            close_paren_tk_ = tz.is_next_char_token(')');

            if (not close_paren_tk_.is_empty()) {
                return;
            }

            assert_not_string_argument(tz);

            args_.emplace_back(tc, tz, tc.builtins().default_type(),
                               literal_typing::from_destination);

            const token delim_tk{tz.is_next_char_token(',')};

            if (not delim_tk.is_empty()) {
                tz.assert_no_trailing_delimiter(delim_tk, ')', "a call");
                arg_delims_tk_.emplace_back(delim_tk);
                continue;
            }

            // e.g. 'write(1, text n)' lacks the ','
            close_paren_tk_ = tz.expect_char_token(
                ')', "expected ',' or ')' after the argument");

            return;
        }
    }

    // the name is read, a generic function has its type arguments before the
    // '(', e.g. 'convert<i8>(x)', and 'func_name_' becomes the name of its
    // instance; returns 'generic_call::deducing_args'
    [[nodiscard]] auto read_generic_callee(toc& tc, tokenizer& tz,
                                           const type* const expected_type)
        -> std::vector<size_t> {

        if (not tc.generics().has_func(func_name_)) {
            return {};
        }

        generic_call generic{
            generic_call::parse(tc, tz, tok(), func_name_, expected_type),
        };

        func_name_ = std::move(generic.func_name);
        generic_tks_ = std::move(generic.generic_tks);

        return std::move(generic.deducing_args);
    }

    // the type is the return type of the callee, 'what' names the callee in
    // the error of a missing '('
    // note: an unknown callee is reported before a missing '('
    auto read_open_paren(const toc& tc, tokenizer& tz,
                         const std::string_view what) -> void {

        open_paren_tk_ = tz.is_next_char_token('(');

        set_type(tc.get_func_return_type_or_throw(tok(), func_name_));

        assert_open_paren_found(tz, what);
    }

    // what the callee reaches through a parameter the caller reaches through
    // the argument, and the globals it uses are the caller's too; a call of
    // the function being parsed has no complete record yet, so a 'mut'
    // argument counts as written whole
    auto record_uses_of_call(toc& tc, const stmt_def_func& func) const -> void {

        // note: a call outside a function is rejected when it is compiled

        if (not tc.scopes().is_in_func()) {
            return;
        }

        const body_uses& callee{func.uses()};

        tc.record_callee_globals(callee);

        const bool is_recursive{
            tc.scopes().current_func_frame().is_name(func.name()),
        };

        for (size_t i{}; i < args_.size(); ++i) {
            const expr_any& arg{args_.at(i)};

            if (call_site::is_copied_value(arg)) {
                continue;
            }

            const std::string_view root{
                ident_info::root_of(arg.name_or_literal()),
            };

            if (is_recursive) {
                tc.record_use(root, arg.accessed_path(),
                              not func.param(i).is_read_only());

                continue;
            }

            for (const body_uses::binding_use& u : callee.bindings()) {
                if (u.param_index == i) {
                    tc.record_use(root, arg.accessed_path().extended_by(u.path),
                                  u.is_written);
                }
            }
        }
    }

    // what 'inline_call' and 'noinline_call' need of this call
    [[nodiscard]] auto site(const machine::runtime_check& uops_check) const
        -> call_site {

        return {
            .tk{tok()},
            .begin_tk{call_begin_token()},
            .close_paren_tk{close_paren_tk_},
            .source{statement::trimmed_source(*this)},
            .args{args_},
            .uops{get_unary_ops()},
            .uops_check{uops_check},
        };
    }

    // e.g. 'f', 'lst.add' or 'point.at'
    auto source_callee_to(std::ostream& os) const -> void {
        if (is_constructor()) {
            expression::source_to(os);
            constructor_dot_tk_.source_to(os);
            constructor_name_tk_.source_to(os);
            return;
        }

        if (not is_method()) {
            expression::source_to(os);
            source_generic_to(os);
            return;
        }

        get_unary_ops().source_to(os);
        args_.front().source_to(os);
        method_dot_tk_.source_to(os);
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

        throw compile_error{tz, std::format("expected {} ('{}')",
                                            describe_argument(index),
                                            param.name())};
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
                std::format("{} cannot be the receiver of 'mut' method '{}'{}",
                            what, func_name_, hint)};
        }

        throw compile_error{
            arg.tok(), std::format("{} cannot be passed to a 'mut' parameter{}",
                                   what, hint)};
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
            const bool is_array_arg{
                arg.is_name_or_literal() and
                    not call_site::is_computed_value(arg) and
                    make_ident_info(tc, arg).is_array(),
            };

            // an element would give the parameter the whole array's
            // length, pass the array and a start index instead
            if (not is_array_arg) {
                throw compile_error{
                    arg.tok(),
                    std::format("'{}' is not an array, parameter '{}' takes "
                                "one",
                                trimmed_source(arg), param.name())};
            }

            return;
        }

        // the binding would make the parameter name the whole array
        if (arg.is_name_or_literal()) {
            assert_not_whole_array(arg, make_ident_info(tc, arg));
        }
    }

    // a string is in read-only data that a parameter would refer to
    static auto assert_not_string_argument(tokenizer& tz) -> void {
        if (tz.peek_char_after_whitespace() != '"') {
            return;
        }

        throw compile_error{tz.next_token(), "a string is not an argument, it "
                                             "would refer to read-only data"};
    }

    // 'point.at' for the type named as written, e.g. an alias with constructors
    // of its own; else the name of the type that a type parameter is bound to,
    // e.g. 'T.at' where 'T' is 'point'
    [[nodiscard]] static auto
    constructor_function_name(const toc& tc, const token& type_tk,
                              const token& constructor_name_tk) -> std::string {

        std::string name{
            std::format("{}.{}", type_tk.text(), constructor_name_tk.text()),
        };

        if (tc.is_func(name) or not tc.types().has(type_tk.text())) {
            return name;
        }

        name = std::format("{}.{}", tc.get_type_or_throw(type_tk).name(),
                           constructor_name_tk.text());

        return name;
    }

    [[nodiscard]] static auto recursion_message(const stmt_def_func& func)
        -> std::string {

        return std::format("recursion of inlined function '{}' cannot be "
                           "expanded at compile time, declare it 'noinline'",
                           func.name());
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
