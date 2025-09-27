#pragma once

#include "stmt_break.hpp"
#include "stmt_comment.hpp"
#include "stmt_continue.hpp"
#include "stmt_def_var.hpp"
#include "stmt_return.hpp"
#include "tokenizer.hpp"

class stmt_block final : public statement {
    bool is_one_statement_{};
    std::vector<std::unique_ptr<statement>> stms_;

  public:
    stmt_block(toc& tc, tokenizer& tz)
        : statement{tz.next_whitespace_token()},
          is_one_statement_{not tz.is_next_char('{')} {

        set_type(tc.get_type_void());

        tc.enter_block();
        while (true) {
            // comments, semi-colon not considered a statment
            bool last_statement_considered_no_statment{};
            // end-of block?
            if (tz.is_next_char('}')) {
                if (not is_one_statement_) {
                    break;
                }
                throw compiler_exception(
                    tz, "unexpected '}' in single statement block");
            }

            token tk{tz.next_token()};
            if (tk.is_empty()) {
                throw compiler_exception(
                    tk, "unexpected '" + std::string{tz.peek_char()} + "'");
            }

            if (tk.is_name("var")) {
                stms_.emplace_back(
                    std::make_unique<stmt_def_var>(tc, std::move(tk), tz));
            } else if (tz.is_next_char('=')) {
                stms_.emplace_back(std::make_unique<stmt_assign_var>(
                    tc, std::move(tk), token{}, tz));
            } else if (tk.is_name("break")) {
                stms_.emplace_back(
                    std::make_unique<stmt_break>(tc, std::move(tk)));
            } else if (tk.is_name("continue")) {
                stms_.emplace_back(
                    std::make_unique<stmt_continue>(tc, std::move(tk)));
            } else if (tk.is_name("return")) {
                stms_.emplace_back(
                    std::make_unique<stmt_return>(tc, std::move(tk)));
            } else if (tk.name().starts_with("#")) {
                stms_.emplace_back(
                    std::make_unique<stmt_comment>(tc, std::move(tk), tz));
                last_statement_considered_no_statment = true;
            } else if (tk.is_name("")) {
                // whitespace
                //? should this be able to happend?
                // throw panic_exception("stmt_block:1");
                stms_.emplace_back(
                    std::make_unique<statement>(std::move(tk), unary_ops{}));
                // stms_.back()->set_type(tc.get_type_void());
            } else { // circular reference resolver
                stms_.emplace_back(create_statement_from_tokenizer(
                    tc, unary_ops{}, std::move(tk), tz));
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
        }
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
