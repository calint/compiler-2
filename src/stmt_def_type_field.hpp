#pragma once

#include <cstddef>
#include <ostream>
#include <string_view>

#include "compile_error.hpp"
#include "statement.hpp"
#include "stmt_const.hpp"
#include "toc.hpp"
#include "token.hpp"

class stmt_def_type_field final : public statement {
    token type_tk_;
    token open_bracket_tk_;
    token close_bracket_tk_;
    stmt_const array_count_const_;
    size_t array_count_{};

  public:
    stmt_def_type_field(const toc& tc, const token tk, tokenizer& tz)
        : statement{tk} {

        set_type(tc.builtins().void_type());

        if (not tk.is_word()) {
            throw compile_error{tk, "expected field name"};
        }

        // e.g. 'tags[4]' has the default type
        if (tz.peek_char_after_whitespace() != '[') {
            type_tk_ = tz.next_token();
        }

        open_bracket_tk_ = tz.is_next_char_token('[');

        if (open_bracket_tk_.is_empty()) {
            return;
        }

        array_count_const_ = {tc, tz};
        array_count_ = array_count_const_.array_size();

        close_bracket_tk_ =
            tz.expect_char_token(']', "expected ']' after array size");
    }

    stmt_def_type_field() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);

        if (not type_tk_.is_empty()) {
            type_tk_.source_to(os);
        }

        if (is_array()) {
            open_bracket_tk_.source_to(os);
            array_count_const_.source_to(os);
            close_bracket_tk_.source_to(os);
        }
    }

    //
    // class methods
    //

    [[nodiscard]] auto array_count() const -> size_t { return array_count_; }

    [[nodiscard]] auto is_array() const -> bool {
        return not open_bracket_tk_.is_empty();
    }

    [[nodiscard]] auto name() const -> std::string_view { return tok().text(); }

    [[nodiscard]] auto type_str() const -> std::string_view {
        return type_tk_.text();
    }

    [[nodiscard]] auto type_token() const -> const token& { return type_tk_; }
};
