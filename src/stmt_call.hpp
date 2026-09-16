#pragma once
// reviewed: 2025-09-28
//           2025-10-08
//           2026-09-08

#include <algorithm>
#include <format>
#include <ranges>
#include <string_view>
#include <utility>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "stmt_def_func.hpp"

class stmt_call : public expression {
    token open_paren_tk_;
    std::vector<expr_any> args_;
    std::vector<token> args_delims_tk_;
    token close_paren_tk_;

  public:
    stmt_call(toc& tc, unary_ops uops, token tk, token open_paren_tk,
              tokenizer& tz)
        : expression{tk, std::move(uops)}, open_paren_tk_{open_paren_tk} {

        set_type(
            tc.get_func_return_type_or_throw(tok(), statement::identifier()));

        if (open_paren_tk_.is_empty()) {
            throw compiler_exception{tok(), "expected '(' after function name"};
        }

        if (not tc.is_func_builtin(statement::identifier())) {

            // user defined function

            const stmt_def_func& func{
                tc.get_func_or_throw(tok(), statement::identifier())};

            const size_t n{func.params().size()};
            args_.reserve(n);
            for (const auto [i, param] : std::views::enumerate(func.params())) {
                if (i != 0) {
                    const token t{tz.is_next_char_token(',')};
                    if (t.is_empty()) {
                        throw compiler_exception{
                            tz, std::format("expected argument {} named '{}'",
                                            i + 1, param.name())};
                        // note: +1 because 'i' starts at 0
                    }
                    args_delims_tk_.emplace_back(t);
                }
                args_.emplace_back(tc, tz, param.get_type(), true, false, 0);
            }

            close_paren_tk_ = tz.is_next_char_token(')');
            if (close_paren_tk_.is_empty()) {
                throw compiler_exception{tz, "expected ')' after arguments"};
            }

            for (const auto [index, arg, param] :
                 std::views::zip(std::views::iota(1), args_, func.params())) {

                if (param.is_array()) {
                    const ident_info arg_info{tc.make_ident_info(arg)};
                    if (not arg_info.is_array) {
                        throw compiler_exception{
                            arg.tok(),
                            std::format("parameter {} expected an array",
                                        index)};
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
                    throw compiler_exception(close_paren_tk_,
                                             "expected argument after ','");
                }
                break;
            }
            args_.emplace_back(tc, tz, tc.get_type_default(), true, false, 0);
            const token delim_tk{tz.is_next_char_token(',')};
            expect_arg = not delim_tk.is_empty();
            if (expect_arg) {
                args_delims_tk_.emplace_back(delim_tk);
            }
        }
    }

    stmt_call() = default;

    auto source_to(std::ostream& os) const -> void override {
        expression::source_to(os);
        open_paren_tk_.source_to(os);
        if (not args_.empty()) {
            args_.front().source_to(os);
            for (const auto [d, e] : std::views::zip(
                     args_delims_tk_, args_ | std::views::drop(1))) {
                d.source_to(os);
                e.source_to(os);
            }
        }
        close_paren_tk_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 const ident_info& dst_info) const -> void override {

        x86::comment_source(tc, *this, os, indent);

        const stmt_def_func& func{
            tc.get_func_or_throw(tok(), statement::identifier())};

        // buffer the aliases of arguments and function return
        std::vector<alias_info> aliases_to_add;

        // validate return type
        const std::optional<func_return_info> ret{func.returns()};
        if (not dst_info.operand.is_empty()) {
            // expect return to write to 'dst_info'

            if (not ret) {
                throw compiler_exception{tok(),
                                         "function does not return a value"};
            }

            // alias return identifier to 'dst_info'
            aliases_to_add.emplace_back(
                std::string{ret->ident_tk.text()}, dst_info.id,
                dst_info.operand.address_str(), ret->type_ptr);

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
            const stmt_def_func_param& param{func.param(i)};
            ++i;

            // allocate named register if parameter requires it

            std::string arg_reg{param.get_register_name_or_empty()};

            if (not arg_reg.empty()) {
                tc.alloc_named_register_or_throw(arg.tok(), os, indent, arg_reg,
                                                 param.get_type());
                allocated_named_registers.emplace_back(arg_reg);
                allocated_registers_in_order.emplace_back(arg_reg);
            }

            // if the argument is an identifier containing indexing, then save
            // the 'lea' address to access the argument
            // examples: [rsp + r14 * 4 - 205] or [r15 + r14] or simply [r15]

            if (not arg.is_expression() and
                (arg.is_indexed() or tc.has_lea(arg))) {

                const ident_info arg_info{tc.make_ident_info(arg)};

                std::vector<std::string> regs_lea;

                const operand lea{arg.compile_lea(arg.tok(), tc, os, indent,
                                                  regs_lea, "",
                                                  arg_info.lea_path)};

                for (const std::string& r : regs_lea) {
                    allocated_scratch_registers.emplace_back(r);
                    allocated_registers_in_order.emplace_back(r);
                }

                if (not arg.get_unary_ops().is_empty()) {
                    throw compiler_exception(
                        arg.tok(),
                        "unary operations are not allowed on references to "
                        "types");
                }

                aliases_to_add.emplace_back(std::string{param.identifier()},
                                            std::string{arg.identifier()},
                                            lea.address_str(),
                                            &param.get_type());

                continue;
            }

            // handle expression arguments

            if (arg.is_expression()) {
                if (arg_reg.empty()) {
                    // no particular register requested
                    arg_reg = tc.alloc_scratch_register(arg.tok(), os, indent,
                                                        param.get_type());
                    allocated_scratch_registers.emplace_back(arg_reg);
                    allocated_registers_in_order.emplace_back(arg_reg);
                }

                const std::string& reg_sized{tc.get_sized_register_operand(
                    arg_reg, param.get_type().size())};

                arg.compile(tc, os, indent,
                            tc.make_ident_info_for_register(reg_sized));

                aliases_to_add.emplace_back(std::string{param.identifier()},
                                            reg_sized, "", &param.get_type());

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

                if (arg_info.is_const()) {
                    // identifier is constant

                    aliases_to_add.emplace_back(
                        std::string{param.identifier()},
                        std::format("{}{}", arg.get_unary_ops().to_string(),
                                    arg_info.const_value),
                        "", &param.get_type());

                } else {
                    // identifier with unary ops

                    const std::string scratch_reg{tc.alloc_scratch_register(
                        arg.tok(), os, indent, param.get_type())};

                    allocated_registers_in_order.emplace_back(scratch_reg);
                    allocated_scratch_registers.emplace_back(scratch_reg);

                    x86::mov(tc, param.tok(), os, indent, scratch_reg,
                             arg_info.operand.str());

                    // apply unary ops
                    arg.get_unary_ops().compile(tc, os, indent, scratch_reg);

                    aliases_to_add.emplace_back(std::string{param.identifier()},
                                                scratch_reg, "",
                                                &param.get_type());
                }

                continue;
            }

            // handle non-expression with register

            aliases_to_add.emplace_back(std::string{param.identifier()},
                                        arg_reg, "", &param.get_type());

            const ident_info& arg_info{tc.make_ident_info(arg)};

            if (arg_info.is_const()) {
                x86::mov(tc, param.tok(), os, indent, arg_reg,
                         std::format("{}{}", arg.get_unary_ops().to_string(),
                                     arg_info.const_value));
            } else {
                x86::mov(tc, param.tok(), os, indent, arg_reg,
                         arg_info.operand.str());
                arg.get_unary_ops().compile(tc, os, indent + 1, arg_reg);
            }
        }

        // create unique labels for in-lined functions
        const std::string_view call_path{tc.get_call_path()};
        const std::string src_loc{tc.source_location_for_use_in_label(tok())};
        const std::string new_call_path{
            call_path.empty() ? src_loc
                              : std::format("{}_{}", src_loc, call_path)};
        const std::string ret_jmp_label{
            std::format("{}_{}_end", func.name(), new_call_path)};

        func.source_def_comment_to(tc, os, indent);

        x86::label(tc, os, indent,
                   std::format("{}_{}", func.name(), new_call_path));

        // enter function scope

        tc.enter_func(func.name(), func.returns(), new_call_path,
                      ret_jmp_label);

        // add aliases
        for (const alias_info& e : aliases_to_add) {
            x86::comment_start(tc, tok(), os, indent + 1);

            std::print(os, "alias {} -> {}", e.from, e.to);
            if (not e.lea.empty()) {
                std::print(os, " (lea: {})", e.lea);
            }
            std::println(os);
            tc.add_alias(e);
        }

        // compile in-lined code
        func.code().compile(tc, os, indent, dst_info);

        // free allocated registers in reverse order
        for (const std::string& reg :
             allocated_registers_in_order | std::views::reverse) {

            if (std::ranges::contains(allocated_scratch_registers, reg)) {
                tc.free_scratch_register(tok(), os, indent + 1, reg);
            } else if (std::ranges::contains(allocated_named_registers, reg)) {
                tc.free_named_register(tok(), os, indent + 1, reg);
            } else {
                std::unreachable();
            }
        }

        // provide the exit label for 'return' to jump to

        x86::label(tc, os, indent, ret_jmp_label);

        // apply unary ops to result if present

        if (not get_unary_ops().is_empty()) {
            if (not func.returns()) {
                std::unreachable();
            }
            const func_return_info& return_info{*func.returns()};
            const ident_info& ret_info{
                tc.make_ident_info(tok(), return_info.ident_tk.text())};

            get_unary_ops().compile(tc, os, indent, ret_info.operand.str());
        }

        tc.exit_func(func.name());
    }

    [[nodiscard]] auto argument(const size_t ix) const -> const statement& {
        return args_[ix];
    }

    [[nodiscard]] auto arguments_size() const -> size_t { return args_.size(); }

    auto assert_var_not_used(const std::string_view var) const
        -> void override {

        for (const expr_any& e : args_) {
            e.assert_var_not_used(var);
        }
    }
};
