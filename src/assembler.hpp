#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>
#include <print>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// buffers generated assembly with its labels and jumps known so every target
// optimizes jumps the same way, subclasses supply the target's syntax
//
// jump optimizations done by 'optimize_jumps', in x86_64 and rv32i syntax:
//
// jumps_to_next:
//     jmp if.16.8.code                  j if.16.8.code
//     if.16.8.code:                     if.16.8.code:
//   to
//     if.16.8.code:                     if.16.8.code:
//
// unreachable_jumps:
//     jmp loop.10.5.end                 j loop.10.5.end
//     jmp loop.10.5                     j loop.10.5
//   to
//     jmp loop.10.5.end                 j loop.10.5.end
//
// same_outcome_branches:
//     jne bool.15.19.end                beq t0, zero, bool.15.19.end
//     jmp bool.15.19.end                j bool.15.19.end
//   to
//     jmp bool.15.19.end                j bool.15.19.end
//
// inverted_branches:
//     jne cmp.19.27                     bne t0, t1, cmp.19.27
//     jmp if.19.8.code                  j if.19.8.code
//     cmp.19.27:                        cmp.19.27:
//   to
//     je if.19.8.code                   beq t0, t1, if.19.8.code
//     cmp.19.27:                        cmp.19.27:
//
// a label between the lines stops a rule only when a jump or another line,
// such as a call, names it

class assembler {
  public:
    // 'as_emitted' writes directly, 'resolved' buffers without optimizing
    enum class jump_mode : uint8_t { as_emitted, resolved, optimized };

    // changes made by 'optimize_jumps', added by 'add_optimization_counts'
    struct optimization_counts {
        size_t jumps_to_next{};
        size_t unreachable_jumps{};
        size_t same_outcome_branches{};
        size_t inverted_branches{};
    };

    struct jump_info {
        std::string mnemonic;
        // operands before the target, empty when the target is the only one
        std::string operands;
        std::string target;
        // a free register for targets that need one to reach far
        std::string scratch;
    };

    struct line {
        std::string text;
        std::string label;
        std::unique_ptr<jump_info> jump;
        // in the target's unit, zero when the line emits no code
        size_t code_size{};
        // a label in code where execution can enter
        bool entry{};
        bool removed{};
        // index of the target's structured form of the line
        std::optional<size_t> record;
    };

    assembler() = default;
    assembler(const assembler&) = delete;
    assembler(assembler&&) = delete;
    auto operator=(const assembler&) -> assembler& = delete;
    auto operator=(assembler&&) -> assembler& = delete;
    virtual ~assembler() = default;

    // lines are written to 'os' as they are added, or buffered when null
    auto set_direct_output(std::ostream* const os) -> void {
        direct_output_ = os;
    }

    [[nodiscard]] auto direct_output() const -> std::ostream* {
        return direct_output_;
    }

    [[nodiscard]] auto is_buffering() const -> bool {
        return direct_output_ == nullptr;
    }

    // a blank line that separates parts of the output
    auto add_separator_newline() -> void { add_text(""); }

    // lines added by 'emit' are kept apart so a version can be chosen
    [[nodiscard]] auto capture(const std::function_ref<void()> emit)
        -> std::vector<line> {

        captures_.emplace_back();
        emit();
        std::vector<line> captured{std::move(captures_.back())};
        captures_.pop_back();

        return captured;
    }

    auto append(std::vector<line> lines) -> void {
        std::ranges::move(lines, std::back_inserter(current_lines()));
    }

    // removes jumps that change nothing and turns a branch over a jump into
    // the inverse branch, repeating because each change can enable another
    auto optimize_jumps() -> void {
        assert(captures_.empty());

        const std::unordered_map<std::string_view, size_t> labels{
            label_lines()};

        const std::unordered_set<std::string_view> named{
            labels_named_outside_jumps(labels)};

        bool changed{true};
        while (changed) {
            changed = false;

            // removed jumps no longer reference their targets
            const std::unordered_set<std::string_view> referenced{
                referenced_labels(labels, named)};

            for (size_t index{}; index < lines_.size(); ++index) {
                changed = optimize_jump(index, labels, referenced) or changed;
            }
        }
    }

