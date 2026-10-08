#pragma once
// reviewed: 2025-09-28

#include <ostream>
#include <string_view>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"

class stmt_def_func_param final : public statement {
    token mut_tk_;
    token type_tk_;
    token open_bracket_tk_;
    token close_bracket_tk_;
    bool is_array_{};

    // not declared 'mut', so the body cannot write it and arguments sharing
    // storage are not a hazard
    bool is_read_only_{};

  public:
    // the tokens of a parameter 'name [mut] [type][[]]', before the type is
    // resolved
    struct syntax {
        token name_tk;
        token mut_tk;
        token type_tk;
        token open_bracket_tk;
        token close_bracket_tk;

        [[nodiscard]] auto is_array() const -> bool {
            return not open_bracket_tk.is_empty();
        }

        //
        // statics
        //

        // e.g. 'v', 'v i32', 'arr[]' and 'arr i32[]' have the default type or
        // an explicit one, brackets follow the type
        [[nodiscard]] static auto read(tokenizer& tz) -> syntax {
            const token name_tk{tz.next_token()};

            if (name_tk.text().empty()) {
                throw compiler_exception{tz, "expected a parameter name"};
            }

            const token mut_tk{read_mut(tz)};

            const token type_tk{
                is_end_of_type(tz.peek_char_after_whitespace())
                    ? token{}
                    : tz.next_token(),
            };

            const token open_bracket_tk{tz.is_next_char_token('[')};

            const token close_bracket_tk{
                open_bracket_tk.is_empty() ? token{}
                                           : tz.is_next_char_token(']'),
            };

            if (not open_bracket_tk.is_empty() and
                close_bracket_tk.is_empty()) {

                throw compiler_exception{tz, "expected ']'"};
            }

            return {
                .name_tk{name_tk},
                .mut_tk{mut_tk},
                .type_tk{type_tk},
                .open_bracket_tk{open_bracket_tk},
                .close_bracket_tk{close_bracket_tk},
            };
        }

      private:
        //
        // statics
        //

        [[nodiscard]] static auto is_end_of_type(const char next) -> bool {
            return next == ',' or next == ')' or next == '[';
        }

        // 'mut' after the name allows the body to write the parameter
        [[nodiscard]] static auto read_mut(tokenizer& tz) -> token {
            token mut_tk;

            if (not is_end_of_type(tz.peek_char_after_whitespace())) {
                const token tk{tz.next_token()};

                if (tk.is_text("mut")) {
                    mut_tk = tk;
                } else {
                    tz.put_back_token(tk);
                }
            }

            return mut_tk;
        }
    };

    stmt_def_func_param(const toc& tc, tokenizer& tz)
        : stmt_def_func_param{tc, syntax::read(tz)} {}

    stmt_def_func_param(const token tk, const type& tp, const bool is_read_only)
        : statement{tk}, is_read_only_{is_read_only} {

        set_type(tp);
    }

    stmt_def_func_param() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        mut_tk_.source_to(os);

        if (not type_tk_.is_empty()) {
            type_tk_.source_to(os);
        }

        if (is_array_) {
            open_bracket_tk_.source_to(os);
            close_bracket_tk_.source_to(os);
        }
    }

    //
    // class methods
    //

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }

    [[nodiscard]] auto is_read_only() const -> bool { return is_read_only_; }

    [[nodiscard]] auto name() const -> std::string_view { return tok().text(); }

  private:
    stmt_def_func_param(const toc& tc, const syntax& tokens)
        : statement{tokens.name_tk}, mut_tk_{tokens.mut_tk},
          type_tk_{tokens.type_tk}, open_bracket_tk_{tokens.open_bracket_tk},
          close_bracket_tk_{tokens.close_bracket_tk},
          is_array_{tokens.is_array()},
          is_read_only_{tokens.mut_tk.is_empty()} {

        toc::assert_valid_name(tok());

        set_type(type_tk_.is_empty()
                     ? tc.get_type_default()
                     : tc.get_type_or_throw(type_tk_, type_tk_.text()));
    }
};
