#pragma once
// reviewed: 2025-09-28
//           2025-10-08

#include <algorithm>
#include <format>
#include <ranges>
#include <string_view>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "stmt_def_func.hpp"

class stmt_call : public expression {
    std::vector<expr_any> args_;
    token ws_after_;

  public:
    stmt_call(toc& tc, unary_ops uops, token tk, tokenizer& tz)
        : expression{tk, std::move(uops)} {

        set_type(
            tc.get_func_return_type_or_throw(tok(), statement::identifier()));

        if (not tz.is_next_char('(')) {
            throw compiler_exception{tok(), "expected '(' after function name"};
        }

        if (not tc.is_func_builtin(tok(), statement::identifier())) {
            // user defined function
            const stmt_def_func& func{
                tc.get_func_or_throw(tok(), statement::identifier())};
            const size_t n{func.params().size()};
            for (size_t i{}; const stmt_def_func_param& param : func.params()) {
                args_.emplace_back(tc, tz, param.get_type(), true,
                                   param.is_array());
                if (++i < n) {
                    if (not tz.is_next_char(',')) {
                        throw compiler_exception{
                            tz, std::format("expected argument {} '{}'", i + 1,
                                            param.name())};
                    }
                }
            }
            if (not tz.is_next_char(')')) {
                throw compiler_exception{tz, "expected ')' after arguments"};
            }
        } else {
            // built-in function
            bool expect_arg{};
            while (true) {
                if (tz.is_next_char(')')) {
                    if (expect_arg) {
                        throw compiler_exception{tz,
                                                 "expected argument after ','"};
                    }
                    break;
                }
                args_.emplace_back(tc, tz, tc.get_type_default(), true);
                expect_arg = tz.is_next_char(',');
            }
        }
        ws_after_ = tz.next_whitespace_token();
    }

    ~stmt_call() override = default;
    stmt_call() = default;
    stmt_call(const stmt_call&) = default;
    stmt_call(stmt_call&&) = default;
    auto operator=(const stmt_call&) -> stmt_call& = default;
    auto operator=(stmt_call&&) -> stmt_call& = default;

