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
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

// buffers generated assembly with its labels and jumps known so every target
// optimizes jumps the same way, subclasses supply the target's syntax,
// including the data directives and sections, so 'machine' writes the data
// layout once
//
// sections: 'switch_section' starts one of 'section', a subclass writes its
// directive ('add_section_directive') and the rv32i image is indexed by it;
// sizes and entries count only in code
//
// jump optimizations done by 'optimize_jumps', in x86_64 and rv32i syntax:
//
// 'jumps_to_next':
//     jmp if.16.8.code                  j if.16.8.code
//     if.16.8.code:                     if.16.8.code:
//   to
//     if.16.8.code:                     if.16.8.code:
//
// 'unreachable_jumps':
//     jmp loop.10.5.end                 j loop.10.5.end
//     jmp loop.10.5                     j loop.10.5
//   to
//     jmp loop.10.5.end                 j loop.10.5.end
//
// 'same_target_branches':
//     jne bool.15.19.end                beq t0, zero, bool.15.19.end
//     jmp bool.15.19.end                j bool.15.19.end
//   to
//     jmp bool.15.19.end                j bool.15.19.end
//
// 'inverted_branches':
//     jne cmp.19.27                     bne t0, t1, cmp.19.27
//     jmp if.19.8.code                  j if.19.8.code
//     cmp.19.27:                        cmp.19.27:
//   to
//     je if.19.8.code                   beq t0, t1, if.19.8.code
//     cmp.19.27:                        cmp.19.27:
//
// a label between the two jumps of the last three rules stops the rule only
// when it is in code and a line names it, e.g. a jump or a call, or when it
// is numeric, e.g. '1:' named by '1b'
//
// reloads removed by 'remove_reloads', after the jump optimizations; such a
// label in between keeps the load:
//
//     mov qword [rbp + 8], r15          sw t0, -2032(s0)
//     mov r15, qword [rbp + 8]          lw t0, -2032(s0)
//   to
//     mov qword [rbp + 8], r15          sw t0, -2032(s0)
//
// copies removed by 'remove_copies', after 'remove_reloads', on rv32i: the
// reads of a copied register take the source until an instruction writes the
// register, with no such label, jump, call or write of the source in between:
//
//                                       mv t0, a0
//                                       sw t0, -2016(s0)
//                                       addi t0, t0, 1
//   to
//                                       sw a0, -2016(s0)
//                                       addi t0, a0, 1
//
// tests removed by 'remove_retests', after 'remove_copies', on x86_64: the
// branch after a test of what a 'setcc' just stored reads the flags of the
// comparison, with no such label in between:
//
//     sete r15b
//     test r15b, r15b
//     jne if.4.5.end
//   to
//     sete r15b
//     je if.4.5.end
class assembler {
  public:
    // in the order of the rv32i image; the variables are zeroed when defined,
    // so a target need not store them in its image
    enum class section : uint8_t { text, rodata, data, variables };

  protected:
    // a jump the optimizer sees; the target keeps the condition of a branch
    // in the record of its line
    struct jump_info {
        std::string target;
        // a free register for targets that need one to reach far
        std::string scratch;
        bool conditional{};
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

    // changes made by 'optimize'
    struct optimization_counts {
        size_t jumps_to_next{};
        size_t unreachable_jumps{};
        size_t same_target_branches{};
        size_t inverted_branches{};
        size_t removed_reloads{};
        size_t removed_copies{};
        size_t removed_retests{};
    };

    // the bodies of one noinline function, valid once jumps are resolved
    // (on rv32i 'resolve_jumps')
    struct function_summary {
        std::string function;
        size_t body_count{};
        size_t call_count{};
        size_t instruction_count{};
    };

  protected:
    // the structured forms of a target's lines, a variant 'record_t' with an
    // alternative for each kind of line, found by 'line::record'
    template <typename record_t> class line_records {
        std::vector<record_t> records_;

      public:
        // the index for 'line::record'
        [[nodiscard]] auto add(record_t structured) -> size_t {
            records_.push_back(std::move(structured));
            return records_.size() - 1;
        }

        // none for a line without a record
        [[nodiscard]] auto find(const line& l) const -> const record_t* {
            if (not l.record) {
                return nullptr;
            }

            return &records_.at(*l.record);
        }

        // none for a line without a record or with another record
        template <typename alternative_t>
        [[nodiscard]] auto find(const line& l) const -> const alternative_t* {
            return std::get_if<alternative_t>(find(l));
        }

        template <typename alternative_t>
        [[nodiscard]] auto find(const line& l) -> alternative_t* {
            if (not l.record) {
                return nullptr;
            }

            return std::get_if<alternative_t>(&records_.at(*l.record));
        }
    };

