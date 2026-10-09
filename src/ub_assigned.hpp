#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "token.hpp"
#include "type.hpp"

// definite assignment: which bytes of a variable are set before they are read
// and before a function returns

// byte ranges of a variable that are definitely assigned
class assigned_bytes final {
    // sorted by offset, neither overlapping nor adjacent
    std::vector<byte_range> ranges_;
    size_t size_bytes_{};

  public:
    explicit assigned_bytes(const size_t size_bytes)
        : size_bytes_{size_bytes} {}

    assigned_bytes() = default;

    auto add(const byte_range assigned) -> void {
        if (assigned.size_bytes == 0) {
            return;
        }

        ranges_.push_back(assigned);
        normalize();
    }

    [[nodiscard]] auto covers(const byte_range accessed) const -> bool {
        if (accessed.size_bytes == 0) {
            return true;
        }

        const size_t accessed_end{accessed.offset + accessed.size_bytes};

        return std::ranges::any_of(ranges_, [&](const byte_range& r) -> bool {
            return r.offset <= accessed.offset and
                   accessed_end <= r.offset + r.size_bytes;
        });
    }

    // keeps only the bytes assigned on both paths
    auto intersect(const assigned_bytes& other) -> void {
        std::vector<byte_range> common;
        for (const byte_range& a : ranges_) {
            for (const byte_range& b : other.ranges_) {
                const size_t begin{std::max(a.offset, b.offset)};

                const size_t end{
                    std::min(a.offset + a.size_bytes, b.offset + b.size_bytes),
                };

                if (begin < end) {
                    common.push_back({
                        .offset{begin},
                        .size_bytes{end - begin},
                    });
                }
            }
        }

        ranges_ = std::move(common);
        normalize();
    }

    [[nodiscard]] auto is_full() const -> bool {
        return covers({
            .offset{},
            .size_bytes{size_bytes_},
        });
    }

    [[nodiscard]] auto size_bytes() const -> size_t { return size_bytes_; }

    //
    // statics
    //

    [[nodiscard]] static auto full(const size_t size_bytes) -> assigned_bytes {
        assigned_bytes coverage{size_bytes};

        coverage.add({
            .offset{},
            .size_bytes{size_bytes},
        });

        return coverage;
    }

  private:
    auto normalize() -> void {
        std::ranges::sort(ranges_, {}, &byte_range::offset);

        std::vector<byte_range> merged;
        for (const byte_range& r : ranges_) {
            if (merged.empty() or
                merged.back().offset + merged.back().size_bytes < r.offset) {

                merged.push_back(r);
                continue;
            }

            // overlapping or adjacent ranges become one
            byte_range& last{merged.back()};

            last.size_bytes = std::max(last.offset + last.size_bytes,
                                       r.offset + r.size_bytes) -
                              last.offset;
        }

        ranges_ = std::move(merged);
    }
};

// definite-assignment walk of one variable through a function body
struct assignment_flow {
    std::string_view var;

    // the declaration of the result, reported when a 'return' may leave 'var'
    // unset
    token src_loc_tk;

    // bytes assigned on every path reaching the current statement
    assigned_bytes assigned;

    // bytes assigned at every 'break' of the innermost loop
    std::optional<assigned_bytes> at_breaks;

    bool is_reachable{true};

    auto assert_set_at_return() const -> void {
        if (assigned.is_full()) {
            return;
        }

        throw compiler_exception{
            src_loc_tk, "function may return without setting its return value"};
    }

    // a path that never reaches the next statement cannot narrow a merge
    auto end_path() -> void {
        assigned = assigned_bytes::full(assigned.size_bytes());
        is_reachable = false;
    }

    auto record_break() -> void {
        // the first break sets the coverage, later ones narrow it
        if (not at_breaks) {
            at_breaks = assigned;
            end_path();
            return;
        }

        at_breaks->intersect(assigned);
        end_path();
    }
};
