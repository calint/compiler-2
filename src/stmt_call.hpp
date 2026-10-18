#pragma once
// reviewed: 2025-09-28
//           2025-10-08
//           2026-09-08

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

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "expression.hpp"
#include "generics.hpp"
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
//   'call_site'       what compiling the call needs of it
//   'inline_call'     compiles the body of the function in place
//   'noninline_call'  passes addresses to the body that is emitted once
//
// the formatter sorts the functions of a class by name, so the comment above
// each class lists them in the order they run
//

// the instance of a generic function that a call names, its type arguments
// written, e.g. 'tz.to<name>()', or deduced from the arguments, e.g. 'f(x)' of
// 'func f<T type>(s T)', see 'etc/docs/generic-types.txt' section 10
//
// 'parse' reads '<...>' with 'written_type_arguments' or, without '<',
// deduces the types with 'deduced_type_arguments', which looks ahead at the
// arguments with 'argument_type' and 'value_type'; then the instance is made
struct generic_call {
    // the name of the instance
    std::string func_name;
    // e.g. '<', 'name' and '>' in 'tz.to<name>()', empty when deduced
    std::vector<token> generic_tks;

    //
    // statics
    //

    // the instance of a generic function, e.g. '<name>' in 'tz.to<name>()'
    // without '<' the types are taken from the arguments. 'tz' is after the
    // name and before the '<' or the '('
    // note: 'name_tk' is the name as written, e.g. 'to', and 'generic_name'
    //       the generic function, e.g. 'tokenizer.to'
    [[nodiscard]] static auto
    parse(toc& tc, tokenizer& tz, const token& name_tk,
          const std::string_view generic_name, const type* const expected_type)
        -> generic_call {

        const token open_tk{tz.is_next_char_token('<')};

        if (open_tk.is_empty()) {
            const std::vector<const type*> type_args{
                deduced_type_arguments(tc, tz, name_tk, generic_name,
                                       expected_type),
            };

            return {
                .func_name{
                    stmt_def_func_generic::instantiate_func(
                        tc, name_tk, generic_name, type_args),
                },
                .generic_tks{},
            };
        }

        const generic_arg_tokens args{generic_arg_tokens::parse(tz, open_tk)};

        const std::vector<const type*> type_args{
            written_type_arguments(tc, generic_name, args.arg_tks),
        };

        return {
            .func_name{
                stmt_def_func_generic::instantiate_func(
                    tc, name_tk, generic_name, type_args),
            },
            .generic_tks{args.list_tks},
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

        if (not tc.is_var_or_alias(tk.text())) {
            return value_type(tc, tz, tk);
        }

        const stmt_identifier si{tc, {}, tk, tz};

        const char next{tz.peek_char_after_whitespace()};

        if (si.is_method_receiver() or (next != ',' and next != ')')) {
            return nullptr;
        }

        return &tc.make_ident_info(si).type_ref();
    }

    // the type arguments of 'f(x)' for 'func f<T type>(s T)': a type parameter
    // is the type of the argument for a parameter declared with that type, when
    // the argument is a variable, or the type the call is assigned to when it
    // is the result. a literal or an expression does not tell its type
    [[nodiscard]] static auto
    deduced_type_arguments(toc& tc, const tokenizer& tz, const token& name_tk,
                           const std::string_view generic_name,
                           const type* const expected_type)
        -> std::vector<const type*> {

        const generic_func_info& generic{tc.generics().get_func(generic_name)};

        // the arguments are looked at ahead, the call reads them later
        tokenizer scan{tz};

        if (scan.is_next_char_token('(').is_empty()) {
            throw compiler_exception{
                name_tk, std::format("generic function '{}' needs type "
                                     "arguments, e.g. '{}<...>'",
                                     generic_name, name_tk.text())};
        }

        std::vector<const type*> type_args;

        for (const auto [param_tk, deduction] :
             std::views::zip(generic.param_tks, generic.deductions)) {

            const type* arg_type{
                deduction.param_index
                    ? argument_type(tc, scan, *deduction.param_index)
                    : nullptr,
            };

            if (arg_type == nullptr and deduction.is_result) {
                arg_type = expected_type;
            }

            if (arg_type == nullptr) {
                throw compiler_exception{
                    name_tk,
                    std::format("cannot deduce '{}' of generic function '{}', "
                                "write '{}<...>'",
                                param_tk.text(), generic_name, name_tk.text())};
            }

            type_args.push_back(arg_type);
        }

        return type_args;
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

        const bool is_type{tc.has_type(tk.text())};

        // e.g. 'point{1, 2}'
        if (is_type and tz.peek_char_after_whitespace() == '{') {
            tz.skip_braced_block();

            return is_whole_argument() ? &tc.get_type_or_throw(tk, tk.text())
                                       : nullptr;
        }

        // e.g. 'point.at(1, 2)'
        if (is_type and not tz.is_next_char_token('.').is_empty()) {
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
                       not return_type.is_same(tc.get_type_void())
                   ? &return_type
                   : nullptr;
    }

    // the types between '<' and '>', e.g. 'name' of 'tz.to<name>()'
    [[nodiscard]] static auto
    written_type_arguments(const toc& tc, const std::string_view generic_name,
                           const std::span<const token> arg_tks)
        -> std::vector<const type*> {

        const std::span<const token> param_tks{
            tc.generics().get_func(generic_name).param_tks,
        };

        std::vector<const type*> type_args;
        type_args.reserve(arg_tks.size());

        for (const auto [ix, type_tk] : std::views::enumerate(arg_tks)) {
            // a surplus argument is reported with the count
            if (std::cmp_less(ix, param_tks.size())) {
                const std::string_view param_name{
                    param_tks.at(static_cast<size_t>(ix)).text(),
                };

                type_args.emplace_back(
                    &resolve_type_arg(tc, generic_name, param_name, type_tk));
            } else {
                type_args.emplace_back(
                    &tc.get_type_or_throw(type_tk, type_tk.text()));
            }
        }

        return type_args;
    }
};

// what a non-inline call passes for an argument: the variable it names or
// a temporary that holds its value
struct noninline_arg {
    ident_info info;
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

    // the error was found inside the inlined body, unless it is in the call
    // itself where the call is already the error location
    auto add_call_frame(compiler_exception& e) const -> void {
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
                       .pointer_register{},
                       .base_register{},
                       .value_register{},
                       .foo_array{},
                   },
                   var_kind::var);

        return tc.make_ident_info(tk, name);
    }