  private:
    // the lines from 'first_line' up to 'end_line' are a body of noinline
    // 'function'
    struct body {
        std::string function;
        std::string label;
        size_t first_line{};
        size_t end_line{};
    };

    // the lines from 'first' up to 'end', entered only at the first, so a
    // pass relates an instruction to those executed right before it
    struct straight_run {
        size_t first{};
        size_t end{};
    };

    std::vector<line> lines_;
    // versions being emitted by 'capture', innermost last
    std::vector<captured_lines> captures_;
    bool code_section_{true};
    optimization_counts optimizations_{};
    std::vector<body> bodies_;

  protected:
    static constexpr size_t section_count_{
        std::to_underlying(section::variables) + 1,
    };

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

    virtual auto comment(const size_t indent, const std::string_view text)
        -> void = 0;

    // 'location' is the source the comment is about, as 'file:line:column'
    virtual auto comment(const size_t indent, const std::string_view location,
                         const std::string_view text) -> void = 0;

    virtual auto data(const size_t element_size_bytes,
                      const std::span<const int64_t> values) -> void = 0;

    virtual auto define_constant(const std::string_view name,
                                 const int64_t value) -> void = 0;

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
            lines_, [&](const line& l) -> bool { return l.label == name; });
    }

    auto end_body() -> void {
        assert(captures_.empty());
        assert(not bodies_.empty());

        bodies_.back().end_line = lines_.size();
    }

    // machine instructions, so pseudo instructions count as the ones they
    // expand to, valid once jumps are resolved (on rv32i 'resolve_jumps')
    [[nodiscard]] auto instruction_count() const -> size_t {
        return instructions_in(code_size_of(lines_));
    }

    // the size of captured lines in machine instructions
    [[nodiscard]] auto instruction_count_of(const captured_lines& lines) const
        -> size_t {

        return instructions_in(code_size_of(lines));
    }

    // inside 'capture', e.g. a version or a dry run
    [[nodiscard]] auto is_capturing() const -> bool {
        return not captures_.empty();
    }

    // nothing is buffered
    [[nodiscard]] auto is_empty() const -> bool { return lines_.empty(); }

    auto label(const size_t indent, const std::string_view name) -> void {
        add_text(std::format("{}{}:", indentation(indent), name));

        line& added{current_lines().back()};
        added.label = name;
        added.entry = code_section_;
    }

    // the bodies of each noinline function with their calls and size, in
    // the order their first bodies are emitted
    [[nodiscard]] auto noinline_summaries() const
        -> std::vector<function_summary> {

        assert(captures_.empty());

        std::unordered_map<std::string_view, size_t> calls;
        for (const line& l : lines_) {
            if (not l.call_target.empty()) {
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

    // the jump optimizations unless '--nopt', then 'remove_reloads',
    // 'remove_copies' and 'remove_retests'; none changes a label or names one
    // outside a jump, so the labels and the labels named outside jumps are
    // found once
    auto optimize(const bool is_jumps_optimized) -> void {
        assert(captures_.empty());

        const std::unordered_map<std::string_view, size_t> labels{
            label_lines(),
        };

        const std::unordered_set<std::string_view> named{
            labels_named_outside_jumps(labels),
        };

        if (is_jumps_optimized) {
            optimize_jumps(labels, named);
        }

        const std::unordered_set<std::string_view> referenced{
            referenced_labels(labels, named),
        };

        // note: the passes below only remove and rewrite instructions, so the
        //       entries stay

        const std::vector<straight_run> runs{straight_runs(referenced)};

        // note: a removed reload lets more copies fold, the reverse order
        //       gives larger code

        remove_reloads(runs);
        remove_copies(runs);
        remove_retests(runs);
    }

    // e.g. back to the code after the data of a failure handler
    auto switch_section(const section which) -> void {
        // code sizes and entries only count in code
        code_section_ = which == section::text;
        add_section_directive(which);
    }

  protected:
    // 'text' is the instruction of 'jump' without indentation, 'record' the
    // target's form of a branch
    auto add_jump(const size_t indent, const std::string_view text,
                  jump_info jump,
                  const std::optional<size_t> record = std::nullopt) -> void {

        assert(code_section_);

        add_line(std::format("{}{}", indentation(indent), text),
                 instruction_size(), record);

        current_lines().back().jump =
            std::make_unique<jump_info>(std::move(jump));
    }

    auto add_line(std::string text, const size_t code_size,
                  const std::optional<size_t> record = std::nullopt) -> void {

        assert(code_section_ or code_size == 0);

        current_lines().push_back({
            .text{std::move(text)},
            .label{},
            .call_target{},
            .jump{},
            .code_size{code_size},
            .entry{},
            .removed{},
            .record{record},
        });
    }

    // a line that emits no code, e.g. a comment or a directive
    auto add_text(std::string text) -> void { add_line(std::move(text), 0); }

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

    // the line of the instruction added last, none when a label is after it
    [[nodiscard]] auto last_instruction() -> line* {
        for (line& l : std::views::reverse(current_lines())) {
            if (not l.label.empty()) {
                return nullptr;
            }

            if (l.code_size != 0) {
                return &l;
            }
        }

        return nullptr;
    }

    [[nodiscard]] auto lines() -> std::vector<line>& { return lines_; }

    [[nodiscard]] auto lines() const -> const std::vector<line>& {
        return lines_;
    }

    // the line just added calls the label 'target'
    auto mark_call(const std::string_view target) -> void {
        current_lines().back().call_target = target;
    }

    //
    // statics
    //

    // e.g. '.word 1, 2'
    [[nodiscard]] static auto data_text(const std::string_view directive,
                                        const std::span<const int64_t> values)
        -> std::string {

        std::string text{directive};
        std::string_view separator{" "};
        for (const int64_t value : values) {
            text += std::format("{}{}", separator, value);
            separator = ", ";
        }

        return text;
    }

    // the line of the label 'jump' goes to; the backend defines the labels
    // it jumps to
    [[nodiscard]] static auto
    destination(const std::unordered_map<std::string_view, size_t>& labels,
                const jump_info& jump) -> size_t {

        const auto found{labels.find(jump.target)};

        assert(found != labels.end());

        return found->second;
    }

    // 'bytes' between the quotes of a string directive: a backslash before
    // a backslash and each of 'quote_chars', a line end or a tab by name, a
    // printable character as it is and another byte by 'numeric_escape', a
    // format of the byte value, e.g. '\x{:02x}'
    [[nodiscard]] static auto
    escaped_text(const std::string_view bytes,
                 const std::string_view quote_chars,
                 const std::string_view numeric_escape) -> std::string {

        std::string text;

        for (const char c : bytes) {
            if (c == '\\' or quote_chars.contains(c)) {
                text += '\\';
                text += c;
                continue;
            }

            if (const std::optional<std::string_view> named{named_escape(c)}) {
                text += *named;
                continue;
            }

            if (c >= ' ' and c <= '~') {
                text += c;
                continue;
            }

            const unsigned int byte{static_cast<unsigned char>(c)};

            text += std::vformat(numeric_escape, std::make_format_args(byte));
        }

        return text;
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

    [[nodiscard]] static auto is_symbol_char(const char ch) -> bool {
        return std::isalnum(static_cast<unsigned char>(ch)) != 0 or ch == '_' or
               ch == '.' or ch == '$';
    }

    [[nodiscard]] static auto leading_whitespace(const std::string_view text)
        -> std::string_view {

        return text.substr(0, text.find_first_not_of(" \t"));
    }

    static auto remove(line& l) -> void {
        l.jump.reset();
        l.code_size = 0;
        l.removed = true;
    }

    // the instruction of 'l' changed to 'text', the indentation kept
    static auto replace_text(line& l, const std::string_view text) -> void {
        l.text = std::format("{}{}", leading_whitespace(l.text), text);
    }

  private:
    //
    // virtual methods
    //

    // the line that starts 'which'
    virtual auto add_section_directive(const section which) -> void = 0;

    // folds the copy that starts 'block' into the lines after it, see above
    // the class; false when the first line is no copy or the copy is kept
    virtual auto fold_copy(std::span<line> block) -> bool = 0;

    // the code size of one instruction in the target's unit
    [[nodiscard]] virtual auto instruction_size() const -> size_t = 0;

    // rewrites the branch 'l' as the branch taken exactly when it is not, to
    // the target of its jump
    virtual auto invert_branch(line& l) -> void = 0;

    // 'load' leaves every register as it is after 'store', the instruction
    // before it
    [[nodiscard]] virtual auto is_reload(const line& store,
                                         const line& load) const -> bool = 0;

    // rewrites 'branch' to read the flags that 'set' left when 'test' only
    // tests the value 'set' stored, false when it does not
    [[nodiscard]] virtual auto retest_branch(const line& set, const line& test,
                                             line& branch) -> bool = 0;

    //
    // class methods
    //

    [[nodiscard]] auto current_lines() -> std::vector<line>& {
        if (captures_.empty()) {
            return lines_;
        }

        return captures_.back();
    }

    // the unconditional jump right after line 'index' in its run, which ends
    // at 'run_end'; an entry in between would let execution enter between
    // the two jumps
    [[nodiscard]] auto following_unconditional_jump(const size_t index,
                                                    const size_t run_end) const
        -> std::optional<size_t> {

        const size_t next{next_instruction(index + 1)};

        // the run or the code ends, e.g. with the jump of a loop that never
        // ends
        if (next >= run_end) {
            return std::nullopt;
        }

        const line& l{lines_.at(next)};

        if (not l.jump or l.jump->conditional) {
            return std::nullopt;
        }

        return next;
    }

    [[nodiscard]] auto instructions_in(const size_t code_size) const -> size_t {
        return code_size / instruction_size();
    }

    // every branch the optimizer sees has an inverse; the branch takes the
    // removed jump's scratch, chosen after the compared registers were freed
    auto invert(line& l, const jump_info& removed) -> void {
        jump_info& jump{*l.jump};

        jump.target = removed.target;
        jump.scratch = removed.scratch;
        invert_branch(l);
    }

    // labels named by lines other than jumps, e.g. calls, addresses and
    // '.globl', these keep their references while jumps are optimized
    [[nodiscard]] auto labels_named_outside_jumps(
        const std::unordered_map<std::string_view, size_t>& labels) const
        -> std::unordered_set<std::string_view> {

        std::unordered_set<std::string_view> named;
        for (const line& l : lines_) {
            if (l.jump or not l.label.empty()) {
                continue;
            }

            add_named_labels(l.text, labels, named);
        }

        return named;
    }

    [[nodiscard]] auto lines_of(const straight_run run) -> std::span<line> {
        return std::span{lines_}.subspan(run.first, run.end - run.first);
    }

    // labels, comments, other sections and removed lines emit no code
    [[nodiscard]] auto next_instruction(size_t index) const -> size_t {
        while (index < lines_.size() and lines_.at(index).code_size == 0) {
            ++index;
        }

        return index;
    }

    // 'run_end' is the end of the run of line 'index'
    [[nodiscard]] auto
    optimize_jump(const size_t index, const size_t run_end,
                  const std::unordered_map<std::string_view, size_t>& labels)
        -> bool {

        line& branch{lines_.at(index)};

        if (not branch.jump) {
            return false;
        }

        const size_t target{destination(labels, *branch.jump)};

        const size_t target_code{next_instruction(target)};

        // execution continues at the target anyway
        if (target_code == next_instruction(index + 1)) {
            remove(branch);
            ++optimizations_.jumps_to_next;
            return true;
        }

        const std::optional<size_t> jump_index{
            following_unconditional_jump(index, run_end),
        };

        if (not jump_index) {
            return false;
        }

        line& jump{lines_.at(*jump_index)};

        // nothing reaches a jump right after an unconditional jump
        if (not branch.jump->conditional) {
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

        invert(branch, *jump.jump);

        remove(jump);
        ++optimizations_.inverted_branches;

        return true;
    }

    // removes jumps that change nothing and turns a branch over a jump into
    // the inverse branch, repeating because each change can enable another
    auto
    optimize_jumps(const std::unordered_map<std::string_view, size_t>& labels,
                   const std::unordered_set<std::string_view>& named) -> void {

        bool changed{true};
        while (changed) {
            changed = false;

            // removed jumps no longer reference their targets
            const std::unordered_set<std::string_view> referenced{
                referenced_labels(labels, named),
            };

            for (const straight_run run : straight_runs(referenced)) {
                for (size_t index{run.first}; index < run.end; ++index) {
                    changed = optimize_jump(index, run.end, labels) or changed;
                }
            }
        }
    }

    // a jump target is viewed in its label line because inverting a branch
    // replaces its target string
    [[nodiscard]] auto referenced_labels(
        const std::unordered_map<std::string_view, size_t>& labels,
        const std::unordered_set<std::string_view>& named) const
        -> std::unordered_set<std::string_view> {

        std::unordered_set<std::string_view> referenced{named};
        for (const line& l : lines_) {
            if (not l.jump) {
                continue;
            }

            referenced.insert(lines_.at(destination(labels, *l.jump)).label);
        }

        return referenced;
    }

    // removes a copy whose reads can take its source, see above the class
    auto remove_copies(const std::span<const straight_run> runs) -> void {
        for (const straight_run run : runs) {
            const std::span<line> run_lines{lines_of(run)};

            for (size_t first{run_lines.size()}; first > 0; --first) {
                const std::span<line> block{run_lines.subspan(first - 1)};

                if (block.front().code_size == 0) {
                    continue;
                }

                if (fold_copy(block)) {
                    ++optimizations_.removed_copies;
                }
            }
        }
    }

    // removes a load that reads back into the register what the instruction
    // before it stored, see above the class
    auto remove_reloads(const std::span<const straight_run> runs) -> void {
        for (const straight_run run : runs) {
            // the instruction executed right before 'l'
            const line* previous{};
            for (line& l : lines_of(run)) {
                if (l.code_size == 0) {
                    continue;
                }

                if (previous != nullptr and is_reload(*previous, l)) {
                    remove(l);
                    ++optimizations_.removed_reloads;
                    continue;
                }

                previous = &l;
            }
        }
    }

    // removes a test of the value stored by the instruction before it, see
    // above the class
    auto remove_retests(const std::span<const straight_run> runs) -> void {
        for (const straight_run run : runs) {
            // the two instructions executed right before 'l'
            line* set{};
            line* test{};
            for (line& l : lines_of(run)) {
                if (l.code_size == 0) {
                    continue;
                }

                const bool is_branch{l.jump and l.jump->conditional};

                if (set != nullptr and is_branch and
                    retest_branch(*set, *test, l)) {

                    remove(*test);
                    ++optimizations_.removed_retests;

                    // note: the next instructions see 'set' before 'l'
                    test = set;
                }

                set = test;
                test = &l;
            }
        }
    }

    // the lines split at each entry
    [[nodiscard]] auto
    straight_runs(const std::unordered_set<std::string_view>& referenced) const
        -> std::vector<straight_run> {

        std::vector<straight_run> runs;

        size_t first{};
        for (size_t index{1}; index < lines_.size(); ++index) {
            if (is_enterable(lines_.at(index), referenced)) {
                runs.push_back({
                    .first{first},
                    .end{index},
                });

                first = index;
            }
        }

        runs.push_back({
            .first{first},
            .end{lines_.size()},
        });

        return runs;
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

    // numeric labels, named by references such as '1b', are entries without a
    // lookup
    [[nodiscard]] static auto
    is_enterable(const line& l,
                 const std::unordered_set<std::string_view>& referenced)
        -> bool {

        if (not l.entry) {
            return false;
        }

        // note: only 'label' sets 'entry', with a name
        assert(not l.label.empty());

        if (is_numeric(l.label)) {
            return true;
        }

        return referenced.contains(l.label);
    }

    // the escape of a line end or a tab, which reads better than a numeric
    // escape
    [[nodiscard]] static auto named_escape(const char ch)
        -> std::optional<std::string_view> {

        switch (ch) {
        case '\n':
            return "\\n";

        case '\r':
            return "\\r";

        case '\t':
            return "\\t";

        default:
            return std::nullopt;
        }
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
};
