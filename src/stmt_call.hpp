#pragma once
// reviewed: 2025-09-28
//           2025-10-08
//           2026-09-08

#include <algorithm>
#include <cassert>
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "stmt_def_func.hpp"
#include "stmt_identifier.hpp"

class stmt_call : public expression {
    // e.g. the '.' in 'lst.add(x)', the receiver is the first argument
    token method_dot_tk_;
    // e.g. the '.' and 'at' in 'point.at(1, 2)', 'tok()' is the type
    token constructor_dot_tk_;
    token constructor_name_tk_;
    std::string func_name_;
    token open_paren_tk_;
    std::vector<expr_any> args_;
    std::vector<token> arg_delims_tk_;
    token close_paren_tk_;

    struct storage_conflict {
        std::string reason;
        std::string fix;
    };

  public:
    stmt_call(toc& tc, unary_ops uops, const token tk,
              const token open_paren_tk, tokenizer& tz)
        : expression{tk, std::move(uops)}, func_name_{tk.text()},
          open_paren_tk_{open_paren_tk} {

        set_type(tc.get_func_return_type_or_throw(tok(), func_name_));

        if (open_paren_tk_.is_empty()) {
            throw compiler_exception{tz, "expected '(' after function name"};
        }

        if (not tc.is_func_builtin(func_name_)) {
            parse_arguments(tc, tz, tc.get_func_or_throw(tok(), func_name_));
            return;
        }

        parse_builtin_arguments(tc, tz);
    }

    // e.g. 'lst.add(x)' calls 'list.add' with 'lst' as 'self'
    stmt_call(toc& tc, unary_ops uops, stmt_identifier receiver, tokenizer& tz)
        : expression{receiver.method_name_token(), std::move(uops)},
          method_dot_tk_{receiver.method_dot_token()},
          func_name_{
              std::format("{}.{}", receiver.get_type().name(),
                          receiver.method_name_token().text()),
          },
          open_paren_tk_{tz.is_next_char_token('(')} {

        set_type(tc.get_func_return_type_or_throw(tok(), func_name_));

        if (open_paren_tk_.is_empty()) {
            throw compiler_exception{tz, "expected '(' after method name"};
        }

        const stmt_def_func& func{tc.get_func_or_throw(tok(), func_name_)};

        // a constructor has no receiver, it builds its value
        if (func.is_constructor()) {
            throw compiler_exception{
                tok(), std::format("'{}' is a constructor, call it as "
                                   "'{}(...)'",
                                   func_name_, func_name_)};
        }

        // an element of an array can be the receiver
        if (receiver.is_array()) {
            throw compiler_exception{
                receiver.first_token(),
                std::format("receiver of method '{}' cannot be an array",
                            func_name_)};
        }

        // the whitespace before the receiver belongs to its first token
        const token& first_tk{receiver.first_token()};
        const token receiver_pos_tk{
            "", first_tk.start_index(), "",    first_tk.start_index(),
            "", first_tk.at_line(),     false,
        };

        args_.emplace_back(
            receiver_pos_tk,
            expr_type{std::make_shared<stmt_identifier>(std::move(receiver))});

        parse_arguments(tc, tz, func);
    }

    // e.g. 'point.at(1, 2)' calls the constructor 'point.at'
    stmt_call(toc& tc, unary_ops uops, const token type_tk, tokenizer& tz)
        : expression{type_tk, std::move(uops)},
          constructor_dot_tk_{tz.is_next_char_token('.')},
          constructor_name_tk_{tz.next_token()},
          func_name_{
              std::format("{}.{}", type_tk.text(), constructor_name_tk_.text()),
          } {

        assert(not constructor_dot_tk_.is_empty());

        if (constructor_name_tk_.text().empty()) {
            throw compiler_exception{tz, "expected constructor name after '.'"};
        }

        if (not tc.is_func(func_name_)) {
            throw compiler_exception{
                constructor_name_tk_,
                std::format("type '{}' has no constructor '{}'", type_tk.text(),
                            constructor_name_tk_.text())};
        }

        const stmt_def_func& func{
            tc.get_func_or_throw(constructor_name_tk_, func_name_),
        };

        if (not func.is_constructor()) {
            throw compiler_exception{
                constructor_name_tk_,
                std::format("'{}' is a method, call it on a value of type '{}'",
                            func_name_, type_tk.text())};
        }

        set_type(func.get_type());

        open_paren_tk_ = tz.is_next_char_token('(');
        if (open_paren_tk_.is_empty()) {
            throw compiler_exception{tz, "expected '(' after constructor name"};
        }

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

        const stmt_def_func& func{tc.get_func_or_throw(tok(), func_name_)};

        assert_no_shared_storage(tc, dst_info, func);

        if (not func.is_inlined()) {
            compile_noninline(tc, indent, dst_info, func);
            return;
        }

        // an error found in the inlined body reports where it was called from
        try {
            compile_inline_call(tc, indent, dst_info, func);
        } catch (compiler_exception& e) {
            add_call_frame(e);
            throw;
        }
    }

