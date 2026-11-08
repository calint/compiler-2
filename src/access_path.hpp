#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

// aliasing: two accesses that cannot be the same bytes because they name
// different variables or their paths select different fields or different
// constant elements

// one step of the path from a root variable to what is accessed: a field or
// the range of indexes it selects, e.g. 'es[1].items[i]' is the range of the
// one index 1, a field and a run-time index, which may select any element.
// a field is the range of its offset alone, so two steps overlap when their
// ranges do; an index always follows a field or the root variable; a longer
// range is e.g. the elements a literal zeroes or those from the start of a
// bulk operand on
struct path_step {
    bool is_field{};

    // the offset of a field in its parent, which identifies the field, or the
    // first index of a range
    size_t first{};

    // after the offset of a field or the last index of a range
    size_t end{};

    [[nodiscard]] auto is_same(const path_step& other) const -> bool {
        return is_field == other.is_field and first == other.first and
               end == other.end;
    }
};

// where a variable is accessed: the path of fields and indexes from its root
// variable. two accesses cannot be the same bytes when their paths select
// different fields or different constant elements of the same object
struct access_path {
    std::vector<path_step> steps;

    // the end of the range of a run-time index, past every element
    static constexpr size_t run_time_end{std::numeric_limits<size_t>::max()};

    auto add_field(const size_t offset) -> void {
        steps.push_back({
            .is_field{true},
            .first{offset},
            .end{offset + 1},
        });
    }

    // an element of the array at a constant index
    auto add_fixed_index(const size_t index) -> void {
        add_index_range(index, index + 1);
    }

    // the elements from 'first' to before 'end' of the array
    auto add_index_range(const size_t first, const size_t end) -> void {
        assert(first < end);

        steps.push_back({
            .is_field{},
            .first{first},
            .end{end},
        });
    }

    // an element of the array at a run-time index, any of them
    auto add_run_time_index() -> void { add_index_range(0, run_time_end); }

    // the element is not known at compile time, e.g. 'ps[i].x'
    [[nodiscard]] auto has_run_time_index() const -> bool {
        return std::ranges::any_of(steps, [](const path_step& step) -> bool {
            return not step.is_field and step.end == run_time_end;
        });
    }

    [[nodiscard]] auto is_same(const access_path& other) const -> bool {
        return std::ranges::equal(
            steps, other.steps,
            [](const path_step& lhs, const path_step& rhs) -> bool {
                return lhs.is_same(rhs);
            });
    }

    // false only when the bytes cannot be the same: the paths select
    // different fields or different constant elements of the same object,
    // whatever their run-time indexes are, e.g. 'es[i].items' and
    // 'es[1].other'. the steps of both start at the same root variable so they
    // line up until a field or a constant index differs
    [[nodiscard]] auto overlaps(const access_path& other) const -> bool {
        const size_t common_count{std::min(steps.size(), other.steps.size())};

        for (size_t i{}; i < common_count; ++i) {
            const path_step& lhs_step{steps.at(i)};
            const path_step& rhs_step{other.steps.at(i)};

            // note: an index follows a field, so steps at the same position
            //       are both fields or both indexes

            assert(lhs_step.is_field == rhs_step.is_field);

            if (lhs_step.end <= rhs_step.first or
                rhs_step.end <= lhs_step.first) {

                return false;
            }
        }

        return true;
    }

    //
    // statics
    //

    // the empty path
    [[nodiscard]] static auto whole_variable() -> access_path { return {}; }
};

// a variable: its name and the scope it is declared in, a callee's local of
// the name of a variable of the caller is another variable; 'scope_index' is
// the index on the scope stack, so two roots compare only while the scopes up
// to it are unchanged: a root is kept no longer than its scope (a global,
// index 0, always)
struct root_variable {
    std::string name;
    size_t scope_index{};

    [[nodiscard]] auto is_global() const -> bool { return scope_index == 0; }

    [[nodiscard]] auto is_same(const root_variable& other) const -> bool {
        return name == other.name and scope_index == other.scope_index;
    }
};

// storage under the name of a root variable: the root variable and the path
// to what is accessed, empty for the whole variable
struct storage_path {
    root_variable root{};
    access_path path{};

    // 'suffix' inside the element at a run-time index of the array this
    // names, e.g. the element 'e' of a 'foo'
    [[nodiscard]] auto any_element(const access_path& suffix) const
        -> storage_path {

        storage_path element{*this};
        element.path.add_run_time_index();
        return element.extended_by(suffix);
    }

    // the storage of something inside the object this names, 'suffix' is
    // relative to that object
    [[nodiscard]] auto extended_by(const access_path& suffix) const
        -> storage_path {

        storage_path result{*this};

        result.path.steps.append_range(suffix.steps);

        return result;
    }

    // true when both may name the same bytes, a read of other bytes or of
    // another variable is not
    [[nodiscard]] auto may_overlap(const storage_path& other) const -> bool {
        return root.is_same(other.root) and path.overlaps(other.path);
    }
};