    auto source_to(std::ostream& os) const -> void override {
        expression::source_to(os);
        std::print(os, "(");
        for (size_t i{}; const expr_any& e : args_) {
            if (i++) {
                std::print(os, ",");
            }
            e.source_to(os);
        }
        std::print(os, ")");
        ws_after_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 const std::string_view dst) const -> void override {

        tc.comment_source(*this, os, indent);

        const stmt_def_func& func{
            tc.get_func_or_throw(tok(), statement::identifier())};

        // validate argument count
        if (func.params().size() != args_.size()) {
            throw compiler_exception{
                tok(),
                std::format(
                    "function '{}' expects {} argument{} but {} {} provided",
                    func.name(), func.params().size(),
                    (func.params().size() == 1 ? "" : "s"), args_.size(),
                    (args_.size() == 1 ? "is" : "are"))};
        }

        // validate argument types
        for (size_t i{}; i < args_.size(); i++) {
            const expr_any& arg{args_[i]};
            const stmt_def_func_param& param{func.param(i)};
            const type& arg_type{arg.get_type()};
            const type& param_type{param.get_type()};

            if (param.is_array()) {
                const ident_info arg_info{tc.make_ident_info(arg)};
                if (not arg_info.is_array) {
                    throw compiler_exception{
                        arg.tok(),
                        std::format("parameter {} expected an array", i + 1)};
                }
            }
            if (arg_type.is_built_in() and param_type.is_built_in()) {
                if (param_type.size() < arg_type.size()) {
                    throw compiler_exception{
                        arg.tok(),
                        std::format(
                            "argument {} of type '{}' would be truncated when "
                            "passed to parameter of type '{}'",
                            i + 1, arg_type.name(), param_type.name())};
                }
                continue;
            }
            if (arg_type.name() != param_type.name()) {
                throw compiler_exception{
                    arg.tok(),
                    std::format("argument {} of type '{}' does not match "
                                "parameter of type '{}'",
                                i + 1, arg_type.name(), param_type.name())};
            }
        }

        // buffer the aliases of arguments and return
        std::vector<alias_info> aliases_to_add;

        // validate return type
        std::optional<func_return_info> ret{func.returns()};
        if (not dst.empty()) {
            if (not ret) {
                throw compiler_exception{tok(),
                                         "function does not return value"};
            }
            // alias return identifier to 'dst'
            aliases_to_add.emplace_back(std::string{ret->ident_tk.text()},
                                        std::string{dst}, "", ret->type_ptr);
        } else if (ret) {
            throw compiler_exception{tok(),
                                     "function returns but value is discarded"};
        }

        // track allocated registers
        std::vector<std::string> allocated_named_registers;
        std::vector<std::string> allocated_scratch_registers;
        std::vector<std::string> allocated_registers_in_order;

        // process each argument
        for (size_t i{}; const expr_any& arg : args_) {
            const stmt_def_func_param& param{func.param(i++)};

            // allocate named register if parameter requires it
            std::string arg_reg{param.get_register_name_or_empty()};
            if (not arg_reg.empty()) {
                tc.alloc_named_register_or_throw(arg.tok(), os, indent,
                                                 arg_reg);
                allocated_named_registers.emplace_back(arg_reg);
                allocated_registers_in_order.emplace_back(arg_reg);
            }

            // if the argument is an identifier with indexing then save the
            // "lea" address to the base of that array
            if (arg.is_identifier() and
                (arg.is_expression() or tc.make_ident_info(arg).has_lea())) {
                const ident_info ii{tc.make_ident_info(arg)};

                std::vector<std::string> regs_lea;

                const std::string lea{arg.compile_lea(
                    arg.tok(), tc, os, indent, regs_lea, "", ii.lea_path)};

                for (const std::string& r : regs_lea) {
                    allocated_scratch_registers.emplace_back(r);
                    allocated_registers_in_order.emplace_back(r);
                }

                aliases_to_add.emplace_back(std::string{param.identifier()},
                                            std::string{arg.identifier()}, lea,
                                            &param.get_type(), ii.array_size);
                continue;
            }

            // handle expression arguments
            if (arg.is_expression()) {
                if (arg_reg.empty()) {
                    arg_reg = tc.alloc_scratch_register(arg.tok(), os, indent);
                    allocated_scratch_registers.emplace_back(arg_reg);
                    allocated_registers_in_order.emplace_back(arg_reg);
                }
                arg.compile(tc, os, indent, arg_reg);
                aliases_to_add.emplace_back(std::string{param.identifier()},
                                            arg_reg, "", &param.get_type());
                continue;
            }

            // handle non-expression without the register and without unary
            // ops
            if (arg_reg.empty() and arg.get_unary_ops().is_empty()) {
                aliases_to_add.emplace_back(std::string{param.identifier()},
                                            std::string{arg.identifier()}, "",
                                            &param.get_type());
                continue;
            }

            // handle non-expression with unary ops but no register
            if (arg_reg.empty()) {
                const ident_info& arg_info{tc.make_ident_info(arg)};
                const std::string scratch_reg{
                    tc.alloc_scratch_register(arg.tok(), os, indent)};
                allocated_registers_in_order.emplace_back(scratch_reg);
                allocated_scratch_registers.emplace_back(scratch_reg);
                tc.asm_cmd(param.tok(), os, indent, "mov", scratch_reg,
                           arg_info.id_nasm);
                arg.get_unary_ops().compile(tc, os, indent, scratch_reg);
                aliases_to_add.emplace_back(std::string{param.identifier()},
                                            scratch_reg, "", &param.get_type());
                continue;
            }

            // handle non-expression with register
            aliases_to_add.emplace_back(std::string{param.identifier()},
                                        arg_reg, "", &param.get_type());
            const ident_info& arg_info{tc.make_ident_info(arg)};

            if (arg_info.is_const()) {
                tc.asm_cmd(param.tok(), os, indent, "mov", arg_reg,
                           std::format("{}{}", arg.get_unary_ops().to_string(),
                                       arg_info.id_nasm));
            } else {
                tc.asm_cmd(param.tok(), os, indent, "mov", arg_reg,
                           arg_info.id_nasm);
                arg.get_unary_ops().compile(tc, os, indent + 1, arg_reg);
            }
        }

        // create unique labels for in-lined functions
        const std::string_view call_path{tc.get_call_path(tok())};
        const std::string src_loc{tc.source_location_for_use_in_label(tok())};
        const std::string new_call_path{
            call_path.empty() ? src_loc
                              : std::format("{}_{}", src_loc, call_path)};
        const std::string ret_jmp_label{
            std::format("{}_{}_end", func.name(), new_call_path)};
        func.source_def_comment_to(tc, os, indent);

        toc::asm_label(tok(), os, indent,
                       std::format("{}_{}", func.name(), new_call_path));

        // enter function scope
        tc.enter_func(func.name(), func.returns(), new_call_path,
                      ret_jmp_label);

        // add aliases
        for (const alias_info& e : aliases_to_add) {
            tc.comment_start(tok(), os, indent + 1);
            std::println(os, "alias {} -> {}  (lea: {}, len: {})", e.from, e.to,
                         e.lea, e.array_size);
            tc.add_alias(e);
        }

        // compile in-lined code
        func.code().compile(tc, os, indent, dst);

        // free allocated registers in reverse order
        for (const auto& reg :
             allocated_registers_in_order | std::views::reverse) {
            if (std::ranges::contains(allocated_scratch_registers, reg)) {
                tc.free_scratch_register(tok(), os, indent + 1, reg);
            } else if (std::ranges::contains(allocated_named_registers, reg)) {
                tc.free_named_register(tok(), os, indent + 1, reg);
            } else {
                throw panic_exception("unexpected code path");
            }
        }

        // provide the exit label for 'return' to jump to
        toc::asm_label(tok(), os, indent, ret_jmp_label);

        // apply unary ops to result if present
        if (not get_unary_ops().is_empty()) {
            if (not func.returns()) {
                throw compiler_exception{tok(),
                                         "function call has unary operations "
                                         "but it does not return a value"};
            }
            const ident_info& ret_info{
                tc.make_ident_info(tok(), func.returns()->ident_tk.text())};
            get_unary_ops().compile(tc, os, indent, ret_info.id_nasm);
        }

        tc.exit_func(func.name());
    }

    [[nodiscard]] auto argument(const size_t ix) const -> const statement& {
        return args_.at(ix);
    }

    [[nodiscard]] auto arguments_size() const -> size_t { return args_.size(); }

    auto assert_var_not_used(const std::string_view var) const
        -> void override {

        for (const expr_any& e : args_) {
            e.assert_var_not_used(var);
        }
    }
};
