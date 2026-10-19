#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "token.hpp"

class type;

// the registers of a machine that hold a value: named ones are fixed by an
// instruction or the calling convention, scratch ones are picked by the
// machine, all are freed in the reverse order of allocation. a register is
// 'unavailable' while it is allocated and while an operation protects the
// registers of its operands. the machine names the registers, the pool counts
// them by their index in the names of the machine and by masks of 1 << index
class register_pool final {
  public:
    struct allocation {
        size_t index{};
        // where it was allocated, 'indent' is the indent of that code
        token src_loc_tk;
        size_t indent{};
        const type* type_ptr{};
        bool named{};
        // the index of the inlined call that allocated it, see 'call_frame'
        size_t frame{};
    };

  private:
    uint32_t unavailable_{};
    // registers a lower bounds check found non-negative, allocating a register
    // forgets it since the new owner writes its own value
    uint32_t lower_checked_{};
    std::vector<allocation> allocations_;

  public:
    [[nodiscard]] auto allocations() const -> std::span<const allocation> {
        return allocations_;
    }

    [[nodiscard]] auto is_empty() const -> bool { return allocations_.empty(); }

    [[nodiscard]] auto is_lower_checked(const uint32_t mask) const -> bool {
        return (lower_checked_ & mask) != 0;
    }

    // 'mask' has a register that is allocated or protected
    [[nodiscard]] auto is_unavailable(const uint32_t mask) const -> bool {
        return (unavailable_ & mask) != 0;
    }

    auto mark_lower_checked(const uint32_t mask) -> void {
        lower_checked_ |= mask;
    }

    // the register of the last allocation, freeing is in reverse order
    auto pop(const size_t index) -> allocation {
        assert(top().index == index);

        const allocation entry{allocations_.back()};

        allocations_.pop_back();
        unavailable_ &= ~(uint32_t{1} << index);

        return entry;
    }

    auto protect(const uint32_t mask) -> void { unavailable_ |= mask; }

    auto push(const allocation& entry) -> void {
        const uint32_t mask{uint32_t{1} << entry.index};

        assert(not is_unavailable(mask));

        allocations_.push_back(entry);
        unavailable_ |= mask;
        lower_checked_ &= ~mask;
    }

    // forgets what 'protect' added since 'saved' was read by 'unavailable_mask'
    auto restore_unavailable(const uint32_t saved) -> void {
        unavailable_ = saved;
    }

    [[nodiscard]] auto scratch_count() const -> size_t {
        return static_cast<size_t>(
            std::ranges::count(allocations_, false, &allocation::named));
    }

    [[nodiscard]] auto top() const -> const allocation& {
        assert(not allocations_.empty());

        return allocations_.back();
    }

    // the registers that are allocated or protected, none when all is freed
    [[nodiscard]] auto unavailable_mask() const -> uint32_t {
        return unavailable_;
    }
};

// the inlined calls being compiled and the busiest use of registers so far,
// collected for the report of the use of registers
class register_trace final {
  public:
    // an inlined call, the first frame is the function being compiled
    struct frame {
        std::string name;
        token call_site_tk;
        bool is_noinline_body{};
    };

    // what the calls of a callee hold themselves
    struct callee_use {
        size_t own_peak{};
        size_t calls{};
    };

    // what the calls of a function with a body of its own save
    struct noinline_use {
        size_t calls{};
        size_t saved_peak{};
        token peak_site_tk;
    };

    // the registers held when the most were
    struct peak_use {
        size_t held{};
        std::vector<register_pool::allocation> allocations;
        std::vector<frame> frames;
    };

  private:
    // the name of the frame of a function with a body of its own ends with this
    static constexpr std::string_view noinline_body_suffix{" (noinline body)"};

    bool enabled_{};
    std::vector<frame> frames_;
    peak_use peak_;
    std::map<std::string, callee_use> callees_;
    std::map<std::string, noinline_use> noinline_uses_;

  public:
    // the registers allocated until 'end_frame' belong to the call of
    // 'name' made at 'call_site_tk'
    auto begin_frame(const token& call_site_tk, std::string name,
                     const bool is_noinline_body) -> void {

        if (is_noinline_body) {
            name += noinline_body_suffix;
        }

        if (enabled_) {
            ++callees_[name].calls;
        }

        frames_.push_back({
            .name{std::move(name)},
            .call_site_tk{call_site_tk},
            .is_noinline_body{is_noinline_body},
        });
    }

    [[nodiscard]] auto callees() const
        -> const std::map<std::string, callee_use>& {

        return callees_;
    }

    // the frame that allocations belong to
    [[nodiscard]] auto current_frame() const -> size_t {
        return frames_.empty() ? 0 : frames_.size() - 1;
    }

    auto enable() -> void { enabled_ = true; }

    auto end_frame() -> void { frames_.pop_back(); }

    [[nodiscard]] auto frames() const -> const std::vector<frame>& {
        return frames_;
    }

    [[nodiscard]] auto is_enabled() const -> bool { return enabled_; }

    [[nodiscard]] auto noinline_uses() const
        -> const std::map<std::string, noinline_use>& {

        return noinline_uses_;
    }

    [[nodiscard]] auto peak() const -> const peak_use& { return peak_; }

    // 'pool' has just got a register
    auto record_allocation(const register_pool& pool) -> void {
        if (not enabled_) {
            return;
        }

        record_own_use(pool);
        record_peak(pool);
    }

    // a call of the function with the body 'label' saves 'saved' registers
    auto record_noinline_call(const token& call_site_tk,
                              const std::string_view label, const size_t saved)
        -> void {

        if (not enabled_) {
            return;
        }

        noinline_use& use{noinline_uses_[std::string{label}]};

        ++use.calls;

        if (saved > use.saved_peak or use.calls == 1) {
            use.saved_peak = saved;
            use.peak_site_tk = call_site_tk;
        }
    }

    //
    // statics
    //

    // the registers of a backend that no source location asked for are left
    // out of the report
    [[nodiscard]] static auto
    is_listed(const register_pool::allocation& allocated) -> bool {

        const bool held_by_backend{
            allocated.named and allocated.src_loc_tk.at_line() == 0,
        };

        return not held_by_backend;
    }

  private:
    // what the frame being compiled holds itself
    auto record_own_use(const register_pool& pool) -> void {
        // the registers a backend allocates before the first frame are nobody's
        if (frames_.empty()) {
            return;
        }

        const size_t frame_index{current_frame()};

        size_t own{};

        for (const register_pool::allocation& allocated : pool.allocations()) {
            if (allocated.frame == frame_index and is_listed(allocated)) {
                ++own;
            }
        }

        callee_use& use{callees_[frames_.back().name]};

        use.own_peak = std::max(use.own_peak, own);
    }

    auto record_peak(const register_pool& pool) -> void {
        const std::span<const register_pool::allocation> live{
            pool.allocations(),
        };

        const size_t held{
            static_cast<size_t>(std::ranges::count_if(live, is_listed)),
        };

        if (held <= peak_.held) {
            return;
        }

        peak_.held = held;
        peak_.allocations.assign(live.begin(), live.end());
        peak_.frames = frames_;
    }
};
