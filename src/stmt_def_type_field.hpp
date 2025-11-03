#pragma once
// reviewed: 2025-09-28

#include <string_view>

#include "compiler_exception.hpp"
#include "statement.hpp"
#include "toc.hpp"

class stmt_def_type_field final : public statement {
    token type_tk_;
    token array_size_tk_;
    token ws1_;
    size_t array_size_{};
    bool is_array_{};

  public:
    stmt_def_type_field(const toc& tc, token tk, tokenizer& tz)
        : statement{tk} {

        set_type(tc.get_type_void());

        if (identifier().empty()) {
            throw compiler_exception{tk, "expected field name"};
        }
        // is the type specified?
        if (not tz.is_next_char(':')) {
            // it is not
            return;
        }
        // get type name
        type_tk_ = tz.next_token();
        if (tz.is_next_char('[')) {
            is_array_ = true;
            array_size_tk_ = tz.next_token();

            if (const std::optional<int64_t> value{
                    toc::parse_to_constant(array_size_tk_.text())};
                value) {
                array_size_ = static_cast<size_t>(*value);
            } else {
                throw compiler_exception{array_size_tk_,
                                         "expected array size as constant"};
            }

            if (not tz.is_next_char(']')) {
                throw compiler_exception{array_size_tk_,
                                         "expected ']' after array size"};
            }
            ws1_ = tz.next_whitespace_token();
        }
    }

    ~stmt_def_type_field() override = default;

    stmt_def_type_field() = default;
    stmt_def_type_field(const stmt_def_type_field&) = default;
    stmt_def_type_field(stmt_def_type_field&&) = default;
    auto operator=(const stmt_def_type_field&)
        -> stmt_def_type_field& = default;
    auto operator=(stmt_def_type_field&&) -> stmt_def_type_field& = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        if (type_tk_.is_empty()) {
            return;
        }
        std::print(os, ":");
        type_tk_.source_to(os);
        if (is_array_) {
            std::print(os, "[");
            array_size_tk_.source_to(os);
            std::print(os, "]");
            ws1_.source_to(os);
        }
    }

    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] std::ostream& os,
                 [[maybe_unused]] const size_t indent,
                 [[maybe_unused]] const std::string_view dst) const
        -> void override {}

    [[nodiscard]] auto name() const -> std::string_view { return tok().text(); }

    [[nodiscard]] auto type_str() const -> std::string_view {
        return type_tk_.text();
    }

    [[nodiscard]] auto type_token() const -> const token& { return type_tk_; }

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }

    [[nodiscard]] auto array_size() const -> size_t { return array_size_; }
};
