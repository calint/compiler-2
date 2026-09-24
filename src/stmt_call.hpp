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

                // todo: literals and call results need a temporary to be
                //       passed, see etc/todo.txt
                if (not param.get_type().is_builtin() and
                    not arg.is_identifier()) {

                    throw compiler_exception{
                        arg.tok(),
                        std::format("argument {} cannot be a temporary",
                                    arg_number)};
                }

                if (param.is_array()) {
                    const ident_info arg_info{tc.make_ident_info(arg)};

                    // an element would give the parameter the whole array's
                    // length, pass the array and a start index instead
                    if (not arg_info.is_array or arg.is_array_element()) {
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

    auto assert_result_type(const ident_info& dst_info,
                            const stmt_def_func& func) const -> void {

        if (&dst_info.type_ref() == &func.get_type()) {
            return;
        }

        const std::string message{
            std::format("result type mismatch: function returns '{}', "
                        "destination is '{}'",
                        func.get_type().name(), dst_info.type_ref().name())};

        // a non-inline result needs a memory destination, not the register
        // the conversion computes into
        if (not func.is_inlined()) {
            throw compiler_exception{tok(), message};
        }

        throw compiler_exception{tok(),
                                 std::format("{}, use '{}(...)'", message,
                                             dst_info.type_ref().name())};
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

    static auto assert_alias_type(const toc& tc, const expr_any& arg,
                                  const stmt_def_func_param& param) -> void {

        const ident_info info{tc.make_ident_info(arg)};

        // constants and registers are values, not storage
        if (not info.is_var()) {
            return;
        }

        if (&info.type_ref() == &param.get_type()) {
            return;
        }

        throw_parameter_type_mismatch(arg, param, info);
    }

    // compares resolved variable roots, so any overlap of fields or elements
    // counts as shared
    [[nodiscard]] static auto may_share_storage(const toc& tc,
                                                const ident_info& lhs,
                                                const ident_info& rhs) -> bool {

        const std::string_view lhs_root{lhs.elem_path.front()};
        const std::string_view rhs_root{rhs.elem_path.front()};

        if (lhs_root == rhs_root) {
            return true;
        }

        // a non-inline parameter points into its caller's storage, which can
        // be a global the callee also names directly. two parameters cannot
        // share storage because their own call site was checked
        if (lhs.is_pointer and tc.is_global_var(rhs_root)) {
            return true;
        }

        return rhs.is_pointer and tc.is_global_var(lhs_root);
    }

    // the callee writes a result or by-reference parameter in place, so
    // storage shared with another reference could be read after it changed
    auto assert_no_shared_storage(const toc& tc,
                                  const ident_info& dst_info) const -> void {

        if (not tc.is_alias_check()) {
            return;
        }

        std::vector<std::pair<size_t, ident_info>> references;

        for (const auto [arg_number, arg] :
             std::views::zip(std::views::iota(size_t{1}), args_)) {

            // expressions and unary operators pass a copied value
            if (arg.is_expression() or not arg.get_unary_ops().is_empty()) {
                continue;
            }

            ident_info info{tc.make_ident_info(arg)};

            // constants pass their value
            if (not info.is_var()) {
                continue;
            }

            if (dst_info.is_var() and may_share_storage(tc, dst_info, info)) {
                throw compiler_exception{
                    arg.tok(),
                    std::format("argument {} may share storage with the "
                                "result destination, use a separate variable",
                                arg_number)};
            }

            for (const auto& [other_number, other] : references) {
                if (may_share_storage(tc, other, info)) {
                    throw compiler_exception{
                        arg.tok(),
                        std::format("argument {} may share storage with "
                                    "argument {}, use a separate variable",
                                    arg_number, other_number)};
                }
            }

            references.emplace_back(arg_number, std::move(info));
        }
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

            assert_result_type(dst_info, func);
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

            if (&info.type_ref() != &param.get_type()) {
                throw_parameter_type_mismatch(arg, param, info);
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

        assert_no_shared_storage(tc, dst_info);

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

        // the result names the destination, so the widths must agree
        if (ret) {
            assert_result_type(dst_info, func);
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

        // scratch registers stay allocated until the inlined body is compiled
        std::vector<operand> allocated_registers;

        // process each argument
        for (const auto [arg, param] : std::views::zip(args_, func.params())) {

            const bool is_reference{not arg.is_expression() and
                                    (arg.is_indexed() or tc.has_lea(arg))};

            if (is_reference and not arg.get_unary_ops().is_empty()) {
                throw compiler_exception{
                    arg.tok(),
                    "unary operators on reference arguments are unsupported"};
            }

            // an alias uses the argument's storage so its type must match
            if (not arg.is_expression() and arg.get_unary_ops().is_empty()) {

                assert_alias_type(tc, arg, param);
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
                    allocated_registers.push_back(r);
                }

                aliases_to_add.emplace_back(std::string{param.identifier()},
                                            std::string{arg.identifier()}, lea,
                                            &param.get_type());

                continue;
            }

            // handle expression arguments

            if (arg.is_expression()) {
                const operand arg_reg{x.alloc_scratch_register(
                    arg.tok(), indent, param.get_type())};

                allocated_registers.push_back(arg_reg);

                arg.compile(tc, indent,
                            toc::make_ident_info_from_register(arg_reg));

                aliases_to_add.push_back(alias_info::make_register(
                    param.identifier(), param.get_type(), arg_reg));

                continue;
            }

            // constants and unary ops pass a value, not storage, so the value
            // must fit the parameter
            arg.assert_not_narrowed(tc, param.get_type());

            // handle non-expression without unary ops
            if (arg.get_unary_ops().is_empty()) {
                const ident_info arg_info{tc.make_ident_info(arg)};

                // a name is resolved in the callee where its own constants
                // would shadow the caller's, so constants pass their value
                const std::string alias_to{
                    arg_info.is_const()
                        ? arg.make_constant_operand(arg_info).immediate()
                        : std::string{arg.identifier()}};

                aliases_to_add.emplace_back(std::string{param.identifier()},
                                            alias_to, operand{},
                                            &param.get_type());

                continue;
            }

            // handle non-expression with unary ops

            const ident_info& arg_info{tc.make_ident_info(arg)};

            // the alias target must be a plain integer, e.g. '~1' is not
            if (arg_info.is_const()) {
                aliases_to_add.emplace_back(
                    std::string{param.identifier()},
                    std::format("{}", arg.get_unary_ops().evaluate_constant(
                                          arg_info.const_value)),
                    operand{}, &param.get_type());

                continue;
            }

            const operand scratch_reg{
                x.alloc_scratch_register(arg.tok(), indent, param.get_type())};

            allocated_registers.push_back(scratch_reg);

            x.copy_value(param.tok(), indent, scratch_reg, arg_info.operand);

            // apply unary ops
            arg.get_unary_ops().compile(tc, indent, scratch_reg);

            aliases_to_add.push_back(alias_info::make_register(
                param.identifier(), param.get_type(), scratch_reg));
        }

        // create unique labels for inlined functions
        const std::string_view call_path{tc.get_call_path()};
        const std::string src_loc{tc.source_location_for_use_in_label(tok())};
        const std::string new_call_path{
            call_path.empty() ? src_loc
                              : std::format("{}.{}", src_loc, call_path)};

        const std::string call_label{
            std::format("{}.{}", func.body_label(), new_call_path)};

        const std::string ret_jmp_label{std::format("{}.end", call_label)};

        func.source_def_comment_to(x, indent);

        x.label(indent, call_label);

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
        for (const operand& r : allocated_registers | std::views::reverse) {
            x.free_scratch_register(tok(), indent + 1, r);
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

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

        for (const expr_any& e : args_) {
            e.visit_reads(var, reader);
        }
    }

    // reported at the call because an inlined result aliases the destination
    auto assert_not_narrowed([[maybe_unused]] const toc& tc,
                             const type& dst_type) const -> void override {

        assert_own_type_not_narrowed(dst_type);
    }
};
