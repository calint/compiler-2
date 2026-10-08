#pragma once
// reviewed: 2025-09-28

#include <cstddef>
#include <memory>
#include <optional>
#include <ostream>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "stmt_assign_var.hpp"
#include "stmt_break.hpp"
#include "stmt_builtin_array_copy.hpp"
#include "stmt_continue.hpp"
#include "stmt_def_const.hpp"
#include "stmt_def_dat.hpp"
#include "stmt_def_var.hpp"
#include "stmt_identifier.hpp"
#include "stmt_return.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "ub_unset_var.hpp"
#include "unary_ops.hpp"

class stmt_block final : public statement {
    token open_brace_tk_;
    std::vector<std::unique_ptr<statement>> statements_;
    token close_brace_tk_;
    bool is_one_statement_{};

  public:
    // note: without '{', a single statement is allowed unless braces are
    // required
    // e.g. '{ a = 1 b = 2 }', or the 'exit(1)' of 'if a == 1 exit(1)'
    stmt_block(toc& tc, tokenizer& tz, const bool braces_required = {})
        : statement{tz.cur_position_token()},
          open_brace_tk_{tz.is_next_char_token('{')},
          is_one_statement_{open_brace_tk_.is_empty()} {

        const tokenizer::nesting_scope nesting{tz};

        set_type(tc.get_type_void());

        if (is_one_statement_ and braces_required) {
            throw compiler_exception{tz, "expected '{' to begin block"};
        }

        tc.enter_block();

        if (is_one_statement_) {
            parse_single_statement(tc, tz);
        } else {
            parse_statements(tc, tz);
        }

        tc.exit_block();
    }

    stmt_block() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        if (not is_one_statement_) {
            open_brace_tk_.source_to(os);
        }

        for (const std::unique_ptr<statement>& s : statements_) {
            s->source_to(os);
        }

        if (not is_one_statement_) {
            close_brace_tk_.source_to(os);
        }
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        tc.enter_block();
        for (const std::unique_ptr<statement>& s : statements_) {
            s->compile(tc, indent + 1, dst_info);
        }