    auto assert_result_type(const ident_info& dst_info,
                            const stmt_def_func& func) const -> void {

        if (dst_info.type_ref().is_same(func.get_type())) {
            return;
        }

        const std::string message{
            std::format("result type mismatch: function returns '{}', "
                        "destination is '{}'",
                        func.get_type().name(), dst_info.type_ref().name()),
        };

        // a non-inline result needs a memory destination, not the register
        // the conversion computes into
        if (not func.is_inlined()) {
            throw compiler_exception{dst_info.error_token(tk), message};
        }

        throw compiler_exception{
            dst_info.error_token(tk),
            std::format("{}, use '{}(...)'", message,
                        dst_info.type_ref().name()),
        };
    }

    // a result must be stored and a call without one cannot provide it
    auto assert_result_use(const ident_info& dst_info,
                           const stmt_def_func& func) const -> void {

        if (func.returns() and dst_info.is_empty()) {
            throw compiler_exception{tk, "return value is discarded"};
        }

        if (not func.returns() and not dst_info.is_empty()) {
            throw compiler_exception{tk, "function does not return a value"};
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
                                    ident_info::root_of(arg.identifier()),
                                    arg.accessed_path());
    }

    // an instance literal or a call result, e.g. 'point{1, 2}' or 'mk(1, 2)'
    [[nodiscard]] static auto
    is_instance_temporary(const expr_any& arg, const stmt_def_func_param& param)
        -> bool {

        return not param.get_type().is_builtin() and not arg.is_identifier();
    }

    // the root variable and the bytes of it that the destination names
    [[nodiscard]] static auto result_target(const toc& tc,
                                            const token& src_loc_tk,
                                            const ident_info& dst_info)
        -> storage_target {

        // note: a register destination, its id is a register name, has no
        //       variable
        if (dst_info.is_register()) {
            return {};
        }

        return tc.storage_target_of(src_loc_tk,
                                    ident_info::root_of(dst_info.id),
                                    dst_info.accessed_path);
    }

    [[noreturn]] static auto
    throw_parameter_type_mismatch(const expr_any& arg,
                                  const stmt_def_func_param& param,
                                  const ident_info& info) -> void {

        throw compiler_exception{
            arg.tok(),
            std::format("parameter '{}': required '{}', got '{}'", param.name(),
                        param.get_type().name(), info.type_ref().name())};
    }

