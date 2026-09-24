#pragma once
// reviewed: 2025-09-28
//           2025-10-08
//           2026-09-08

#include <format>
#include <ranges>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "stmt_def_func.hpp"

class stmt_call : public expression {
    token open_paren_tk_;
    std::vector<expr_any> args_;
    std::vector<token> arg_delims_tk_;
    token close_paren_tk_;

  public:
    stmt_call(toc& tc, unary_ops uops, const token tk,
              const token open_paren_tk, tokenizer& tz)
        : expression{tk, std::move(uops)}, open_paren_tk_{open_paren_tk} {

        set_type(
            tc.get_func_return_type_or_throw(tok(), statement::identifier()));

        if (open_paren_tk_.is_empty()) {
            throw compiler_exception{tok(), "expected '(' after function name"};
        }

        if (not tc.is_func_builtin(statement::identifier())) {

            // user-defined function

            const stmt_def_func& func{
                tc.get_func_or_throw(tok(), statement::identifier())};

            const size_t param_count{func.params().size()};
            args_.reserve(param_count);
            for (const auto [i, param] : std::views::enumerate(func.params())) {
                if (i != 0) {
                    const token t{tz.is_next_char_token(',')};
                    if (t.is_empty()) {
                        throw compiler_exception{
                            tz, std::format("expected argument {} ('{}')",
                                            i + 1, param.name())};

                        // note: +1 because 'i' starts at 0
                    }
                    arg_delims_tk_.emplace_back(t);
                }
                args_.emplace_back(tc, tz, param.get_type(), true, false, 0);
            }

            close_paren_tk_ = tz.is_next_char_token(')');
            if (close_paren_tk_.is_empty()) {
                throw compiler_exception{tz, "expected ')' after arguments"};
            }

            for (const auto [arg_number, arg, param] :
                 std::views::zip(std::views::iota(1), args_, func.params())) {

                if (param.is_array()) {
                    const ident_info arg_info{tc.make_ident_info(arg)};
                    if (not arg_info.is_array) {
                        throw compiler_exception{
                            arg.tok(),
                            std::format("parameter {} requires an array",
                                        arg_number)};
                    }
                }

                // note: the types have been checked prior to getting here so
                //       only check if both argument and parameter are arrays
            }

            return;
        }

        // built-in function

        bool expect_arg{};
        while (true) {
            close_paren_tk_ = tz.is_next_char_token(')');
            if (not close_paren_tk_.is_empty()) {
                if (expect_arg) {
                    throw compiler_exception{close_paren_tk_,
                                             "expected argument after ','"};
                }
                break;
            }
            args_.emplace_back(tc, tz, tc.get_type_default(), true, false, 0);
            const token delim_tk{tz.is_next_char_token(',')};
            expect_arg = not delim_tk.is_empty();
            if (expect_arg) {
                arg_delims_tk_.emplace_back(delim_tk);
            }
        }
    }

