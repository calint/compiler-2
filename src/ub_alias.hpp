#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

// aliasing: two accesses to a variable that cannot be the same bytes because
// their paths select different fields or different constant elements

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
struct access_path {
    std::vector<path_step> steps;

    auto add_field(const size_t offset) -> void {
        steps.push_back({
            .kind{path_step::kind_type::field},
            .number{offset},
        });
    }

    // an element of the array at a constant index
    auto add_fixed_index(const size_t index) -> void {
        steps.push_back({
            .kind{path_step::kind_type::fixed_index},
            .number{index},
        });
    }

    // an element of the array at a run-time index
    auto add_run_time_index() -> void {
        steps.push_back({
            .kind{path_step::kind_type::run_time_index},
        });
    }

    // the span of something inside the object this span names, 'inner' is
    // relative to the start of the object
    [[nodiscard]] auto narrow_to(const access_path& inner) const
        -> access_path {

        access_path result{*this};

        result.steps.append_range(inner.steps);

        return result;
    }

    // false only when the bytes cannot be the same
    [[nodiscard]] auto overlaps(const access_path& other) const -> bool {
        return not are_disjoint(steps, other.steps);
    }

    //
    // statics
    //

    // the paths select different fields or different constant elements of the
    // same object, whatever their run-time indexes are, e.g. 'es[i].items' and
    // 'es[1].other'. the steps of both start at the same root variable so they
    // line up until a field or a constant index differs
    [[nodiscard]] static auto are_disjoint(const std::span<const path_step> lhs,
                                           const std::span<const path_step> rhs)
        -> bool {

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
