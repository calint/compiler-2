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
class machine;
class operand;

class unary_ops final {
    token ws_pre_;          // whitespace before the ops
    std::vector<char> ops_; // the unary ops as character

  public:
    explicit unary_ops(tokenizer& tz) : ws_pre_{tz.next_whitespace_token()} {
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
        return ops_.size() == 1 and ops_.back() == '-';
    }

    auto put_back(tokenizer& tz) const -> void {
        // put back in reverse order
        for (const char o : ops_ | std::views::reverse) {
            tz.put_back_char(o);
        }
        tz.put_back_token(ws_pre_);
    }

    auto source_to(std::ostream& os) const -> void {
        ws_pre_.source_to(os);
        source_to_without_whitespace(os);
    }

    auto source_to_without_whitespace(std::ostream& os) const -> void {
        std::print(os, "{}", to_string());
    }

    // implemented in 'decouple_impl.hpp'
    // solves circular reference: unary_ops -> toc -> statement -> unary_ops
    auto compile(toc& tc, const size_t indent, const operand& dst_info) const
        -> void;

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
