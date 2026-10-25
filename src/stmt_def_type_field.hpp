#pragma once

#include <cstddef>
#include <optional>
#include <ostream>
#include <string_view>

#include "compile_error.hpp"
#include "expr_any.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"

// a field 'name [type][[size]]' of a type, its type resolved when parsed like
// a parameter's, the default type when none is written
class stmt_def_type_field final : public statement {
    // none for the default type
    std::optional<token> type_tk_;
    // none when not an array
    std::optional<token> open_bracket_tk_;
    token close_bracket_tk_;
    stmt_array_size array_size_;
    // empty when not an array
    std::optional<size_t> array_count_;

  public:
    stmt_def_type_field(toc& tc, const token tk, tokenizer& tz)
        : statement{tk} {

        if (not tk.is_word()) {
            throw compile_error{tk, "expected field name"};
        }

        // e.g. 'tags[4]' and the 'x' of '{x, y}' have the default type
        if (tz.peek_char_after_whitespace() != '[') {
            const token type_tk{tz.next_token()};

            if (type_tk.is_none()) {
                tz.put_back_token(type_tk);
            } else {
                type_tk_ = type_tk;
            }
        }

        assert_no_type_arguments(tz, type_tk_);

        set_type(type_tk_ ? tc.get_type_or_throw(*type_tk_)
                          : tc.builtins().default_type());

        open_bracket_tk_ = tz.is_next_char_token('[');

        if (not open_bracket_tk_) {
            return;
        }

        array_size_ = {tc, tz};

        // a field has no initializer to give an unsized array its length
        if (not array_size_.has_value()) {
            throw compile_error{
                array_size_.tok(),
                "expected a constant array size greater than 0"};
        }

        array_count_ = array_size_.array_size();

        close_bracket_tk_ =
            tz.expect_char_token(']', "expected ']' after array size");
    }

    stmt_def_type_field() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);

        token::source_to(os, type_tk_);

        if (open_bracket_tk_) {
            open_bracket_tk_->source_to(os);
            array_size_.source_to(os);
            close_bracket_tk_.source_to(os);
        }
    }

    //
    // class methods
    //

    [[nodiscard]] auto array_count() const -> std::optional<size_t> {
        return array_count_;
    }

    [[nodiscard]] auto is_array() const -> bool {
        return array_count_.has_value();
    }

    [[nodiscard]] auto name() const -> std::string_view { return tok().text(); }
};
