#pragma once
// reviewed: 2025-09-28

#include "decouple.hpp"
#include "stmt_break.hpp"
#include "stmt_comment.hpp"
#include "stmt_continue.hpp"
#include "stmt_def_var.hpp"
#include "stmt_identifier.hpp"
#include "stmt_return.hpp"
#include "tokenizer.hpp"
#include "unary_ops.hpp"

class stmt_block final : public statement {
    bool is_one_statement_{};
    std::vector<std::unique_ptr<statement>> stms_;
    token ws1_;
    token ws2_;

  public:
    stmt_block(toc& tc, tokenizer& tz)
        : statement{tz.next_whitespace_token()},
          is_one_statement_{not tz.is_next_char('{')} {

        set_type(tc.get_type_void());

        tc.enter_block();
        while (true) {
            // comments, semi-colon not considered a statement
            bool last_statement_considered_no_statment{};

            // is it end-of block?
            if (tz.is_next_char('}')) {
                if (not is_one_statement_) {
                    ws2_ = tz.next_whitespace_token();
                    break;
                }
                throw compiler_exception(
                    tz, "unexpected '}' in single statement block");
            }

            // no more tokens in the block?
            token tk{tz.next_token()};
            if (tk.is_empty()) {
                break;
            }

            if (tk.name().starts_with("#")) {
                stms_.emplace_back(std::make_unique<stmt_comment>(
                    tc, unary_ops{}, std::move(tk), tz));
                last_statement_considered_no_statment = true;
            } else if (tk.is_name("var")) {
                stms_.emplace_back(
                    std::make_unique<stmt_def_var>(tc, std::move(tk), tz));
            } else if (tk.is_name("break")) {
                stms_.emplace_back(
                    std::make_unique<stmt_break>(tc, std::move(tk)));
            } else if (tk.is_name("continue")) {
                stms_.emplace_back(
                    std::make_unique<stmt_continue>(tc, std::move(tk)));
            } else if (tk.is_name("return")) {
                stms_.emplace_back(
                    std::make_unique<stmt_return>(tc, std::move(tk)));
            } else if (tk.is_name("loop") or tk.is_name("if") or
                       tk.is_name("mov") or tk.is_name("syscall")) {
                // note: solves circular reference problem
                //       'loop' and 'if' uses this class
                //       'mov' and 'syscall' are 'stmt_call'
                stms_.emplace_back(
                    create_statement_in_stmt_block(tc, tz, std::move(tk)));
            } else {
                if (tk.is_name("")) {
                    ws1_ = std::move(tk);
                    continue;
                }
                // resolve identifier
                stmt_identifier si{tc, {}, std::move(tk), tz};

                tk = si.first_token();
                if (tz.is_next_char('=')) {
                    stms_.emplace_back(std::make_unique<stmt_assign_var>(
                        tc, tz, std::move(si)));
                } else {
                    // note: solves circular reference
                    stms_.emplace_back(create_stmt_call(tc, tz, si));
                }
            }
            if (is_one_statement_ and
                not last_statement_considered_no_statment) {
                break;
            }
        }
        tc.exit_block();
    }

    stmt_block() = default;
    stmt_block(const stmt_block&) = default;
    stmt_block(stmt_block&&) = default;
    auto operator=(const stmt_block&) -> stmt_block& = default;
    auto operator=(stmt_block&&) -> stmt_block& = default;

    ~stmt_block() override = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        if (not is_one_statement_) {
            os << "{";
        }
        for (const auto& s : stms_) {
            s->source_to(os);
        }
        if (not is_one_statement_) {
            os << "}";
            ws2_.source_to(os);
        }
        ws1_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 [[maybe_unused]] const std::string& dst = "") const
        -> void override {

        tc.enter_block();
        for (const auto& s : stms_) {
            s->compile(tc, os, indent + 1);
        }
        tc.exit_block();
    }

    [[nodiscard]] auto is_empty() const -> bool { return stms_.empty(); }
};
