#pragma once
// reviewed: 2025-09-28

#include <algorithm>
#include <ranges>

#include "expr_any.hpp"
#include "stmt_def_func.hpp"

class stmt_call : public expression {
    std::vector<expr_any> args_;
    token ws_after_; // whitespace after arguments

  public:
    stmt_call(toc& tc, unary_ops uops, token tk, tokenizer& tz)
        : expression{std::move(tk), std::move(uops)} {

        set_type(tc.get_func_return_type_or_throw(tok(), identifier()));

        if (not tz.is_next_char('(')) {
            throw compiler_exception(tok(), "expected '(' after function name");
        }

        if (not tc.is_func_builtin(tok(), identifier())) {
            // user defined function
            const stmt_def_func& func{
                tc.get_func_or_throw(tok(), identifier())};
            // try to create argument expressions of same type as parameter
            size_t i{};
            const size_t n{func.params().size()};
            for (const stmt_def_func_param& param : func.params()) {
                args_.emplace_back(tc, tz, param.get_type(), true);
                i++;
                if (i < n) {
                    if (not tz.is_next_char(',')) {
                        throw compiler_exception(
                            tz, "expected argument " + std::to_string(i + 1) +
                                    " '" + param.name() + "'");
                    }
                }
            }
            if (not tz.is_next_char(')')) {
                throw compiler_exception(tz, "expected ')' after arguments");
            }
        } else {
            // built-in function
            bool expect_arg{};
            while (true) {
                if (tz.is_next_char(')')) { // foo()
                    if (expect_arg) {
                        throw compiler_exception(tz,
                                                 "expected argument after ','");
                    }
                    break;
                }
                args_.emplace_back(tc, tz, tc.get_type_default(), true);
                // note: default type because built-in function use registers
                expect_arg = tz.is_next_char(',');
            }
        }
        ws_after_ = tz.next_whitespace_token();
    }

    stmt_call() = default;
    stmt_call(const stmt_call&) = default;
    stmt_call(stmt_call&&) = default;
    auto operator=(const stmt_call&) -> stmt_call& = default;
    auto operator=(stmt_call&&) -> stmt_call& = default;

    ~stmt_call() override = default;