  private:
    [[nodiscard]] auto is_inside_call(const compiler_exception& e) const
        -> bool {

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
// a 'noninline_call' runs step 3 as a dry run to check its aliasing
class inline_call {
    call_site call_;

  public:
    explicit inline_call(call_site call) : call_{std::move(call)} {}

    // an instance literal or a call result is made in a variable of the
    // parameter's type that the body reaches through the alias of its
    // parameter
    auto compile(toc& tc, const size_t indent, const ident_info& dst_info,
                 const stmt_def_func& func) const -> void {

        const bool has_instance_temporary{
            std::ranges::any_of(std::views::zip(call_.args, func.params()),
                                [](const auto& arg_and_param) -> bool {
                                    return call_site::is_instance_temporary(
                                        std::get<0>(arg_and_param),
                                        std::get<1>(arg_and_param));
                                }),
        };

        if (not has_instance_temporary) {
            compile_to_destination(tc, indent, dst_info, func);
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

            arg.compile(tc, indent, temporary_info);
        }

        compile_to_destination(tc, indent, dst_info, func);

        tc.exit_block();
    }

    // 'call_arguments' are those of a 'noinline' call that is only checked,
    // the body sees an argument that went through a temporary as that
    // temporary
    auto compile_with_aliases(
        toc& tc, const size_t indent, const ident_info& dst_info,
        const stmt_def_func& func,
        const std::span<const noninline_arg> call_arguments = {}) const
        -> void {

        // scratch registers stay allocated until the inlined body is compiled
        std::vector<operand> allocated_registers;

        const std::vector<alias_info> aliases{
            make_aliases(tc, indent, dst_info, func, call_arguments,
                         allocated_registers),
        };

        // the result alias has its own address choice in
        // 'compile_to_destination'
        // note: it comes first, if any
        const size_t first_argument_alias{aliases.size() - call_.args.size()};

        const bool has_indexed_argument{
            std::ranges::any_of(aliases |
                                    std::views::drop(first_argument_alias),
                                is_indexed_reference),
        };

        machine const& x{tc.machine()};

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

        const ident_info& ret_info{
            tc.make_ident_info(call_.tk, return_info.ident_tk.text()),
        };

        call_.uops.compile(tc, indent, call_.tk, ret_info.operand);
    }

    // the id has no indexes, so an id naming an array means the result is
    // one of its elements
    auto comment_result_address(toc& tc, const size_t indent,
                                const ident_info& dst_info) const -> void {

        machine& x{tc.machine()};

        if (tc.make_ident_info(call_.tk, dst_info.id).is_array) {
            x.comment(call_.tk, indent,
                      "address of result element in array '{}'", dst_info.id);

            return;
        }

        x.comment(call_.tk, indent, "address of indexed result '{}'",
                  dst_info.id);
    }

    // the registers are freed before the exit label
    auto compile_body(toc& tc, const size_t indent, const stmt_def_func& func,
                      const std::span<const alias_info> aliases_to_add,
                      const std::span<const operand> registers_to_free) const
        -> void {

        machine& x{tc.machine()};

        // create unique labels for inlined functions
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

        // enter function scope

        tc.enter_func(func.name(), new_call_path, ret_jmp_label);

        const machine::call_frame_scope call_frame{
            x,
            call_.tk,
            std::string{func.name()},
        };

        func.add_constants(tc, indent + 1);

        // add aliases
        for (const alias_info& e : aliases_to_add) {
            x.comment_alias(call_.tk, indent + 1, e.from, e.to, e.lea);
            tc.add_alias(e);
        }

        // the result reaches the destination through the 'res' alias, so the
        // statements of the body have no destination
        func.code().compile(tc, indent, ident_info::make_empty());

        x.free_scratch_registers(call_.tk, indent + 1, registers_to_free);

        // provide the exit label for 'return' to jump to

        x.label(indent, ret_jmp_label);

        // apply unary ops to result if present

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

            const operand address{
                x.alloc_scratch_register(call_.tk, indent,
                                         tc.get_type_address()),
            };

            address_registers.push_back(address);

            x.comment(call_.tk, indent, "address of parameter '{}'",
                      alias.from);

            alias.lea = load_indexed_address(x, indent, address, alias.lea,
                                             keeps_displacement);
        }

        compile_body(tc, indent, func, aliases_to_add, address_registers);
    }

    auto compile_to_destination(toc& tc, const size_t indent,
                                const ident_info& dst_info,
                                const stmt_def_func& func) const -> void {

        if (not dst_info.operand.is_memory()) {
            compile_with_aliases(tc, indent, dst_info, func);
            return;
        }

        machine& x{tc.machine()};

        // the result is written to memory by the body of the function
        const bool has_unary_ops{not call_.uops.is_empty()};

        const bool has_indexed_result{
            not dst_info.operand.index_register().empty(),
        };

        // without base + index addressing each result access adds base and
        // index again, computing the address once can be shorter
        if (not has_unary_ops and has_indexed_result and
            not x.has_indexed_addressing()) {

            x.emit_most_efficient(
                call_.tk, indent,
                [&] -> void {
                    compile_with_aliases(tc, indent, dst_info, func);
                },
                [&] -> void {
                    emit_most_efficient_address(
                        x, indent, [&](const bool keeps) -> void {
                            compile_with_result_address_register(
                                tc, indent, dst_info, func, keeps);
                        });
                });

            return;
        }

        if (not has_unary_ops) {
            compile_with_aliases(tc, indent, dst_info, func);
            return;
        }

        // unary ops on a memory result are a load, modify and store on a
        // load/store machine, a scratch register can be shorter
        x.emit_most_efficient(
            call_.tk, indent,
            [&] -> void { compile_with_aliases(tc, indent, dst_info, func); },
            [&] -> void {
                const operand reg{
                    x.alloc_scratch_register(call_.tk, indent,
                                             dst_info.type_ref()),
                };

                compile_with_aliases(
                    tc, indent, toc::make_ident_info_from_register(reg), func);

                x.copy_value(call_.tk, indent, dst_info.operand, reg);
                x.free_scratch_register(call_.tk, indent, reg);
            });
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

        x.emit_most_efficient(
            call_.tk, indent,
            [&] -> void { compile_body(tc, indent, func, aliases, {}); },
            [&] -> void {
                emit_most_efficient_address(
                    x, indent, [&](const bool keeps) -> void {
                        compile_body_with_address_registers(
                            tc, indent, func, aliases, first_argument_alias,
                            keeps);
                    });
            });

        // freed after both versions since both use the argument registers
        x.free_scratch_registers(call_.tk, indent + 1, allocated_registers);
    }

    // a new register because the index register may belong to an enclosing
    // alias that is used after the call
    auto compile_with_result_address_register(
        toc& tc, const size_t indent, const ident_info& dst_info,
        const stmt_def_func& func, const bool keeps_displacement) const
        -> void {

        machine& x{tc.machine()};

        const operand address{
            x.alloc_scratch_register(call_.tk, indent, tc.get_type_address()),
        };

        comment_result_address(tc, indent, dst_info);

        ident_info address_info{dst_info};

        address_info.operand = load_indexed_address(
            x, indent, address, dst_info.operand, keeps_displacement);

        compile_with_aliases(tc, indent, address_info, func);

        x.free_scratch_register(call_.tk, indent, address);
    }

    // the address register holds the address with or without the
    // displacement, the first is shorter when the accesses exceed the load
    // and store immediate range
    auto emit_most_efficient_address(
        machine& x, const size_t indent,
        const std::function_ref<void(bool keeps_displacement)> emit) const
        -> void {

        x.emit_most_efficient(
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
    make_aliases(toc& tc, const size_t indent, const ident_info& dst_info,
                 const stmt_def_func& func,
                 const std::span<const noninline_arg> call_arguments,
                 std::vector<operand>& allocated_registers) const
        -> std::vector<alias_info> {

        assert(call_arguments.empty() or
               call_arguments.size() == call_.args.size());
        // note: 'call_arguments.at(i)' below needs one entry per argument

        call_.assert_result_use(dst_info, func);

        // buffer the aliases of arguments and function return
        std::vector<alias_info> aliases;

        // the result names the destination, so the widths must agree
        if (const std::optional<func_return_info> ret{func.returns()}; ret) {
            call_.assert_result_type(dst_info, func);

            aliases.push_back(make_result_alias(tc, dst_info, *ret));
        }

        for (const auto [i, arg, param] : std::views::zip(
                 std::views::iota(size_t{}), call_.args, func.params())) {

            // note: 'compile' made its temporary
            // a 'noinline' call made its own, the dry run must not name the
            // register of the caller that the body does not see
            const bool is_call_temporary{
                not call_arguments.empty() and
                    call_arguments.at(i).is_temporary,
            };

            if (is_call_temporary or
                call_site::is_instance_temporary(arg, param)) {

                aliases.push_back(
                    make_value_alias(param, call_.temporary_argument_name(i)));

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

        return alias.lea.is_memory() and not alias.lea.index_register().empty();
    }

    // the inlined body reaches an argument through its storage, its constant
    // value or a scratch register holding its value
    [[nodiscard]] static auto
    make_argument_alias(toc& tc, const size_t indent, const expr_any& arg,
                        const stmt_def_func_param& param,
                        std::vector<operand>& allocated_registers)
        -> alias_info {

        const bool is_reference{
            not arg.is_expression() and (arg.is_indexed() or tc.has_lea(arg)),
        };

        if (is_reference and not arg.get_unary_ops().is_empty()) {
            throw compiler_exception{
                arg.tok(),
                "unary operators on reference arguments are unsupported"};
        }

        // an alias uses the argument's storage so its type must match
        if (not arg.is_expression() and arg.get_unary_ops().is_empty()) {
            assert_alias_type(tc, arg, param);
        }

        if (is_reference) {
            return make_reference_alias(tc, indent, arg, param,
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
            return make_value_alias(param, std::format("{}", *value));
        }

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register(arg.tok(), indent, param.get_type()),
        };

        allocated_registers.push_back(reg);
        arg.compile(tc, indent, toc::make_ident_info_from_register(reg));

        return alias_info::make_register(param.identifier(), param.get_type(),
                                         reg);
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
            return make_value_alias(
                param, std::format("{}", arg.get_unary_ops().evaluate_constant(
                                             arg_info.const_value)));
        }

        if (arg.get_unary_ops().is_empty()) {
            return make_value_alias(param, arg.identifier(),
                                    call_site::argument_target(tc, arg));
        }

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register(arg.tok(), indent, param.get_type()),
        };

        allocated_registers.push_back(reg);
        x.copy_value(param.tok(), indent, reg, arg_info.operand);
        arg.get_unary_ops().compile(tc, indent, arg.tok(), reg);

        return alias_info::make_register(param.identifier(), param.get_type(),
                                         reg);
    }

    // an indexed argument keeps its computed address, e.g. [rbp + r14 * 4 +
    // 205] or [r15 + r14] or simply [r15]
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
                                .lea_path{arg_info.lea_path},
                                .address_register{},
                            }),
        };

