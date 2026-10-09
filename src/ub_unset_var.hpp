#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <optional>
#include <span>
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

    // keeps only the bytes assigned on both paths
    auto intersect(const field_coverage& other) -> void {
        std::vector<range> common;
        for (const range& a : ranges_) {
            for (const range& b : other.ranges_) {
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

    [[nodiscard]] static auto full(const size_t size_bytes) -> field_coverage {
        field_coverage coverage{size_bytes};

        coverage.add({
            .offset{},
            .size_bytes{size_bytes},
        });

        return coverage;
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

// one run-time index of a path: the bytes reached are a run of slots of
// 'period_bytes', e.g. the elements of an array, and within each slot the bytes
// from 'offset' for 'size_bytes'. the slots start where the bytes of the level
// before it start, for the first level where 'access_span::range' starts
struct index_level {
    size_t period_bytes{};
    size_t offset{};
    size_t size_bytes{};
};

// where a variable is accessed: bytes of its root variable. a path without a
// run-time index reaches exactly 'range'. a run-time index makes 'range' the
// whole array and adds a level, an index inside the bytes of that level adds
// another, e.g. 'rooms[i].items[j].name' has two
struct access_span {
    field_coverage::range range;
    std::vector<index_level> levels;

    // an element of the array this span names at a run-time index
    auto add_level(const size_t element_size_bytes) -> void {
        levels.push_back({
            .period_bytes{element_size_bytes},
            .offset{},
            .size_bytes{element_size_bytes},
        });
    }

    // the bytes of a field or an element of the object this span names,
    // 'offset' is relative to the start of the object
    auto narrow(const size_t offset, const size_t size_bytes_in) -> void {
        if (levels.empty()) {
            range = {
                .offset{range.offset + offset},
                .size_bytes{size_bytes_in},
            };

            return;
        }

        levels.back().offset += offset;
        levels.back().size_bytes = size_bytes_in;
    }

    // false only when the bytes cannot be the same
    [[nodiscard]] auto overlaps(const access_span& other) const -> bool {
        if (not range.overlaps(other.range)) {
            return false;
        }

        if (levels.empty()) {
            return place_overlaps(range, other.range, other.levels);
        }

        if (other.levels.empty()) {
            return place_overlaps(other.range, range, levels);
        }

        return levels_overlap(*this, other);
    }

    // the span of something inside the object this span names, 'inner' is
    // relative to the start of the object
    [[nodiscard]] auto shifted(const access_span& inner) const -> access_span {
        access_span result{*this};

        result.narrow(inner.range.offset, inner.range.size_bytes);
        result.levels.append_range(inner.levels);

        return result;
    }

    // the size of the object this span names
    [[nodiscard]] auto size_bytes() const -> size_t {
        return levels.empty() ? range.size_bytes : levels.back().size_bytes;
    }

    //
    // statics
    //

    // both reach bytes by levels: the levels are followed while their slots
    // line up and their bytes in a slot overlap
    [[nodiscard]] static auto levels_overlap(const access_span& lhs,
                                             const access_span& rhs) -> bool {

        const size_t common_count{
            std::min(lhs.levels.size(), rhs.levels.size()),
        };

        // the start of the slots of a level, in a frame both share
        size_t lhs_origin{lhs.range.offset};
        size_t rhs_origin{rhs.range.offset};

        field_coverage::range lhs_part;
        field_coverage::range rhs_part;

        for (size_t i{}; i < common_count; ++i) {
            const index_level& lhs_level{lhs.levels.at(i)};
            const index_level& rhs_level{rhs.levels.at(i)};

            const bool is_aligned{
                lhs_level.period_bytes == rhs_level.period_bytes and
                    lhs_origin % lhs_level.period_bytes ==
                        rhs_origin % rhs_level.period_bytes,
            };

            // note: slots that do not line up could overlap anywhere
            if (not is_aligned) {
                return true;
            }

            lhs_part = {
                .offset{lhs_level.offset},
                .size_bytes{lhs_level.size_bytes},
            };

            rhs_part = {
                .offset{rhs_level.offset},
                .size_bytes{rhs_level.size_bytes},
            };

            if (not lhs_part.overlaps(rhs_part)) {
                return false;
            }

            lhs_origin = lhs_level.offset;
            rhs_origin = rhs_level.offset;
        }

        // note: the side without a level left reaches all of its part
        if (lhs.levels.size() > common_count) {
            return place_overlaps(rhs_part, lhs_part,
                                  std::span{lhs.levels}.subspan(common_count));
        }

        if (rhs.levels.size() > common_count) {
            return place_overlaps(lhs_part, rhs_part,
                                  std::span{rhs.levels}.subspan(common_count));
        }

        return true;
    }

    // a place of bytes against the bytes that 'levels' reach in 'window'
    [[nodiscard]] static auto
    place_overlaps(const field_coverage::range& place,
                   const field_coverage::range& window,
                   const std::span<const index_level> levels) -> bool {

        const size_t begin{std::max(place.offset, window.offset)};

        const size_t end{
            std::min(place.offset + place.size_bytes,
                     window.offset + window.size_bytes),
        };

        return begin < end and reaches(begin, end, window.offset, levels);
    }

    // the bytes from 'begin' to 'end', in the window that starts at
    // 'window_begin', against the bytes that 'levels' reach
    [[nodiscard]] static auto reaches(const size_t begin, const size_t end,
                                      const size_t window_begin,
                                      const std::span<const index_level> levels)
        -> bool {

        if (levels.empty()) {
            return true;
        }

        const index_level& level{levels.front()};

        // note: a place of a whole slot or more reaches every part
        if (end - begin >= level.period_bytes) {
            return true;
        }

        const size_t first_slot{(begin - window_begin) / level.period_bytes};
        const size_t last_slot{(end - 1 - window_begin) / level.period_bytes};

        // note: a place reaches the slots it is in, one or two
        for (size_t slot{first_slot}; slot <= last_slot; ++slot) {
            const size_t part_begin{
                window_begin + (slot * level.period_bytes) + level.offset,
            };

            const size_t part_end{part_begin + level.size_bytes};

            const size_t reached_begin{std::max(begin, part_begin)};
            const size_t reached_end{std::min(end, part_end)};

            if (reached_begin < reached_end and
                reaches(reached_begin, reached_end, part_begin,
                        levels.subspan(1))) {

                return true;
            }
        }

        return false;
    }
};

// the variable whose reads 'visit_reads' reports, none for every variable
using read_filter = std::optional<std::string_view>;

// receives each read of a variable, an empty span reads the whole variable
using read_visitor =
    std::function_ref<void(const token& src_loc_tk, std::string_view read_text,
                           const std::optional<access_span>& accessed)>;

// definite-assignment walk of one variable through a function body
struct assignment_flow {
    std::string_view var;

    // the declaration of the result, reported when a 'return' may leave 'var'
    // unset
    token src_loc_tk;

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
            src_loc_tk, "function may return without setting its return value"};
    }

    // a path that never reaches the next statement cannot narrow a merge
    auto end_path() -> void {
        assigned = field_coverage::full(assigned.size_bytes());
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