    auto source_to(std::ostream& os) const -> void override {
        expression::source_to(os);
        os << "(";
        size_t i{};
        for (const expr_any& e : args_) {
            if (i++) {
                os << ",";
            }
            e.source_to(os);
        }
        os << ")";
        ws_after_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 const std::string& dst = "") const -> void override {

        tc.comment_source(*this, os, indent);

        const stmt_def_func& func{tc.get_func_or_throw(tok(), identifier())};
        const std::string& func_name{func.name()};

        // check that the same number of arguments are provided as expected
        if (func.params().size() != args_.size()) {
            throw compiler_exception(
                tok(), "function '" + func_name + "' expects " +
                           std::to_string(func.params().size()) + " argument" +
                           (func.params().size() == 1 ? "" : "s") + " but " +
                           std::to_string(args_.size()) + " " +
                           (args_.size() == 1 ? "is" : "are") + " provided");
        }

        // check that argument types match the parameters
        for (size_t i{}; i < args_.size(); i++) {
            const expr_any& arg{args_.at(i)};
            const stmt_def_func_param& param{func.param(i)};
            const type& arg_type{arg.get_type()};
            const type& param_type{param.get_type()};
            if (arg_type.is_built_in() and param_type.is_built_in()) {
                //? check if it is integral (not bool)
                if (param_type.size() < arg_type.size()) {
                    throw compiler_exception(
                        arg.tok(), "argument " + std::to_string(i + 1) +
                                       " of type '" + arg_type.name() +
                                       "' would be truncated when passed to "
                                       "parameter of type '" +
                                       param_type.name() + "'");
                }
                continue;
            }
            if (arg_type.name() != param_type.name()) {
                throw compiler_exception(
                    arg.tok(), "argument " + std::to_string(i + 1) +
                                   " of type '" + arg_type.name() +
                                   "' does not match parameter of type '" +
                                   param_type.name() + "'");
            }
        }

        // check that return value matches the type
        if (not dst.empty()) {
            if (func.returns().empty()) {
                throw compiler_exception(tok(),
                                         "function does not return value");
            }

            const type& return_type{func.get_type()};
            const ident_info& dst_info{tc.make_ident_info(tok(), dst, false)};
            //?
            if (dst_info.type_ref.size() < return_type.size()) {
                throw compiler_exception(
                    tok(), "return type '" + return_type.name() +
                               "' would be truncated when copied to '" + dst +
                               "' of type '" + dst_info.type_ref.name() + "'");
            }
        }

        // create unique labels for in-lined functions based on location of
        // source where the call occurred
        const std::string& call_path{tc.get_call_path(tok())};
        const std::string& src_loc{tc.source_location_for_use_in_label(tok())};
        const std::string& new_call_path{
            call_path.empty() ? src_loc : (src_loc + "_" + call_path)};
        const std::string& ret_jmp_label{func_name + "_" + new_call_path +
                                         "_end"};

        // keep track of allocated registers
        std::vector<std::string> allocated_named_registers;
        std::vector<std::string> allocated_scratch_registers;
        std::vector<std::string> allocated_registers_in_order;

        // buffer the aliases of arguments
        std::vector<std::pair<std::string, std::string>> aliases_to_add;

        // if function returns value
        if (not dst.empty()) {
            // alias return identifier to 'dst'
            const std::string& from{func.returns().at(0).ident_tk.name()};
            const std::string& to{dst};
            aliases_to_add.emplace_back(from, to);
        }

        size_t i{};
        for (const expr_any& arg : args_) {
            const stmt_def_func_param& param{func.param(i)};
            i++;

            // does the parameter want the value passed through a register?
            std::string arg_reg{param.get_register_name_or_empty()};
            if (not arg_reg.empty()) {
                // argument is passed through register
                tc.alloc_named_register_or_throw(arg, os, indent, arg_reg);
                allocated_named_registers.emplace_back(arg_reg);
                allocated_registers_in_order.emplace_back(arg_reg);
            }

            // is argument an expression?
            if (arg.is_expression()) {
                // argument is an expression, evaluate and store in arg_reg
                if (arg_reg.empty()) {
                    // no particular register requested for the argument
                    // re-assign 'arg_reg' to scratch register
                    arg_reg = tc.alloc_scratch_register(arg.tok(), os, indent);
                    allocated_scratch_registers.emplace_back(arg_reg);
                    allocated_registers_in_order.emplace_back(arg_reg);
                }
                // compile expression and store result in 'arg_reg'
                // note: 'unary_ops' are part of 'expr_ops_list'
                arg.compile(tc, os, indent, arg_reg);
                // alias parameter name to the register containing its value
                aliases_to_add.emplace_back(param.identifier(), arg_reg);
                continue;
            }

            // argument is not an expression
            // is argument passed through register?
            if (arg_reg.empty()) {
                // argument not passed through register
                // does unary ops need to be applied?
                if (arg.get_unary_ops().is_empty()) {
                    // no unary ops
                    // alias parameter name to the argument identifier
                    const std::string& arg_id{arg.identifier()};
                    aliases_to_add.emplace_back(param.identifier(), arg_id);
                    continue;
                }
                // unary ops must be applied
                // allocate a scratch register and evaluate
                const ident_info& arg_info{tc.make_ident_info(arg, true)};
                const std::string& scratch_reg{
                    tc.alloc_scratch_register(arg.tok(), os, indent)};
                allocated_registers_in_order.emplace_back(scratch_reg);
                allocated_scratch_registers.emplace_back(scratch_reg);
                tc.asm_cmd(param.tok(), os, indent, "mov", scratch_reg,
                           arg_info.id_nasm);
                arg.get_unary_ops().compile(tc, os, indent, scratch_reg);
                // alias parameter to scratch register
                aliases_to_add.emplace_back(param.identifier(), scratch_reg);
                continue;
            }

            // argument is not an expression and passed through register
            // alias parameter name to the register
            aliases_to_add.emplace_back(param.identifier(), arg_reg);
            // move argument to register specified in param
            const ident_info& arg_info{tc.make_ident_info(arg, true)};
            // is argument a constant?
            if (arg_info.is_const()) {
                // argument is a constant
                // assign it to the register
                tc.asm_cmd(param.tok(), os, indent, "mov", arg_reg,
                           arg.get_unary_ops().to_string() + arg_info.id_nasm);
                continue;
            }
            // argument is not a constant
            // assign argument to register
            tc.asm_cmd(param.tok(), os, indent, "mov", arg_reg,
                       arg_info.id_nasm);
            arg.get_unary_ops().compile(tc, os, indent + 1, arg_reg);
        }

        toc::indent(os, indent, true);
        func.source_def_comment_to(os);

        // note: not necessary but makes reading the generated code without
        // comments easier
        toc::asm_label(tok(), os, indent, func_name + "_" + new_call_path);

        // enter function creating a new scope from which prior variables are
        // not visible
        const std::vector<func_return_info>& returns{func.returns()};
        tc.enter_func(func_name, returns, new_call_path, ret_jmp_label);

        // add the aliases to the context of the new scope
        for (const auto& alias : aliases_to_add) {
            const std::string& from{alias.first};
            const std::string& to{alias.second};
            tc.add_alias(from, to);

            toc::indent(os, indent + 1, true);
            os << "alias " << from << " -> " << to << '\n';
        }

        // compile in-lined code
        func.code().compile(tc, os, indent);

        // if function returns then check that the return variable has been
        // assigned
        if (not returns.empty()) {
            //? FIX temporary disabled for progress
            // const std::string& ret_var{returns.at(0).ident_tk.name()};
            // if (not tc.is_var_initiated(ret_var)) {
            //     throw compiler_exception(returns.at(0).ident_tk,
            //                              "return variable '" + ret_var +
            //                                  "' has not been assigned");
            // }
        }

        // free allocated registers in reverse order of allocation
        for (const auto& reg :
             allocated_registers_in_order | std::views::reverse) {
            if (std::ranges::find(allocated_scratch_registers, reg) !=
                allocated_scratch_registers.end()) {
                tc.free_scratch_register(os, indent + 1, reg);
                continue;
            }
            if (std::ranges::find(allocated_named_registers, reg) !=
                allocated_named_registers.end()) {
                tc.free_named_register(os, indent + 1, reg);
                continue;
            }
            throw panic_exception("unexpected code path");
        }

        // provide an exit label for 'return' to jump to
        toc::asm_label(tok(), os, indent, ret_jmp_label);

        // does the result of the call have unary ops?
        // e.g.: var x = ~foo*()
        if (not get_unary_ops().is_empty()) {
            // has unary ops
            // does the function have a return?
            if (func.returns().empty()) {
                throw compiler_exception(
                    tok(), "function call has unary operations but it "
                           "does not return a value");
            }

            // compile the result using the unary ops
            const ident_info& ret_info{tc.make_ident_info(
                tok(), func.returns().at(0).ident_tk.name(), true)};
            get_unary_ops().compile(tc, os, indent, ret_info.id_nasm);
        }
        // exit scope
        tc.exit_func(func_name);
    }

    [[nodiscard]] auto arg(size_t ix) const -> const statement& {
        return args_.at(ix);
    }

    [[nodiscard]] auto arg_count() const -> size_t { return args_.size(); }
};
