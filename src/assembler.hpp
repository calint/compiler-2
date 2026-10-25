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
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// buffers generated assembly with its labels and jumps known so every target
// optimizes jumps the same way, subclasses supply the target's syntax,
// including the data directives and sections, so 'machine' writes the data
// layout once
//
// sections: 'machine' asks for the three it writes ('begin_data_section',
// 'begin_read_only_section', 'begin_variables_section'); a subclass maps them
// onto its own 'section' enum, which also has the code section and what only
// it uses (the x86_64 '.bss' of its number buffer), and the rv32i image is
// indexed by it; every 'switch_section' calls 'set_code_section', since sizes
// and entries count only in code
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
// same_target_branches:
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
// such as a call, names it, or when it is numeric, e.g. '1:' named by '1b'

class assembler {
  public:
    // 'optimized' applies the jump optimizations listed above the class
    enum class jump_mode : uint8_t { as_emitted, optimized };

  protected:
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
        // the label a call line calls, empty for other lines
        std::string call_target;
        std::unique_ptr<jump_info> jump;
        // in the target's unit, zero when the line emits no code
        size_t code_size{};
        // a label in code where execution can enter
        bool entry{};
        bool removed{};
        // index of the target's structured form of the line
        std::optional<size_t> record;
    };

  public:
    // lines that 'capture' kept apart, added where 'append' is called
    using captured_lines = std::vector<line>;

    // changes made by 'optimize_jumps'
    struct optimization_counts {
        size_t jumps_to_next{};
        size_t unreachable_jumps{};
        size_t same_target_branches{};
        size_t inverted_branches{};
    };

    // the bodies of one noinline function, valid once jumps are resolved
    struct function_summary {
        std::string function;
        size_t body_count{};
        size_t call_count{};
        size_t instruction_count{};
    };

  protected:
    // a branch mnemonic and its inverse
    using mnemonic_pair = std::pair<std::string_view, std::string_view>;

  private:
    // the lines [first_line, end_line) are a body of noinline 'function'
    struct body {
        std::string function;
        std::string label;
        size_t first_line{};
        size_t end_line{};
    };

    std::vector<line> lines_;
    // versions being emitted by 'capture', innermost last
    std::vector<captured_lines> captures_;
    bool code_section_{true};
    optimization_counts optimizations_;
    std::vector<body> bodies_;

  public:
    assembler() = default;
    assembler(const assembler&) = delete;
    assembler(assembler&&) = delete;
    auto operator=(const assembler&) -> assembler& = delete;
    auto operator=(assembler&&) -> assembler& = delete;

    virtual ~assembler() = default;

    //
    // virtual methods
    //

    virtual auto align(const size_t size_bytes) -> void = 0;

    virtual auto begin_data_section() -> void = 0;

    virtual auto begin_read_only_section() -> void = 0;

    // the variables are zeroed when defined, so a target need not store them
    // in its image
    virtual auto begin_variables_section() -> void = 0;

    virtual auto comment(const size_t indent, const std::string_view text)
        -> void = 0;

    // 'location' is the source the comment is about, as 'file:line:column'
    virtual auto comment(const size_t indent, const std::string_view location,
                         const std::string_view text) -> void = 0;

    virtual auto data(const size_t element_size_bytes,
                      const std::span<const int64_t> values) -> void = 0;

    virtual auto define_constant(const std::string_view name,
                                 const int64_t value) -> void = 0;

    virtual auto repeated_data(const size_t element_size_bytes,
                               const size_t count, const int64_t value)
        -> void = 0;

    // space in the variables section, whose content is not stored
    virtual auto reserve(const size_t size_bytes) -> void = 0;

    // the target writes the bytes in the escapes of its assembler
    virtual auto string_data(const std::string_view bytes) -> void = 0;

    virtual auto zero_data(const size_t size_bytes) -> void = 0;

    //
    // class methods
    //

    // a blank line that separates parts of the output
    auto add_separator_newline() -> void { add_text(""); }

    auto append(captured_lines lines) -> void {
        std::ranges::move(lines, std::back_inserter(current_lines()));
    }

    // the lines added up to 'end_body' are a body of noinline 'function'
    // entered at 'label'
    auto begin_body(std::string function, std::string label) -> void {
        assert(captures_.empty());

        bodies_.push_back({
            .function{std::move(function)},
            .label{std::move(label)},
            .first_line{lines_.size()},
            .end_line{},
        });
    }

    // lines added by 'emit' are kept apart so a version can be chosen; an
    // error that 'emit' throws drops them
    [[nodiscard]] auto capture(const std::function_ref<void()> emit)
        -> captured_lines {

        captures_.emplace_back();

        try {
            emit();
        } catch (...) {
            captures_.pop_back();
            throw;
        }

        captured_lines captured{std::move(captures_.back())};
        captures_.pop_back();

        return captured;
    }

    [[nodiscard]] auto defines_label(const std::string_view name) const
        -> bool {

        return std::ranges::any_of(
            lines_, [&](const line& ln) -> bool { return ln.label == name; });
    }

    auto end_body() -> void {
        assert(captures_.empty());
        assert(not bodies_.empty());

        bodies_.back().end_line = lines_.size();
    }

    // machine instructions, so pseudo instructions count as the ones they
    // expand to, valid once jumps are resolved
    [[nodiscard]] auto instruction_count() const -> size_t {
        return instructions_in(code_size());
    }

    // the size of captured lines in machine instructions
    [[nodiscard]] auto instruction_count_of(const captured_lines& lines) const
        -> size_t {

        return instructions_in(code_size_of(lines));
    }

    // nothing is buffered
    [[nodiscard]] auto is_empty() const -> bool { return lines_.empty(); }

    auto label(const size_t indent, const std::string_view name) -> void {
        add_label(std::string{name},
                  std::format("{}{}:", indentation(indent), name));
    }

    [[nodiscard]] auto line_count() const -> size_t { return lines_.size(); }

    // the bodies of each noinline function with their calls and size, in
    // the order of definition
    [[nodiscard]] auto noinline_summaries() const
        -> std::vector<function_summary> {

        assert(captures_.empty());

        std::unordered_map<std::string_view, size_t> calls;
        for (const line& l : lines_) {
            if (not l.removed and not l.call_target.empty()) {
                ++calls[l.call_target];
            }
        }

        std::vector<function_summary> summaries;
        for (const body& b : bodies_) {
            function_summary& summary{summary_of(summaries, b.function)};

            ++summary.body_count;
            summary.call_count += calls[b.label];

            const size_t size{
                code_size_of(std::span<const line>{lines_}.subspan(
                    b.first_line, b.end_line - b.first_line)),
            };

            summary.instruction_count += instructions_in(size);
        }

        return summaries;
    }

    [[nodiscard]] auto optimizations() const -> const optimization_counts& {
        return optimizations_;
    }

    // removes jumps that change nothing and turns a branch over a jump into
    // the inverse branch, repeating because each change can enable another
    auto optimize_jumps() -> void {
        assert(captures_.empty());

        const std::unordered_map<std::string_view, size_t> labels{
            label_lines(),
        };

        const std::unordered_set<std::string_view> named{
            labels_named_outside_jumps(labels),
        };

        bool changed{true};
        while (changed) {
            changed = false;

            // removed jumps no longer reference their targets
            const std::unordered_set<std::string_view> referenced{
                referenced_labels(labels, named),
            };

            for (size_t index{}; index < lines_.size(); ++index) {
                changed = optimize_jump(index, labels, referenced) or changed;
            }
        }
    }

    // the text of the lines from 'first_line' on, as 'write' writes them
    [[nodiscard]] auto text_from(const size_t first_line) const -> std::string {
        assert(captures_.empty());

        std::string text;

        for (const line& l : lines_ | std::views::drop(first_line)) {
            if (not l.removed) {
                text += l.text;
                text += '\n';
            }
        }

        return text;
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
    // 'mnemonic' is the unconditional jump or a branch taking 'operands'
    auto add_jump(std::string text, const std::string_view mnemonic,
                  const std::string_view operands,
                  const std::string_view target, const std::string_view scratch,
                  const std::optional<size_t> record = std::nullopt) -> void {

        const size_t code_size{text_code_size(text)};

        current_lines().push_back({
            .text{std::move(text)},
            .label{},
            .call_target{},
            .jump{
                std::make_unique<jump_info>(jump_info{
                    .mnemonic{std::string{mnemonic}},
                    .operands{std::string{operands}},
                    .target{std::string{target}},
                    .scratch{std::string{scratch}},
                }),
            },
            .code_size{code_size},
            .entry{},
            .removed{},
            .record{record},
        });
    }

    auto add_label(std::string name, std::string text) -> void {
        current_lines().push_back({
            .text{std::move(text)},
            .label{std::move(name)},
            .call_target{},
            .jump{},
            .code_size{},
            .entry{code_section_},
            .removed{},
            .record{},
        });
    }

    // a line whose size and structured form the target already knows
    auto add_record_line(std::string text, const size_t code_size,
                         const std::optional<size_t> record) -> void {

        current_lines().push_back({
            .text{std::move(text)},
            .label{},
            .call_target{},
            .jump{},
            .code_size{code_section_ ? code_size : 0},
            .entry{},
            .removed{},
            .record{record},
        });
    }

    auto add_text(std::string text) -> void {
        const size_t code_size{code_section_ ? text_code_size(text) : 0};

        current_lines().push_back({
            .text{std::move(text)},
            .label{},
            .call_target{},
            .jump{},
            .code_size{code_size},
            .entry{},
            .removed{},
            .record{},
        });
    }

    // in the target's unit, removed lines have no size
    [[nodiscard]] auto code_size() const -> size_t {
        return code_size_of(lines_);
    }

    [[nodiscard]] auto is_capturing() const -> bool {
        return not captures_.empty();
    }

    [[nodiscard]] auto is_code_section() const -> bool { return code_section_; }

    [[nodiscard]] auto is_conditional(const jump_info& jump) const -> bool {
        return jump.mnemonic != unconditional_jump_mnemonic();
    }

    [[nodiscard]] auto label_lines() const
        -> std::unordered_map<std::string_view, size_t> {

        std::unordered_map<std::string_view, size_t> labels;
        for (size_t index{}; index < lines_.size(); ++index) {
            if (not lines_.at(index).label.empty()) {
                labels.emplace(lines_.at(index).label, index);
            }
        }

        return labels;
    }

    [[nodiscard]] auto lines() -> std::vector<line>& { return lines_; }

    [[nodiscard]] auto lines() const -> const std::vector<line>& {
        return lines_;
    }

    // the line just added calls the label 'target'
    auto mark_call(const std::string_view target) -> void {
        current_lines().back().call_target = target;
    }

    // code sizes and entries only count in code
    auto set_code_section(const bool code_section) -> void {
        code_section_ = code_section;
    }

    //
    // statics
    //

    // the text before 'comment_marker' without surrounding whitespace
    [[nodiscard]] static auto code_before(const std::string_view text,
                                          const std::string_view comment_marker)
        -> std::string_view {

        return trim(text.substr(0, text.find(comment_marker)));
    }

    // the spaces before an instruction or a label, four for each level
    [[nodiscard]] static auto indentation(const size_t indent) -> std::string {
        constexpr size_t spaces_per_level{4};
        std::string text(indent * spaces_per_level, ' ');
        return text;
    }

    [[nodiscard]] static auto is_numeric(const std::string_view text) -> bool {
        return std::ranges::all_of(text, [](const char ch) -> bool {
            return std::isdigit(static_cast<unsigned char>(ch)) != 0;
        });
    }

    [[nodiscard]] static auto leading_whitespace(const std::string_view text)
        -> std::string_view {

        return text.substr(0, text.find_first_not_of(" \t"));
    }

    // the other mnemonic of the pair holding 'mnemonic'
    [[nodiscard]] static auto
    paired_mnemonic(const std::span<const mnemonic_pair> pairs,
                    const std::string_view mnemonic) -> std::string_view {

        for (const auto& [first, second] : pairs) {
            if (mnemonic == first) {
                return second;
            }

            if (mnemonic == second) {
                return first;
            }
        }

        // the branches the optimizer inverts are all paired
        std::unreachable();
    }

  private:
    //
    // virtual methods
    //

    // the jump instruction without indentation
    [[nodiscard]] virtual auto format_jump(const jump_info& jump) const
        -> std::string = 0;

    // the code size in instructions
    [[nodiscard]] virtual auto instructions_in(size_t code_size) const
        -> size_t = 0;

    // the branch taken exactly when 'mnemonic' is not taken
    [[nodiscard]] virtual auto
    inverse_branch_mnemonic(std::string_view mnemonic) const
        -> std::string_view = 0;

    // zero for comments and directives that emit no code, a label is added
    // apart
    [[nodiscard]] virtual auto text_code_size(std::string_view text) const
        -> size_t = 0;

    // every other jump is a conditional branch
    [[nodiscard]] virtual auto unconditional_jump_mnemonic() const
        -> std::string_view = 0;

    //
    // class methods
    //

    [[nodiscard]] auto current_lines() -> std::vector<line>& {
        if (captures_.empty()) {
            return lines_;
        }

        return captures_.back();
    }

    // a label in between would let execution enter between the two jumps
    [[nodiscard]] auto following_unconditional_jump(
        const size_t index,
        const std::unordered_set<std::string_view>& referenced) const
        -> std::optional<size_t> {

        for (size_t next{index + 1}; next < lines_.size(); ++next) {
            const line& l{lines_.at(next)};

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

        // the code ends, e.g. with the jump of a loop that never ends
        return std::nullopt;
    }

    // every branch the optimizer sees has an inverse
    auto invert(line& l, std::string target) const -> void {
        jump_info& jump{*l.jump};

        jump.mnemonic = inverse_branch_mnemonic(jump.mnemonic);
        jump.target = std::move(target);
        l.text = std::string{leading_whitespace(l.text)} + format_jump(jump);
    }

    // labels named by lines other than jumps, e.g. calls, addresses and
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

    // labels, comments, other sections and removed lines emit no code
    [[nodiscard]] auto next_instruction(size_t index) const -> size_t {
        while (index < lines_.size() and lines_.at(index).code_size == 0) {
            ++index;
        }

        return index;
    }

    [[nodiscard]] auto
    optimize_jump(const size_t index,
                  const std::unordered_map<std::string_view, size_t>& labels,
                  const std::unordered_set<std::string_view>& referenced)
        -> bool {

        line& branch{lines_.at(index)};

        if (not branch.jump) {
            return false;
        }

        // the backend defines the labels it jumps to
        const size_t target{destination(labels, *branch.jump)};

        const size_t target_code{next_instruction(target)};

        // execution continues at the target anyway
        if (target_code == next_instruction(index + 1)) {
            remove(branch);
            ++optimizations_.jumps_to_next;
            return true;
        }

        const std::optional<size_t> jump_index{
            following_unconditional_jump(index, referenced),
        };

        if (not jump_index) {
            return false;
        }

        line& jump{lines_.at(*jump_index)};

        // nothing reaches a jump right after an unconditional jump
        if (not is_conditional(*branch.jump)) {
            remove(jump);
            ++optimizations_.unreachable_jumps;
            return true;
        }

        const size_t jump_target{destination(labels, *jump.jump)};

        // both outcomes continue at the same place
        if (target_code == next_instruction(jump_target)) {
            remove(branch);
            ++optimizations_.same_target_branches;
            return true;
        }

        // branching over the jump is the inverse branch to its target
        if (target_code != next_instruction(*jump_index + 1)) {
            return false;
        }

        invert(branch, jump.jump->target);

        remove(jump);
        ++optimizations_.inverted_branches;

        return true;
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

    //
    // statics
    //

    // comments naming a label only keep it referenced, which is safe
    static auto
    add_named_labels(const std::string_view text,
                     const std::unordered_map<std::string_view, size_t>& labels,
                     std::unordered_set<std::string_view>& named) -> void {

        size_t begin{};
        while (begin < text.size()) {
            if (not is_symbol_char(text.at(begin))) {
                ++begin;
                continue;
            }

            size_t end{begin};
            while (end < text.size() and is_symbol_char(text.at(end))) {
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

    // in the target's unit, removed lines have no size
    [[nodiscard]] static auto code_size_of(const std::span<const line> lines)
        -> size_t {

        size_t size{};
        for (const line& l : lines) {
            size += l.code_size;
        }

        return size;
    }

    [[nodiscard]] static auto
    destination(const std::unordered_map<std::string_view, size_t>& labels,
                const jump_info& jump) -> size_t {

        const auto found{labels.find(jump.target)};

        assert(found != labels.end());

        return found->second;
    }

    // numeric labels, named by references such as '1b', are entries without a
    // lookup
    [[nodiscard]] static auto
    is_enterable(const line& l,
                 const std::unordered_set<std::string_view>& referenced)
        -> bool {

        if (not l.entry) {
            return false;
        }

        // note: only 'add_label' sets 'entry', with a name
        assert(not l.label.empty());

        if (is_numeric(l.label)) {
            return true;
        }

        return referenced.contains(l.label);
    }

    [[nodiscard]] static auto is_symbol_char(const char ch) -> bool {
        return std::isalnum(static_cast<unsigned char>(ch)) != 0 or ch == '_' or
               ch == '.' or ch == '$';
    }

    static auto remove(line& l) -> void {
        l.jump.reset();
        l.code_size = 0;
        l.removed = true;
    }

    [[nodiscard]] static auto
    summary_of(std::vector<function_summary>& summaries,
               const std::string_view function) -> function_summary& {

        for (function_summary& summary : summaries) {
            if (summary.function == function) {
                return summary;
            }
        }

        return summaries.emplace_back(function_summary{
            .function{std::string{function}},
            .body_count{},
            .call_count{},
            .instruction_count{},
        });
    }

    [[nodiscard]] static auto trim(const std::string_view text)
        -> std::string_view {

        const size_t first{text.find_first_not_of(" \t\r")};

        if (first == std::string_view::npos) {
            return {};
        }

        return text.substr(first, text.find_last_not_of(" \t\r") - first + 1);
    }
};
