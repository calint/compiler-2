#pragma once
// reviewed: 2025-09-28

#include <string_view>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "stmt_const.hpp"
#include "toc.hpp"

class stmt_def_type_field final : public statement {
    token type_delim_tk_;
    token type_tk_;
    token open_bracket_tk_;
    token close_bracket_tk_;
    stmt_const array_size_const_;
    size_t array_size_{};
    bool is_array_{};

  public:
    stmt_def_type_field(const toc& tc, token tk, tokenizer& tz)
        : statement{tk} {

        set_type(tc.get_type_void());

        if (tk.text().empty()) {
            throw compiler_exception{tk, "expected field name"};
        }

        // is the type specified?
        type_delim_tk_ = tz.is_next_char_token(':');
        if (type_delim_tk_.is_empty()) {
            // it is not
            return;
        }

        // get type name
        type_tk_ = tz.next_token();

        // array?
        open_bracket_tk_ = tz.is_next_char_token('[');
        if (not open_bracket_tk_.is_empty()) {
            is_array_ = true;

            array_size_const_ = stmt_const{tc, tz, 0};

            if (not array_size_const_.has_value() or
                array_size_const_.value() < 1) {
                throw compiler_exception{
                    array_size_const_.tok(),
                    "expected a constant array size greater than 0"};
            }

            array_size_ = static_cast<size_t>(array_size_const_.value());

            close_bracket_tk_ = tz.is_next_char_token(']');
            if (close_bracket_tk_.is_empty()) {
                throw compiler_exception{array_size_const_.tok(),
                                         "expected ']' after array size"};
            }
        }
    }

    stmt_def_type_field() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        if (type_tk_.is_empty()) {
            return;
        }
        type_delim_tk_.source_to(os);
        type_tk_.source_to(os);
        if (is_array_) {
            open_bracket_tk_.source_to(os);
            array_size_const_.source_to(os);
            close_bracket_tk_.source_to(os);
        }
    }

    [[nodiscard]] auto name() const -> std::string_view { return tok().text(); }

    [[nodiscard]] auto type_str() const -> std::string_view {
        return type_tk_.text();
    }

    [[nodiscard]] auto type_token() const -> const token& { return type_tk_; }

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }

    [[nodiscard]] auto array_size() const -> size_t { return array_size_; }
};
