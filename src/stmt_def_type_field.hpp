#pragma once
// reviewed: 2025-09-28

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
    stmt_def_type_field(toc& tc, token tk, tokenizer& tz)
        : statement{std::move(tk)} {

        set_type(tc.get_type_void());

        // is type specified?
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
                    toc::parse_to_constant(array_size_tk_.name())};
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

    stmt_def_type_field() = default;
    stmt_def_type_field(const stmt_def_type_field&) = default;
    stmt_def_type_field(stmt_def_type_field&&) = default;
    auto operator=(const stmt_def_type_field&)
        -> stmt_def_type_field& = default;
    auto operator=(stmt_def_type_field&&) -> stmt_def_type_field& = default;

    ~stmt_def_type_field() override = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        if (type_tk_.is_empty()) {
            return;
        }
        os << ':';
        type_tk_.source_to(os);
        if (is_array_) {
            os << '[';
            array_size_tk_.source_to(os);
            os << ']';
            ws1_.source_to(os);
        }
    }

    [[nodiscard]] auto name() const -> const std::string& {
        return tok().name();
    }

    [[nodiscard]] auto type_str() const -> const std::string& {
        return type_tk_.name();
    }

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }

    [[nodiscard]] auto array_size() const -> size_t { return array_size_; }
};
