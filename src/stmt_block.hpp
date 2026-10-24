#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <ostream>
#include <vector>

#include "compile_error.hpp"
#include "decouple.hpp"
#include "ident_info.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "ub_assigned.hpp"

// whether a block may be one statement without braces, a function body may not
enum class block_braces : uint8_t { optional, required };

// a scope of statements, '{...}' or one statement without braces; its
// declarations end with it, in 'toc' and in the definite-assignment walk,
// and each block is one level of the parse nesting limit
class stmt_block final : public statement {
    token open_brace_tk_;
    std::vector<std::unique_ptr<statement>> statements_;
    token close_brace_tk_;

  public:
    // without '{' the block is one statement unless braces are required,
    // e.g. the 'exit(1)' of 'if a == 1 exit(1)'
    stmt_block(toc& tc, tokenizer& tz, const block_braces braces = {})
        : statement{tz.cur_position_token()},
          open_brace_tk_{tz.is_next_char_token('{')} {

        const toc::parse_nesting_scope nesting{tc, tz};

        set_type(tc.builtins().void_type());

        if (is_one_statement() and braces == block_braces::required) {
            throw compile_error{tz, "expected '{' to begin block"};
        }

        const toc::frame_scope block{tc, frame::block()};

        if (is_one_statement()) {
            parse_single_statement(tc, tz);
        } else {
            parse_statements(tc, tz);
        }
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

    // a statement in a block has no destination, its value is not used
    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        const toc::frame_scope block{tc, frame::block()};

        // the statements after a 'return', 'break', 'continue' or 'exit' that
        // every path takes do not run and are not compiled
        for (const std::unique_ptr<statement>& s : statements_) {
            s->compile(tc, indent + 1, destination{});

            if (not tc.is_path_open()) {
                break;
            }
        }
    }

    auto trace_assignment(assignment_flow& flow) const -> void override {
        const assignment_flow::block_scope scope{flow};

        for (const std::unique_ptr<statement>& s : statements_) {
            s->trace_assignment(flow);

            // statements after 'return', 'break', 'continue' or 'exit' never
            // run
            if (not flow.is_reachable()) {
                break;
            }
        }
    }

    //
    // class methods
    //

    [[nodiscard]] auto is_empty() const -> bool { return statements_.empty(); }

  private:
    [[nodiscard]] auto is_one_statement() const -> bool {
        return open_brace_tk_.is_empty();
    }

    // a block without braces is one statement, none at the end of the source
    auto parse_single_statement(toc& tc, tokenizer& tz) -> void {
        close_brace_tk_ = tz.is_next_char_token('}');

        if (not close_brace_tk_.is_empty()) {
            throw compile_error{close_brace_tk_,
                                "unexpected '}' in single statement block"};
        }

        const token tk{tz.next_token()};

        if (tk.is_none()) {
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
            if (tk.is_none()) {
                tz.assert_not_at_delimiter();

                throw compile_error{tz, "expected '}' to close block"};
            }

            statements_.emplace_back(parse_statement(tc, tz, tk));
        }
    }
};
