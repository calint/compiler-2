#pragma once
// reviewed: 2025-09-28

#include <cstdint>
#include <optional>
#include <string>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "unary_ops.hpp"

class stmt_const final : public statement {
    unary_ops uops_;
    token literal_tk_;
    int64_t value_{};
    bool has_value_{};

  public:
    stmt_const(const toc& tc, tokenizer& tz, const int64_t default_value)
        : statement{tz.next_whitespace_token()}, uops_{tz},
          literal_tk_{tz.next_token()}, value_(default_value) {

        set_type(tc.get_type_default());

        if (literal_tk_.has_whitespace_before()) {
            throw compiler_exception(
                tok(), "expected a constant value or a constant identifier");
        }

        if (literal_tk_.text().empty()) {
            return;
        }

        has_value_ = true;

        if (tc.has_const(literal_tk_.text())) {
            value_ = uops_.evaluate_constant(tc.get_const(literal_tk_.text()));
            return;
        }

        std::string num_str{uops_.to_string()};
        num_str += literal_tk_.text();

        if (std::optional<int64_t> num{
                toc::parse_constant(literal_tk_, num_str)}) {

            value_ = *num;
        } else {
            throw compiler_exception(
                literal_tk_,
                std::format("cannot parse constant '{}'", num_str));
        }
    }

    stmt_const() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        uops_.source_to(os);
        literal_tk_.source_to(os);
    }

    [[nodiscard]] auto value() const -> int64_t { return value_; }

    [[nodiscard]] auto has_value() const -> bool { return has_value_; }
};
