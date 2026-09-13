#pragma once
// reviewed: 2025-09-28

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "toc.hpp"

class stmt_def_func_param final : public statement {
    token type_delim_tk_;
    token type_tk_;
    token open_array_tk_;
    token close_array_tk_;
    bool is_array_{};

  public:
    stmt_def_func_param(const toc& tc, tokenizer& tz)
        : statement{tz.next_token()},
          type_delim_tk_(tz.is_next_char_token(':')) {
        assert(not tok().text().empty());

        if (type_delim_tk_.is_empty()) {
            // no type defined, set default
            set_type(tc.get_type_default());
            return;
        }

        // get type
        type_tk_ = tz.next_token();
        if (type_tk_.text().starts_with("reg_")) {
            // register parameter, set default type
            set_type(tc.get_type_default());
            return;
        }

        set_type(tc.get_type_or_throw(type_tk_, type_tk_.text()));

        open_array_tk_ = tz.is_next_char_token('[');
        if (not open_array_tk_.is_empty()) {
            close_array_tk_ = tz.is_next_char_token(']');
            if (close_array_tk_.is_empty()) {
                throw compiler_exception{tz, "expected ']'"};
            }
            is_array_ = true;
        }
    }

    stmt_def_func_param() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        if (type_tk_.text().empty()) {
            return;
        }
        type_delim_tk_.source_to(os);
        type_tk_.source_to(os);
        if (is_array_) {
            open_array_tk_.source_to(os);
            close_array_tk_.source_to(os);
        }
    }

    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] std::ostream& os,
                 [[maybe_unused]] const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {}

    [[nodiscard]] auto name() const -> std::string_view { return tok().text(); }

    [[nodiscard]] auto get_register_name_or_empty() const -> std::string_view {
        const std::string_view type_name{type_tk_.text()};
        if (type_name.starts_with("reg_")) {
            return type_name.substr(4, type_name.size() - 4);
            // note: 4 is the length of "reg_"
        }
        return "";
    }

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }
};
