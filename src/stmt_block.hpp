#pragma once
// reviewed: 2025-09-28

#include <memory>
#include <ostream>
#include <string_view>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "stmt_break.hpp"
#include "stmt_builtin_array_copy.hpp"
#include "stmt_comment.hpp"
#include "stmt_continue.hpp"
#include "stmt_def_const.hpp"
#include "stmt_def_dat.hpp"
#include "stmt_def_var.hpp"
#include "stmt_identifier.hpp"
#include "stmt_return.hpp"
#include "tokenizer.hpp"
#include "unary_ops.hpp"

class stmt_block final : public statement {
    token open_brace_tk_;
    std::vector<std::unique_ptr<statement>> stms_;
    token close_brace_tk_;
    bool is_one_statement_{};

  public:
    // note: parser assumes the tokenizer is at a '{' or it is considered a
    // single statement block
    stmt_block(toc& tc, tokenizer& tz)
        : statement{tz.current_position_token()},
          open_brace_tk_{tz.is_next_char_token('{')} {

        set_type(tc.get_type_void());

        if (open_brace_tk_.is_empty()) {
            is_one_statement_ = true;
        }

        tc.enter_block();
        while (true) {
            // comments, semi-colon not considered a statement
            bool last_statement_considered_no_statement{};

            // is it the end of the block?
            close_brace_tk_ = tz.is_next_char_token('}');
            if (not close_brace_tk_.is_empty()) {
                if (not is_one_statement_) {
                    break;
                }
                throw compiler_exception{
                    close_brace_tk_,
                    "unexpected '}' in single statement block"};
            }

            // is it a subblock?
            if (const token t = tz.is_next_char_token('{'); not t.is_empty()) {
                tz.put_back_token(t);
                stms_.emplace_back(std::make_unique<stmt_block>(tc, tz));
                continue;
            }

            const token tk{tz.next_token()};

            if (tk.is_empty()) {
                // no more tokens in the block?
                if (not is_one_statement_) {
                    throw compiler_exception{tz, "expected '}' to close block"};
                }
                break;
            }

            if (tk.text().starts_with("#")) {
                stms_.emplace_back(
                    std::make_unique<stmt_comment>(tc, unary_ops{}, tk, tz));

                last_statement_considered_no_statement = true;
            } else if (tk.is_text("var")) {
                stms_.emplace_back(std::make_unique<stmt_def_var>(tc, tk, tz));
            } else if (tk.is_text("const")) {
                stms_.emplace_back(
                    std::make_unique<stmt_def_const>(tc, tk, tz));
            } else if (tk.is_text("dat")) {
                stms_.emplace_back(std::make_unique<stmt_def_dat>(tc, tk, tz));
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
                       tk.is_text("mov") or tk.is_text("syscall") or
                       tk.is_text("foo")) {

                stms_.emplace_back(create_statement_in_stmt_block(tc, tz, tk));
                // note: solves circular reference problem
                //       'loop' and 'if' uses this class
                //       'mov' and 'syscall' are 'stmt_call'
            } else {
                // resolve identifier
                stmt_identifier si{tc, {}, tk, tz};
                // note: 'unary_ops' not allowed before destination identifier

                if (const token t{tz.is_next_char_token('=')};
                    not t.is_empty()) {

                    stms_.emplace_back(std::make_unique<stmt_assign_var>(
                        tc, tz, std::move(si), t, si.is_array(),
                        si.array_size()));

                } else if (const token tt{tz.is_next_char_token('(')};
                           not tt.is_empty()) {

                    stms_.emplace_back(create_stmt_call(tc, tz, si, tt));
                    // note: solves circular reference

                } else {
                    throw compiler_exception{
                        tz, "unexpected character; expected '=' for assignment "
                            "or '(' for function call"};
                }
            }
            if (is_one_statement_ and
                not last_statement_considered_no_statement) {

                break;
            }
        }
        tc.exit_block();
    }

    stmt_block() = default;

    auto source_to(std::ostream& os) const -> void override {
        if (not is_one_statement_) {
            open_brace_tk_.source_to(os);
        }
        for (const std::unique_ptr<statement>& s : stms_) {
            s->source_to(os);
        }
        if (not is_one_statement_) {
            close_brace_tk_.source_to(os);
        }
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        tc.enter_block();
        for (const std::unique_ptr<statement>& s : stms_) {
            s->compile(tc, indent + 1, dst_info);
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
