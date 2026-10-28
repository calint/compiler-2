#pragma once

// the texts of the report of the use of registers, after the code with
// '--report=registers' and in the error of registers that ran out

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <format>
#include <functional>
#include <map>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compile_error.hpp"
#include "machine.hpp"
#include "register_pool.hpp"
#include "token.hpp"

// the registers each frame holds, as the text of the report of the use of
// registers: who holds what and what a 'noinline' frame would give
class register_use_report final {
  public:
    // a frame and the registers it holds, each as 'name location'
    struct frame_use {
        // the name of the function
        std::string callee;

        // the function and where it was called
        std::string label;

        std::vector<std::string> registers;
    };

  private:
    // a frame that the registers held above it make worth making 'noinline'
    struct candidate {
        std::string name;
        size_t inside{};
        size_t saved{};
    };

    // the column of the registers held by a frame, its number is right aligned
    // under the heading
    static constexpr std::string_view held_heading_{"held"};
    static constexpr size_t held_width_{held_heading_.size() + 2};
    // note: +2 because the heading is indented by two

    std::string heading_;
    // the scratch registers that can be live, the bases held are not
    size_t register_total_{};
    // the scratch registers that a 'noinline' body starts with
    size_t noinline_register_total_{};
    size_t named_count_{};
    std::vector<frame_use> frames_;

  public:
    register_use_report(std::string heading, const size_t register_total,
                        const size_t noinline_register_total,
                        const size_t named_count, std::vector<frame_use> frames)
        : heading_{std::move(heading)}, register_total_{register_total},
          noinline_register_total_{noinline_register_total},
          named_count_{named_count}, frames_{std::move(frames)} {}

    [[nodiscard]] auto text() const -> std::string {
        return heading_text() + frames_text() + noinline_text();
    }

  private:
    // the frames below the first with registers held above them: what they
    // and what is below them hold, and what the call of them would save
    [[nodiscard]] auto candidates() const -> std::vector<candidate> {
        std::vector<candidate> found;
        size_t saved{};
        size_t inside{live_count()};

        for (const frame_use& use : frames_) {
            if (saved > 0) {
                found.push_back({
                    .name{use.callee},
                    .inside{inside},
                    .saved{saved},
                });
            }

            saved += use.registers.size();
            inside -= use.registers.size();
        }

        return found;
    }

    [[nodiscard]] auto
    candidates_text(const std::vector<candidate>& found) const -> std::string {

        // the name is last so that a long one does not push the numbers apart,
        // the numbers are right aligned under their headings
        constexpr std::string_view inside_heading{"held inside"};
        constexpr std::string_view saved_heading{"saved at the call"};
        constexpr size_t column_gap{2};
        constexpr size_t inside_width{inside_heading.size() + column_gap};
        constexpr size_t saved_width{saved_heading.size() + column_gap};

        std::string text{
            std::format(
                "\n\nthese frames are not noinline, making one noinline "
                "starts its body with all {} registers free and "
                "saves the registers held above it around the "
                "call:\n\n  {}  {}  candidate frame",
                noinline_register_total_, inside_heading, saved_heading),
        };

        for (const candidate& entry : found) {
            text += std::format(
                "\n{}{}  {}", right_aligned(entry.inside, inside_width),
                right_aligned(entry.saved, saved_width), entry.name);
        }

        return text;
    }

    [[nodiscard]] auto frames_text() const -> std::string {
        // the label of a frame follows the number and two spaces, its
        // registers are indented two more
        const std::string register_indent(held_width_ + 4, ' ');

        std::string text;

        for (const frame_use& use : frames_) {
            text += std::format(
                "\n{}  {}", right_aligned(use.registers.size(), held_width_),
                use.label);

            for (const std::string& held : use.registers) {
                // the registers are indented under the label of the frame
                text += std::format("\n{}{}", register_indent, held);
            }
        }

        return text;
    }

    [[nodiscard]] auto heading_text() const -> std::string {
        const std::string named_note{
            named_count_ == 0
                ? std::string{}
                : std::format(", {} named by instructions", named_count_),
        };

        return std::format("{}: {} of {} registers live{}\n\n{}  frame, "
                           "registers (allocated at)",
                           heading_, live_count(), register_total_, named_note,
                           right_aligned(held_heading_, held_width_));
    }

    [[nodiscard]] auto live_count() const -> size_t {
        size_t live{};

        for (const frame_use& use : frames_) {
            live += use.registers.size();
        }

        return live;
    }

    [[nodiscard]] auto noinline_text() const -> std::string {
        if (frames_.size() < 2) {
            return {};
        }

        const std::vector<candidate> found{candidates()};

        if (found.empty()) {
            return "\n\nno noinline frame helps, the registers are held by "
                   "the innermost frame, simplify its expression";
        }

        return candidates_text(found);
    }

