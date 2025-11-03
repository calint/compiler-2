#pragma once
// reviewed: 2025-09-28

#include <ostream>
#include <print>
#include <string_view>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "stmt_break.hpp"
#include "stmt_builtin_array_copy.hpp"
#include "stmt_comment.hpp"
#include "stmt_continue.hpp"
#include "stmt_def_var.hpp"
#include "stmt_identifier.hpp"
#include "stmt_return.hpp"
#include "tokenizer.hpp"
#include "unary_ops.hpp"
#include <memory>

class stmt_block final : public statement {
    bool is_one_statement_{};
    std::vector<std::unique_ptr<statement>> stms_;
    token ws1_; // leading whitespace
    token ws2_; // end of block additional whitespace
    token ws3_; // whitespace after '}'

  public:
    // note: parser assumes the tokenizer is at a '{' or it is considered a
    // single statement block
    stmt_block(toc& tc, tokenizer& tz)
        : statement{tz.current_position_token()},
          is_one_statement_{not tz.is_next_char('{')} {

        set_type(tc.get_type_void());

        ws1_ = tz.next_whitespace_token();

        tc.enter_block();
        while (true) {
            // comments, semi-colon not considered a statement
            bool last_statement_considered_no_statment{};

            // is it the end of the block?
            if (tz.is_next_char('}')) {
                if (not is_one_statement_) {
                    ws3_ = tz.next_whitespace_token();
                    break;
                }
                throw compiler_exception{
                    tz, "unexpected '}' in single statement block"};
            }

            // is it a subblock?
            if (tz.peek_char() == '{') {
                stms_.emplace_back(std::make_unique<stmt_block>(tc, tz));
                continue;
            }

            const token tk{tz.next_token()};

            // no more tokens in the block?
            if (tk.is_empty()) {
                break;
            }

            if (tk.text().starts_with("#")) {
                stms_.emplace_back(
                    std::make_unique<stmt_comment>(tc, unary_ops{}, tk, tz));
                last_statement_considered_no_statment = true;
            } else if (tk.is_text("var")) {
                stms_.emplace_back(std::make_unique<stmt_def_var>(tc, tk, tz));
            } else if (tk.is_text("break")) {
                stms_.emplace_back(std::make_unique<stmt_break>(tc, tk));
            } else if (tk.is_text("continue")) {
                stms_.emplace_back(std::make_unique<stmt_continue>(tc, tk));
            } else if (tk.is_text("return")) {
                stms_.emplace_back(std::make_unique<stmt_return>(tc, tk));
            } else if (tk.is_text("array_copy")) {
                stms_.emplace_back(
                    std::make_unique<stmt_builtin_array_copy>(tc, tk, tz));
            } else if (tk.is_text("loop") or tk.is_text("if") or
                       tk.is_text("mov") or tk.is_text("syscall")) {
                // note: solves circular reference problem
                //       'loop' and 'if' uses this class
                //       'mov' and 'syscall' are 'stmt_call'
                stms_.emplace_back(create_statement_in_stmt_block(tc, tz, tk));
            } else {
                // is it at end of block and there is whitespace before '}'?
                if (tk.is_text("")) {
                    // yes, save it as this block whitespace
                    ws2_ = tk;
                    continue;
                }
                // resolve identifier
                // note: 'unary_ops' not allowed before destination identifier
                stmt_identifier si{tc, {}, tk, tz};

                if (tz.is_next_char('=')) {
                    stms_.emplace_back(std::make_unique<stmt_assign_var>(
                        tc, tz, std::move(si)));

                } else if (tz.is_peek_char('(')) {
                    // note: solves circular reference
                    stms_.emplace_back(create_stmt_call(tc, tz, si));

                } else {
                    throw compiler_exception{
                        tz, "unexpected character. expected '=' for assignment "
                            "or '(' for function call"};
                }
            }
            if (is_one_statement_ and
                not last_statement_considered_no_statment) {
                break;
            }
        }
        tc.exit_block();
    }

    ~stmt_block() override = default;

    stmt_block() = default;
    stmt_block(const stmt_block&) = default;
    stmt_block(stmt_block&&) = default;
    auto operator=(const stmt_block&) -> stmt_block& = default;
    auto operator=(stmt_block&&) -> stmt_block& = default;

    auto source_to(std::ostream& os) const -> void override {
        if (not is_one_statement_) {
            std::print(os, "{{");
        }
        ws1_.source_to(os);
        for (const auto& s : stms_) {
            s->source_to(os);
        }
        if (not is_one_statement_) {
            std::print(os, "}}");
            ws3_.source_to(os);
        }
        ws2_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        tc.enter_block();
        for (const auto& s : stms_) {
            s->compile(tc, os, indent + 1, dst_info);
        }
        tc.exit_block();
    }

    [[nodiscard]] auto is_var_set(const std::string_view var) const
        -> bool override {

        for (const std::unique_ptr<statement>& st : stms_) {
            if (st->is_var_set(var)) {
                return true;
            }
            if (st->is_code_after_this_unreachable()) {
                break;
            }
        }
        return false;
    }

    auto assert_no_ub_for_var(const std::string_view var) const -> void {
        for (const std::unique_ptr<statement>& st : stms_) {
            st->assert_var_not_used(var);
            if (st->is_var_set(var)) {
                return;
            }
        }
    }

    [[nodiscard]] auto is_empty() const -> bool { return stms_.empty(); }
};