    // adds the optimization counts as comments aligned with the usage
    // statistics that follow
    auto add_optimization_counts() -> void {
        const std::string_view prefix{comment_prefix()};

        add_text("");

        add_text(std::format("{} {:>28}: {}", prefix,
                             "removed jumps to next code",
                             optimizations_.jumps_to_next));

        add_text(std::format("{} {:>28}: {}", prefix,
                             "removed unreachable jumps",
                             optimizations_.unreachable_jumps));

        add_text(std::format("{} {:>28}: {}", prefix,
                             "removed same target branches",
                             optimizations_.same_outcome_branches));

        add_text(std::format("{} {:>28}: {}", prefix,
                             "inverted branches over jumps",
                             optimizations_.inverted_branches));

        optimizations_ = {};
    }

    // writes the lines as they are, far jumps are left to the caller
    auto write(std::ostream& os) -> void {
        assert(captures_.empty());

        for (const line& l : lines_) {
            if (not l.removed) {
                std::println(os, "{}", l.text);
            }
        }
        lines_.clear();
    }

  protected:
    // code sizes and entries only count in code
    auto set_code_section(const bool code_section) -> void {
        code_section_ = code_section;
    }

    auto add_text(std::string text) -> void {
        if (write_directly(text)) {
            return;
        }

        if (not code_section_) {
            current_lines().push_back({
                .text{std::move(text)},
                .label{},
                .jump{},
                .code_size{},
                .entry{},
                .removed{},
                .record{},
            });

            return;
        }

        const size_t code_size{text_code_size(text)};

        // text labels such as rv32i numeric labels still let execution enter
        const bool entry{is_label_text(text)};

        current_lines().push_back({
            .text{std::move(text)},
            .label{},
            .jump{},
            .code_size{code_size},
            .entry{entry},
            .removed{},
            .record{},
        });
    }

    auto add_label(std::string name, std::string text) -> void {
        if (write_directly(text)) {
            return;
        }

        current_lines().push_back({
            .text{std::move(text)},
            .label{std::move(name)},
            .jump{},
            .code_size{},
            .entry{code_section_},
            .removed{},
            .record{},
        });
    }

    // 'mnemonic' is the unconditional jump or a branch taking 'operands'
    auto add_jump(std::string text, const std::string_view mnemonic,
                  const std::string_view operands,
                  const std::string_view target, const std::string_view scratch,
                  const std::optional<size_t> record = std::nullopt) -> void {

        if (write_directly(text)) {
            return;
        }

        const size_t code_size{text_code_size(text)};

        current_lines().push_back({
            .text{std::move(text)},
            .label{},
            .jump{std::make_unique<jump_info>(jump_info{
                .mnemonic{std::string{mnemonic}},
                .operands{std::string{operands}},
                .target{std::string{target}},
                .scratch{std::string{scratch}},
            })},
            .code_size{code_size},
            .entry{},
            .removed{},
            .record{record},
        });
    }

    // a line whose size and structured form the target already knows
    auto add_record_line(std::string text, const size_t code_size,
                         const std::optional<size_t> record) -> void {

        if (write_directly(text)) {
            return;
        }

        current_lines().push_back({
            .text{std::move(text)},
            .label{},
            .jump{},
            .code_size{code_section_ ? code_size : 0},
            .entry{},
            .removed{},
            .record{record},
        });
    }

    [[nodiscard]] auto lines() -> std::vector<line>& { return lines_; }

    [[nodiscard]] auto lines() const -> const std::vector<line>& {
        return lines_;
    }

    [[nodiscard]] auto is_capturing() const -> bool {
        return not captures_.empty();
    }

    [[nodiscard]] auto is_conditional(const jump_info& jump) const -> bool {
        return jump.mnemonic != unconditional_jump_mnemonic();
    }