    // reported at the call because an inlined result aliases the destination
    auto assert_not_narrowed([[maybe_unused]] const toc& tc,
                             const type& dst_type) const -> void override {

        assert_own_type_not_narrowed(dst_type);
    }

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

        for (const expr_any& e : args_) {
            e.visit_reads(var, reader);
        }
    }

    //
    // class methods
    //

    [[nodiscard]] auto argument(const size_t arg_index) const
        -> const statement& {

        return args_.at(arg_index);
    }

    [[nodiscard]] auto argument_count() const -> size_t { return args_.size(); }

    // the callee writes a 'mut' parameter in place, so storage shared with
    // another argument could be read after it changed, unless both
    // parameters are read-only. the result destination is only compared with
    // '--checks=alias'
    auto assert_no_shared_storage(const toc& tc, const ident_info& dst_info,
                                  const stmt_def_func& func) const -> void {

        const bool is_result_checked{tc.is_alias_check()};

        struct reference {
            size_t index;
            ident_info info;
            bool is_read_only;
        };

        std::vector<reference> references;

        for (size_t i{}; i < args_.size(); ++i) {
            const expr_any& arg{args_.at(i)};

            // expressions and unary operators pass a copied value
            if (arg.is_expression() or not arg.get_unary_ops().is_empty()) {
                continue;
            }

            ident_info info{tc.make_ident_info(arg)};

            // constants pass their value
            if (not info.is_var()) {
                continue;
            }

            if (is_result_checked and dst_info.is_var()) {
                const std::optional<storage_conflict> conflict{
                    shared_storage_conflict(dst_info, info),
                };

                if (conflict) {
                    throw compiler_exception{
                        arg.tok(),
                        std::format("{} '{}' may share storage with the "
                                    "result destination '{}' ({}), {}",
                                    describe_argument(i), arg.identifier(),
                                    dst_info.elem_path.front(),
                                    conflict->reason, conflict->fix)};
                }
            }

            const bool is_read_only{func.param(i).is_read_only()};

            for (const reference& other : references) {
                if (is_read_only and other.is_read_only) {
                    continue;
                }

                const std::optional<storage_conflict> conflict{
                    shared_storage_conflict(other.info, info),
                };

                if (conflict and
                    not reach_disjoint_bytes(args_.at(other.index), arg)) {

                    throw compiler_exception{
                        arg.tok(),
                        std::format("{} '{}' may share storage with {} '{}' "
                                    "({}), {}",
                                    describe_argument(i), arg.identifier(),
                                    describe_argument(other.index),
                                    args_.at(other.index).identifier(),
                                    conflict->reason, conflict->fix)};
                }
            }

            references.push_back({
                .index{i},
                .info{std::move(info)},
                .is_read_only{is_read_only},
            });
        }
    }

    auto assert_result_type(const ident_info& dst_info,
                            const stmt_def_func& func) const -> void {

        if (&dst_info.type_ref() == &func.get_type()) {
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
            throw compiler_exception{tok(), message};
        }

        throw compiler_exception{tok(),
                                 std::format("{}, use '{}(...)'", message,
                                             dst_info.type_ref().name())};
    }

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

    auto compile_inline(toc& tc, const size_t indent,
                        const ident_info& dst_info,
                        const stmt_def_func& func) const -> void {

        machine& x{tc.machine()};

        // buffer the aliases of arguments and function return
        std::vector<alias_info> aliases_to_add;

        assert_result_use(dst_info, func);

        const std::optional<func_return_info> ret{func.returns()};

        // the result names the destination, so the widths must agree
        if (ret) {
            assert_result_type(dst_info, func);

            aliases_to_add.push_back(make_result_alias(dst_info, *ret));
        }

        // scratch registers stay allocated until the inlined body is compiled
        std::vector<operand> allocated_registers;

        // the result alias has its own address choice in 'compile'
        const size_t first_argument_alias{aliases_to_add.size()};

        // process each argument
        for (const auto [arg, param] : std::views::zip(args_, func.params())) {
            aliases_to_add.push_back(make_argument_alias(tc, indent, arg, param,
                                                         allocated_registers));
        }

        const bool has_indexed_argument{
            std::ranges::any_of(aliases_to_add |
                                    std::views::drop(first_argument_alias),
                                is_indexed_reference),
        };

        if (not has_indexed_argument) {
            compile_inline_body(tc, indent, func, aliases_to_add,
                                allocated_registers);

            return;
        }

        // note: the body is compiled in three versions and the one with the
        //       least code is kept
        //
        //       1. aliases use the indexed addresses as they are, e.g.
        //          '[s0 + t0 * 4 + 28]'
        //       2. each indexed address goes once into a new register, the
        //          displacement included, e.g. '[t1]'
        //       3. as 2 but the displacement stays in each access, e.g.
        //          '[t1 + 28]'
        //
        //       rv32i has no base + index addressing, so in version 1 every
        //       access adds base and index again; a register pays off only
        //       when the body accesses the argument more than once, and a
        //       kept displacement saves an 'addi' unless the field offsets
        //       then exceed the 12-bit immediate range; which is shorter is
        //       only known after compiling the body
        //
        //       the argument registers are shared by all versions, so they
        //       are freed after the choice instead of inside the body

        x.emit_most_efficient(
            tok(), indent,
            [&] -> void {
                compile_inline_body(tc, indent, func, aliases_to_add, {});
            },
            [&] -> void {
                emit_most_efficient_address(
                    x, indent, [&](const bool keeps) -> void {
                        compile_inline_body_with_address_registers(
                            tc, indent, func, aliases_to_add,
                            first_argument_alias, keeps);
                    });
            });

        // freed after both versions since both use the argument registers
        free_in_reverse(x, tok(), indent + 1, allocated_registers);
    }

    auto compile_noninline(toc& tc, const size_t indent,
                           const ident_info& dst_info,
                           const stmt_def_func& func) const -> void {

        assert_noninline_call(tc, dst_info, func);
        check_noninline_aliasing(tc, indent, dst_info, func);

        machine& x{tc.machine()};

        // the registers stay allocated until the callee frame is populated
        std::vector<operand> address_registers;

        const std::vector<operand> addresses{
            frame_slot_addresses(tc, indent, dst_info, func, address_registers),
        };

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
            comment_frame_slot(x, indent, func, static_cast<size_t>(i));

            x.address_of(tok(), indent, slot, addr);

            slot.increment_offset(address_offset(x.address_size_bytes()));
        }

        x.free_scratch_registers(tok(), indent, address_registers);

        x.call_function(tok(), indent, func.body_label(), frame_address);
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

        if (&info.type_ref() == &param.get_type()) {
            return;
        }

        throw_parameter_type_mismatch(arg, param, info);
    }

    // 'p' of 'p.x.y'
    [[nodiscard]] static auto named_variable(const expr_any& arg)
        -> std::string_view {

        const std::string_view path{arg.identifier()};

        return path.substr(0, path.find('.'));
    }

    // the ranges are offsets into the variable the argument names, so they
    // are only comparable when both name the same one
    [[nodiscard]] static auto reach_disjoint_bytes(const expr_any& lhs,
                                                   const expr_any& rhs)
        -> bool {

        const std::optional<field_coverage::range> lhs_range{
            lhs.accessed_range(),
        };
        const std::optional<field_coverage::range> rhs_range{
            rhs.accessed_range(),
        };

        assert(lhs_range and rhs_range);

        if (named_variable(lhs) != named_variable(rhs)) {
            return false;
        }

        return not lhs_range->overlaps(*rhs_range);
    }

    // compares resolved variable roots, so any overlap of fields or elements
    // counts as shared
    [[nodiscard]] static auto shared_storage_conflict(const ident_info& lhs,
                                                      const ident_info& rhs)
        -> std::optional<storage_conflict> {

        const std::string_view lhs_root{lhs.elem_path.front()};
        const std::string_view rhs_root{rhs.elem_path.front()};

        if (lhs_root == rhs_root) {
            return storage_conflict{
                .reason{std::format("both name '{}'", lhs_root)},
                .fix{"use a separate variable"},
            };
        }

        return std::nullopt;
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
    // the error was found inside the inlined body, unless it is in the call
    // itself where the call is already the error location
    auto add_call_frame(compiler_exception& e) const -> void {
        if (is_inside_call(e)) {
            return;
        }

        e.add_call_frame(call_begin_token(), statement::trimmed_source(*this));
    }

    // the callee and which arguments are globals or the same local, the only
    // facts the aliasing checks of the body depend on. empty when an argument
    // is a parameter of the enclosing non-inline function, the calls of that
    // function are checked with their own arguments
    [[nodiscard]] auto aliasing_signature(const toc& tc,
                                          const ident_info& dst_info,
                                          const stmt_def_func& func) const
        -> std::optional<std::string> {

        std::vector<ident_info> infos;

        if (func.returns()) {
            infos.push_back(dst_info);
        }

        for (const expr_any& arg : args_) {
            infos.push_back(tc.make_ident_info(arg));
        }

        std::string signature{func.name()};
        std::vector<std::string_view> locals;

        for (const ident_info& info : infos) {
            if (info.is_pointer) {
                return std::nullopt;
            }

            // a temporary shares storage with nothing
            if (info.elem_path.empty()) {
                signature += " temporary";
                continue;
            }

            const std::string_view root{info.elem_path.front()};

            if (tc.is_global_var(root)) {
                signature += std::format(" global {}", root);
                continue;
            }

            if (std::ranges::find(locals, root) == locals.end()) {
                locals.push_back(root);
            }

            signature += std::format(
                " local {}", std::ranges::find(locals, root) - locals.begin());
        }

        return signature;
    }

    // an argument reaches its parameter by reference, so a value that has no
    // storage or an array of the wrong shape cannot be passed
    auto assert_argument_usable(const toc& tc, const size_t index,
                                const expr_any& arg,
                                const stmt_def_func_param& param) const
        -> void {

        // todo: literals and call results need a temporary to be
        //       passed, see etc/todo.txt
        if (not param.get_type().is_builtin() and not arg.is_identifier()) {
            throw compiler_exception{arg.tok(),
                                     std::format("{} cannot be a temporary",
                                                 describe_argument(index))};
        }

        if (param.is_array()) {
            const ident_info arg_info{tc.make_ident_info(arg)};

            // an element would give the parameter the whole array's
            // length, pass the array and a start index instead
            if (not arg_info.is_array) {
                throw compiler_exception{
                    arg.tok(), std::format("parameter {} requires an array",
                                           index + 1 - first_argument_index())};
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

    // the callee reaches the result and arguments through their addresses
    auto assert_noninline_call(const toc& tc, const ident_info& dst_info,
                               const stmt_def_func& func) const -> void {

        assert_result_use(dst_info, func);

        if (not get_unary_ops().is_empty()) {
            throw compiler_exception{
                tok(), "unary operators on non-inline calls are unsupported"};
        }

        if (func.returns()) {
            if (not dst_info.operand.is_memory()) {
                throw compiler_exception{
                    tok(), "result destination must be a memory location"};
            }

            // an array literal gives its call elements an element destination
            assert(not dst_info.is_array);

            assert_result_type(dst_info, func);
        }

        for (const auto [arg, param] : std::views::zip(args_, func.params())) {
            assert_noninline_argument(tc, arg, param);
        }
    }

    // a result must be stored and a call without one cannot provide it
    auto assert_result_use(const ident_info& dst_info,
                           const stmt_def_func& func) const -> void {

        if (func.returns() and dst_info.is_empty()) {
            throw compiler_exception{tok(), "return value is discarded"};
        }

        if (not func.returns() and not dst_info.is_empty()) {
            throw compiler_exception{tok(), "function does not return a value"};
        }
    }

    // the receiver of a method is written before the name
    [[nodiscard]] auto call_begin_token() const -> const token& {
        return is_method() ? args_.at(0).tok() : tok();
    }

    // the body is compiled once for all callers, so this call's aliasing is
    // checked by compiling the body as an inline call and dropping the code
    auto check_noninline_aliasing(toc& tc, const size_t indent,
                                  const ident_info& dst_info,
                                  const stmt_def_func& func) const -> void {

        const std::optional<std::string> signature{
            aliasing_signature(tc, dst_info, func),
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

        try {
            tc.check_only(
                [&] -> void { compile_inline(tc, indent, dst_info, func); });
        } catch (compiler_exception& e) {
            add_call_frame(e);
            throw;
        }
    }

    // the result address comes first, then one address per argument
    auto comment_frame_slot(machine& x, const size_t indent,
                            const stmt_def_func& func,
                            const size_t slot_index) const -> void {

        if (func.returns() and slot_index == 0) {
            x.comment(tok(), indent, "result address in callee frame");
            return;
        }

        const size_t arg_idx{slot_index - (func.returns() ? 1 : 0)};

        x.comment(tok(), indent, "address of argument '{}' to parameter '{}'",
                  statement::trimmed_source(args_.at(arg_idx)),
                  func.params().at(arg_idx).name());
    }

    // the id has no indexes, so an id naming an array means the result is
    // one of its elements
    auto comment_result_address(toc& tc, const size_t indent,
                                const ident_info& dst_info) const -> void {

        machine& x{tc.machine()};

        if (tc.make_ident_info(tok(), dst_info.id).is_array) {
            x.comment(tok(), indent, "address of result element in array '{}'",
                      dst_info.id);

            return;
        }

        x.comment(tok(), indent, "address of indexed result '{}'", dst_info.id);
    }

    // the registers are freed before the exit label
    auto
    compile_inline_body(toc& tc, const size_t indent, const stmt_def_func& func,
                        const std::span<const alias_info> aliases_to_add,
                        const std::span<const operand> registers_to_free) const
        -> void {

        machine& x{tc.machine()};

        // create unique labels for inlined functions
        const std::string_view call_path{tc.get_call_path()};
        const std::string src_loc{tc.source_location_for_use_in_label(tok())};
        const std::string new_call_path{
            call_path.empty() ? src_loc
                              : std::format("{}.{}", src_loc, call_path),
        };

        const std::string call_label{
            std::format("{}.{}", func.body_label(), new_call_path),
        };

        const std::string ret_jmp_label{std::format("{}.end", call_label)};

        func.source_def_comment_to(x, indent);

        x.label(indent, call_label);

        // enter function scope

        tc.enter_func(func.name(), new_call_path, ret_jmp_label);

        // add aliases
        for (const alias_info& e : aliases_to_add) {
            x.comment_alias(tok(), indent + 1, e.from, e.to, e.lea);
            tc.add_alias(e);
        }

        // the result reaches the destination through the 'res' alias, so the
        // statements of the body have no destination
        func.code().compile(tc, indent, ident_info::make_empty());

        free_in_reverse(x, tok(), indent + 1, registers_to_free);

        // provide the exit label for 'return' to jump to

        x.label(indent, ret_jmp_label);

        // apply unary ops to result if present

        if (not get_unary_ops().is_empty()) {
            assert(func.returns());

            const func_return_info& return_info{*func.returns()};
            const ident_info& ret_info{
                tc.make_ident_info(tok(), return_info.ident_tk.text()),
            };

            get_unary_ops().compile(tc, indent, tok(), ret_info.operand);
        }

        tc.exit_func(func.name());
    }

    // a new register per argument because the index register may belong to
    // an enclosing alias that is used after the call
    auto compile_inline_body_with_address_registers(
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
                x.alloc_scratch_register(tok(), indent, tc.get_type_address()),
            };

            address_registers.push_back(address);

            x.comment(tok(), indent, "address of parameter '{}'", alias.from);
            alias.lea = load_indexed_address(x, indent, address, alias.lea,
                                             keeps_displacement);
        }

        compile_inline_body(tc, indent, func, aliases_to_add,
                            address_registers);
    }

    auto compile_inline_call(toc& tc, const size_t indent,
                             const ident_info& dst_info,
                             const stmt_def_func& func) const -> void {

        if (not dst_info.operand.is_memory()) {
            compile_inline(tc, indent, dst_info, func);
            return;
        }

        compile_inline_to_memory(tc, indent, dst_info, func);
    }

    // the result is written to memory by the body of the function
    auto compile_inline_to_memory(toc& tc, const size_t indent,
                                  const ident_info& dst_info,
                                  const stmt_def_func& func) const -> void {

        machine& x{tc.machine()};

        const bool has_unary_ops{not get_unary_ops().is_empty()};
        const bool has_indexed_result{
            not dst_info.operand.index_register().empty(),
        };

        // without base + index addressing each result access adds base and
        // index again, computing the address once can be shorter
        if (not has_unary_ops and has_indexed_result) {
            x.emit_most_efficient(
                tok(), indent,
                [&] -> void { compile_inline(tc, indent, dst_info, func); },
                [&] -> void {
                    emit_most_efficient_address(
                        x, indent, [&](const bool keeps) -> void {
                            compile_inline_with_address_register(
                                tc, indent, dst_info, func, keeps);
                        });
                });

            return;
        }

        if (not has_unary_ops) {
            compile_inline(tc, indent, dst_info, func);
            return;
        }

        // unary ops on a memory result are a load, modify and store on a
        // load/store machine, a scratch register can be shorter
        x.emit_most_efficient(
            tok(), indent,
            [&] -> void { compile_inline(tc, indent, dst_info, func); },
            [&] -> void {
                const operand reg{
                    x.alloc_scratch_register(tok(), indent,
                                             dst_info.type_ref()),
                };

                compile_inline(tc, indent,
                               toc::make_ident_info_from_register(reg), func);

                x.copy_value(tok(), indent, dst_info.operand, reg);
                x.free_scratch_register(tok(), indent, reg);
            });
    }

    // a new register because the index register may belong to an enclosing
    // alias that is used after the call
    auto compile_inline_with_address_register(
        toc& tc, const size_t indent, const ident_info& dst_info,
        const stmt_def_func& func, const bool keeps_displacement) const
        -> void {

        machine& x{tc.machine()};

        const operand address{
            x.alloc_scratch_register(tok(), indent, tc.get_type_address()),
        };

        comment_result_address(tc, indent, dst_info);

        ident_info address_info{dst_info};
        address_info.operand = load_indexed_address(
            x, indent, address, dst_info.operand, keeps_displacement);

        compile_inline(tc, indent, address_info, func);

        x.free_scratch_register(tok(), indent, address);
    }

    [[nodiscard]] auto describe_argument(const size_t index) const
        -> std::string {

        if (index < first_argument_index()) {
            return "receiver";
        }

        return std::format("argument {}", index + 1 - first_argument_index());
    }

    // the address register holds the address with or without the
    // displacement, the first is shorter when the accesses exceed the load
    // and store immediate range
    auto emit_most_efficient_address(
        machine& x, const size_t indent,
        const std::function_ref<void(bool keeps_displacement)> emit) const
        -> void {

        x.emit_most_efficient(
            tok(), indent, [&] -> void { emit(false); },
            [&] -> void { emit(true); });
    }

    // the receiver is not counted as an argument
    [[nodiscard]] auto first_argument_index() const -> size_t {
        return is_method() ? 1 : 0;
    }

    // the result address precedes the argument addresses
    [[nodiscard]] auto
    frame_slot_addresses(toc& tc, const size_t indent,
                         const ident_info& dst_info, const stmt_def_func& func,
                         std::vector<operand>& address_registers) const
        -> std::vector<operand> {

        std::vector<operand> addresses;

        if (func.returns()) {
            // note: 'stmt_assign_var' resolves a pointer slot before the call
            assert(not dst_info.is_pointer or dst_info.use_operand);

            addresses.push_back(dst_info.operand);
        }

        for (const expr_any& arg : args_) {
            const ident_info info{tc.make_ident_info(arg)};

            addresses.push_back(
                tc.get_lea_operand(indent, arg, info, address_registers));
        }

        return addresses;
    }

    [[nodiscard]] auto is_constructor() const -> bool {
        return not constructor_dot_tk_.is_empty();
    }

    [[nodiscard]] auto is_inside_call(const compiler_exception& e) const
        -> bool {

        // note: a recursive call is checked inside its own body, so an error
        //       can be located after the call
        return e.start_index >= call_begin_token().start_index() and
               e.start_index <= close_paren_tk_.end_index();
    }

    [[nodiscard]] auto is_method() const -> bool {
        return not method_dot_tk_.is_empty();
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
            x.address_of(tok(), indent, dst, location);
            return operand::mem(dst, location.type_ref());
        }

        x.address_of(tok(), indent, dst,
                     operand::mem(location.base_register(),
                                  location.index_register(), location.scale(),
                                  0, location.type_ref()));

        return operand::mem(dst.base_register(), {}, 1, location.displacement(),
                            location.type_ref());
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
            if (i == first and tz.peek_char_after_whitespace() == ')') {
                throw_missing_argument(tz, params.at(i), i);
            }

            if (i != first) {
                const token t{tz.is_next_char_token(',')};
                if (t.is_empty()) {
                    throw_missing_argument(tz, params.at(i), i);
                }
                arg_delims_tk_.emplace_back(t);
            }

            args_.emplace_back(tc, tz, params.at(i).get_type(), true, false, 0);
        }

        close_paren_tk_ = tz.is_next_char_token(')');
        if (close_paren_tk_.is_empty()) {
            throw compiler_exception{tz, "expected ')' after arguments"};
        }

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
            }
        }
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
            return;
        }

        get_unary_ops().source_to(os);
        args_.front().source_to(os);
        method_dot_tk_.source_to(os);
        tok().source_to(os);
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

    // a non-inline call passes addresses, so only a plain variable of the
    // parameter's type fits
    static auto assert_noninline_argument(const toc& tc, const expr_any& arg,
                                          const stmt_def_func_param& param)
        -> void {

        if (arg.is_expression()) {
            throw compiler_exception{arg.tok(),
                                     "expression arguments are unsupported"};
        }

        if (not arg.get_unary_ops().is_empty()) {
            throw compiler_exception{
                arg.tok(), "unary operators on arguments are unsupported"};
        }

        const ident_info info{tc.make_ident_info(arg)};

        if (not info.is_var()) {
            throw compiler_exception{arg.tok(), "argument must be a variable"};
        }

        if (info.is_array) {
            throw compiler_exception{arg.tok(),
                                     "whole-array arguments are unsupported"};
        }

        // note: a non-array argument for an array parameter is rejected when
        //       the call is parsed and an array argument just above
        assert(not param.is_array());

        if (&info.type_ref() != &param.get_type()) {
            throw_parameter_type_mismatch(arg, param, info);
        }
    }

    static auto free_in_reverse(machine& x, const token& src_loc_tk,
                                const size_t indent,
                                const std::span<const operand> registers)
        -> void {

        for (const operand& r : registers | std::views::reverse) {
            x.free_scratch_register(src_loc_tk, indent, r);
        }
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
            return make_value_alias(param, arg.identifier());
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

    // a constant lets the inlined body be decided at compile time
    [[nodiscard]] static auto
    make_expression_alias(toc& tc, const size_t indent, const expr_any& arg,
                          const stmt_def_func_param& param,
                          std::vector<operand>& allocated_registers)
        -> alias_info {

        const std::optional<int64_t> value{arg.constant_value(tc)};
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
            arg.compile_lea(tc, indent, arg.tok(), regs_lea, {},
                            arg_info.lea_path, {}),
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
        };
    }

    // the result name of the function refers to the destination
    [[nodiscard]] static auto make_result_alias(const ident_info& dst_info,
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
        };
    }

    [[nodiscard]] static auto make_value_alias(const stmt_def_func_param& param,
                                               const std::string_view to)
        -> alias_info {

        return {
            .from{std::string{param.identifier()}},
            .to{std::string{to}},
            .lea{},
            .type_ptr{&param.get_type()},
            .register_operand{},
            .is_element{},
        };
    }
};