        tc.exit_block();
    }

    auto trace_assignment(assignment_flow& flow) const -> void override {
        for (const std::unique_ptr<statement>& s : statements_) {
            s->trace_assignment(flow);

            // statements after 'return', 'break', 'continue' or 'exit' never
            // run
            if (not flow.is_reachable) {
                return;
            }
        }
    }

    //
    // class methods
    //

    [[nodiscard]] auto is_empty() const -> bool { return statements_.empty(); }

    // every iteration starts with at least the coverage at loop entry
    // returns the coverage common to every 'break' of this loop body
    [[nodiscard]] auto trace_loop_body(assignment_flow& flow) const
        -> std::optional<field_coverage> {

        const field_coverage entry{flow.assigned};

        std::optional<field_coverage> outer_breaks{
            std::exchange(flow.at_breaks, std::nullopt),
        };

        trace_assignment(flow);

        std::optional<field_coverage> breaks{
            std::exchange(flow.at_breaks, std::move(outer_breaks)),
        };

        flow.assigned = entry;
        flow.is_reachable = true;

        return breaks;
    }

  private:
    // a block without braces is one statement, none at the end of the source
    // e.g. 'exit(1)' of 'if a == 1 exit(1)'
    auto parse_single_statement(toc& tc, tokenizer& tz) -> void {
        close_brace_tk_ = tz.is_next_char_token('}');

        if (not close_brace_tk_.is_empty()) {
            throw compiler_exception{
                close_brace_tk_, "unexpected '}' in single statement block"};
        }

        const token tk{tz.next_token()};

        if (tk.text().empty() and not tk.is_string()) {
            tz.assert_not_at_delimiter();
            return;
        }

        statements_.emplace_back(parse_statement(tc, tz, tk));
    }

    // the statements up to the '}', e.g. 'var a = 1 exit(a)' of
    // '{ var a = 1 exit(a) }'
    auto parse_statements(toc& tc, tokenizer& tz) -> void {
        while (true) {
            // the '}' ends the block
            close_brace_tk_ = tz.is_next_char_token('}');

            if (not close_brace_tk_.is_empty()) {
                return;
            }

            // a '{' starts a subblock, e.g. '{ { a = 1 } b = 2 }'
            if (const token open_brace_tk{tz.is_next_char_token('{')};
                not open_brace_tk.is_empty()) {

                tz.put_back_token(open_brace_tk);
                statements_.emplace_back(std::make_unique<stmt_block>(tc, tz));
                continue;
            }

            const token tk{tz.next_token()};

            // the source ended before the '}'
            if (tk.text().empty() and not tk.is_string()) {
                tz.assert_not_at_delimiter();

                throw compiler_exception{tz, "expected '}' to close block"};
            }

            statements_.emplace_back(parse_statement(tc, tz, tk));
        }
    }

    //
    // statics
    //

    // a method call, an assignment or a function call
    // note: 'unary_ops' not allowed before destination identifier
    [[nodiscard]] static auto parse_identifier_statement(toc& tc, tokenizer& tz,
                                                         const token tk)
        -> std::unique_ptr<statement> {

        // the discarded result is rejected at compile like any other call
        // e.g. 'point.at(1, 2)'
        if (is_constructor_call(tc, tk, tz)) {
            return create_stmt_constructor_call(tc, tz, tk);
        }

        assert_no_type_args_for_plain_func(tc, tk, tz);

        stmt_identifier si{tc, {}, tk, tz};

        // e.g. 'lst.add(1)'
        if (si.is_method_receiver()) {
            return create_stmt_method_call(tc, tz, std::move(si));
        }

        // e.g. 'show<name>(x)', the call reads the type arguments
        if (is_generic_call(tc, si.identifier(), tz)) {
            return create_stmt_call(tc, tz, si, token{});
        }

        // e.g. 'p1 = p2'
        if (const token equals_tk{tz.is_next_char_token('=')};
            not equals_tk.is_empty()) {

            return std::make_unique<stmt_assign_var>(tc, tz, std::move(si),
                                                     equals_tk);
        }

        // e.g. 'show(x)'
        // note: solves circular reference
        if (const token open_paren_tk{tz.is_next_char_token('(')};
            not open_paren_tk.is_empty()) {

            return create_stmt_call(tc, tz, si, open_paren_tk);
        }

        throw compiler_exception{
            tz, "unexpected character; expected '=' for assignment or '(' "
                "for function call"};
    }

    // 'tk' is the first token of the statement
    [[nodiscard]] static auto parse_statement(toc& tc, tokenizer& tz,
                                              const token tk)
        -> std::unique_ptr<statement> {

        if (tk.is_string()) {
            throw compiler_exception{tk, "expected a statement"};
        }

        if (tk.is_text("var")) {
            return std::make_unique<stmt_def_var>(tc, tk, tz);
        }

        if (tk.is_text("let")) {
            return stmt_def_const::parse_let(tc, tz, tk);
        }

        if (tk.is_text("dat")) {
            return std::make_unique<stmt_def_dat>(tc, tk, tz);
        }

        if (tk.is_text("break")) {
            return std::make_unique<stmt_break>(tc, tk);
        }

        if (tk.is_text("continue")) {
            return std::make_unique<stmt_continue>(tc, tk);
        }

        if (tk.is_text("return")) {
            return std::make_unique<stmt_return>(tc, tk);
        }

        if (tk.is_text("array_copy")) {
            return std::make_unique<stmt_builtin_array_copy>(tc, tk, tz);
        }

        // note: solves circular reference problem, 'loop' and 'if' use this
        //       class
        if (std::unique_ptr<statement> built{
                create_statement_in_stmt_block(tc, tz, tk),
            }) {

            return built;
        }

        return parse_identifier_statement(tc, tz, tk);
    }
};