    //
    // statics
    //

    // 'std::format' takes a dynamic width of 'int' but not of 'size_t'
    [[nodiscard]] static auto right_aligned(const std::string_view text,
                                            const size_t width) -> std::string {

        const size_t padding{width > text.size() ? width - text.size() : 0};
        return std::string(padding, ' ') + std::string{text};
    }

    [[nodiscard]] static auto right_aligned(const size_t number,
                                            const size_t width) -> std::string {

        return right_aligned(std::format("{}", number), width);
    }
};

// the texts of the report of the use of registers, from what the trace of a
// machine recorded and how the machine names its registers
class register_reporter final {
    std::reference_wrapper<const machine> machine_;
    std::reference_wrapper<const register_trace> trace_;

  public:
    register_reporter(const machine& backend, const register_trace& trace)
        : machine_{backend}, trace_{trace} {}

    // see 'machine::register_error'
    [[nodiscard]] auto make_error(const token& src_loc_tk, std::string message,
                                  const register_pool& pool) const
        -> compile_error {

        compile_error error{src_loc_tk, std::move(message)};

        // the lines of the message of an error are not longer than this
        constexpr size_t max_detail_width{80};

        const std::string text{
            make_register_report("register use at the failure",
                                 pool.allocations(), trace_.get().frames())
                .text(),
        };

        for (const std::string& part :
             wrapped_lines(split_lines(text), max_detail_width)) {

            error.detail += error.detail.empty() ? "" : "\n";
            error.detail += part;
        }

        return error;
    }

    // the use of registers at the busiest point of the build, as lines after
    // the code when asked for, else none
    [[nodiscard]] auto peak_report() const -> std::vector<std::string> {
        // room for the comment marker and a space of the assembly output
        constexpr size_t max_line_width{78};
        return wrapped_lines(register_peak_lines(), max_line_width);
    }

  private:
    // e.g. 'top called at 36:33'
    [[nodiscard]] auto called_at(const register_trace::frame& frame) const
        -> std::string {

        return std::format("{} called at {}", frame.name,
                           machine_.get().location_text(frame.call_site_tk));
    }

    // the most registers one call of each callee holds itself
    [[nodiscard]] auto callee_lines() const -> std::vector<std::string> {
        std::vector<std::string> lines{
            "",
            "per callee, the most registers one call holds itself",
            "",
            "  own  calls  callee",
        };

        for (const auto& [name, use] :
             descending(trace_.get().callees(),
                        &register_trace::callee_use::own_peak)) {

            lines.push_back(
                std::format("{:>5}{:>7}  {}", use.own_peak, use.calls, name));
        }

        return lines;
    }

    // the frames without registers: the first is the function being compiled,
    // the others are the calls inlined in it
    [[nodiscard]] auto
    frame_uses(const std::vector<register_trace::frame>& frames) const
        -> std::vector<register_use_report::frame_use> {

        // note: only the global initializers compile outside a function
        if (frames.empty()) {
            return {
                {
                    .callee{"global initializers"},
                    .label{"global initializers"},
                    .registers{},
                },
            };
        }

        std::vector<register_use_report::frame_use> uses;

        for (const auto [index, frame] : std::views::enumerate(frames)) {
            uses.push_back({
                .callee{frame.name},
                .label{index == 0 ? frame.name : called_at(frame)},
                .registers{},
            });
        }

        return uses;
    }

    // what each frame of 'frames' holds of the 'allocations'
    [[nodiscard]] auto make_register_report(
        const std::string_view heading,
        const std::span<const register_pool::allocation> allocations,
        const std::vector<register_trace::frame>& frames) const
        -> register_use_report {

        std::vector<register_use_report::frame_use> uses{frame_uses(frames)};
        size_t named{};

        for (const register_pool::allocation& allocated : allocations) {
            if (not register_trace::is_listed(allocated)) {
                continue;
            }

            assert(allocated.frame < uses.size());

            uses.at(allocated.frame)
                .registers.push_back(register_text(allocated));

            if (allocated.holder == register_pool::holder_kind::named) {
                ++named;
            }
        }

        const machine& backend{machine_.get()};

        return {
            std::string{heading},
            backend.scratch_register_count(allocations),
            backend.noinline_body_register_count(),
            named,
            std::move(uses),
        };
    }

    // the registers that the calls of each function with a body of its own save
    [[nodiscard]] auto noinline_call_lines() const -> std::vector<std::string> {
        if (trace_.get().noinline_uses().empty()) {
            return {};
        }

        std::vector<std::string> lines{
            "",
            "calls of functions with a body of their own save the registers "
            "held at the call",
            "",
            "  saved  calls  callee, most saved at",
        };

        for (const auto& [label, use] :
             descending(trace_.get().noinline_uses(),
                        &register_trace::noinline_use::saved_peak)) {

            lines.push_back(
                std::format("{:>7}{:>7}  {} {}", use.saved_peak, use.calls,
                            without_prefix(label, machine::body_label_prefix),
                            machine_.get().location_text(use.peak_site_tk)));
        }

        return lines;
    }