    [[nodiscard]] auto label_lines() const
        -> std::unordered_map<std::string_view, size_t> {

        std::unordered_map<std::string_view, size_t> labels;
        for (size_t index{}; index < lines_.size(); ++index) {
            if (not lines_[index].label.empty()) {
                labels.emplace(lines_[index].label, index);
            }
        }

        return labels;
    }

    [[nodiscard]] static auto leading_whitespace(const std::string_view text)
        -> std::string_view {

        return text.substr(0, text.find_first_not_of(" \t"));
    }

  private:
    std::vector<line> lines_;
    // versions being emitted by 'capture', innermost last
    std::vector<std::vector<line>> captures_;
    bool code_section_{true};
    optimization_counts optimizations_;
    std::ostream* direct_output_{};

    [[nodiscard]] auto write_directly(const std::string_view text) const
        -> bool {

        if (direct_output_ == nullptr) {
            return false;
        }

        std::println(*direct_output_, "{}", text);

        return true;
    }

    // every other jump is a conditional branch
    [[nodiscard]] virtual auto unconditional_jump_mnemonic() const
        -> std::string_view = 0;

    // the branch taken exactly when 'mnemonic' is not taken
    [[nodiscard]] virtual auto
    inverse_branch_mnemonic(std::string_view mnemonic) const
        -> std::optional<std::string_view> = 0;

    // the jump instruction without indentation
    [[nodiscard]] virtual auto format_jump(const jump_info& jump) const
        -> std::string = 0;

    [[nodiscard]] virtual auto comment_prefix() const -> std::string_view = 0;

    // zero for labels, comments and directives that emit no code
    [[nodiscard]] virtual auto text_code_size(std::string_view text) const
        -> size_t = 0;

    [[nodiscard]] virtual auto is_label_text(std::string_view text) const
        -> bool = 0;

    [[nodiscard]] auto current_lines() -> std::vector<line>& {
        if (captures_.empty()) {
            return lines_;
        }

        return captures_.back();
    }

    [[nodiscard]] static auto
    destination(const std::unordered_map<std::string_view, size_t>& labels,
                const jump_info& jump) -> std::optional<size_t> {

        const auto found{labels.find(jump.target)};
        if (found == labels.end()) {
            return std::nullopt;
        }

        return found->second;
    }

    // labels, comments, other sections and removed lines emit no code
    [[nodiscard]] auto next_instruction(size_t index) const -> size_t {
        while (index < lines_.size() and lines_[index].code_size == 0) {
            ++index;
        }

        return index;
    }

    // labels named by lines other than jumps e.g. calls, addresses and
    // '.globl', these keep their references while jumps are optimized
    [[nodiscard]] auto labels_named_outside_jumps(
        const std::unordered_map<std::string_view, size_t>& labels) const
        -> std::unordered_set<std::string_view> {

        std::unordered_set<std::string_view> named;
        for (const line& l : lines_) {
            if (l.removed or l.jump or not l.label.empty()) {
                continue;
            }

            add_named_labels(l.text, labels, named);
        }

        return named;
    }

    // comments naming a label only keep it referenced, which is safe
    static auto
    add_named_labels(const std::string_view text,
                     const std::unordered_map<std::string_view, size_t>& labels,
                     std::unordered_set<std::string_view>& named) -> void {

        size_t begin{};
        while (begin < text.size()) {
            if (not is_symbol_char(text[begin])) {
                ++begin;
                continue;
            }

            size_t end{begin};
            while (end < text.size() and is_symbol_char(text[end])) {
                ++end;
            }

            // the key views the label line, which outlives the optimization
            const auto found{labels.find(text.substr(begin, end - begin))};
            if (found != labels.end()) {
                named.insert(found->first);
            }

            begin = end;
        }
    }

    [[nodiscard]] static auto is_symbol_char(const char ch) -> bool {
        return std::isalnum(static_cast<unsigned char>(ch)) != 0 or ch == '_' or
               ch == '.' or ch == '$';
    }

