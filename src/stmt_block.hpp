#pragma once
#include "stmt_break.hpp"
#include "stmt_comment.hpp"
#include "stmt_continue.hpp"
#include "stmt_def_var.hpp"
#include "stmt_return.hpp"
#include "tokenizer.hpp"

class stmt_block final : public statement {
  bool is_one_statement_{};
  vector<unique_ptr<statement>> stms_{};

public:
  inline stmt_block(toc &tc, tokenizer &tz)
      : statement{{}, tz.next_whitespace_token()},
        is_one_statement_{not tz.is_next_char('{')} {

    tc.enter_block();
    while (true) {
      // comments, semi-colon not considered a statment
      bool last_statement_considered_no_statment{false};
      if (tz.is_next_char('}')) {
        if (not is_one_statement_) {
          break;
        }
        throw compiler_exception(tz,
                                 "unexpected '}' in single statement block");
      }

      const token tk{tz.next_token()};
      if (tk.is_empty()) {
        throw compiler_exception(tk,
                                 "unexpected '" + string{tz.peek_char()} + "'");
      }

      if (tk.is_name("var")) {
        stms_.emplace_back(make_unique<stmt_def_var>(tc, tk, tz));
      } else if (tz.is_next_char('=')) {
        stms_.emplace_back(make_unique<stmt_assign_var>(tc, tk, token{}, tz));
      } else if (tk.is_name("break")) {
        stms_.emplace_back(make_unique<stmt_break>(tc, tk));
      } else if (tk.is_name("continue")) {
        stms_.emplace_back(make_unique<stmt_continue>(tc, tk));
      } else if (tk.is_name("return")) {
        stms_.emplace_back(make_unique<stmt_return>(tc, tk));
      } else if (tk.is_name("#")) {
        stms_.emplace_back(make_unique<stmt_comment>(tc, tk, tz));
        last_statement_considered_no_statment = true;
      } else if (tk.is_name("")) {
        // whitespace
        //? should this be able to happend?
        // throw panic_exception("stmt_block:1");
        stms_.emplace_back(make_unique<statement>(unary_ops{}, tk));
        // stms_.back()->set_type(tc.get_type_void());
      } else { // circular reference resolver
        const unary_ops uops{};
        stms_.emplace_back(create_statement_from_tokenizer(tc, uops, tk, tz));
      }

      if (is_one_statement_ and not last_statement_considered_no_statment) {
        break;
      }
    }
    tc.exit_block();
    set_type(tc.get_type_void());
  }

  inline stmt_block() = default;
  inline stmt_block(const stmt_block &) = default;
  inline stmt_block(stmt_block &&) = default;
  inline auto operator=(const stmt_block &) -> stmt_block & = default;
  inline auto operator=(stmt_block &&) -> stmt_block & = default;

  inline ~stmt_block() override = default;

  inline void source_to(ostream &os) const override {
    statement::source_to(os);
    if (not is_one_statement_) {
      os << "{";
    }
    for (const auto &s : stms_) {
      s->source_to(os);
    }
    if (not is_one_statement_) {
      os << "}";
    }
  }

  inline void compile(toc &tc, ostream &os, size_t indent,
                      [[maybe_unused]] const string &dst = "") const override {
    tc.enter_block();
    for (const auto &s : stms_) {
      s->compile(tc, os, indent + 1);
    }
    tc.exit_block();
  }

  [[nodiscard]] inline auto is_empty() const -> bool { return stms_.empty(); }
};
