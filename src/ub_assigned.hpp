#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "compile_error.hpp"
#include "token.hpp"
#include "type.hpp"

// definite assignment: which bytes of a variable are set before they are read
// and before a function returns

// how the end of a loop is reached: only at a 'break', e.g. 'loop', also
// when it runs out, e.g. 'foo', or, known only to the compile, at a 'break'
// or where its counter step is reached, e.g. a 'foo' whose count is a
// constant above 0, so it runs its body
enum class loop_end : uint8_t { at_break, also_when_done, when_stepped };

// the bytes of a variable that are definitely assigned, one flag per byte
class assigned_bytes final {
    std::vector<bool> is_assigned_;

  public:
    // note: parentheses, braces would make a list of one flag
    explicit assigned_bytes(const size_t size_bytes)
        : is_assigned_(size_bytes, false) {}

    auto add(const byte_range assigned) -> void {
        for (size_t i{}; i < assigned.size_bytes; ++i) {
            is_assigned_.at(assigned.offset + i) = true;
        }
    }

    [[nodiscard]] auto covers(const byte_range accessed) const -> bool {
        for (size_t i{}; i < accessed.size_bytes; ++i) {
            if (not is_assigned_.at(accessed.offset + i)) {
                return false;
            }
        }

        return true;
    }

    // keeps only the bytes assigned on both paths
    auto intersect(const assigned_bytes& other) -> void {
        assert(is_assigned_.size() == other.is_assigned_.size());

        for (size_t i{}; i < is_assigned_.size(); ++i) {
            is_assigned_.at(i) =
                is_assigned_.at(i) and other.is_assigned_.at(i);
        }
    }

    [[nodiscard]] auto is_full() const -> bool {
        return std::ranges::all_of(
            is_assigned_, [](const bool is_set) -> bool { return is_set; });
    }

    [[nodiscard]] auto size_bytes() const -> size_t {
        return is_assigned_.size();
    }

    //
    // statics
    //

    [[nodiscard]] static auto full(const size_t size_bytes) -> assigned_bytes {
        assigned_bytes coverage{size_bytes};
        coverage.is_assigned_.flip();
        return coverage;
    }
};

// definite-assignment walk of the result through a function body, which also
// finds the statements that are not reached; a function without a result has
// no bytes to assign. the statements report what they do ('assign',
// 'end_path', 'record_break'), the parse tells which reads name the result
// ('stmt_identifier::names_result'), and the rules that join paths are here:
// 'trace_loop' traces a body by its 'loop_end', 'path_merge' joins the paths
// of an 'if'
class assignment_flow final {
    // the declaration of the result, reported when a 'return' may leave it
    // unset
    token src_loc_tk_;

    // bytes assigned on every path reaching the current statement
    assigned_bytes assigned_;

    // bytes assigned at every 'break' of the innermost loop
    std::optional<assigned_bytes> at_breaks_;

    bool is_reachable_{true};

  public:
    // every path of an 'if' starts at the 'if', after it only what every path
    // that reaches its end assigned remains, and its end is reached when one
    // path reaches it; the flow continues from the merge when it ends
    class path_merge final {
        assignment_flow& flow_;
        assigned_bytes entry_;
        assigned_bytes merged_;
        bool is_reachable_{};

      public:
        explicit path_merge(assignment_flow& flow)
            : flow_{flow}, entry_{flow.assigned_},
              merged_{assigned_bytes::full(flow.assigned_.size_bytes())} {}

        path_merge(const path_merge&) = delete;
        path_merge(path_merge&&) = delete;
        auto operator=(const path_merge&) -> path_merge& = delete;
        auto operator=(path_merge&&) -> path_merge& = delete;

        ~path_merge() {
            flow_.assigned_ = std::move(merged_);
            flow_.is_reachable_ = is_reachable_;
        }

        // 'path' has 'trace_assignment(assignment_flow&)'
        auto trace(const auto& path) -> void {
            flow_.assigned_ = entry_;
            flow_.is_reachable_ = true;
            path.trace_assignment(flow_);
            merged_.intersect(flow_.assigned_);
            is_reachable_ = is_reachable_ or flow_.is_reachable_;
        }

        // a path without statements, e.g. that of an 'if' without 'else'
        auto trace_empty() -> void {
            merged_.intersect(entry_);
            is_reachable_ = true;
        }
    };

    assignment_flow(const token& src_loc_tk, const size_t size_bytes)
        : src_loc_tk_{src_loc_tk}, assigned_{size_bytes} {}

    auto assert_set_at_return() const -> void {
        if (assigned_.is_full()) {
            return;
        }

        throw compile_error{
            src_loc_tk_,
            "function may return without setting its return value"};
    }

    // the result is assigned these bytes
    auto assign(const byte_range bytes) -> void { assigned_.add(bytes); }

    [[nodiscard]] auto assigned() const -> const assigned_bytes& {
        return assigned_;
    }

    // a path that never reaches the next statement cannot narrow a merge
    auto end_path() -> void {
        assigned_ = assigned_bytes::full(assigned_.size_bytes());
        is_reachable_ = false;
    }

    [[nodiscard]] auto is_reachable() const -> bool { return is_reachable_; }

    auto record_break() -> void {
        // the first break sets the coverage, later ones narrow it
        if (not at_breaks_) {
            at_breaks_ = assigned_;
        } else {
            at_breaks_->intersect(assigned_);
        }

        end_path();
    }

    // 'body' has 'trace_assignment(assignment_flow&)'; a loop that also ends
    // when it runs out may run zero times, so after it the coverage is the
    // one at the loop; a loop left only at a 'break' has the coverage common
    // to its breaks after it, without a 'break' only 'return' or 'exit'
    // leave it
    auto trace_loop(const auto& body, const loop_end end) -> void {
        assert(end != loop_end::when_stepped);

        const assigned_bytes entry{assigned_};

        // every iteration starts with at least the coverage at the loop, the
        // breaks of the body are its own
        std::optional<assigned_bytes> outer_breaks{
            std::exchange(at_breaks_, std::nullopt),
        };

        body.trace_assignment(*this);

        const std::optional<assigned_bytes> breaks{
            std::exchange(at_breaks_, std::move(outer_breaks)),
        };

        if (end == loop_end::also_when_done) {
            assigned_ = entry;
            is_reachable_ = true;
            return;
        }

        if (not breaks) {
            end_path();
            return;
        }

        assigned_ = *breaks;
        is_reachable_ = true;
    }
};
