#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <ostream>
#include <utility>
#include <vector>

#include "compile_error.hpp"
#include "decouple.hpp"
#include "ident_info.hpp"
#include "stmt_break.hpp"
#include "stmt_builtin_array_copy.hpp"
#include "stmt_continue.hpp"
#include "stmt_def_dat.hpp"
#include "stmt_def_var.hpp"
#include "stmt_return.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "ub_assigned.hpp"

class stmt_block final : public statement {
    token open_brace_tk_;
    std::vector<std::unique_ptr<statement>> statements_;
    token close_brace_tk_;

  public:
    // note: without '{', a single statement is allowed unless braces are
    // required
    // e.g. '{ a = 1 b = 2 }', or the 'exit(1)' of 'if a == 1 exit(1)'
    stmt_block(toc& tc, tokenizer& tz, const bool braces_required = {})
        : statement{tz.cur_position_token()},
          open_brace_tk_{tz.is_next_char_token('{')} {

        const tokenizer::nesting_scope nesting{tz};

        set_type(tc.builtins().void_type());

        if (is_one_statement() and braces_required) {
            throw compile_error{tz, "expected '{' to begin block"};
        }

        tc.enter_block();

        if (is_one_statement()) {
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
        if (not is_one_statement()) {
            open_brace_tk_.source_to(os);
        }

        for (const std::unique_ptr<statement>& s : statements_) {
            s->source_to(os);
        }

        if (not is_one_statement()) {
            close_brace_tk_.source_to(os);
        }
    }

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        tc.enter_block();
        for (const std::unique_ptr<statement>& s : statements_) {
            s->compile(tc, indent + 1, dst);
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
        -> std::optional<assigned_bytes> {

        const assigned_bytes entry{flow.assigned};

        std::optional<assigned_bytes> outer_breaks{
            std::exchange(flow.at_breaks, std::nullopt),
        };

        trace_assignment(flow);

        std::optional<assigned_bytes> breaks{
            std::exchange(flow.at_breaks, std::move(outer_breaks)),
        };

        flow.assigned = entry;
        flow.is_reachable = true;

        return breaks;
    }

  private:
    [[nodiscard]] auto is_one_statement() const -> bool {
        return open_brace_tk_.is_empty();
    }

    // a block without braces is one statement, none at the end of the source
    // e.g. 'exit(1)' of 'if a == 1 exit(1)'
    auto parse_single_statement(toc& tc, tokenizer& tz) -> void {
        close_brace_tk_ = tz.is_next_char_token('}');

        if (not close_brace_tk_.is_empty()) {
            throw compile_error{close_brace_tk_,
                                "unexpected '}' in single statement block"};
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

                throw compile_error{tz, "expected '}' to close block"};
            }

            statements_.emplace_back(parse_statement(tc, tz, tk));
        }
    }

    //
    // statics
    //

    // 'tk' is the first token of the statement
    [[nodiscard]] static auto parse_statement(toc& tc, tokenizer& tz,
                                              const token tk)
        -> std::unique_ptr<statement> {

        if (tk.is_string()) {
            throw compile_error{tk, "expected a statement"};
        }

        if (tk.is_text("var") or tk.is_text("let")) {
            return std::make_unique<stmt_def_var>(tc, tk, tz);
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

        // e.g. 'lst.add(1)', 'p1 = p2' or 'show<name>(x)'
        return create_statement_of_identifier(tc, tz, tk);
    }
};
