#pragma once

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

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
