#pragma once
// reviewed: 2025-09-28

#include <cstdint>
#include <vector>

#include "panic_exception.hpp"
#include "tokenizer.hpp"

class toc;

class unary_ops final {
    token ws_before_; // whitespace before the ops
    std::vector<char> ops_;

  public:
    explicit unary_ops(tokenizer& tz) : ws_before_{tz.next_whitespace_token()} {

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
    unary_ops(const unary_ops&) = default;
    unary_ops(unary_ops&&) = default;
    auto operator=(const unary_ops&) -> unary_ops& = default;
    auto operator=(unary_ops&&) -> unary_ops& = default;

    ~unary_ops() = default;

    [[nodiscard]] auto is_only_negated() const -> bool {
        return ops_.size() == 1 && ops_.back() == '-';
    }

    auto put_back(tokenizer& tz) const -> void {
        // put back in reverse order
        size_t i{ops_.size()};
        while (i--) {
            tz.put_back_char(ops_.at(i));
        }
        tz.put_back_token(ws_before_);
    }

    auto source_to(std::ostream& os) const -> void {
        ws_before_.source_to(os);
        for (const char op : ops_) {
            os << op;
        }
    }

    // implemented in 'main.cpp'
    // solves circular reference: unary_ops -> toc -> statement -> unary_ops
    inline auto compile(toc& tc, std::ostream& os, size_t indnt,
                        const std::string& dst_resolved) const -> void;

    [[nodiscard]] auto is_empty() const -> bool { return ops_.empty(); }

    [[nodiscard]] auto to_string() const -> std::string {
        return {ops_.begin(), ops_.end()};
    }

    [[nodiscard]] auto evaluate_constant(int64_t v) const -> int64_t {
        size_t i{ops_.size()};
        while (i--) {
            switch (ops_.at(i)) {
            case '-':
                v = -v;
                break;
            case '~':
                v = ~v; // NOLINT(hicpp-signed-bitwise)
                break;
            default:
                throw panic_exception("unexpected code path");
            }
        }
        return v;
    }
};
