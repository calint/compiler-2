#pragma once

#include <cstddef>
#include <optional>
#include <ostream>
#include <string_view>
#include <utility>

#include "compile_error.hpp"
#include "expr_any.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"

// a field 'name [type][[size]]' of a type, its type resolved when parsed like
// a parameter's, the default type when none is written
class stmt_def_type_field final : public statement {
    // e.g. the '[4]' of 'tags[4]'
    struct array_part {
        token open_bracket_tk;
        stmt_array_size size;
        token close_bracket_tk;
    };

    // none for the default type
    std::optional<token> type_tk_;
    // none when not an array
    std::optional<array_part> array_;

  public:
    // 'type_name_tk' names the type the field is in
    stmt_def_type_field(toc& tc, const token tk, tokenizer& tz,
                        const token& type_name_tk)
        : statement{tk} {

        if (not tk.is_word()) {
            throw compile_error{tk, "expected a field name"};
        }

        // note: checked where it is read, before the errors of later fields
        toc::assert_valid_name(tk);

        // e.g. 'tags[4]' and the 'x' of '{x, y}' have the default type
        if (tz.peek_char_after_whitespace() != '[') {
            const token type_tk{tz.next_token()};

            if (type_tk.is_none()) {
                tz.put_back_token(type_tk);
            } else {
                type_tk_ = type_tk;
            }
        }

        assert_no_type_arguments(tc, tz, type_tk_);

        // e.g. the 'p' of 'type p {x p}', the type is defined after its fields
        if (type_tk_ and type_tk_->text() == type_name_tk.text()) {
            throw compile_error{*type_tk_,
                                std::format("type {} cannot contain itself",
                                            in_quotes(type_name_tk.text()))};
        }

        set_type(type_tk_ ? tc.get_type_or_throw(*type_tk_)
                          : tc.builtins().default_type());

        const std::optional<token> open_bracket_tk{tz.is_next_char_token('[')};

        if (not open_bracket_tk) {
            return;
        }

        stmt_array_size size{tc, tz};

        // a field has no initializer to give an unsized array its length
        if (not size.has_value()) {
            throw compile_error{
                size.tok(), "expected a constant array size greater than 0"};
        }

        const token close_bracket_tk{
            tz.expect_char_token(']', "expected ']' after array size"),
        };

        array_ = {
            .open_bracket_tk{*open_bracket_tk},
            .size{std::move(size)},
            .close_bracket_tk{close_bracket_tk},
        };
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);

        token::source_to(os, type_tk_);

        if (array_) {
            array_->open_bracket_tk.source_to(os);
            array_->size.source_to(os);
            array_->close_bracket_tk.source_to(os);
        }
    }

    //
    // class methods
    //

    [[nodiscard]] auto array_count() const -> std::optional<size_t> {
        if (not array_) {
            return std::nullopt;
        }

        return array_->size.array_size();
    }

    [[nodiscard]] auto name() const -> std::string_view { return tok().text(); }
};
