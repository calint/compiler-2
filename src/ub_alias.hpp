#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <vector>

// aliasing: two accesses that cannot be the same bytes because they name
// different variables or their paths select different fields or different
// constant elements

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

    [[nodiscard]] auto is_same(const path_step& other) const -> bool {
        return kind == other.kind and number == other.number;
    }
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

    // the path of something inside the object this path names, 'suffix' is
    // relative to that object
    [[nodiscard]] auto extended_by(const access_path& suffix) const
        -> access_path {

        access_path result{*this};

        result.steps.append_range(suffix.steps);

        return result;
    }

    // the element is not known at compile time, e.g. 'ps[i].x'
    [[nodiscard]] auto has_run_time_index() const -> bool {
        return std::ranges::any_of(steps, [](const path_step& step) -> bool {
            return step.kind == path_step::kind_type::run_time_index;
        });
    }

    [[nodiscard]] auto is_same(const access_path& other) const -> bool {
        return std::ranges::equal(
            steps, other.steps,
            [](const path_step& lhs, const path_step& rhs) -> bool {
                return lhs.is_same(rhs);
            });
    }

    // false only when the bytes cannot be the same
    [[nodiscard]] auto overlaps(const access_path& other) const -> bool {
        return not are_disjoint(steps, other.steps);
    }

    // e.g. '[1].8[?]' is element 1, its field at offset 8 and an element of
    // that at a run-time index
    [[nodiscard]] auto text() const -> std::string {
        std::string result;

        for (const path_step& step : steps) {
            if (step.kind == path_step::kind_type::field) {
                result += std::format(".{}", step.number);
            } else if (step.kind == path_step::kind_type::fixed_index) {
                result += std::format("[{}]", step.number);
            } else {
                result += "[?]";
            }
        }

        return result;
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

    // the empty path
    [[nodiscard]] static auto whole_variable() -> access_path { return {}; }
};

// a variable: its name and the frame it is declared in, a callee's local of
// the name of a variable of the caller is another variable
struct root_variable {
    std::string name;
    size_t frame_index{};

    [[nodiscard]] auto is_global() const -> bool { return frame_index == 0; }

    [[nodiscard]] auto is_same(const root_variable& other) const -> bool {
        return name == other.name and frame_index == other.frame_index;
    }
};

// storage under the name of a root variable: the root variable and the path
// to what is accessed, empty for the whole variable
struct storage_path {
    root_variable root;
    access_path path;

    // 'suffix' inside the element at a run-time index of the array this
    // names, e.g. the element 'e' of a 'foo'
    [[nodiscard]] auto any_element(const access_path& suffix) const
        -> storage_path {

        access_path element_path{path};
        element_path.add_run_time_index();

        return {
            .root{root},
            .path{element_path.extended_by(suffix)},
        };
    }

    // true when both may name the same bytes, a read of other bytes or of
    // another variable is not
    [[nodiscard]] auto may_overlap(const storage_path& other) const -> bool {
        return root.is_same(other.root) and path.overlaps(other.path);
    }
};