    stmt_call() = default;

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
                tok(), indent, registers[index], tc.get_type_default()));

            argument(index).compile(
                tc, indent, toc::make_ident_info_from_register(args.back()));
        }

        return args;
    }

    auto compile_noninline(toc& tc, const size_t indent,
                           const ident_info& dst_info,
                           const stmt_def_func& func) const -> void {

        if (func.returns() and dst_info.is_empty()) {
            throw compiler_exception{tok(), "return value is discarded"};
        }

        if (not func.returns() and not dst_info.is_empty()) {
            throw compiler_exception{tok(), "function does not return a value"};
        }

        if (not get_unary_ops().is_empty()) {
            throw compiler_exception{
                tok(), "unary operators on non-inline calls are unsupported"};
        }

        if (func.returns()) {
            if (not dst_info.operand.is_memory()) {
                throw compiler_exception{
                    tok(), "result destination must be a memory location"};
            }

            if (dst_info.is_array) {
                throw compiler_exception{
                    tok(), "array result destinations are unsupported"};
            }

            if (&dst_info.type_ref() != &func.get_type()) {
                throw compiler_exception{
                    tok(), std::format("result type mismatch: function returns "
                                       "'{}', destination is '{}'",
                                       func.get_type().name(),
                                       dst_info.type_ref().name())};
            }
        }

        for (const auto [arg, param] : std::views::zip(args_, func.params())) {
            if (arg.is_expression()) {
                throw compiler_exception{
                    arg.tok(), "expression arguments are unsupported"};
            }

            if (not arg.get_unary_ops().is_empty()) {
                throw compiler_exception{
                    arg.tok(), "unary operators on arguments are unsupported"};
            }

            const ident_info info{tc.make_ident_info(arg)};
            if (not info.is_var()) {
                throw compiler_exception{arg.tok(),
                                         "argument must be a variable"};
            }

            if (info.is_array and not arg.is_array_element()) {
                throw compiler_exception{
                    arg.tok(), "whole-array arguments are unsupported"};
            }

            if (param.is_array()) {
                throw compiler_exception{arg.tok(),
                                         "array parameters are unsupported"};
            }

            if (not param.get_register_name_or_empty().empty()) {
                throw compiler_exception{
                    arg.tok(),
                    std::format("register-bound parameter '{}' is unsupported "
                                "(register '{}')",
                                param.name(),
                                param.get_register_name_or_empty())};
            }

            if (&info.type_ref() != &param.get_type()) {
                throw compiler_exception{
                    arg.tok(),
                    std::format("parameter '{}': required '{}', got '{}'",
                                param.name(), param.get_type().name(),
                                info.type_ref().name())};
            }
        }

        machine& x{tc.machine()};

        std::vector<operand> address_registers;
        std::vector<operand> addresses;

        if (func.returns()) {
            // start with the result destination
            operand result_address{dst_info.operand};

            // resolve a pointer slot unless the operand is already resolved
            if (dst_info.is_pointer and not dst_info.use_operand) {

                const operand pointer{x.alloc_scratch_register(
                    tok(), indent, tc.get_type_address())};

                // keep the register until the callee frame is populated
                address_registers.push_back(pointer);

                // load the result address into the pointer register
                x.copy_value(
                    tok(), indent, pointer,
                    operand::mem(result_address, tc.get_type_address()));

                // refer to the result storage through the loaded address
                result_address = operand::mem(pointer.base_register(), {}, 1, 0,
                                              dst_info.type_ref());
            }

            // the result address precedes the argument addresses
            addresses.push_back(result_address);
        }

        for (const expr_any& arg : args_) {
            const ident_info info{tc.make_ident_info(arg)};

            addresses.push_back(
                tc.get_lea_operand(indent, arg, info, address_registers));
        }

        // start the callee frame after the caller's storage, not on rsp
        // example: caller uses 24 bytes; callee returns a value and takes one
        // argument
        //
        // rbx      +------------------------+
        //          | caller's storage       | 24 bytes
        // rbx + 24 +------------------------+ <- frame_address; callee's rbx
        //          | result address         | 8 bytes
        //          +------------------------+
        //          | argument address       | 8 bytes
        //          +------------------------+
        //          | callee's locals        |
        //          +------------------------+
        //
        // root calls use rbp instead of rbx as the base
        const operand frame_address{tc.next_frame_address()};

        x.check_frame_capacity(
            tok(), indent, frame_address,
            operand::imm(func.frame_size_label(), tc.get_type_address()),
            "baz_frame_overflow", tc.is_frame_check());

        // write pointers into the callee frame: result (if any), then arguments
        // each slot holds an address, not the value stored at that address
        operand slot{frame_address};

        for (const auto [i, addr] : std::views::enumerate(addresses)) {

            if (func.returns() and i == 0) {
                x.comment(tok(), indent, "result address in callee frame");
            } else {

                const size_t arg_idx{
                    static_cast<size_t>(i - (func.returns() ? 1 : 0))};

                x.comment(tok(), indent,
                          "address of argument '{}' to parameter '{}'",
                          statement::trimmed_source(args_[arg_idx]),
                          func.params()[arg_idx].name());
            }

            x.address_of(tok(), indent, slot, addr);

            slot.increment_offset(address_offset(x.address_size_bytes()));
        }

        x.free_scratch_registers(tok(), indent, address_registers);

        x.call_function(indent, func.body_label(), frame_address);
    }

    auto source_to(std::ostream& os) const -> void override {
        expression::source_to(os);
        open_paren_tk_.source_to(os);
        if (not args_.empty()) {
            args_.front().source_to(os);
            for (const auto [d, e] :
                 std::views::zip(arg_delims_tk_, args_ | std::views::drop(1))) {

                d.source_to(os);
                e.source_to(os);
            }
        }
        close_paren_tk_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const stmt_def_func& func{
            tc.get_func_or_throw(tok(), statement::identifier())};

        if (not func.is_inlined()) {
            compile_noninline(tc, indent, dst_info, func);

            return;
        }

        // buffer the aliases of arguments and function return
        std::vector<alias_info> aliases_to_add;

        // validate return type
        const std::optional<func_return_info> ret{func.returns()};

        if (ret and dst_info.is_empty()) {
            throw compiler_exception{tok(), "return value is discarded"};
        }

        if (not ret and not dst_info.is_empty()) {
            throw compiler_exception{tok(), "function does not return a value"};
        }

        if (ret) {
            operand dst_lea{dst_info.use_operand or dst_info.has_lea()
                                ? dst_info.operand
                                : operand{}};

            aliases_to_add.emplace_back(
                std::string{ret->ident_tk.text()}, dst_info.id,
                std::move(dst_lea), ret->type_ptr,
                dst_info.is_register() ? dst_info.operand : operand{});
        }

        // track allocated registers
        struct allocated_register {
            operand reg;
            bool is_named{};
        };

        std::vector<allocated_register> allocated_registers;

        // process each argument
        for (const auto [arg, param] : std::views::zip(args_, func.params())) {

            const bool is_reference{not arg.is_expression() and
                                    (arg.is_indexed() or tc.has_lea(arg))};

            if (is_reference and not arg.get_unary_ops().is_empty()) {
                throw compiler_exception{
                    arg.tok(),
                    "unary operators on reference arguments are unsupported"};
            }

            // allocate named register if parameter requires it

            const std::string_view register_name{
                param.get_register_name_or_empty()};

            operand arg_reg;

            if (not register_name.empty()) {
                arg_reg = x.alloc_named_register(
                    arg.tok(), indent, register_name, param.get_type());

                allocated_registers.push_back({
                    .reg{arg_reg},
                    .is_named{true},
                });
            }

            // if the argument is an identifier containing indexing, then save
            // the 'lea' address to access the argument
            // examples: [rbp + r14 * 4 + 205] or [r15 + r14] or simply [r15]

            if (is_reference) {

                const ident_info arg_info{tc.make_ident_info(arg)};

                std::vector<operand> regs_lea;

                const operand lea{arg.compile_lea(tc, indent, arg.tok(),
                                                  regs_lea, {},
                                                  arg_info.lea_path, {})};

                for (const operand& r : regs_lea) {
                    allocated_registers.push_back({
                        .reg{r},
                        .is_named{},
                    });
                }

                aliases_to_add.emplace_back(std::string{param.identifier()},
                                            std::string{arg.identifier()}, lea,
                                            &param.get_type());

                continue;
            }

            // handle expression arguments

            if (arg.is_expression()) {
                if (arg_reg.is_empty()) {
                    // no particular register requested
                    arg_reg = x.alloc_scratch_register(arg.tok(), indent,
                                                       param.get_type());

                    allocated_registers.push_back({
                        .reg{arg_reg},
                        .is_named{},
                    });
                }

                arg.compile(tc, indent,
                            toc::make_ident_info_from_register(arg_reg));

                aliases_to_add.push_back(alias_info::make_register(
                    param.identifier(), param.get_type(), arg_reg));

                continue;
            }

            // handle non-expression without the register and without unary
            // ops
            if (arg_reg.is_empty() and arg.get_unary_ops().is_empty()) {
                aliases_to_add.emplace_back(std::string{param.identifier()},
                                            std::string{arg.identifier()},
                                            operand{}, &param.get_type());

                continue;
            }

            // handle non-expression with unary ops but no register

            if (arg_reg.is_empty()) {
                const ident_info& arg_info{tc.make_ident_info(arg)};

                if (arg_info.is_const()) {
                    // identifier is constant

                    aliases_to_add.emplace_back(
                        std::string{param.identifier()},
                        arg.make_constant_operand(arg_info).immediate(),
                        operand{}, &param.get_type());

                } else {
                    // identifier with unary ops

                    const operand scratch_reg{x.alloc_scratch_register(
                        arg.tok(), indent, param.get_type())};

                    allocated_registers.push_back({
                        .reg{scratch_reg},
                        .is_named{},
                    });

                    x.copy_value(param.tok(), indent, scratch_reg,
                                 arg_info.operand);

                    // apply unary ops
                    arg.get_unary_ops().compile(tc, indent, scratch_reg);

                    aliases_to_add.push_back(alias_info::make_register(
                        param.identifier(), param.get_type(), scratch_reg));
                }

                continue;
            }

            // handle non-expression with register

            aliases_to_add.push_back(alias_info::make_register(
                param.identifier(), param.get_type(), arg_reg));

            const ident_info& arg_info{tc.make_ident_info(arg)};

            if (arg_info.is_const()) {
                x.copy_value(param.tok(), indent, arg_reg,
                             arg.make_constant_operand(arg_info));
            } else {
                x.copy_value(param.tok(), indent, arg_reg, arg_info.operand);
                arg.get_unary_ops().compile(tc, indent + 1, arg_reg);
            }
        }

        // create unique labels for inlined functions
        const std::string_view call_path{tc.get_call_path()};
        const std::string src_loc{tc.source_location_for_use_in_label(tok())};
        const std::string new_call_path{
            call_path.empty() ? src_loc
                              : std::format("{}_{}", src_loc, call_path)};

        const std::string ret_jmp_label{
            std::format("{}_{}_end", func.name(), new_call_path)};

        func.source_def_comment_to(x, indent);

        x.label(indent, std::format("{}_{}", func.name(), new_call_path));

        // enter function scope

        tc.enter_func(func.name(), func.returns(), new_call_path,
                      ret_jmp_label);

        // add aliases
        for (const alias_info& e : aliases_to_add) {
            x.comment_alias(tok(), indent + 1, e.from, e.to, e.lea);
            tc.add_alias(e);
        }

        // compile inlined code
        func.code().compile(tc, indent, dst_info);

        // free allocated registers in reverse order
        for (const allocated_register& allocation :
             allocated_registers | std::views::reverse) {

            if (allocation.is_named) {
                x.free_named_register(tok(), indent + 1, allocation.reg);
            } else {
                x.free_scratch_register(tok(), indent + 1, allocation.reg);
            }
        }

        // provide the exit label for 'return' to jump to

        x.label(indent, ret_jmp_label);

        // apply unary ops to result if present

        if (not get_unary_ops().is_empty()) {
            if (not func.returns()) {
                std::unreachable();
            }
            const func_return_info& return_info{*func.returns()};
            const ident_info& ret_info{
                tc.make_ident_info(tok(), return_info.ident_tk.text())};

            get_unary_ops().compile(tc, indent, ret_info.operand);
        }

        tc.exit_func(func.name());
    }

    [[nodiscard]] auto argument(const size_t arg_index) const
        -> const statement& {
        return args_[arg_index];
    }

    [[nodiscard]] auto argument_count() const -> size_t { return args_.size(); }

    auto assert_var_not_used(const std::string_view var) const
        -> void override {

        for (const expr_any& e : args_) {
            e.assert_var_not_used(var);
        }
    }
};
