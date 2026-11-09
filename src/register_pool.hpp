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

// the registers of a machine that hold a value: named ones are fixed by an
// instruction or the calling convention, scratch ones are picked by the
// machine, and the backend holds some for itself (the variables base, the
// frame base of a noinline body), which are named too; all are freed in the
// reverse order of allocation. a register is 'unavailable' while it is
// allocated and while an operation protects the registers of its operands.
// the machine names the registers, the pool counts them by their index in the
// names of the machine and by masks of 1 << index
class register_pool final {
  public:
    // who holds a register: picked by the machine, fixed by an instruction or
    // the calling convention, or held by the backend itself, not asked for by
    // a source location
    enum class holder_kind : uint8_t { scratch, named, backend };

    struct allocation {
        size_t index{};
        // where it was allocated, 'indent' is the indent of that code
        token src_loc_tk;
        size_t indent{};
        holder_kind holder{};
        // the index of the call level that allocated it, see
        // 'register_trace::frame'
        size_t frame{};
    };

  private:
    uint32_t unavailable_{};
    std::vector<allocation> allocations_;

  public:
    [[nodiscard]] auto allocations() const -> std::span<const allocation> {
        return allocations_;
    }

    // the allocation of the register 'index', which is allocated
    [[nodiscard]] auto holder_of(const size_t index) const
        -> const allocation& {

        const auto holder{
            std::ranges::find(allocations_, index, &allocation::index),
        };

        // note: a register is protected without being allocated only inside a
        //       backend operation, which reserves no named register there

        assert(holder != allocations_.end());

        return *holder;
    }

    [[nodiscard]] auto is_empty() const -> bool { return allocations_.empty(); }

    // 'mask' has a register that is allocated or protected
    [[nodiscard]] auto is_unavailable(const uint32_t mask) const -> bool {
        return (unavailable_ & mask) != 0;
    }

    // the register of the last allocation, freeing is in reverse order
    auto pop(const size_t index) -> void {
        assert(top().index == index);

        allocations_.pop_back();
        unavailable_ &= ~(uint32_t{1} << index);
    }

    auto protect(const uint32_t mask) -> void { unavailable_ |= mask; }

    auto push(const allocation& entry) -> void {
        const uint32_t mask{uint32_t{1} << entry.index};

        assert(not is_unavailable(mask));

        allocations_.push_back(entry);
        unavailable_ |= mask;
    }

    // forgets what 'protect' added since 'saved' was read by 'unavailable_mask'
    auto restore_unavailable(const uint32_t saved) -> void {
        unavailable_ = saved;
    }

    [[nodiscard]] auto scratch_count() const -> size_t {
        return static_cast<size_t>(std::ranges::count(
            allocations_, holder_kind::scratch, &allocation::holder));
    }

    [[nodiscard]] auto top() const -> const allocation& {
        assert(not allocations_.empty());

        return allocations_.back();
    }

    // the registers that are allocated or protected, none when all is freed
    [[nodiscard]] auto unavailable_mask() const -> uint32_t {
        return unavailable_;
    }

    // the protected registers of 'mask' may be allocated, none is allocated
    auto unprotect(const uint32_t mask) -> void {
        assert(std::ranges::none_of(
            allocations_, [&](const allocation& allocated) -> bool {
                return (mask & (uint32_t{1} << allocated.index)) != 0;
            }));

        unavailable_ &= ~mask;
    }
};

// the inlined calls being compiled and the busiest use of registers so far,
// collected for the report of the use of registers
class register_trace final {
  public:
    // a body compiled in place of its call, or one of its own
    enum class frame_kind : uint8_t { inlined, noinline_body };

    // a call level: the first is 'main' or the noinline body being compiled,
    // the others the calls inlined in it
    struct frame {
        std::string name;
        token call_site_tk;
        frame_kind kind{};
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

    // the listed registers held when the most were
    struct peak_use {
        std::vector<register_pool::allocation> allocations;
        std::vector<frame> frames;
    };

  private:
    bool enabled_{};
    std::vector<frame> frames_;
    peak_use peak_{};
    std::map<std::string, callee_use> callees_;
    std::map<std::string, noinline_use> noinline_uses_;

  public:
    // the registers allocated until 'end_frame' belong to the call of
    // 'name' made at 'call_site_tk'
    auto begin_frame(const token& call_site_tk, std::string name,
                     const frame_kind kind) -> void {

        // note: a body is compiled once for each list of array lengths, its
        //       calls are counted at the calls, see 'record_noinline_call'

        if (kind == frame_kind::noinline_body) {
            name = body_frame_name(name);
        } else if (enabled_) {
            ++callees_[name].calls;
        }

        frames_.push_back({
            .name{std::move(name)},
            .call_site_tk{call_site_tk},
            .kind{kind},
        });

        // a build that holds no register has its peak in its first frame,
        // 'main', not in the global initializers
        if (enabled_ and peak_.allocations.empty() and peak_.frames.empty()) {
            peak_.frames = frames_;
        }
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

    // a call of the function 'name', which has a body of its own, saves
    // 'saved' registers
    auto record_noinline_call(const token& call_site_tk,
                              const std::string_view name, const size_t saved)
        -> void {

        if (not enabled_) {
            return;
        }

        ++callees_[body_frame_name(name)].calls;

        noinline_use& use{noinline_uses_[std::string{name}]};

        ++use.calls;

        if (saved > use.saved_peak or use.calls == 1) {
            use.saved_peak = saved;
            use.peak_site_tk = call_site_tk;
        }
    }

    //
    // statics
    //

    // the frame name of the body of 'name'
    [[nodiscard]] static auto body_frame_name(const std::string_view name)
        -> std::string {

        return std::format("{} (noinline body)", name);
    }

    // the registers the backend holds itself are left out of the report
    [[nodiscard]] static auto
    is_listed(const register_pool::allocation& allocated) -> bool {

        return allocated.holder != register_pool::holder_kind::backend;
    }

  private:
    // what the frame being compiled holds itself
    auto record_own_use(const register_pool& pool) -> void {
        // the registers allocated before the first frame, the variables base
        // and those of the global initializers, are no callee's
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

        if (held <= peak_.allocations.size()) {
            return;
        }

        peak_.allocations.assign_range(live | std::views::filter(is_listed));
        peak_.frames = frames_;
    }
};