    // a jump target is viewed through the label map because inverting a
    // branch replaces its target string
    [[nodiscard]] auto referenced_labels(
        const std::unordered_map<std::string_view, size_t>& labels,
        const std::unordered_set<std::string_view>& named) const
        -> std::unordered_set<std::string_view> {

        std::unordered_set<std::string_view> referenced{named};
        for (const line& l : lines_) {
            if (not l.jump) {
                continue;
            }

            const auto found{labels.find(l.jump->target)};
            if (found != labels.end()) {
                referenced.insert(found->first);
            }
        }

        return referenced;
    }

    // text labels and numeric labels, named by references such as '1b', are
    // entries without a lookup
    [[nodiscard]] static auto
    is_enterable(const line& l,
                 const std::unordered_set<std::string_view>& referenced)
        -> bool {

        if (not l.entry) {
            return false;
        }

        if (l.label.empty() or is_numeric(l.label)) {
            return true;
        }

        return referenced.contains(l.label);
    }

    [[nodiscard]] static auto is_numeric(const std::string_view text) -> bool {
        return std::ranges::all_of(text, [](const char ch) -> bool {
            return std::isdigit(static_cast<unsigned char>(ch)) != 0;
        });
    }

    // a label in between would let execution enter between the two jumps
    [[nodiscard]] auto following_unconditional_jump(
        const size_t index,
        const std::unordered_set<std::string_view>& referenced) const
        -> std::optional<size_t> {

        for (size_t next{index + 1}; next < lines_.size(); ++next) {
            const line& l{lines_[next]};
            if (is_enterable(l, referenced)) {
                return std::nullopt;
            }

            if (l.code_size == 0) {
                continue;
            }

            if (not l.jump or is_conditional(*l.jump)) {
                return std::nullopt;
            }

            return next;
        }

        return std::nullopt;
    }

    static auto remove(line& l) -> void {
        l.jump.reset();
        l.code_size = 0;
        l.removed = true;
    }

    // branches without an inverse are kept
    [[nodiscard]] auto invert(line& l, std::string target) const -> bool {
        jump_info& jump{*l.jump};
        const std::optional<std::string_view> inverted{
            inverse_branch_mnemonic(jump.mnemonic)};

        if (not inverted) {
            return false;
        }

        jump.mnemonic = *inverted;
        jump.target = std::move(target);
        l.text = std::string{leading_whitespace(l.text)} + format_jump(jump);

        return true;
    }

    [[nodiscard]] auto
    optimize_jump(const size_t index,
                  const std::unordered_map<std::string_view, size_t>& labels,
                  const std::unordered_set<std::string_view>& referenced)
        -> bool {

        line& branch{lines_[index]};
        if (not branch.jump) {
            return false;
        }

        // an undefined target is left to the target's assembling
        const std::optional<size_t> target{destination(labels, *branch.jump)};
        if (not target) {
            return false;
        }

        const size_t target_code{next_instruction(*target)};

        // execution continues at the target anyway
        if (target_code == next_instruction(index + 1)) {
            remove(branch);
            ++optimizations_.jumps_to_next;

            return true;
        }

        const std::optional<size_t> jump_index{
            following_unconditional_jump(index, referenced)};

        if (not jump_index) {
            return false;
        }

        line& jump{lines_[*jump_index]};

        // nothing reaches a jump right after an unconditional jump
        if (not is_conditional(*branch.jump)) {
            remove(jump);
            ++optimizations_.unreachable_jumps;

            return true;
        }

        const std::optional<size_t> jump_target{
            destination(labels, *jump.jump)};

        if (not jump_target) {
            return false;
        }

        // both outcomes continue at the same place
        if (target_code == next_instruction(*jump_target)) {
            remove(branch);
            ++optimizations_.same_outcome_branches;

            return true;
        }

        // branching over the jump is the inverse branch to its target
        if (target_code != next_instruction(*jump_index + 1)) {
            return false;
        }

        if (not invert(branch, jump.jump->target)) {
            return false;
        }

        remove(jump);
        ++optimizations_.inverted_branches;

        return true;
    }
};
