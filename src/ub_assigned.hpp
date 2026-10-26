#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "compile_error.hpp"
#include "token.hpp"
#include "type.hpp"

// how the end of a loop is reached: only at a 'break', e.g. 'loop', or also
// when it runs out, e.g. 'foo'
enum class loop_end : uint8_t { at_break, also_when_done };

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

// definite-assignment walk of one variable through a function body, also the
// walk of which statements are reached ('reachability_only', no variable);
// the statements report what they do ('declare', 'assign', 'end_path',
// 'record_break') and the rules that join paths are here: a block scopes
// its declarations ('block_scope'), 'trace_loop' traces a body by its
// 'loop_end', 'path_merge' joins the paths of an 'if'
class assignment_flow final {
    // empty when the walk answers reachability only
    std::optional<std::string_view> var_;

    // the declaration of the result, reported when a 'return' may leave 'var_'
    // unset
    token src_loc_tk_;

    // bytes assigned on every path reaching the current statement
    assigned_bytes assigned_;

    // bytes assigned at every 'break' of the innermost loop
    std::optional<assigned_bytes> at_breaks_;

    bool is_reachable_{true};

    // a declaration in the current block hides 'var_', the name then means
    // that declaration until the block ends
    bool is_shadowed_{};

  public:
    // a declaration hides 'var_' only until the end of its block
    class block_scope final {
        assignment_flow& flow_;
        bool was_shadowed_;

      public:
        explicit block_scope(assignment_flow& flow)
            : flow_{flow}, was_shadowed_{flow.is_shadowed_} {}

        block_scope(const block_scope&) = delete;
        block_scope(block_scope&&) = delete;
        auto operator=(const block_scope&) -> block_scope& = delete;
        auto operator=(block_scope&&) -> block_scope& = delete;

        ~block_scope() { flow_.is_shadowed_ = was_shadowed_; }
    };

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
    };

    assignment_flow(const std::optional<std::string_view> var,
                    const token& src_loc_tk, const size_t size_bytes)
        : var_{var}, src_loc_tk_{src_loc_tk}, assigned_{size_bytes} {}

    auto assert_set_at_return() const -> void {
        if (assigned_.is_full()) {
            return;
        }

        throw compile_error{
            src_loc_tk_,
            "function may return without setting its return value"};
    }

    // the traced variable is assigned these bytes
    auto assign(const byte_range bytes) -> void { assigned_.add(bytes); }

    [[nodiscard]] auto assigned() const -> const assigned_bytes& {
        return assigned_;
    }

    // a declaration of 'name' hides 'var_' for the rest of its block
    auto declare(const std::string_view name) -> void {
        if (name == var_) {
            is_shadowed_ = true;
        }
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
        const assigned_bytes entry{assigned_};

        const std::optional<assigned_bytes> breaks{trace_loop_body(body)};

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

    // the variable that a name in the current statement may mean, none when
    // a declaration hides it or the walk traces no variable
    [[nodiscard]] auto traced_var() const -> std::optional<std::string_view> {
        if (is_shadowed_) {
            return std::nullopt;
        }

        return var_;
    }

    //
    // statics
    //

    // a walk that traces no variable and answers only which statements are
    // reached, e.g. the error of a statement after a 'return'
    [[nodiscard]] static auto reachability_only(const token& src_loc_tk)
        -> assignment_flow {

        return {std::nullopt, src_loc_tk, 0};
    }

  private:
    // every iteration starts with at least the coverage at the loop, the
    // breaks of the body are its own; returns the coverage common to them
    [[nodiscard]] auto trace_loop_body(const auto& body)
        -> std::optional<assigned_bytes> {

        std::optional<assigned_bytes> outer_breaks{
            std::exchange(at_breaks_, std::nullopt),
        };

        body.trace_assignment(*this);

        return std::exchange(at_breaks_, std::move(outer_breaks));
    }
};