    // what to know when the peak is in a function with a body of its own
    [[nodiscard]] auto
    peak_note_lines(const std::vector<register_trace::frame>& frames) const
        -> std::vector<std::string> {

        if (frames.empty() or
            frames.front().kind != register_trace::frame_kind::noinline_body) {

            return {};
        }

        return {
            "",
            std::format("the peak is in a function with a body of its own, "
                        "compiled with all {} registers free, its callers are "
                        "not on this stack",
                        machine_.get().noinline_body_register_count()),
        };
    }

    [[nodiscard]] auto register_peak_lines() const -> std::vector<std::string> {
        if (not trace_.get().is_enabled()) {
            return {};
        }

        const register_trace::peak_use& peak{trace_.get().peak()};

        const std::string text{
            make_register_report("register use at the peak", peak.allocations,
                                 peak.frames)
                .text(),
        };

        std::vector<std::string> lines{split_lines(text)};

        lines.append_range(peak_note_lines(peak.frames));
        lines.append_range(callee_lines());
        lines.append_range(noinline_call_lines());

        return lines;
    }

    // e.g. 'r15 153:13' or 'rax 153:31 (named)'
    [[nodiscard]] auto
    register_text(const register_pool::allocation& allocated) const
        -> std::string {

        return std::format(
            "{} {}{}", machine_.get().register_display_name(allocated.index),
            machine_.get().location_text(allocated.src_loc_tk),
            allocated.holder == register_pool::holder_kind::named ? " (named)"
                                                                  : "");
    }

    //
    // statics
    //

    // the entries ordered by the member 'peak', the highest first
    template <typename use_t>
    [[nodiscard]] static auto
    descending(const std::map<std::string, use_t>& entries,
               size_t use_t::* const peak)
        -> std::vector<std::pair<std::string, use_t>> {

        std::vector<std::pair<std::string, use_t>> ordered{
            entries.begin(),
            entries.end(),
        };

        std::ranges::stable_sort(
            ordered, [peak](const auto& lhs, const auto& rhs) -> bool {
                return lhs.second.*peak > rhs.second.*peak;
            });

        return ordered;
    }

    [[nodiscard]] static auto split_lines(const std::string_view text)
        -> std::vector<std::string> {

        std::vector<std::string> lines;

        for (const auto line : std::views::split(text, '\n')) {
            lines.emplace_back(std::string_view{line});
        }

        return lines;
    }

    [[nodiscard]] static auto without_prefix(const std::string_view text,
                                             const std::string_view prefix)
        -> std::string_view {

        return text.starts_with(prefix) ? text.substr(prefix.size()) : text;
    }

    // breaks a line that is too long at spaces, the parts keep its indentation
    [[nodiscard]] static auto wrapped(const std::string& line,
                                      const size_t width)
        -> std::vector<std::string> {

        std::vector<std::string> parts;

        const std::string indentation(
            std::min(line.find_first_not_of(' '), line.size()), ' ');

        std::string rest{line};

        while (rest.size() > width) {
            const size_t space{rest.rfind(' ', width)};

            // note: a word longer than the width is left as it is
            if (space == std::string::npos or space <= indentation.size()) {
                break;
            }

            const size_t word_end{
                std::min(rest.find(' ', space + 1), rest.size()),
            };

            // a word that does not fit on a line of its own stays after the
            // words before it
            if (indentation.size() + word_end - (space + 1) > width) {
                break;
            }

            // the spaces before the break are dropped with it
            parts.push_back(
                rest.substr(0, rest.find_last_not_of(' ', space) + 1));

            std::string next{indentation};
            next += rest.substr(space + 1);
            // note: +1 because the space that the line broke at is dropped

            rest = std::move(next);
        }

        parts.push_back(std::move(rest));

        return parts;
    }

    [[nodiscard]] static auto
    wrapped_lines(const std::vector<std::string>& lines, const size_t width)
        -> std::vector<std::string> {

        std::vector<std::string> parts;

        for (const std::string& line : lines) {
            for (std::string& part : wrapped(line, width)) {
                parts.push_back(std::move(part));
            }
        }

        return parts;
    }
};

// declared in 'machine'
inline auto machine::register_error(const token& src_loc_tk,
                                    std::string message) const
    -> compile_error {

    return register_reporter{*this, state_.trace}.make_error(
        src_loc_tk, std::move(message), state_.registers);
}

// declared in 'machine'
inline auto machine::register_peak_report() const -> std::vector<std::string> {
    return register_reporter{*this, state_.trace}.peak_report();
}
