#pragma once

#include <format>
#include <optional>
#include <ostream>
#include <string_view>
#include <vector>

#include "compile_error.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"

// a parameter 'name [mut] [type][[]]', or the implicit 'self' of a method,
// which has no source
class stmt_def_func_param final : public statement {
    // the '[]' of 'arr i32[]'
    struct bracket_tokens {
        token open_bracket_tk;
        token close_bracket_tk;
    };

    std::optional<token> mut_tk_;
    // none for the default type
    std::optional<token> type_tk_;
    // none when not an array
    std::optional<bracket_tokens> brackets_;

  public:
    // the tokens of a parameter 'name [mut] [type][[]]', before the type is
    // resolved
    struct syntax {
        token name_tk;
        std::optional<token> mut_tk;
        // none for the default type
        std::optional<token> type_tk;
        // none when not an array
        std::optional<bracket_tokens> brackets;

        [[nodiscard]] auto is_array() const -> bool {
            return brackets.has_value();
        }

        //
        // statics
        //

        // e.g. 'v', 'v i32', 'arr[]' and 'arr i32[]' have the default type or
        // an explicit one, brackets follow the type
        [[nodiscard]] static auto read(const toc& tc, tokenizer& tz) -> syntax {
            const token name_tk{tz.next_token()};

            if (not name_tk.is_word()) {
                throw compile_error{name_tk, "expected a parameter name"};
            }

            // where it is read, also in a generic definition that no instance
            // parses, e.g. the 'mut' of 'func f(a, mut b i8)'
            tc.assert_valid_unqualified_name(name_tk);

            const std::optional<token> mut_tk{read_mut(tz)};

            const std::optional<token> type_tk{read_declared_type(tc, tz)};

            return {
                .name_tk{name_tk},
                .mut_tk{mut_tk},
                .type_tk{type_tk},
                .brackets{read_brackets(tz, type_tk)},
            };
        }

      private:
        //
        // statics
        //

        // 'type_tk' names the element type in the message
        [[nodiscard]] static auto
        read_brackets(tokenizer& tz, const std::optional<token>& type_tk)
            -> std::optional<bracket_tokens> {

            const std::optional<token> open_bracket_tk{
                tz.is_next_char_token('['),
            };

            if (not open_bracket_tk) {
                return std::nullopt;
            }

            const std::optional<token> close_bracket_tk{
                tz.is_next_char_token(']'),
            };

            // e.g. the '4' of 'a i8[4]', each size gets a body of its own
            if (not close_bracket_tk) {
                throw compile_error{
                    tz, std::format("an array parameter takes arrays of any "
                                    "size, write '{}[]'",
                                    type_tk ? type_tk->text() : "")};
            }

            return bracket_tokens{
                .open_bracket_tk{*open_bracket_tk},
                .close_bracket_tk{*close_bracket_tk},
            };
        }

        // 'mut' after the name allows the body to write the parameter
        [[nodiscard]] static auto read_mut(tokenizer& tz)
            -> std::optional<token> {

            const token tk{tz.next_token()};

            if (not tk.is_text("mut")) {
                tz.put_back_token(tk);
                return std::nullopt;
            }

            return tk;
        }
    };

    // the parameters of a signature up to its ')', e.g. 'a i32, b i32)'
    struct syntax_list {
        std::vector<syntax> params;
        // the ',' between the parameters
        std::vector<token> delimiter_tks;
        token close_paren_tk;

        //
        // statics
        //

        // 'tz' is after the '('
        [[nodiscard]] static auto read(const toc& tc, tokenizer& tz)
            -> syntax_list {

            syntax_list list{};

            std::optional<token> close_paren_tk{tz.is_next_char_token(')')};

            while (not close_paren_tk) {
                if (not list.params.empty()) {
                    list.delimiter_tks.push_back(
                        read_delimiter(tz, list.params.back()));
                }

                list.params.push_back(syntax::read(tc, tz));

                close_paren_tk = tz.is_next_char_token(')');
            }

            list.close_paren_tk = *close_paren_tk;

            return list;
        }

      private:
        //
        // statics
        //

        [[nodiscard]] static auto read_delimiter(tokenizer& tz,
                                                 const syntax& previous)
            -> token {

            const token delimiter_tk{
                tz.expect_char_token(
                    ',', std::format("expected ',' or ')' after parameter {}",
                                     in_quotes(previous.name_tk.text()))),
            };

            tz.assert_no_trailing_delimiter(delimiter_tk, ')', "parameters");

            return delimiter_tk;
        }
    };

    stmt_def_func_param(const toc& tc, const syntax& tokens)
        : statement{tokens.name_tk}, mut_tk_{tokens.mut_tk},
          type_tk_{tokens.type_tk}, brackets_{tokens.brackets} {

        set_type(type_tk_ ? tc.get_type_or_throw(*type_tk_)
                          : tc.builtins().default_type());
    }

    // the implicit 'self' ('stmt_def_func::add_self_param'), 'mut_tk' that of
    // the head, e.g. 'func mut point.move(dx)'
    stmt_def_func_param(const token tk, const type& tp,
                        const std::optional<token>& mut_tk)
        : statement{tk}, mut_tk_{mut_tk} {

        set_type(tp);
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        token::source_to(os, mut_tk_);
        token::source_to(os, type_tk_);

        if (brackets_) {
            brackets_->open_bracket_tk.source_to(os);
            brackets_->close_bracket_tk.source_to(os);
        }
    }

    //
    // class methods
    //

    [[nodiscard]] auto is_array() const -> bool {
        return brackets_.has_value();
    }

    // not declared 'mut', so the body cannot write it and arguments sharing
    // storage are not a hazard
    [[nodiscard]] auto is_read_only() const -> bool { return not mut_tk_; }

    [[nodiscard]] auto name() const -> std::string_view { return tok().text(); }
};
