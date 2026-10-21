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
#include <variant>
#include <vector>

#include "compile_error.hpp"
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

        for (size_t i{}; i < index; ++i) {
            tz.skip_argument();

            if (tz.is_next_char_token(',').is_empty()) {
                return nullptr;
            }
        }

        const token tk{tz.next_token()};
        const name_meaning meaning{tc.scopes().lookup(tk.text())};

        // note: a constant does not tell its type, like a literal
        if (meaning.is_constant()) {
            return nullptr;
        }

        // e.g. 'point{1, 2}', a variable or a parameter hides the type
        if (not meaning.is_var_or_alias()) {
            return value_type(tc, tz, tk);
        }

        // note: read from the copy to find where the argument ends and its
        //       type, the call parses it again

        const stmt_identifier si{tc, {}, tk, tz};

        const char next{tz.peek_char_after_whitespace()};

        if (si.is_method_receiver() or (next != ',' and next != ')')) {
            return nullptr;
        }

        return &tc.make_ident_info(si).type_ref();
    }

    // e.g. 'f(1)' of 'func f<T type>(a, x T)' lacks the argument that would
    // tell 'T', which is the error; 'tz' is after the '(' and is a copy
    static auto assert_argument_present(tokenizer tz, const size_t index,
                                        const std::string_view param_name)
        -> void {

        for (size_t i{}; i < index; ++i) {
            tz.skip_argument();

            if (tz.is_next_char_token(',').is_empty()) {
                break;
            }
        }

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

            return is_whole_argument() ? &tc.get_type_or_throw(tk, tk.text())
                                       : nullptr;
        }

        // e.g. 'point.at(1, 2)'
        if (is_type and not tz.is_next_char_token('.').is_empty()) {
            // the type names the result, the constructor, e.g. 'at', does not
            std::ignore = tz.next_token();

            if (tz.is_next_char_token('(').is_empty()) {
                return nullptr;
            }

            tz.skip_to_close_paren();

            return is_whole_argument() ? &tc.get_type_or_throw(tk, tk.text())
                                       : nullptr;
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
    storage_target target;
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

    // a frame is a scope being compiled: a block, a loop or a function body,
    // inlined or the dry run of a noinline call; each recurses in the compiler
    static constexpr size_t max_frame_count{256};

    // a level is the dry run of a noinline call nested in the dry run of the
    // call that reaches it, see 'noinline_call::check_aliasing'
    static constexpr size_t max_noinline_dry_run_level{100};

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
                       .pointer_register{},
                       .base_register{},
                       .value_register{},
                       .foo_array{},
                   },
                   var_kind::var);

        return tc.make_ident_info(tk, name);
    }

    // the result has the type of the destination, an operand widens a
    // narrower integer, see 'destination::as_operand'
    auto assert_result_type(const destination& dst,
                            const stmt_def_func& func) const -> void {

        if (dst.type_ref().is_same(func.get_type())) {
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

    // unique per call and argument, an inlined body names it through the alias
    // of its parameter and a call in the body has temporaries of its own
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
        -> storage_target {

        return tc.storage_target_of(arg.tok(),
                                    ident_info::root_of(arg.name_or_literal()),
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

    // the root variable and the bytes of it that the destination names
    [[nodiscard]] static auto result_target(const toc& tc,
                                            const token& src_loc_tk,
                                            const destination& dst)
        -> storage_target {

        // note: a register destination, its id is a register name, has no
        //       variable

        if (dst.is_register()) {
            return {};
        }

        return tc.storage_target_of(src_loc_tk, ident_info::root_of(dst.name()),
                                    dst.accessed_path());
    }

    // the frames ran out in the dry run of a noinline call, see
    // 'noinline_call::check_aliasing'
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

        return e.start_index >= begin_tk.start_index() and
               e.start_index <= close_paren_tk.end_index();
    }
};

// compiles a call of an inlined function: the body is compiled in place, its
// parameters and its result are aliases of the arguments and the destination
//
// the steps:
//
//   1. 'compile': an instance literal or a call result argument is made in a
//      temporary
//   2. 'compile_to_destination': a memory destination may get its address in
//      a register ('compile_with_result_address_register'), or unary
//      operators may work on a scratch register
//   3. 'compile_with_aliases': the aliases of the result and the arguments
//      ('make_aliases'), an indexed argument may get its address in a
//      register ('compile_with_address_choice',
//      'compile_body_with_address_registers')
//   4. 'compile_body': the body under labels unique to the call, with the
//      aliases added
//
// a 'noinline_call' runs step 3 as a dry run to check its aliasing
class inline_call {
    call_site call_;

  public:
    explicit inline_call(call_site call) : call_{std::move(call)} {}

    // an instance literal or a call result is made in a variable of the
    // parameter's type that the body reaches through the alias of its
    // parameter
    auto compile(toc& tc, const size_t indent, const destination& dst,
                 const stmt_def_func& func) const -> void {

        if (func.returns() and not dst.type_ref().is_same(func.get_type())) {
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

            arg.compile(tc, indent, destination::of_name(temporary_info));
        }

        compile_to_destination(tc, indent, dst, func);

        tc.exit_block();
    }

    // 'call_arguments' are those of a 'noinline' call that is only checked,
    // the body sees an argument that went through a temporary as that
    // temporary
    auto compile_with_aliases(
        toc& tc, const size_t indent, const destination& dst,
        const stmt_def_func& func,
        const std::span<const noinline_arg> call_arguments = {}) const -> void {

        // scratch registers stay allocated until the inlined body is compiled
        std::vector<operand> allocated_registers;

        const std::vector<alias_info> aliases{
            make_aliases(tc, indent, dst, func, call_arguments,
                         allocated_registers),
        };

        // the result alias has its own address choice in
        // 'compile_to_destination'
        // note: the result alias is made before the aliases of the arguments,
        //       which are the last 'call_.args.size()'
        const size_t first_argument_alias{aliases.size() - call_.args.size()};

        const bool has_indexed_argument{
            std::ranges::any_of(aliases |
                                    std::views::drop(first_argument_alias),
                                is_indexed_reference),
        };

        const machine& x{tc.machine()};

        // an access can use the indexed address as it is, so computing it
        // into a register is not tried
        if (not has_indexed_argument or x.has_indexed_addressing()) {
            compile_body(tc, indent, func, aliases, allocated_registers);
            return;
        }

        compile_with_address_choice(tc, indent, func, aliases,
                                    first_argument_alias, allocated_registers);
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
                           tc.checks().overflow_options());
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
                      const std::span<const alias_info> aliases_to_add,
                      const std::span<const operand> registers_to_free) const
        -> void {

        machine& x{tc.machine()};

        // the body is inlined at every call, so its labels are named by the
        // call
        const std::string_view call_path{tc.get_call_path()};

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
            tc.enter_func(func.name(), new_call_path, ret_jmp_label);
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

        func.add_constants(tc, indent + 1);

        // the body names the arguments and the result by the parameters
        for (const alias_info& e : aliases_to_add) {
            const bound_storage* const storage{
                std::get_if<bound_storage>(&e.binding),
            };

            x.comment_alias(call_.tk, indent + 1, e.from, e.target_text(),
                            storage == nullptr ? operand{} : storage->lea);

            tc.add_alias(e);
        }

        // the result reaches the destination through the 'res' alias, so the
        // statements of the body have no destination
        func.compile_statements(tc, indent);

        x.free_scratch_registers(call_.tk, indent + 1, registers_to_free);

        // provide the exit label for 'return' to jump to
        x.label(indent, ret_jmp_label);

        apply_unary_ops_to_result(tc, indent, func);

        tc.exit_func(func.name());
    }

    // a new register per argument because the index register may belong to
    // an enclosing alias that is used after the call
    auto compile_body_with_address_registers(
        toc& tc, const size_t indent, const stmt_def_func& func,
        std::vector<alias_info> aliases_to_add,
        const size_t first_argument_alias, const bool keeps_displacement) const
        -> void {

        machine& x{tc.machine()};

        std::vector<operand> address_registers;

        for (alias_info& alias :
             aliases_to_add | std::views::drop(first_argument_alias)) {

            if (not is_indexed_reference(alias)) {
                continue;
            }

            bound_storage& storage{std::get<bound_storage>(alias.binding)};

            const operand address{
                x.alloc_scratch_register(call_.tk, indent,
                                         tc.builtins().address()),
            };

            address_registers.push_back(address);

            x.comment(call_.tk, indent, "address of parameter '{}'",
                      alias.from);

            storage.lea = load_indexed_address(x, indent, address, storage.lea,
                                               keeps_displacement);
        }

        compile_body(tc, indent, func, aliases_to_add, address_registers);
    }

    auto compile_to_destination(toc& tc, const size_t indent,
                                const destination& dst,
                                const stmt_def_func& func) const -> void {

        if (not dst.operand().is_memory()) {
            compile_with_aliases(tc, indent, dst, func);
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
                [&] -> void { compile_with_aliases(tc, indent, dst, func); },
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
            compile_with_aliases(tc, indent, dst, func);
            return;
        }

        // unary ops on a memory result are a load, modify and store on a
        // load/store machine, a scratch register can be shorter
        tc.emit_most_efficient(
            call_.tk, indent,
            [&] -> void { compile_with_aliases(tc, indent, dst, func); },
            [&] -> void {
                const operand reg{
                    x.alloc_scratch_register(call_.tk, indent, dst.type_ref()),
                };

                compile_with_aliases(tc, indent, destination::of_register(reg),
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
                           tc.checks().overflow_options());
    }

    // note: the body is compiled in three versions and the one with the least
    //       code is kept
    //
    //       1. aliases use the indexed addresses as they are, e.g.
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
        const std::vector<alias_info>& aliases,
        const size_t first_argument_alias,
        const std::span<const operand> allocated_registers) const -> void {

        machine& x{tc.machine()};

        tc.emit_most_efficient(
            call_.tk, indent,
            [&] -> void { compile_body(tc, indent, func, aliases, {}); },
            [&] -> void {
                emit_most_efficient_address(
                    tc, indent, [&](const bool keeps) -> void {
                        compile_body_with_address_registers(
                            tc, indent, func, aliases, first_argument_alias,
                            keeps);
                    });
            });

        x.free_scratch_registers(call_.tk, indent + 1, allocated_registers);
    }

    // a new register because the index register may belong to an enclosing
    // alias that is used after the call
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

        compile_with_aliases(tc, indent, address_info, func);

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

    // the aliases of the result and of the arguments of the inlined body
    [[nodiscard]] auto
    make_aliases(toc& tc, const size_t indent, const destination& dst,
                 const stmt_def_func& func,
                 const std::span<const noinline_arg> call_arguments,
                 std::vector<operand>& allocated_registers) const
        -> std::vector<alias_info> {

        assert(call_arguments.empty() or
               call_arguments.size() == call_.args.size());

        // note: the call checked the use and the type of the result before
        //       the body was entered

        assert(func.returns().has_value() != dst.is_empty());

        std::vector<alias_info> aliases;

        // the result names the destination, so the widths must agree
        if (const std::optional<func_return_info> ret{func.returns()}; ret) {
            assert(dst.type_ref().is_same(func.get_type()));

            aliases.push_back(make_result_alias(tc, dst, *ret));
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

                aliases.push_back(make_storage_alias(
                    param, call_.temporary_argument_name(i)));

                continue;
            }

            aliases.push_back(make_argument_alias(tc, indent, arg, param,
                                                  allocated_registers));
        }

        return aliases;
    }

    //
    // statics
    //

    static auto assert_alias_type(const toc& tc, const expr_any& arg,
                                  const stmt_def_func_param& param) -> void {

        const ident_info info{tc.make_ident_info(arg)};

        // constants and registers are values, not storage
        if (not info.is_var()) {
            return;
        }

        if (info.type_ref().is_same(param.get_type())) {
            return;
        }

        call_site::throw_parameter_type_mismatch(arg, param, info);
    }

    // e.g. [s0 + t0 * 4 + 28] but not [t1 + 28]
    [[nodiscard]] static auto is_indexed_reference(const alias_info& alias)
        -> bool {

        const bound_storage* const storage{
            std::get_if<bound_storage>(&alias.binding),
        };

        return storage != nullptr and storage->lea.is_memory() and
               not storage->lea.index_register().empty();
    }

    // the inlined body reaches an argument through its storage, its constant
    // value or a scratch register holding its value
    [[nodiscard]] static auto
    make_argument_alias(toc& tc, const size_t indent, const expr_any& arg,
                        const stmt_def_func_param& param,
                        std::vector<operand>& allocated_registers)
        -> alias_info {

        const bool is_reference{
            not arg.is_expression() and
                (arg.is_indexed() or
                 (arg.is_name_or_literal() and
                  tc.make_ident_info(arg.tok(), arg.name_or_literal())
                      .is_address_computed())),
        };

        // an alias uses the argument's storage so its type must match
        if (not call_site::is_computed_value(arg)) {
            assert_alias_type(tc, arg, param);
        }

        if (is_reference and arg.get_unary_ops().is_empty()) {
            return make_reference_alias(tc, indent, arg, param,
                                        allocated_registers);
        }

        // e.g. '-a[i]', a value computed like an expression that must fit
        // the parameter like a name with unary operators
        if (is_reference) {
            arg.assert_not_narrowed(tc, param.get_type());

            return make_expression_alias(tc, indent, arg, param,
                                         allocated_registers);
        }

        if (arg.is_expression()) {
            return make_expression_alias(tc, indent, arg, param,
                                         allocated_registers);
        }

        return make_plain_argument_alias(tc, indent, arg, param,
                                         allocated_registers);
    }

    // a constant lets the inlined body be decided at compile time
    [[nodiscard]] static auto
    make_expression_alias(toc& tc, const size_t indent, const expr_any& arg,
                          const stmt_def_func_param& param,
                          std::vector<operand>& allocated_registers)
        -> alias_info {

        const std::optional<int64_t> value{
            arg.constant_value(tc, param.get_type()),
        };

        if (value) {
            return alias_info::make_constant(param.name_or_literal(),
                                             param.get_type(), *value);
        }

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register(arg.tok(), indent, param.get_type()),
        };

        allocated_registers.push_back(reg);
        arg.compile(tc, indent, destination::of_register(reg));

        return alias_info::make_register(param.name_or_literal(),
                                         param.get_type(), reg);
    }

    // an identifier or a constant, with or without unary operators
    [[nodiscard]] static auto
    make_plain_argument_alias(toc& tc, const size_t indent, const expr_any& arg,
                              const stmt_def_func_param& param,
                              std::vector<operand>& allocated_registers)
        -> alias_info {

        // constants and unary ops pass a value, not storage, so the value
        // must fit the parameter
        arg.assert_not_narrowed(tc, param.get_type());

        const ident_info arg_info{tc.make_ident_info(arg)};

        // a name is resolved in the callee where its own constants would
        // shadow the caller's, and the alias target must be a plain integer,
        // e.g. '~1' is not
        if (arg_info.is_const()) {
            const std::optional<int64_t> value{
                arg.constant_value(tc, param.get_type()),
            };

            // note: a constant name or literal folds
            assert(value);

            return alias_info::make_constant(param.name_or_literal(),
                                             param.get_type(), *value);
        }

        if (arg.get_unary_ops().is_empty()) {
            return make_storage_alias(param, arg.name_or_literal(),
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

        return alias_info::make_register(param.name_or_literal(),
                                         param.get_type(), reg);
    }

    // an indexed argument keeps its computed address, e.g.
    // '[rbp + r14 * 4 + 205]', '[r15 + r14]' or simply '[r15]'
    [[nodiscard]] static auto
    make_reference_alias(toc& tc, const size_t indent, const expr_any& arg,
                         const stmt_def_func_param& param,
                         std::vector<operand>& allocated_registers)
        -> alias_info {

        const ident_info arg_info{tc.make_ident_info(arg)};

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

        return alias_info::make_storage(
            param.name_or_literal(), param.get_type(),
            {
                .path{std::string{arg.name_or_literal()}},
                .lea{lea},
                .is_element{arg.is_array_element()},
                .bound{call_site::argument_target(tc, arg)},
            });
    }

    // the result name of the function refers to the destination
    [[nodiscard]] static auto make_result_alias(const toc& tc,
                                                const destination& dst,
                                                const func_return_info& ret)
        -> alias_info {

        // e.g. the scratch register of an expression, or a parameter that
        // stands for one
        if (dst.is_register()) {
            // note: a register has no address to keep
            assert(not dst.is_address_known());

            return alias_info::make_register(ret.ident_tk.text(), *ret.type_ptr,
                                             dst.operand(), dst.name());
        }

        operand dst_lea{
            dst.is_address_known() ? dst.operand() : operand{},
        };

        // a destination such as 'arr[1]' is not an array
        return alias_info::make_storage(
            ret.ident_tk.text(), *ret.type_ptr,
            {
                .path{dst.name()},
                .lea{std::move(dst_lea)},
                .is_element{not dst.is_array()},
                .bound{call_site::result_target(tc, ret.ident_tk, dst)},
            });
    }

    // a variable of the caller or a temporary of the call, named by 'path'
    [[nodiscard]] static auto
    make_storage_alias(const stmt_def_func_param& param,
                       const std::string_view path,
                       const storage_target& bound = {}) -> alias_info {

        return alias_info::make_storage(param.name_or_literal(),
                                        param.get_type(),
                                        {
                                            .path{std::string{path}},
                                            .lea{},
                                            .is_element{},
                                            .bound{bound},
                                        });
    }
};

// compiles a call of a 'noinline' function: its body is emitted once for all
// calls and reaches the result and the arguments through their addresses in
// the callee frame
//
// the steps:
//
//   1. 'compile': a result or an argument that is not a variable in memory
//      goes through a temporary
//   2. 'compile_with_arguments': the types are checked ('assert_call'), the
//      aliasing of this call is checked by a dry run of the body as an
//      inlined call ('check_aliasing', see
//      'etc/docs/README.md' chapter 4), the addresses are written to
//      the callee frame ('frame_slot_addresses', 'write_frame_slots') and the
//      function is called
class noinline_call {
    // what the aliasing signature reads of the result or of an argument
    struct signature_item {
        // the variable and the bytes of it, e.g. 'v@2' and the field 'x' of
        // 'v.x'
        storage_target target;
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
                           tc.checks().overflow_options());
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

    // the body is compiled once for all callers, so this call's aliasing is
    // checked by compiling the body as an inline call and dropping the code
    auto check_aliasing(toc& tc, const size_t indent, const destination& dst,
                        const stmt_def_func& func,
                        const std::span<const noinline_arg> arguments) const
        -> void {

        const std::optional<std::string> signature{
            aliasing_signature(tc, call_.tk, dst, func, arguments),
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

        if (not signature) {
            return;
        }

        // e.g. 1 for a call in 'main'
        const size_t level{tc.scopes().noinline_dry_run_level() + 1};

        const std::optional<dry_run_height> checked_height{
            tc.funcs().checked_call_height(*signature),
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
        //       where it was entered, by at most the number of its signatures;
        //       a known limitation: near the limit the verdict of a chain
        //       through a cycle depends on the order of the calls (docs 4.3)

        if (checked_height) {
            tc.reach_dry_run_level(level + below.levels);
            return;
        }

        toc::noinline_check_scope check{tc, *signature, level};
        const inline_call dry_run{call_};

        try {
            tc.check_only([&] -> void {
                dry_run.compile_with_aliases(tc, indent, dst, func, arguments);
            });
        } catch (compile_error& e) {
            call_.add_call_frame(e);
            throw;
        }

        check.commit();
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
                    .info{tc.make_ident_info(arg)},
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
                    tc.storage_target_of(call_.tk, temporary.root_id(),
                                         std::nullopt),
                },
                .is_temporary{true},
            });

            arg.compile(tc, indent,
                        destination::of_name(arguments.back().info));
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
                tc, indent, destination::of_name(result_info), func, arguments);

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
        check_aliasing(tc, indent, dst, func, arguments);

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
        if (tc.checks().stack and x.is_stack_bounded()) {
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
            if (std::cmp_equal(i, register_slot.value_or(addresses.size()))) {
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

    // what the checks of the body depend on: the callee, the bytes each
    // argument passes (a global or which local, and the access path), the
    // array lengths and whether each argument is a temporary or a variable.
    // empty when an argument is a parameter of the enclosing noinline
    // function, the calls of that function are checked with their own
    // arguments
    [[nodiscard]] static auto
    aliasing_signature(const toc& tc, const token& src_loc_tk,
                       const destination& dst, const stmt_def_func& func,
                       const std::span<const noinline_arg> arguments)
        -> std::optional<std::string> {

        std::vector<signature_item> items;

        // note: a pointer destination is resolved to its address before
        if (func.returns()) {
            items.push_back({
                .target{call_site::result_target(tc, src_loc_tk, dst)},
                .is_pointer{},
                .is_array{dst.is_array()},
                .array_len{dst.array_len()},
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

        std::string signature{func.name()};
        std::vector<std::string_view> locals;

        for (const signature_item& item : items) {
            const std::optional<std::string> part{
                signature_part(item, locals),
            };

            if (not part) {
                return std::nullopt;
            }

            signature += *part;
        }

        // the body is checked with the value, e.g. a constant, of the first
        // call of its kind
        for (const noinline_arg& argument : arguments) {
            signature += argument.is_temporary ? " temporary" : " variable";
        }

        return signature;
    }

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

    // the callee writes the result through its address at the type of the
    // result
    [[nodiscard]] static auto has_result_temporary(const destination& dst,
                                                   const stmt_def_func& func)
        -> bool {

        return func.returns() and (not dst.operand().is_memory() or
                                   not dst.type_ref().is_same(func.get_type()));
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

        return not tc.make_ident_info(arg).is_var();
    }

    // what an argument adds to the signature, e.g. ' local 0.8' or
    // ' global g[?]', none for a pointer, which the signature cannot tell.
    // 'locals' are the variables met so far
    [[nodiscard]] static auto
    signature_part(const signature_item& item,
                   std::vector<std::string_view>& locals)
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

        const storage_target& target{item.target};

        // note: an unknown path is the whole variable
        const std::string path{target.path ? target.path->text() : "*"};

        if (target.is_global()) {
            return part +
                   std::format(" global {}{}", target.display_root(), path);
        }

        const std::string_view root{target.root};

        if (std::ranges::find(locals, root) == locals.end()) {
            locals.push_back(root);
        }

        const auto local{std::ranges::find(locals, root)};

        return part + std::format(" local {}{}", local - locals.begin(), path);
    }
};

// selects the constructor of 'stmt_call' for 'point.at(1, 2)'
struct constructor_call_tag {};

// a call of a function, a method or a constructor
//
// parsing, in the constructors: the callee and its '(' ('read_callee', a
// generic function through 'generic_call'), the kind of the callee
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
        storage_target target;
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

        if (not tc.funcs().is_builtin(func_name_)) {
            parse_arguments(tc, tz, tc.get_func_or_throw(tok(), func_name_),
                            deducing_args);

            return;
        }

        parse_builtin_arguments(tc, tz);
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

        if (not tc.scopes().is_in_func()) {
            throw compile_error{
                tok(), "a global variable cannot be initialized with a call"};
        }

        // note: 'read', 'write' and 'exit' are built-ins with a statement of
        //       their own

        assert(not tc.funcs().is_builtin(func_name_));

        const call_site call{site()};

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

        const toc::overflow_check_resume checked{tc};

        if (not func.is_inlined()) {
            noinline_call{call}.compile(tc, indent, dst, func);
            return;
        }

        assert_inline_nesting(tc, func);

        // an error found in the inlined body reports where it was called from
        try {
            inline_call{call}.compile(tc, indent, dst, func);
        } catch (compile_error& e) {
            call.add_call_frame(e);
            throw;
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

        const ident_info info{tc.make_ident_info(arg)};

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

        // an inlined recursion ends at compile time when a constant argument
        // makes its condition fold, otherwise it would expand without end
        constexpr size_t max_inlined_nesting{64};

        if (tc.scopes().inlined_nesting(func.name()) >= max_inlined_nesting) {
            throw inlining_limit_error(std::format(
                "recursion of inlined function '{}' does not end at compile "
                "time, declare it 'noinline'",
                func.name()));
        }

        const size_t frame_count{tc.scopes().frames().size()};

        tc.reach_frame_check(frame_count);

        if (frame_count < call_site::max_frame_count) {
            return;
        }

        // note: the chain does not depend on the arguments, see
        //       'noinline_call::check_aliasing'

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
    // '--checks=alias'
    auto assert_no_shared_storage(const toc& tc, const destination& dst,
                                  const stmt_def_func& func) const -> void {

        const bool is_result_checked{
            tc.checks().alias and dst.is_storage(),
        };

        // note: an empty target, the result is not compared, names no variable
        storage_target result;

        if (is_result_checked) {
            result = call_site::result_target(tc, tok(), dst);
        }

        std::vector<reference> references;

        for (size_t i{}; i < args_.size(); ++i) {
            const expr_any& arg{args_.at(i)};

            if (call_site::is_copied_value(arg)) {
                continue;
            }

            // constants pass their value
            if (not tc.make_ident_info(arg).is_var()) {
                continue;
            }

            storage_target target{call_site::argument_target(tc, arg)};

            if (is_result_checked) {
                assert_not_shared_with_result(i, target, dst, result);
            }

            const bool is_read_only{func.param(i).is_read_only()};

            assert_not_shared_with_earlier(references, i, target, is_read_only);

            references.push_back({
                .index{i},
                .target{std::move(target)},
                .is_read_only{is_read_only},
            });
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

        throw compile_error{tz, std::format("'{}' takes {}", func_name_,
                                            counted(param_count, "argument"))};
    }

    // an argument that names the storage of an earlier one, unless both
    // parameters are read-only or the bytes they reach are disjoint
    auto assert_not_shared_with_earlier(
        const std::span<const reference> earlier, const size_t index,
        const storage_target& target, const bool is_read_only) const -> void {

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
                            describe_argument(index), arg.name_or_literal(),
                            describe_argument(other.index),
                            args_.at(other.index).name_or_literal(),
                            target.display_root(), use_temporary_fix_)};
        }
    }

    // an argument that names the result destination, only for '--checks=alias'
    auto assert_not_shared_with_result(const size_t index,
                                       const storage_target& target,
                                       const destination& dst,
                                       const storage_target& result) const
        -> void {

        if (not target.may_overlap(result)) {
            return;
        }

        throw compile_error{
            args_.at(index).tok(),
            std::format("{} '{}' may share storage with the "
                        "result destination '{}' (both name '{}'), {}",
                        describe_argument(index),
                        args_.at(index).name_or_literal(), dst.name(),
                        target.display_root(), use_temporary_fix_)};
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

        if (constructor_name_tk_.text().empty()) {
            throw compile_error{tz, "expected constructor name after '.'"};
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

        for (size_t i{}; i < args_.size(); ++i) {
            assert_argument_usable(tc, args_.at(i), params.at(i));
            assert_argument_writable(tc, i, func);
        }
    }

    // a built-in has no parameter list, so any count of default type
    // arguments is parsed and the built-in checks the count
    auto parse_builtin_arguments(toc& tc, tokenizer& tz) -> void {
        bool expect_arg{};
        while (true) {
            close_paren_tk_ = tz.is_next_char_token(')');

            if (not close_paren_tk_.is_empty()) {
                if (expect_arg) {
                    throw compile_error{close_paren_tk_,
                                        "expected argument after ','"};
                }

                return;
            }

            args_.emplace_back(tc, tz, tc.builtins().default_type(),
                               literal_typing::from_destination);

            const token delim_tk{tz.is_next_char_token(',')};
            expect_arg = not delim_tk.is_empty();

            if (expect_arg) {
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

    // what 'inline_call' and 'noinline_call' need of this call
    [[nodiscard]] auto site() const -> call_site {
        return {
            .tk{tok()},
            .begin_tk{call_begin_token()},
            .close_paren_tk{close_paren_tk_},
            .source{statement::trimmed_source(*this)},
            .args{args_},
            .uops{get_unary_ops()},
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
                    tc.make_ident_info(arg).is_array(),
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

        // the alias would make the parameter name the whole array
        if (arg.is_name_or_literal()) {
            toc::assert_not_whole_array(arg, tc.make_ident_info(arg));
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

        name = std::format("{}.{}",
                           tc.get_type_or_throw(type_tk, type_tk.text()).name(),
                           constructor_name_tk.text());

        return name;
    }

    // the type as written, with the type it is bound to when that differs,
    // e.g. 'T (point)'
    [[nodiscard]] static auto type_description(const toc& tc,
                                               const token& type_tk)
        -> std::string {

        std::string description{type_tk.text()};

        if (tc.types().has(type_tk.text())) {
            const std::string_view bound{
                tc.get_type_or_throw(type_tk, type_tk.text()).name(),
            };

            if (bound != type_tk.text()) {
                description += std::format(" ({})", bound);
            }
        }

        return description;
    }
};
