#pragma once
// reviewed: 2025-09-28

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "toc.hpp"

class stmt_def_func_param final : public statement {
    token type_tk_;
    token open_bracket_tk_;
    token close_bracket_tk_;
    bool is_array_{};

    // the 'self' of a method is not written in the source
    bool is_implicit_{};

  public:
    stmt_def_func_param(const toc& tc, tokenizer& tz)
        : statement{tz.next_token()} {

        assert(not tok().text().empty());

        open_bracket_tk_ = tz.is_next_char_token('[');
        if (not open_bracket_tk_.is_empty()) {
            close_bracket_tk_ = tz.is_next_char_token(']');
            if (close_bracket_tk_.is_empty()) {
                throw compiler_exception{tz, "expected ']'"};
            }
            is_array_ = true;
        }

        token delimiter_tk{tz.is_next_char_token(',')};
        if (delimiter_tk.is_empty()) {
            delimiter_tk = tz.is_next_char_token(')');
        }
        if (not delimiter_tk.is_empty()) {
            tz.put_back_token(delimiter_tk);

            // no type defined, set default
            set_type(tc.get_type_default());

            return;
        }

        type_tk_ = tz.next_token();

        set_type(type_tk_.is_empty()
                     ? tc.get_type_default()
                     : tc.get_type_or_throw(type_tk_, type_tk_.text()));
    }

    stmt_def_func_param(const token tk, const type& tp)
        : statement{tk}, is_implicit_{true} {

        set_type(tp);
    }

    stmt_def_func_param() = default;

    auto source_to(std::ostream& os) const -> void override {
        if (is_implicit_) {
            return;
        }

        statement::source_to(os);
        if (is_array_) {
            open_bracket_tk_.source_to(os);
            close_bracket_tk_.source_to(os);
        }
        if (not type_tk_.is_empty()) {
            type_tk_.source_to(os);
        }
    }

    [[nodiscard]] auto name() const -> std::string_view { return tok().text(); }

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }
};
