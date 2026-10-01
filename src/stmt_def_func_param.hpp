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

    // the 'self' of a method is not written in the source
    bool is_implicit_{};

  public:
    stmt_def_func_param(const toc& tc, tokenizer& tz)
        : stmt_def_func_param{tc, tz, tz.next_token()} {}

    stmt_def_func_param(const token tk, const type& tp, const bool is_read_only)
        : statement{tk}, is_read_only_{is_read_only}, is_implicit_{true} {

        set_type(tp);
    }

    stmt_def_func_param() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        if (is_implicit_) {
            return;
        }

        mut_tk_.source_to(os);
        statement::source_to(os);
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
    // 'first_tk' is the name or the 'mut' before it
    stmt_def_func_param(const toc& tc, tokenizer& tz, const token first_tk)
        : statement{first_tk.is_text("mut") ? read_name_after_mut(tz)
                                            : first_tk},
          mut_tk_{first_tk.is_text("mut") ? first_tk : token{}},
          is_read_only_{not first_tk.is_text("mut")} {

        assert(not tok().text().empty());

        toc::assert_name_not_reserved(tok());

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

        // e.g. 'arr[]' has the default type
        if (tz.peek_char_after_whitespace() != '[') {
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

    //
    // statics
    //

    [[nodiscard]] static auto read_name_after_mut(tokenizer& tz) -> token {
        const char next{tz.peek_char_after_whitespace()};

        if (next == ',' or next == ')') {
            throw compiler_exception{tz, "expected parameter name after 'mut'"};
        }

        return tz.next_token();
    }
};