        // the address registers stay allocated until the inlined body is
        // compiled
        allocated_registers.append_range(regs_lea);

        return {
            .from{std::string{param.identifier()}},
            .to{std::string{arg.identifier()}},
            .lea{lea},
            .type_ptr{&param.get_type()},
            .register_operand{},
            .is_element{arg.is_array_element()},
            .bound{call_site::argument_target(tc, arg)},
        };
    }

    // the result name of the function refers to the destination
    [[nodiscard]] static auto make_result_alias(const toc& tc,
                                                const ident_info& dst_info,
                                                const func_return_info& ret)
        -> alias_info {

        operand dst_lea{
            dst_info.use_operand or dst_info.has_lea() ? dst_info.operand
                                                       : operand{},
        };

        // a destination such as 'arr[1]' is not an array
        return {
            .from{std::string{ret.ident_tk.text()}},
            .to{dst_info.id},
            .lea{std::move(dst_lea)},
            .type_ptr{ret.type_ptr},
            .register_operand{
                dst_info.is_register() ? dst_info.operand : operand{},
            },
            .is_element{not dst_info.is_array},
            .bound{call_site::result_target(tc, ret.ident_tk, dst_info)},
        };
    }

    [[nodiscard]] static auto make_value_alias(const stmt_def_func_param& param,
                                               const std::string_view to,
                                               const storage_target& bound = {})
        -> alias_info {

        return {
            .from{std::string{param.identifier()}},
            .to{std::string{to}},
            .lea{},
            .type_ptr{&param.get_type()},
            .register_operand{},
            .is_element{},
            .bound{bound},
        };
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
//      'etc/docs/noinline-alias-dry-run.txt'), the addresses are written to
//      the callee frame ('frame_slot_addresses', 'write_frame_slots') and the
//      function is called
class noninline_call {
    call_site call_;

  public:
    explicit noninline_call(call_site call) : call_{std::move(call)} {}

    // the callee reaches the result and the arguments through their addresses,
    // a result destination or an argument that is not a variable in memory,
    // e.g. a register or an expression, goes through a temporary
    auto compile(toc& tc, const size_t indent, const ident_info& dst_info,
                 const stmt_def_func& func) const -> void {

        call_.assert_result_use(dst_info, func);

        const bool has_result_temporary{
            func.returns() and not dst_info.operand.is_memory(),
        };

        const bool has_argument_temporary{
            std::ranges::any_of(std::views::zip(call_.args, func.params()),
                                [&](const auto& arg_and_param) -> bool {
                                    return needs_temporary(
                                        tc, std::get<0>(arg_and_param),
                                        std::get<1>(arg_and_param));
                                }),
        };

        if (not has_result_temporary and not has_argument_temporary) {
            compile_with_arguments(tc, indent, dst_info, func,
                                   variable_arguments(tc));

            apply_unary_ops_to_destination(tc, indent, dst_info);

            return;
        }

        // the block frees the temporaries after the call
        tc.enter_block();

        std::vector<noninline_arg> arguments;

        for (const auto [i, arg, param] : std::views::zip(
                 std::views::iota(size_t{}), call_.args, func.params())) {

            if (not needs_temporary(tc, arg, param)) {
                arguments.push_back({
                    .info{tc.make_ident_info(arg)},
                    .is_temporary{},
                });

                continue;
            }

            arguments.push_back({
                .info{
                    call_.add_temporary(tc, indent,
                                        call_.temporary_argument_name(i),
                                        param.get_type()),
                },
                .is_temporary{true},
            });

            arg.compile(tc, indent, arguments.back().info);
        }

        if (has_result_temporary) {
            const ident_info result_info{
                call_.add_temporary(
                    tc, indent, toc::temporary_name(call_.tk, "call-result", 0),
                    func.get_type()),
            };

            compile_with_arguments(tc, indent, result_info, func, arguments);

            machine& x{tc.machine()};

            x.copy_value(call_.tk, indent, dst_info.operand,
                         result_info.operand);
        } else {
            compile_with_arguments(tc, indent, dst_info, func, arguments);
        }

        apply_unary_ops_to_destination(tc, indent, dst_info);

        tc.exit_block();
    }

  private:
    // the callee has written the result, the operators of the call follow
    auto apply_unary_ops_to_destination(toc& tc, const size_t indent,
                                        const ident_info& dst_info) const
        -> void {

        call_.uops.compile(tc, indent, call_.tk, dst_info.operand);
    }

    // the callee reaches the result and arguments through their addresses
    auto assert_call(const ident_info& dst_info, const stmt_def_func& func,
                     const std::span<const noninline_arg> arguments) const
        -> void {

        call_.assert_result_use(dst_info, func);

        if (func.returns()) {
            // note: a result that is not in memory goes through a temporary
            assert(dst_info.operand.is_memory());

            // an array literal gives its call elements an element destination
            assert(not dst_info.is_array);

            call_.assert_result_type(dst_info, func);
        }

        for (const auto [arg, argument, param] :
             std::views::zip(call_.args, arguments, func.params())) {

            assert_argument(arg, argument.info, param);
        }
    }

    // the body is compiled once for all callers, so this call's aliasing is
    // checked by compiling the body as an inline call and dropping the code
    auto check_aliasing(toc& tc, const size_t indent,
                        const ident_info& dst_info, const stmt_def_func& func,
                        const std::span<const noninline_arg> arguments) const
        -> void {

        const std::optional<std::string> signature{
            aliasing_signature(tc, dst_info, func, arguments),
        };

        // note: what the body can reject depends only on which arguments are
        //       the same variable, e.g. 'f(v, v, n)' but not 'f(v, w, n)',
        //       and on which are a global, since the body may name that
        //       global too, e.g. 'f(v, g, n)'; so 'f(v, w, n)' and
        //       'f(x, y, z)' give the same result and are checked once
        //
        //       the call is added before its dry run, so the same kind of
        //       call reached inside it, e.g. a recursive call, returns here;
        //       the kinds are finitely many, so the nested dry runs end

        if (not signature or not tc.add_checked_noninline_call(*signature)) {
            return;
        }

        const inline_call dry_run{call_};

        try {
            tc.check_only([&] -> void {
                dry_run.compile_with_aliases(tc, indent, dst_info, func,
                                             arguments);
            });
        } catch (compiler_exception& e) {
            call_.add_call_frame(e);
            throw;
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

    auto compile_with_arguments(
        toc& tc, const size_t indent, const ident_info& dst_info,
        const stmt_def_func& func,
        const std::span<const noninline_arg> arguments) const -> void {

        assert_call(dst_info, func, arguments);
        check_aliasing(tc, indent, dst_info, func, arguments);

        machine& x{tc.machine()};

        const std::vector<size_t> array_lengths{
            array_argument_lengths(func, arguments),
        };

        // a body is emitted for a function that is called, once for all calls
        // without array parameters
        tc.add_noninline_instance(func, array_lengths);

        // the registers stay allocated until the callee frame is populated
        std::vector<operand> address_registers;

        const std::vector<operand> addresses{
            frame_slot_addresses(tc, indent, dst_info, func, arguments,
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
        //           | result address         | 8 bytes
        //           +------------------------+
        //           | argument address       | 8 bytes
        //           +------------------------+
        //           | callee's locals        |
        //           +------------------------+
        //
        // the base is the frame base register, x86_64 'rbx' and rv32i 's1';
        // root calls use the variables base register instead, x86_64 'rbp' and
        // rv32i 's0'
        const operand frame_address{tc.next_frame_address()};

        x.check_frame_capacity(
            call_.tk, indent, frame_address,
            operand::imm(func.frame_size_label(array_lengths),
                         tc.get_type_address()),
            tc.is_frame_check());

        // the registers holding the addresses of the arguments are still
        // allocated, the check must not change a register that is free but
        // used by the call
        x.check_stack_capacity(call_.tk, indent, tc.is_stack_check());

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
    frame_slot_addresses(toc& tc, const size_t indent,
                         const ident_info& dst_info, const stmt_def_func& func,
                         const std::span<const noninline_arg> arguments,
                         std::vector<operand>& address_registers) const
        -> std::vector<operand> {

        std::vector<operand> addresses;

        if (func.returns()) {
            // note: 'stmt_assign_var' resolves a pointer slot before the call
            assert(not dst_info.is_pointer or dst_info.use_operand);

            addresses.push_back(dst_info.operand);
        }

        for (const auto [arg, argument] :
             std::views::zip(call_.args, arguments)) {

            // a temporary is a plain variable, not indexed like its argument
            if (argument.is_temporary) {
                addresses.push_back(argument.info.operand);
                continue;
            }

            addresses.push_back(tc.get_lea_operand(indent, arg, argument.info,
                                                   address_registers));
        }

        return addresses;
    }

    [[nodiscard]] auto variable_arguments(const toc& tc) const
        -> std::vector<noninline_arg> {

        std::vector<noninline_arg> arguments;
        arguments.reserve(call_.args.size());

        for (const expr_any& arg : call_.args) {
            arguments.push_back({
                .info{tc.make_ident_info(arg)},
                .is_temporary{},
            });
        }

        return arguments;
    }

    // write pointers into the callee frame: result (if any), then arguments
    // each slot holds an address, not the value stored at that address
    auto write_frame_slots(machine& x, const size_t indent,
                           const stmt_def_func& func,
                           const operand& frame_address,
                           const std::vector<operand>& addresses,
                           const std::optional<size_t> register_slot) const
        -> void {

        operand slot{frame_address};

        for (const auto [i, addr] : std::views::enumerate(addresses)) {
            comment_frame_slot(x, indent, func, static_cast<size_t>(i));

            // the callee finds it in a register, it has no slot
            if (std::cmp_equal(i, register_slot.value_or(addresses.size()))) {
                continue;
            }

            x.address_of(call_.tk, indent, slot, addr);

            slot.increment_offset(address_offset(x.address_size_bytes()));
        }
    }

    //
    // statics
    //

    // the callee and which arguments are globals or the same local, the only
    // facts the aliasing checks of the body depend on. empty when an argument
    // is a parameter of the enclosing non-inline function, the calls of that
    // function are checked with their own arguments
    [[nodiscard]] static auto
    aliasing_signature(const toc& tc, const ident_info& dst_info,
                       const stmt_def_func& func,
                       const std::span<const noninline_arg> arguments)
        -> std::optional<std::string> {

        std::vector<ident_info> infos;

        if (func.returns()) {
            infos.push_back(dst_info);
        }

        for (const noninline_arg& argument : arguments) {
            infos.push_back(argument.info);
        }

        std::string signature{func.name()};
        std::vector<std::string_view> locals;

        for (const ident_info& info : infos) {
            const std::optional<std::string> part{
                signature_part(tc, info, locals),
            };

            if (not part) {
                return std::nullopt;
            }

            signature += *part;
        }

        // the body is checked with the value, e.g. a constant, of the first
        // call of its kind
        for (const noninline_arg& argument : arguments) {
            signature += argument.is_temporary ? " temporary" : " variable";
        }

        return signature;
    }

    // the body is compiled for the lengths of the array arguments
    [[nodiscard]] static auto
    array_argument_lengths(const stmt_def_func& func,
                           const std::span<const noninline_arg> arguments)
        -> std::vector<size_t> {

        std::vector<size_t> lengths;

        for (const auto [argument, param] :
             std::views::zip(arguments, func.params())) {

            if (param.is_array()) {
                lengths.push_back(argument.info.array_len);
            }
        }

        return lengths;
    }

    // a non-inline call passes addresses, a value without storage is passed
    // in a temporary of the parameter's type
    static auto assert_argument(const expr_any& arg, const ident_info& info,
                                const stmt_def_func_param& param) -> void {

        // note: a temporary is a variable
        assert(info.is_var());

        // note: parsing the call rejects an array argument for a non-array
        //       parameter and the reverse
        assert(info.is_array == param.is_array());

        if (not info.type_ref().is_same(param.get_type())) {
            call_site::throw_parameter_type_mismatch(arg, param, info);
        }
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

        if (arg.is_expression() or not arg.get_unary_ops().is_empty()) {
            return true;
        }

        return not tc.make_ident_info(arg).is_var();
    }

    // what an argument adds to the signature, e.g. ' local 0' or ' global g',
    // none for a pointer, which the signature cannot tell. 'locals' are the
    // variables met so far
    [[nodiscard]] static auto
    signature_part(const toc& tc, const ident_info& info,
                   std::vector<std::string_view>& locals)
        -> std::optional<std::string> {

        if (info.is_pointer) {
            return std::nullopt;
        }

        std::string part;

        // the body can reject an index by the length of its array
        if (info.is_array) {
            part += std::format(" array {}", info.array_len);
        }

        // note: a temporary is a local of the call
        assert(not info.elem_path.empty());

        const std::string_view root{info.elem_path.front()};

        if (tc.is_global_var(root)) {
            return part + std::format(" global {}", root);
        }

        if (std::ranges::find(locals, root) == locals.end()) {
            locals.push_back(root);
        }

        return part + std::format(" local {}", std::ranges::find(locals, root) -
                                                   locals.begin());
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
// ('assert_no_shared_storage'), then 'inline_call' or 'noninline_call'
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

    static constexpr std::string_view use_temporary_fix{
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

        read_callee(tc, tz, expected_type, "function name");

        if (not tc.is_func_builtin(func_name_)) {
            parse_arguments(tc, tz, tc.get_func_or_throw(tok(), func_name_));
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

        read_callee(tc, tz, expected_type, "method name");

        const stmt_def_func& func{tc.get_func_or_throw(tok(), func_name_)};

        assert_method_receiver(func, receiver);

        add_receiver_argument(std::move(receiver));

        parse_arguments(tc, tz, func);
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

        read_callee(tc, tz, nullptr, "constructor name");

        parse_arguments(tc, tz, func);
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

        if (args_.size() > first) {
            args_.at(first).source_to(os);
            for (const auto [d, e] : std::views::zip(
                     arg_delims_tk_, args_ | std::views::drop(first + 1))) {
                // note: +1 because the first argument is printed above

                d.source_to(os);
                e.source_to(os);
            }
        }

        close_paren_tk_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        if (not tc.is_in_func()) {
            throw compiler_exception{
                tok(), "a global variable cannot be initialized with a call"};
        }

        // note: 'read', 'write' and 'exit' are built-ins with a statement of
        //       their own
        assert(not tc.is_func_builtin(func_name_));

        const call_site call{site()};

        machine& x{tc.machine()};

        x.comment(tok(), indent, call.source);

        const stmt_def_func& func{tc.get_func_or_throw(tok(), func_name_)};

        assert_no_shared_storage(tc, dst_info, func);

        if (not func.is_inlined()) {
            noninline_call{call}.compile(tc, indent, dst_info, func);
            return;
        }

        // an inlined recursion ends at compile time when a constant argument
        // makes its condition fold, otherwise it would expand without end
        constexpr size_t max_inlined_nesting{64};

        if (tc.inlined_nesting(func.name()) >= max_inlined_nesting) {
            throw compiler_exception{
                tok(), std::format("recursion of inlined function '{}' does "
                                   "not end at compile time, declare it "
                                   "'noinline'",
                                   func.name())};
        }

        // each inlined call and block inside it recurses in the compiler
        constexpr size_t max_frame_count{256};

        if (tc.frame_count() >= max_frame_count) {
            throw compiler_exception{
                tok(), std::format("inlined calls are nested too deep, the "
                                   "limit is {} levels, declare a function "
                                   "'noinline'",
                                   max_frame_count)};
        }

        // an error found in the inlined body reports where it was called from
        try {
            inline_call{call}.compile(tc, indent, dst_info, func);
        } catch (compiler_exception& e) {
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

    [[nodiscard]] auto argument_count() const -> size_t { return args_.size(); }

    [[nodiscard]] auto compile_builtin_arguments(
        toc& tc, const size_t indent,
        const std::span<const std::string_view> registers) const
        -> std::vector<operand> {

        assert(registers.size() == args_.size());

        machine& x{tc.machine()};

        std::vector<operand> args;
        args.reserve(registers.size());

        for (size_t index{}; index < registers.size(); ++index) {
            args.push_back(x.alloc_named_register(
                tok(), indent, registers.at(index), tc.get_type_default()));

            argument(index).compile(
                tc, indent, toc::make_ident_info_from_register(args.back()));
        }

        return args;
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

    // an argument reaches its parameter by reference, so a value that has no
    // storage or an array of the wrong shape cannot be passed
    auto assert_argument_usable(const toc& tc, const size_t index,
                                const expr_any& arg,
                                const stmt_def_func_param& param) const
        -> void {

        if (param.is_array()) {
            // an expression is not an array
            const bool is_array_arg{
                arg.is_identifier() and not arg.is_expression() and
                    tc.make_ident_info(arg).is_array,
            };

            // an element would give the parameter the whole array's
            // length, pass the array and a start index instead
            if (not is_array_arg) {
                throw compiler_exception{
                    arg.tok(), std::format("parameter {} requires an array",
                                           index + 1 - first_argument_index())};
                // note: +1 because parameters are numbered from 1
            }

            return;
        }

        // the alias would make the parameter name the whole array
        if (arg.is_identifier()) {
            toc::assert_not_whole_array(arg, tc.make_ident_info(arg));
        }
    }

    // a callee that writes its parameter writes the variable of the caller:
    // a read-only argument is rejected here, also in a function that is never
    // called
    auto assert_argument_writable(const toc& tc, const size_t index,
                                  const stmt_def_func& func) const -> void {

        const expr_any& arg{args_.at(index)};

        // expressions and unary operators pass a copied value
        if (not arg.is_identifier() or arg.is_expression() or
            not arg.get_unary_ops().is_empty()) {

            return;
        }

        // a parameter without 'mut' takes any argument
        if (func.param(index).is_read_only()) {
            return;
        }

        const ident_info info{tc.make_ident_info(arg)};

        // constants pass their value
        if (not info.is_var() or not info.is_read_only()) {
            return;
        }

        if (is_method() and index == 0) {
            throw compiler_exception{
                arg.tok(),
                std::format("read-only '{}' cannot be the receiver of 'mut' "
                            "method '{}'{}",
                            arg.identifier(), func_name_,
                            read_only_hint(info))};
        }

        throw compiler_exception{
            arg.tok(), std::format("read-only '{}' cannot be passed to a "
                                   "'mut' parameter{}",
                                   arg.identifier(), read_only_hint(info))};
    }

    // the function is a method and the receiver is a value
    auto assert_method_receiver(const stmt_def_func& func,
                                const stmt_identifier& receiver) const -> void {

        // a constructor has no receiver, it builds its value
        if (func.is_constructor()) {
            throw compiler_exception{
                tok(), std::format("'{}' is a constructor, call it as "
                                   "'{}(...)'",
                                   func_name_, func_name_)};
        }

        // only an element of an array can be the receiver, 'a[1].f()' but not
        // 'a.f()'
        if (receiver.is_array()) {
            throw compiler_exception{
                receiver.first_token(),
                std::format("receiver of method '{}' cannot be an array",
                            func_name_)};
        }
    }

    // the callee writes a 'mut' parameter in place, so storage shared with
    // another argument could be read after it changed, unless both
    // parameters are read-only. the result destination is only compared with
    // '--checks=alias'
    auto assert_no_shared_storage(const toc& tc, const ident_info& dst_info,
                                  const stmt_def_func& func) const -> void {

        const bool is_result_checked{tc.is_alias_check() and dst_info.is_var()};

        // note: an empty target, the result is not compared, names no variable
        storage_target result;

        if (is_result_checked) {
            result = call_site::result_target(tc, tok(), dst_info);
        }

        std::vector<reference> references;

        for (size_t i{}; i < args_.size(); ++i) {
            const expr_any& arg{args_.at(i)};

            // expressions, instance literals and calls, and unary operators
            // pass a copied value
            if (arg.is_expression() or not arg.is_identifier() or
                not arg.get_unary_ops().is_empty()) {

                continue;
            }

            // constants pass their value
            if (not tc.make_ident_info(arg).is_var()) {
                continue;
            }

            storage_target target{call_site::argument_target(tc, arg)};

            if (is_result_checked) {
                assert_not_shared_with_result(i, target, result);
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

        throw compiler_exception{tz, std::format("'{}' takes {} argument{}",
                                                 func_name_, param_count,
                                                 param_count == 1 ? "" : "s")};
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

            throw compiler_exception{
                arg.tok(),
                std::format("{} '{}' may share storage with {} '{}' "
                            "(both name '{}'), {}",
                            describe_argument(index), arg.identifier(),
                            describe_argument(other.index),
                            args_.at(other.index).identifier(),
                            target.display_root(), use_temporary_fix)};
        }
    }

    // an argument that names the result destination, only for '--checks=alias'
    auto assert_not_shared_with_result(const size_t index,
                                       const storage_target& target,
                                       const storage_target& result) const
        -> void {

        if (not target.may_overlap(result)) {
            return;
        }

        throw compiler_exception{
            args_.at(index).tok(),
            std::format("{} '{}' may share storage with the "
                        "result destination '{}' (both name '{}'), {}",
                        describe_argument(index), args_.at(index).identifier(),
                        result.display_root(), target.display_root(),
                        use_temporary_fix)};
    }

    // 'what' names what precedes the '(' in the error
    auto assert_open_paren_found(const tokenizer& tz,
                                 const std::string_view what) const -> void {

        if (not open_paren_tk_.is_empty()) {
            return;
        }

        throw compiler_exception{tz,
                                 std::format("expected '(' after {}", what)};
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
            throw compiler_exception{tz, "expected constructor name after '.'"};
        }

        if (not tc.is_func(func_name_)) {
            throw compiler_exception{
                constructor_name_tk_,
                std::format("type '{}' has no constructor '{}'",
                            type_description(tc, tok()),
                            constructor_name_tk_.text())};
        }

        const stmt_def_func& func{
            tc.get_func_or_throw(constructor_name_tk_, func_name_),
        };

        if (not func.is_constructor()) {
            throw compiler_exception{
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

    [[nodiscard]] auto is_constructor() const -> bool {
        return not constructor_dot_tk_.is_empty();
    }

    [[nodiscard]] auto is_method() const -> bool {
        return not method_dot_tk_.is_empty();
    }

    // a method receiver is already in 'args_'
    auto parse_arguments(toc& tc, tokenizer& tz, const stmt_def_func& func)
        -> void {

        const std::span<const stmt_def_func_param> params{func.params()};
        const size_t first{args_.size()};

        args_.reserve(params.size());

        for (size_t i{first}; i < params.size(); ++i) {
            // otherwise the missing argument is reported as a parse error of
            // the parameter's type
            // e.g. 'f()' of 'func f(x)'
            if (i == first and tz.peek_char_after_whitespace() == ')') {
                throw_missing_argument(tz, params.at(i), i);
            }

            // e.g. the ',' of 'f(a, b)'
            if (i != first) {
                const token delimiter_tk{tz.is_next_char_token(',')};

                if (delimiter_tk.is_empty()) {
                    throw_missing_argument(tz, params.at(i), i);
                }

                arg_delims_tk_.emplace_back(delimiter_tk);
            }

            // e.g. 'a' and 'b + 1' of 'f(a, b + 1)'
            args_.emplace_back(tc, tz, params.at(i).get_type(), true, false, 0);
        }

        assert_no_surplus_argument(tz, params.size() - first_argument_index());

        // e.g. 'f(a, b' lacks it
        close_paren_tk_ =
            tz.expect_char_token(')', "expected ')' after arguments");

        for (size_t i{}; i < args_.size(); ++i) {
            assert_argument_usable(tc, i, args_.at(i), params.at(i));
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
                    throw compiler_exception{close_paren_tk_,
                                             "expected argument after ','"};
                }

                return;
            }

            args_.emplace_back(tc, tz, tc.get_type_default(), true, false, 0);

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
    // '(', e.g. 'convert<i8>(x)'
    // note: 'func_name_' becomes the name of the instance and the type is its
    //       return type, 'what' names the callee in the error of a missing '('
    auto read_callee(toc& tc, tokenizer& tz, const type* const expected_type,
                     const std::string_view what) -> void {

        if (tc.generics().has_func(func_name_)) {
            const generic_call generic{
                generic_call::parse(tc, tz, tok(), func_name_, expected_type),
            };

            func_name_ = generic.func_name;
            generic_tks_ = generic.generic_tks;
        }

        open_paren_tk_ = tz.is_next_char_token('(');

        set_type(tc.get_func_return_type_or_throw(tok(), func_name_));

        assert_open_paren_found(tz, what);
    }

    // what 'inline_call' and 'noninline_call' need of this call
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

    // e.g. 'foo', 'lst.add' or 'point.at'
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

        throw compiler_exception{tz, std::format("expected {} ('{}')",
                                                 describe_argument(index),
                                                 param.name())};
    }

    //
    // statics
    //

    // 'point.at' for the type named as written, e.g. an alias with constructors
    // of its own; else the name of the type that a type parameter is bound to,
    // e.g. 'T.at' where 'T' is 'point'
    [[nodiscard]] static auto
    constructor_function_name(const toc& tc, const token& type_tk,
                              const token& constructor_name_tk) -> std::string {

        std::string name{
            std::format("{}.{}", type_tk.text(), constructor_name_tk.text()),
        };

        if (tc.is_func(name) or not tc.has_type(type_tk.text())) {
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

        if (tc.has_type(type_tk.text())) {
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
