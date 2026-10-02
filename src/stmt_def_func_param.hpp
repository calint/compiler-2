#pragma once
// reviewed: 2025-09-28

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "toc.hpp"

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
    stmt_def_func_param(const toc& tc, tokenizer& tz)
        : statement{tz.next_token()} {

        assert(not tok().text().empty());

        toc::assert_name_not_reserved(tok());

        read_mut(tz);
        is_read_only_ = mut_tk_.is_empty();

        parse_type(tc, tz);
    }

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
    // e.g. 'v', 'v i32', 'arr[]' and 'arr i32[]' have the default type or an
    // explicit one, brackets follow the type
    auto parse_type(const toc& tc, tokenizer& tz) -> void {
        const char next{tz.peek_char_after_whitespace()};
        if (next != ',' and next != ')' and next != '[') {
            type_tk_ = tz.next_token();
        }

        set_type(type_tk_.is_empty()
                     ? tc.get_type_default()
                     : tc.get_type_or_throw(type_tk_, type_tk_.text()));

        open_bracket_tk_ = tz.is_next_char_token('[');
        if (open_bracket_tk_.is_empty()) {
            return;
        }

        close_bracket_tk_ = tz.is_next_char_token(']');
        if (close_bracket_tk_.is_empty()) {
            throw compiler_exception{tz, "expected ']'"};
        }

        is_array_ = true;
    }

    // 'mut' after the name allows the body to write the parameter
    auto read_mut(tokenizer& tz) -> void {
        const char next{tz.peek_char_after_whitespace()};
        if (next == ',' or next == ')' or next == '[') {
            return;
        }

        const token tk{tz.next_token()};
        if (not tk.is_text("mut")) {
            tz.put_back_token(tk);
            return;
        }

        mut_tk_ = tk;
    }
};
