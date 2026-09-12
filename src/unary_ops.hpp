#pragma once
// reviewed: 2025-09-28
//           2026-09-09

#include <cstdint>
#include <ostream>
#include <print>
#include <ranges>
#include <string_view>
#include <utility>
#include <vector>

#include "tokenizer.hpp"

class toc;

class unary_ops final {
    token ws1_;             // whitespace before the ops
    std::vector<char> ops_; // the unary ops as character

  public:
    explicit unary_ops(tokenizer& tz) : ws1_{tz.next_whitespace_token()} {
        while (true) {
            if (tz.is_next_char('~')) {
                ops_.emplace_back('~');
            } else if (tz.is_next_char('-')) {
                ops_.emplace_back('-');
            } else {
                break;
            }
        }
    }

    unary_ops() = default;

    [[nodiscard]] auto is_only_negated() const -> bool {
        return ops_.size() == 1 && ops_.back() == '-';
    }

    auto put_back(tokenizer& tz) const -> void {
        // put back in reverse order
        for (const char op : ops_ | std::views::reverse) {
            tz.put_back_char(op);
        }
        tz.put_back_token(ws1_);
    }

    auto source_to(std::ostream& os) const -> void {
        ws1_.source_to(os);
        source_to_without_whitespace(os);
    }

    auto source_to_without_whitespace(std::ostream& os) const -> void {
        for (const char op : ops_) {
            std::print(os, "{}", op);
        }
    }

    // implemented in 'main.cpp'
    // solves circular reference: unary_ops -> toc -> statement -> unary_ops
    inline auto compile(toc& tc, std::ostream& os, const size_t indnt,
                        const std::string_view dst_info) const -> void;

    [[nodiscard]] auto is_empty() const -> bool { return ops_.empty(); }

    [[nodiscard]] auto to_string() const -> std::string_view {
        return {ops_.begin(), ops_.end()};
    }

    [[nodiscard]] auto evaluate_constant(int64_t v) const -> int64_t {
        for (const char op : ops_ | std::views::reverse) {
            switch (op) {
            case '-':
                v = -v;
                break;
            case '~':
                v = ~v; // NOLINT(hicpp-signed-bitwise)
                break;
            default:
                std::unreachable();
            }
        }
        return v;
    }
};
