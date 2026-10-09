#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
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

// one step of the path from a root variable to what is accessed: a field, a
// constant index or a run-time index, e.g. 'es[1].items[i]' is a constant
// index, a field and a run-time index. an index always follows a field or the
// root variable
struct path_step {
    enum class kind_type : uint8_t { field, fixed_index, run_time_index };

    kind_type kind{};

    // the offset of a field in its parent, which identifies the field, or the
    // index of an element
    size_t number{};
};

// where a variable is accessed: the path of fields and indexes from its root
// variable. two accesses cannot be the same bytes when their paths select
// different fields or different constant elements of the same object
struct access_span {
    std::vector<path_step> path;

    auto add_field(const size_t offset) -> void {
        path.push_back({
            .kind{path_step::kind_type::field},
            .number{offset},
        });
    }

    // an element of the array at a constant index
    auto add_fixed_index(const size_t index) -> void {
        path.push_back({
            .kind{path_step::kind_type::fixed_index},
            .number{index},
        });
    }

    // an element of the array at a run-time index
    auto add_run_time_index() -> void {
        path.push_back({
            .kind{path_step::kind_type::run_time_index},
        });
    }

    // the span of something inside the object this span names, 'inner' is
    // relative to the start of the object
    [[nodiscard]] auto narrow_to(const access_span& inner) const
        -> access_span {

        access_span result{*this};

        result.path.append_range(inner.path);

        return result;
    }

    // false only when the bytes cannot be the same
    [[nodiscard]] auto overlaps(const access_span& other) const -> bool {
        return not paths_diverge(path, other.path);
    }

    //
    // statics
    //

    // the paths select different fields or different constant elements of the
    // same object, whatever their run-time indexes are, e.g. 'es[i].items' and
    // 'es[1].other'. the steps of both start at the same root variable so they
    // line up until a field or a constant index differs
    [[nodiscard]] static auto
    paths_diverge(const std::span<const path_step> lhs,
                  const std::span<const path_step> rhs) -> bool {

        const size_t common_count{std::min(lhs.size(), rhs.size())};

        for (size_t i{}; i < common_count; ++i) {
            const path_step& lhs_step{lhs.at(i)};
            const path_step& rhs_step{rhs.at(i)};

            // note: an index follows a field, so steps at the same position
            //       are both fields or both indexes
            assert((lhs_step.kind == path_step::kind_type::field) ==
                   (rhs_step.kind == path_step::kind_type::field));

            if (lhs_step.kind == path_step::kind_type::run_time_index or
                rhs_step.kind == path_step::kind_type::run_time_index) {

                // note: a run-time index may select any element
                continue;
            }

            // note: fields never overlap, so a different offset is a
            //       different field, constant indexes work the same
            if (lhs_step.number != rhs_step.number) {
                return true;
            }
        }

        return false;
    }
};

// the variable whose reads 'visit_reads' reports, none for every variable
using read_filter = std::optional<std::string_view>;

// receives each read of a variable: the bytes of the variable that it reaches,
// the whole array after a run-time index, and its path
using read_visitor =
    std::function_ref<void(const token& src_loc_tk, std::string_view read_text,
                           const field_coverage::range& accessed_range,
                           const access_span& accessed_span)>;

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
