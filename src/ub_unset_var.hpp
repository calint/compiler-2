#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "token.hpp"

// byte ranges of a variable that are definitely assigned
class field_coverage final {
  public:
    struct range {
        size_t offset{};
        size_t size_bytes{};

        [[nodiscard]] auto overlaps(const range& other) const -> bool {
            return offset < other.offset + other.size_bytes and
                   other.offset < offset + size_bytes;
        }
    };

  private:
    // sorted by offset, neither overlapping nor adjacent
    std::vector<range> ranges_;
    size_t size_bytes_{};

  public:
    explicit field_coverage(const size_t size_bytes)
        : size_bytes_{size_bytes} {}

    field_coverage() = default;

    [[nodiscard]] static auto full(const size_t size_bytes) -> field_coverage {
        field_coverage coverage{size_bytes};
        coverage.add({.offset{}, .size_bytes{size_bytes}});

        return coverage;
    }

    [[nodiscard]] auto size_bytes() const -> size_t { return size_bytes_; }

    auto add(const range assigned) -> void {
        if (assigned.size_bytes == 0) {
            return;
        }

        ranges_.push_back(assigned);
        normalize();
    }

    [[nodiscard]] auto covers(const range accessed) const -> bool {
        if (accessed.size_bytes == 0) {
            return true;
        }

        const size_t accessed_end{accessed.offset + accessed.size_bytes};

        return std::ranges::any_of(ranges_, [&](const range& r) -> bool {
            return r.offset <= accessed.offset and
                   accessed_end <= r.offset + r.size_bytes;
        });
    }

    [[nodiscard]] auto is_full() const -> bool {
        return covers({.offset{}, .size_bytes{size_bytes_}});
    }

    // keeps only the bytes assigned on both paths
    auto intersect(const field_coverage& other) -> void {
        std::vector<range> common;
        for (const range& a : ranges_) {
            for (const range& b : other.ranges_) {
                const size_t begin{std::max(a.offset, b.offset)};
                const size_t end{
                    std::min(a.offset + a.size_bytes, b.offset + b.size_bytes)};
                if (begin < end) {
                    common.push_back(
                        {.offset{begin}, .size_bytes{end - begin}});
                }
            }
        }

        ranges_ = std::move(common);
        normalize();
    }

  private:
    auto normalize() -> void {
        std::ranges::sort(ranges_, {}, &range::offset);

        std::vector<range> merged;
        for (const range& r : ranges_) {
            if (merged.empty() or
                merged.back().offset + merged.back().size_bytes < r.offset) {
                merged.push_back(r);
                continue;
            }

            // overlapping or adjacent ranges become one
            range& last{merged.back()};
            last.size_bytes = std::max(last.offset + last.size_bytes,
                                       r.offset + r.size_bytes) -
                              last.offset;
        }

        ranges_ = std::move(merged);
    }
};

// receives each read of a variable, an empty range reads the whole variable
using read_visitor = std::function_ref<void(
    const token& use_tk, std::string_view read_text,
    const std::optional<field_coverage::range>& accessed)>;

// definite-assignment walk of one variable through a function body
struct assignment_flow {
    std::string_view var;

    // reported when a 'return' may leave 'var' unset
    token func_tk;

    // bytes assigned on every path reaching the current statement
    field_coverage assigned;

    // bytes assigned at every 'break' of the innermost loop
    std::optional<field_coverage> at_breaks;

    bool is_reachable{true};

    auto assert_set_at_return() const -> void {
        if (assigned.is_full()) {
            return;
        }

        throw compiler_exception{
            func_tk, "function may return without setting its return value"};
    }

    // a path that never reaches the next statement cannot narrow a merge
    auto end_path() -> void {
        assigned = field_coverage::full(assigned.size_bytes());
        is_reachable = false;
    }

    auto record_break() -> void {
        if (at_breaks) {
            at_breaks->intersect(assigned);
        } else {
            at_breaks = assigned;
        }

        end_path();
    }
};
