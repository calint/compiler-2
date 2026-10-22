#include <iostream>
#include <sstream>
#include <stdexcept>

#include "../../src/assembler_rv32i.hpp"
#include "../../src/decouple_impl.hpp" // IWYU pragma: keep
#include "../../src/expr_type_impl.hpp" // IWYU pragma: keep
#include "../../src/machine_rv32i.hpp"
#include "../../src/machine_x86_64.hpp"
#include "../../src/program.hpp"
#include "../../src/register_report.hpp" // IWYU pragma: keep

// ============================================================================
// test driver of the RV32I backend ('test-rv32i.sh' builds and runs it)
//
// The driver calls the backend API directly, so it also covers calls that no
// .baz source can produce. It runs in one of these ways, chosen by the first
// command line argument of 'main' at the end of the file:
//
//   (none)     the host checks (parts 1 to 8 and 12) assert on the emitted
//              text, then the runtime program (part 11) is printed on stdout,
//              which the script assembles and runs under QEMU; a wrong result
//              jumps to the label 'failure'
//   <mode>     print one special program on stdout instead (parts 9 and 10):
//              'noninline', 'frame-checks', 'long-loop', 'far-jumps',
//              'far-jumps-optimized', 'far-foo', 'far-foo-optimized' and
//              'bounds-matrix'
//
// What a .baz program can show is a case of 'cases.sh' instead, e.g. 1108 to
// 1111 were programs of this driver.
//
// Every check is a function named after the behavior it shows. Checks that
// differ only in data are rows of a table.
//
// parts
//   1  the assembler: jump optimizer and jump resolution
//   2  the machine: instruction selection and diagnostics
//   3  compiled .baz programs inspected as assembly text
//   4  the x86_64 backend
//   5  instruction shapes of the RV32I backend
//   6  arithmetic with no scratch register free
//   7  copies, comparisons, addresses and zeroing
//   8  helper routines for multiplication and division
//   9  special programs with hand-written lines between the backend's output
//   10 the bounds check for every combination of its parameters
//   11 the runtime program
//   12 the front end tables: scopes and storage
// ============================================================================

// instruction-shape tests must not depend on register diagnostic comments
class assembly_output final : public std::ostringstream {
  public:
    using std::ostringstream::str;

    auto str() const -> std::string {
        std::istringstream input{std::ostringstream::str()};
        std::string result;
        std::string line;
        while (std::getline(input, line)) {
            const size_t start{line.find_first_not_of(" \t")};
            if (start != std::string::npos and line[start] == '#' and
                (line.contains("allocate scratch register") or
                 line.contains("allocate named register") or
                 line.contains("free scratch register") or
                 line.contains("free named register"))) {
                continue;
            }
            result += line + '\n';
        }

        return result;
    }
};

namespace {

// ----------------------------------------------------------------------------
// shared fixtures
// ----------------------------------------------------------------------------

// the operator of the machine for an operator written as a character
auto op_of(const char op) -> machine::arithmetic_operator {
    using arithmetic_operator = machine::arithmetic_operator;

    if (op == '+') {
        return arithmetic_operator::add;
    }
    if (op == '-') {
        return arithmetic_operator::subtract;
    }
    if (op == '&') {
        return arithmetic_operator::bit_and;
    }
    if (op == '|') {
        return arithmetic_operator::bit_or;
    }
    if (op == '^') {
        return arithmetic_operator::bit_xor;
    }
    if (op == '/') {
        return arithmetic_operator::divide;
    }
    if (op == '%') {
        return arithmetic_operator::remainder;
    }
    if (op == '<') {
        return arithmetic_operator::shift_left;
    }

    assert(op == '>');

    return arithmetic_operator::shift_right;
}

auto unary_of(const char op) -> machine::arithmetic_operator {
    return op == '-' ? machine::arithmetic_operator::negate
                     : machine::arithmetic_operator::complement;
}

// the built-in types the checks give the backend and the front end
const builtin_type_set test_builtins;
const type& integer64{test_builtins.i64};
const type& integer{test_builtins.i32};
const type& half{test_builtins.i16};
const type& byte{test_builtins.i8};
const type& boolean{test_builtins.boolean};

// the diagnostics of the rejected calls need a real position
const token source_tk{token::position(0, 0, 1)};

// the variables section of the compiled .baz programs
constexpr size_t vars_capacity_bytes{4096};

// every scratch register of an RV32I backend that reserves no base register
constexpr size_t rv32i_scratch_count{30};

// whether 'action' is rejected with a compiler error that contains 'text'
template <typename action_t>
auto rejected_with(const action_t& action, const std::string_view text = {})
    -> bool {
    try {
        action();
    } catch (const compile_error& error) {
        return std::string_view{error.msg}.contains(text);
    }

    return false;
}

// an RV32I backend whose output is captured, for checks of emitted text
struct captured_rv32i {
    assembly_output output;
    std::ostream stream{output.rdbuf()};
    machine_rv32i_linux backend{&stream, test_builtins};

    // the text that 'emit' adds, the output is empty again afterwards
    auto emitted(const auto& emit) -> std::string {
        output.str({});
        emit();
        std::string text{output.str()};
        output.str({});

        return text;
    }
};

// an x86_64 backend whose output is captured
struct captured_x86_64 {
    std::ostringstream output;
    machine_x86_64 backend{&output, test_builtins, {}};
};

// allocates scratch registers so that a check proves it needs no more
auto hold_scratch_registers(machine& backend, const size_t count,
                            const type& value_type) -> std::vector<operand> {
    std::vector<operand> held;
    for (size_t i{}; i < count; ++i) {
        held.push_back(backend.alloc_scratch_register(token{}, 0, value_type));
    }

    return held;
}

// an address of a bulk operation that is 'address' whatever the backend
// offers
auto fixed_address(const operand& address) -> machine::address_emitter {
    return [address](const operand&, const operand&,
                     const machine::address_use use) -> void { use(address); };
}

// the assembly of a .baz program compiled for RV32I
auto compiled_rv32i(const std::string_view source) -> std::string {
    std::ostringstream output;
    source_files files;
    files.add("test.baz", std::string{source});

    machine_rv32i_linux compiler{&output, test_builtins};
    program prg{compiler, files, vars_capacity_bytes, check_options{}};
    prg.build(output);

    return output.str();
}

// the assembly of a .baz program compiled for x86_64
auto compiled_x86_64(const std::string_view source) -> std::string {
    std::ostringstream output;
    source_files files;
    files.add("test.baz", std::string{source});

    machine_x86_64 compiler{&output, test_builtins, &files};
    program prg{compiler, files, vars_capacity_bytes, check_options{}};
    prg.build(output);

    return output.str();
}

// the RV32I store of a value of 'value_type'
auto store_of(const type& value_type) -> std::string_view {
    if (&value_type == &integer) {
        return "sw";
    }

    return &value_type == &half ? "sh" : "sb";
}

// the RV32I load of a value of 'value_type', sign extending
auto load_of(const type& value_type) -> std::string_view {
    if (&value_type == &integer) {
        return "lw";
    }

    return &value_type == &half ? "lh" : "lb";
}

// what a register holds for the value 'bits' of 'value_type': the low bits
// of the width, sign extended unless it is a 'bool'
auto wrapped(const uint32_t bits, const type& value_type) -> int32_t {
    const size_t width{value_type.size_bytes() * 8};
    const uint32_t mask{UINT32_MAX >> (32 - width)};
    uint32_t value{bits & mask};

    const bool is_negative{(value & (uint32_t{1} << (width - 1))) != 0};

    if (&value_type != &boolean and is_negative) {
        value |= ~mask;
    }

    return std::bit_cast<int32_t>(value);
}

// ----------------------------------------------------------------------------
// helpers of the jump checks
// ----------------------------------------------------------------------------

// the branches that 'resolved_branch' grows
auto branch_op(const std::string_view mnemonic) -> assembler_rv32i::op {
    using op = assembler_rv32i::op;
    constexpr std::array<std::pair<std::string_view, op>, 6> branches{{
        {"beq", op::beq},
        {"bne", op::bne},
        {"blt", op::blt},
        {"bge", op::bge},
        {"bltu", op::bltu},
        {"bgeu", op::bgeu},
    }};

    for (const auto& [name, code] : branches) {
        if (name == mnemonic) {
            return code;
        }
    }

    std::unreachable();
}

// 'a0, a1' as 'a0' and 'a1'
auto split_operands(const std::string_view operands)
    -> std::vector<std::string_view> {
    std::vector<std::string_view> result;
    for (const auto part :
         operands | std::views::split(std::string_view{", "})) {
        result.emplace_back(part);
    }

    return result;
}

// a jump the assembler grows, 'operands' are empty for 'j'
auto add_jump(assembler_rv32i& assembler, const std::string_view mnemonic,
              const std::string_view operands, const std::string_view target,
              const std::string_view scratch) -> void {
    if (mnemonic == "j") {
        assembler.resolved_jump(0, target, scratch);

        return;
    }

    const std::vector<std::string_view> registers{split_operands(operands)};

    assembler.resolved_branch(0, branch_op(mnemonic), registers.at(0),
                              registers.at(1), target, scratch);
}

// 'count' instructions of 4 bytes, as text and emitted
auto padding_text(const size_t count) -> std::string {
    std::string text;
    for (size_t i{}; i < count; ++i) {
        text += "addi a0, a0, 1\n";
    }

    return text;
}

auto add_padding(assembler_rv32i& assembler, const size_t count) -> void {
    for (size_t i{}; i < count; ++i) {
        assembler.addi(0, "a0", "a0", 1);
    }
}

// the text after the jumps are resolved to their sizes
auto resolved_text(assembler_rv32i& assembler) -> std::string {
    std::ostringstream output;
    assembler.resolve_jumps();
    assembler.write_resolved(output);

    return output.str();
}

// a jump to 'end' over 'count' instructions
auto forward_jump(const std::string_view mnemonic,
                  const std::string_view operands, const size_t count,
                  const std::string_view scratch) -> std::string {
    assembler_rv32i assembler;
    add_jump(assembler, mnemonic, operands, "end", scratch);
    add_padding(assembler, count);
    assembler.label(0, "end");

    return resolved_text(assembler);
}

// a jump back to 'end' over 'count' instructions
auto backward_jump(const std::string_view mnemonic,
                   const std::string_view operands, const size_t count)
    -> std::string {
    assembler_rv32i assembler;
    assembler.label(0, "end");
    add_padding(assembler, count);
    add_jump(assembler, mnemonic, operands, "end", "t0");

    return resolved_text(assembler);
}

// what 'optimize_jumps' leaves of RV32I text: 'name:' lines are labels, '# '
// lines comments, 'j' and the branches jumps, 'addi', 'ecall' and 'call'
// instructions
auto optimized(const std::string_view assembly) -> std::string {
    assembler_rv32i assembler;
    for (const auto part : assembly | std::views::split('\n')) {
        const std::string_view text{part};
        const size_t start{text.find_first_not_of(' ')};
        if (start == std::string_view::npos) {
            continue;
        }
        const size_t indent{start / 4};
        const std::string_view code{text.substr(start)};

        if (code.starts_with("# ")) {
            assembler.comment(indent, code.substr(2));
            continue;
        }

        if (code.back() == ':') {
            assembler.label(indent, code.substr(0, code.size() - 1));
            continue;
        }

        const size_t split{code.find(' ')};
        const std::string_view mnemonic{code.substr(0, split)};
        if (mnemonic == "ecall") {
            assembler.ecall(indent);
            continue;
        }

        const std::vector<std::string_view> operands{
            split_operands(code.substr(split + 1)),
        };

        if (mnemonic == "addi") {
            assert(operands.at(2) == "1");
            assembler.addi(indent, operands.at(0), operands.at(1), 1);
            continue;
        }

        if (mnemonic == "call") {
            assembler.call(indent, operands.at(0));
            continue;
        }

        if (mnemonic == "j") {
            assembler.resolved_jump(indent, operands.at(0), "t0");
            continue;
        }

        assembler.resolved_branch(indent, branch_op(mnemonic), operands.at(0),
                                  operands.at(1), operands.at(2), "t0");
    }

    // real output ends with an instruction, which is no jump
    assembler.ecall(0);
    assembler.optimize_jumps();

    std::string text{resolved_text(assembler)};
    constexpr std::string_view end_instruction{"ecall\n"};
    assert(text.ends_with(end_instruction));
    text.resize(text.size() - end_instruction.size());

    return text;
}

// ============================================================================
// 1. host checks: the assembler (jump optimizer and jump resolution)
// ============================================================================

// a text and what the jump optimizer makes of it
struct rewrite {
    std::string_view input;
    std::string_view expected;
};

// the jumps 'optimize_jumps' removes or inverts
auto check_removed_and_inverted_jumps() -> void {
    constexpr std::array rewrites{
        // a jump to the next instruction is removed
        rewrite{
            .input{"    j cmp_13_26\n    cmp_13_26:\n"},
            .expected{"    cmp_13_26:\n"},
        },
        // a branch and a jump to the label right after them are both removed
        rewrite{
            .input{"    bne a0, a1, bool_end_15_9\n    j bool_end_15_9\n"
                   "    bool_end_15_9:\n"},
            .expected{"    bool_end_15_9:\n"},
        },
        // a branch over a jump becomes the inverted branch to the jump target
        rewrite{
            .input{"    bne a0, a1, cmp_14_26\n    j if_14_8_code\n"
                   "    cmp_14_26:\n    ecall\n    if_14_8_code:\n"},
            .expected{"    beq a0, a1, if_14_8_code\n    cmp_14_26:\n"
                      "    ecall\n    if_14_8_code:\n"},
        },
        // numeric labels are entries and jumps to the next instruction go
        rewrite{
            .input{"    j done\n    1:\n    j done\n    done:\n"},
            .expected{"    1:\n    done:\n"},
        },
        // the same inversion without indentation
        rewrite{
            .input{"beq a0, a1, skip\nj end\nskip:\naddi a0, a0, 1\nend:\n"},
            .expected{"bne a0, a1, end\nskip:\naddi a0, a0, 1\nend:\n"},
        },
        // comments between a jump and its target do not keep the jump
        rewrite{
            .input{"top:\naddi a0, a0, 1\nbne a0, zero, top\nj next\n"
                   "# next\nnext:\n"},
            .expected{"top:\naddi a0, a0, 1\nbne a0, zero, top\n# next\n"
                      "next:\n"},
        },
        // both outcomes continue at the same place
        rewrite{
            .input{"beq a0, zero, end\nj end\naddi a0, a0, 1\nend:\n"},
            .expected{"j end\naddi a0, a0, 1\nend:\n"},
        },
        // nothing reaches a jump right after another
        rewrite{
            .input{"j end\nj other\naddi a0, a0, 1\nother:\necall\nend:\n"},
            .expected{"j end\naddi a0, a0, 1\nother:\necall\nend:\n"},
        },
        // a backward jump after a branch to the next label also inverts
        rewrite{
            .input{"top:\naddi a0, a0, 1\nbeq a0, zero, done\nj top\n"
                   "done:\n"},
            .expected{"top:\naddi a0, a0, 1\nbne a0, zero, top\ndone:\n"},
        },
        // a label nothing names cannot be entered
        rewrite{
            .input{"beq a0, zero, skip\nentry:\nj end\nskip:\necall\nend:\n"},
            .expected{"bne a0, zero, end\nentry:\nskip:\necall\nend:\n"},
        },
    };

    for (const rewrite& r : rewrites) {
        assert(optimized(r.input) == r.expected);
    }
}

// every branch kind inverts to its pair with the operands in order, also with
// a comment between the branch and the jump, and the result is stable
auto check_every_branch_inverts() -> void {
    constexpr std::array<std::pair<std::string_view, std::string_view>, 3>
        inverse_pairs{{
            {"beq", "bne"},
            {"blt", "bge"},
            {"bltu", "bgeu"},
        }};

    for (const auto& [first, second] : inverse_pairs) {
        for (const bool reverse : {false, true}) {
            const std::string_view mnemonic{reverse ? second : first};
            const std::string_view inverted{reverse ? first : second};

            const std::string input{
                std::format("{} a0, a1, skip\n# keep\nj done\nskip:\n"
                            "addi a0, a0, 1\ndone:\n",
                            mnemonic),
            };

            const std::string expected{
                std::format("{} a0, a1, done\n# keep\nskip:\n"
                            "addi a0, a0, 1\ndone:\n",
                            inverted),
            };

            assert(optimized(input) == expected);
            assert(optimized(expected) == expected);
        }
    }
}

// a label between the jumps lets execution enter, so the jumps stay
auto check_jumps_kept_at_entries() -> void {
    for (const std::string_view unchanged :
         {"top:\nj top\n", "call end\nend:\n",
          "call entry\nbeq a0, zero, skip\nentry:\nj end\nskip:\necall\n"
          "end:\n",
          "beq a0, zero, skip\n1:\nj end\nskip:\necall\nend:\n"}) {
        assert(optimized(unchanged) == unchanged);
    }
}

// the folding ignores reach, a branch to a far target grows again when it is
// resolved
auto check_far_branch_folds_and_grows() -> void {
    // note: 1024 instructions put 'end' beyond the 4 KiB a branch reaches
    const std::string distant{
        "beq a0, zero, skip\nj end\nskip:\n" + padding_text(1024) + "end:\n",
    };

    assert(optimized(distant) ==
           "beq a0, zero, .Lbaz_jump.0\nj end\n.Lbaz_jump.0:\n" +
               distant.substr(distant.find("skip:")));
}

// code continues across data placed in another section, so a branch over the
// data still inverts
auto check_jumps_across_data() -> void {
    assembler_rv32i assembler;
    add_jump(assembler, "beq", "a0, a1", "skip", "t0");
    assembler.switch_section(assembler_rv32i::section::data);
    const std::array<int64_t, 1> word{1};
    assembler.data(4, word);
    assembler.switch_section(assembler_rv32i::section::text);
    add_jump(assembler, "j", {}, "end", "t0");
    assembler.label(0, "skip");
    assembler.ecall(0);
    assembler.label(0, "end");
    assembler.optimize_jumps();

    assert(resolved_text(assembler) == "bne a0, a1, end\n.data\n.word 1\n"
                                       ".text\nskip:\necall\nend:\n");
}

// a branch reaches 4094 bytes forward and 4096 bytes backward, beyond that it
// inverts around a 'j'
auto check_branch_reach() -> void {
    // note: 1022 instructions of 4 bytes after the branch end 4092 bytes on
    assert(forward_jump("beq", "a0, a1", 1022, "t0") ==
           "beq a0, a1, end\n" + padding_text(1022) + "end:\n");

    assert(forward_jump("beq", "a0, a1", 1023, "t0") ==
           "bne a0, a1, .Lbaz_jump.0\nj end\n.Lbaz_jump.0:\n" +
               padding_text(1023) + "end:\n");

    assert(backward_jump("beq", "a0, a1", 1024) ==
           "end:\n" + padding_text(1024) + "beq a0, a1, end\n");

    assert(backward_jump("beq", "a0, a1", 1025) ==
           "end:\n" + padding_text(1025) +
               "bne a0, a1, .Lbaz_jump.0\nj end\n.Lbaz_jump.0:\n");
}

// 'jal' reaches 1048574 bytes forward and 1048576 bytes backward, beyond that
// a 'jump' through the scratch register
auto check_jump_reach() -> void {
    assert(forward_jump("j", {}, 262142, "t0") ==
           "j end\n" + padding_text(262142) + "end:\n");

    assert(forward_jump("j", {}, 262143, "t0") ==
           "jump end, t0\n" + padding_text(262143) + "end:\n");

    assert(backward_jump("j", {}, 262144) ==
           "end:\n" + padding_text(262144) + "j end\n");

    assert(backward_jump("j", {}, 262145) ==
           "end:\n" + padding_text(262145) + "jump end, t0\n");

    // a branch beyond 'jal' reach inverts around the long jump
    assert(forward_jump("bne", "a0, zero", 262143, "t1") ==
           "beq a0, zero, .Lbaz_jump.0\njump end, t1\n.Lbaz_jump.0:\n" +
               padding_text(262143) + "end:\n");
}

// a far jump without a scratch register cannot be built
auto check_far_jump_needs_scratch() -> void {
    bool rejected{};

    try {
        std::ignore = forward_jump("j", {}, 262143, {});
    } catch (const std::runtime_error&) {
        rejected = true;
    }

    assert(rejected);
}

// growing the inner branch pushes the outer one out of reach
auto check_growth_moves_other_jumps() -> void {
    assembler_rv32i assembler;
    add_jump(assembler, "beq", "a0, a1", "end", "t0");
    add_padding(assembler, 1021);
    add_jump(assembler, "beq", "a0, a1", "far", "t0");
    assembler.label(0, "end");
    add_padding(assembler, 1100);
    assembler.label(0, "far");

    const std::string chained{resolved_text(assembler)};

    assert(chained.starts_with("bne a0, a1, .Lbaz_jump.0\nj end\n"));
    assert(chained.contains("bne a0, a1, .Lbaz_jump.1\nj far\n"));
}

// other sections do not count towards code offsets
auto check_data_takes_no_code_offset() -> void {
    // note: 2000 words of data would be beyond a branch's reach as code
    constexpr size_t word_count{2000};

    assembler_rv32i assembler;
    add_jump(assembler, "beq", "a0, a1", "end", "t0");
    assembler.switch_section(assembler_rv32i::section::data);
    const std::array<int64_t, 1> word{1};
    for (size_t i{}; i < word_count; ++i) {
        assembler.data(4, word);
    }
    assembler.switch_section(assembler_rv32i::section::text);
    assembler.label(0, "end");

    std::string words;
    for (size_t i{}; i < word_count; ++i) {
        words += ".word 1\n";
    }

    assert(resolved_text(assembler) ==
           "beq a0, a1, end\n.data\n" + words + ".text\nend:\n");
}

// the smaller of two captured versions, the first on ties, added to
// 'assembler' as 'machine::emit_most_efficient' adds it
auto emit_smaller(assembler_rv32i& assembler,
                  const std::function_ref<void()> emit_first,
                  const std::function_ref<void()> emit_second) -> void {

    assembler::captured_lines first{assembler.capture(emit_first)};
    assembler::captured_lines second{assembler.capture(emit_second)};

    const bool keeps_first{
        assembler.instruction_count_of(first) <=
            assembler.instruction_count_of(second),
    };

    assembler.append(keeps_first ? std::move(first) : std::move(second));
}

// the smaller version is kept, a tie keeps the first
auto check_smaller_version_kept() -> void {
    assembler_rv32i assembler;

    emit_smaller(assembler, [&] { add_padding(assembler, 2); },
                 [&] { assembler.addi(0, "a0", "a0", 1); });

    emit_smaller(assembler, [&] { assembler.sw(0, "a0", 0, "sp"); },
                 [&] { add_padding(assembler, 1); });

    assert(resolved_text(assembler) == "addi a0, a0, 1\nsw a0, 0(sp)\n");
}

// a nested choice lands inside the version that contains it
auto check_nested_version_choice() -> void {
    assembler_rv32i assembler;

    emit_smaller(
        assembler,
        [&] {
            assembler.lw(0, "a0", 0, "sp");
            emit_smaller(assembler, [&] { add_padding(assembler, 2); },
                         [&] { assembler.addi(0, "a0", "a0", 1); });
        },
        [&] { add_padding(assembler, 3); });

    assert(resolved_text(assembler) == "lw a0, 0(sp)\naddi a0, a0, 1\n");
}

// a jump inside a kept version still grows
auto check_jump_in_kept_version_grows() -> void {
    assembler_rv32i assembler;

    emit_smaller(
        assembler, [&] { add_jump(assembler, "beq", "a0, a1", "end", "t0"); },
        [&] { add_padding(assembler, 2); });

    add_padding(assembler, 1100);
    assembler.label(0, "end");

    assert(resolved_text(assembler) ==
           "bne a0, a1, .Lbaz_jump.0\nj end\n.Lbaz_jump.0:\n" +
               padding_text(1100) + "end:\n");
}

// ============================================================================
// 2. host checks: the machine (instruction selection and diagnostics)
// ============================================================================

// the byte size 'line_size_bytes' assigns to a line of assembly text, which
// the jump resolution relies on
auto check_line_sizes() -> void {
    // jumps start as one instruction and grow when resolved
    for (const std::string_view jump :
         {"\tj done  # jump", "\tbeq a0, a1, done"}) {
        assert(assembler_rv32i::line_size_bytes(jump) == 4);
    }

    // comments and directives take no space
    for (const std::string_view sizeless :
         {"  # comment", "\t.option norelax"}) {
        assert(assembler_rv32i::line_size_bytes(sizeless) == 0);
    }
}

// 'emit_most_efficient' on versions of equal size keeps the version without
// scratch, in every jump mode
auto check_equal_size_choice() -> void {
    for (const assembler::jump_mode jumps :
         {assembler::jump_mode::resolved, assembler::jump_mode::optimized}) {
        std::ostringstream output;
        machine_rv32i_linux backend{&output, test_builtins, {}, jumps};
        backend.start();

        const operand result{operand::reg("a0", integer)};

        const auto load = [&](const int64_t value) -> void {
            backend.copy_value(token{}, 0, result,
                               operand::imm(value, integer));
        };

        const auto copy = [&] {
            backend.copy_value(token{}, 0, result, operand::reg("a1", integer));
        };

        // note: 2047 is one 'li', 2048 does not fit the 12-bit immediate
        backend.emit_most_efficient(token{}, 0, [&] { load(2047); }, copy);
        backend.emit_most_efficient(token{}, 0, [&] { load(2048); }, copy);

        backend.finish();
        backend.write_assembly(output);

        // note: each choice is commented with the sizes of both versions
        assert(output.str().contains(
            "# smaller of two versions of 1 and 1 instructions\n"
            "li a0, 2047\n"
            "# smaller of two versions of 2 and 1 instructions\n"
            "addi a0, a1, 0\n"));
    }
}

// the multiply helper is emitted only when the kept version calls it, and a
// version that runs out of registers leaves the registers it allocated
auto check_version_state_kept() -> void {
    for (const machine::kept_version kept :
         {machine::kept_version::smaller, machine::kept_version::last}) {
        std::ostringstream output;
        machine_rv32i_linux backend{&output, test_builtins, {}, assembler::jump_mode::resolved};
        backend.set_kept_version(kept);
        backend.start();

        const operand result{operand::reg("a0", integer)};

        const auto load = [&] {
            backend.copy_value(token{}, 0, result, operand::imm(1, integer));
        };

        const auto multiply = [&] {
            backend.multiply(token{}, 0, result, operand::reg("a1", integer),
                             machine::factor_use::read_only);
        };

        // note: more allocations than there are scratch registers
        const auto exhaust = [&] {
            for (size_t i{}; i < 64; ++i) {
                std::ignore = backend.alloc_scratch_register(token{}, 0, integer);
            }
        };

        backend.emit_most_efficient(token{}, 0, load, multiply);
        backend.emit_most_efficient(token{}, 0, load, exhaust);

        // the helpers in use are emitted before the data
        backend.begin_data(4);
        backend.finish();
        backend.write_assembly(output);

        const bool calls_multiply{kept == machine::kept_version::last};

        assert(output.str().contains(".Lbaz_multiply:") == calls_multiply);
        assert(output.str().contains(
            "# first of two versions, the second runs out of registers\n"
            "li a0, 1\n"));
    }
}

// the jump optimizer sees the jumps and labels that the backend emits
auto check_backend_jump_optimization() -> void {
    std::ostringstream output;
    machine_rv32i_linux backend{&output, test_builtins, {}, assembler::jump_mode::optimized};
    backend.start();

    const operand left{operand::reg("a0", integer)};
    const operand right{operand::reg("a1", integer)};

    const auto branch_if_different =
        [&](const std::string_view target) -> void {
        backend.compare_and_branch(
            token{}, 0, left, right,
            {
                .operation{machine::comparison_operator::not_equal},
                .target{target},
                .branch_on_true{true},
            },
            {});
    };

    backend.branch(0, "cmp_13_26");
    backend.label(0, "cmp_13_26");
    branch_if_different("bool_end_15_9");
    backend.branch(0, "bool_end_15_9");
    backend.label(0, "bool_end_15_9");
    branch_if_different("cmp_14_26");
    backend.branch(0, "if_14_8_code");
    backend.label(0, "cmp_14_26");
    backend.copy_value(token{}, 0, operand::reg("a2", integer),
                       operand::reg("a3", integer));
    backend.label(0, "if_14_8_code");
    backend.finish();
    backend.write_assembly(output);

    assert(output.str().contains("la s0, vars\naddi s0, s0, 2032\n\n"
                                 "cmp_13_26:\n"
                                 "bool_end_15_9:\n"
                                 "beq a0, a1, if_14_8_code\n"
                                 "cmp_14_26:\naddi a2, a3, 0\n"
                                 "if_14_8_code:\n"));
}

// a copy to the same address emits nothing, others load and store
auto check_copy_to_same_address() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    const operand address{operand::mem("s0", {}, 1, 24, integer)};

    assert(rv32i
               .emitted([&] {
                   backend.copy_value(token{}, 0, address, address);
               })
               .empty());

    const std::string to_next{
        rv32i.emitted([&] {
            backend.copy_value(token{}, 0,
                               operand::mem("s0", {}, 1, 28, integer), address);
        }),
    };

    assert(to_next.contains("lw t0, 24(s0)\n"));
    assert(to_next.contains("sw t0, 28(s0)\n"));

    // the same address at another width is a narrowing store
    const std::string narrowed{
        rv32i.emitted([&] {
            backend.copy_value(token{}, 0, operand::mem("s0", {}, 1, 24, byte),
                               address);
        }),
    };

    assert(narrowed.contains("lw t0, 24(s0)\n"));
    assert(narrowed.contains("sb t0, 24(s0)\n"));
}

// a variable comment names the size and the place
auto check_variable_comments() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    const std::string comments{
        rv32i.emitted([&] {
            backend.comment_variable(token{}, 0, "arr: i32[4]", 16,
                                     operand::mem("s0", {}, 1, 208, integer));
            backend.comment_variable(token{}, 0, "first", 4,
                                     operand::mem("s0", {}, 1, 0, integer));
        }),
    };

    assert(comments == "# arr: i32[4] (16 B @ [s0 + 208])\n"
                       "# first (4 B @ [s0])\n");
}

// an RV32I backend over a source file, its comments carry source positions
struct located_rv32i {
    std::ostringstream comments;
    source_files files;
    machine_rv32i_linux backend;

    // 'value' on the second line, at column 5
    const token location{0, {}, 10, "value", 15, {}, 2, false};

    located_rv32i() : backend{&comments, test_builtins, &files} {
        files.add("test.baz", "first\n    value");
    }
};

// columns are relative to the source line, not the file, and a comment
// without a token has no position
auto check_comment_positions() -> void {
    located_rv32i located;

    located.backend.comment(located.location, 1, "assignment");
    located.backend.comment(token{}, 0, "generated");

    assert(located.comments.str() ==
           "    # [test.baz:2:5] assignment\n# generated\n");
}

// the allocation of a register is a comment at the token that needs it
auto check_allocation_comments() -> void {
    located_rv32i located;
    machine_rv32i& backend{located.backend};

    const operand scratch{
        backend.alloc_scratch_register(located.location, 1, integer),
    };
    const operand named{
        backend.alloc_named_register(located.location, 1, "a0", integer),
    };
    backend.free_named_register(located.location, 1, named);
    backend.free_scratch_register(located.location, 1, scratch);

    assert(located.comments.str() ==
           "    # [test.baz:2:5] allocate scratch register -> t0\n"
           "    # [test.baz:2:5] allocate named register a0\n"
           "    # [test.baz:2:5] free named register a0\n"
           "    # [test.baz:2:5] free scratch register t0\n");

    backend.finish();
}

// an operation on memory loads, computes and stores through a scratch
// register, all at the position of its token
auto check_memory_operation_comments() -> void {
    located_rv32i located;

    located.backend.add_subtract(located.location, 1, op_of('+'),
                                 operand::mem("a0", {}, 1, 0, integer),
                                 operand::imm(1, integer));

    assert(located.comments.str() ==
           "    # [test.baz:2:5] allocate scratch register -> t0\n"
           "    lw t0, 0(a0)\n    addi t0, t0, 1\n    sw t0, 0(a0)\n"
           "    # [test.baz:2:5] free scratch register t0\n");

    located.backend.finish();
}

// the scratch registers are handed out temporaries first and the argument
// registers last, 'a0' at the very end because results come back in it
auto check_rv32i_scratch_order() -> void {
    located_rv32i located;
    machine_rv32i& backend{located.backend};

    constexpr std::array<std::string_view, rv32i_scratch_count> order{
        "t0", "t1", "t2", "t3", "t4", "t5", "t6", "s0",  "s1",  "s2",
        "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "tp",
        "gp", "ra", "a1", "a2", "a3", "a4", "a5", "a6",  "a7",  "a0",
    };

    std::vector<operand> allocated;
    for (const std::string_view name : order) {
        allocated.push_back(
            backend.alloc_scratch_register(located.location, 1, integer));
        assert(allocated.back().base_register() == name);
    }

    backend.free_scratch_registers(located.location, 1, allocated);
    backend.finish();
}

// the copy helper describes the registers it uses and its steps
auto check_copy_helper_comments() -> void {
    located_rv32i located;
    machine_rv32i& backend{located.backend};

    backend.copy_elements(
        located.location, 1, 4,
        [&](const operand& count) -> void {
            backend.copy_value(located.location, 1, count,
                               operand::imm(2, integer));
        },
        {
            .alignment{4},
            .src{fixed_address(operand::mem("s0", {}, 1, 216, integer))},
            .dst{fixed_address(operand::mem("s0", {}, 1, 208, integer))},
        });

    for (const std::string_view text :
         {"t0: source, t1: destination, t2: count",
          "t2: elements to bytes (4 bytes/element)",
          "t4: end of words, t2: tail bytes", "copy 4-byte words",
          "copy optional 2-byte tail", "copy optional final byte"}) {
        assert(located.comments.str().contains(
            std::format("# [test.baz:2:5] {}\n", text)));
    }

    backend.finish();
}

// ============================================================================
// 3. host checks: compiled .baz programs inspected as assembly text
// ============================================================================

// 'array_copy' on offset arrays: the pointer registers hold the address
// throughout the index arithmetic
auto check_array_copy_pointers() -> void {
    const std::string assembly{compiled_rv32i(R"baz(
func main() {
    var source = i32[4]{}
    var destination = i32[4]{}
    var count = 2
    array_copy(source[2], destination[1], count)
}
)baz")};

    assert(assembly.contains("slli t0, t3, 2\n"));
    assert(assembly.contains("add t0, t0, s0\n"));
    assert(assembly.contains("slli t1, t3, 2\n"));
    assert(assembly.contains("add t1, t1, s0\n"));
    assert(not assembly.contains("addi t0, t3, 0\n"));
    assert(not assembly.contains("addi t1, t3, 0\n"));
}

// 'arrays_equal' on offset arrays: same pointer rule as 'array_copy', no
// extra scratch register or boolean normalization, and described registers
auto check_arrays_equal_registers_rv32i() -> void {
    const std::string assembly{compiled_rv32i(R"baz(
func assert(ok bool) { if not ok exit(1) }
func main() {
    var source = i32[4]{}
    var destination = i32[4]{}
    assert(arrays_equal(source[2], destination[1], 2))
}
)baz")};

    assert(assembly.contains("slli t1, t4, 2\n"));
    assert(assembly.contains("add t1, t1, s0\n"));
    assert(assembly.contains("slli t2, t4, 2\n"));
    assert(assembly.contains("add t2, t2, s0\n"));
    assert(not assembly.contains("addi t1, t4, 0\n"));
    assert(not assembly.contains("addi t2, t4, 0\n"));
    assert(not assembly.contains("sltu t0, zero, t0\n"));
    assert(not assembly.contains("allocate scratch register -> t6\n"));

    for (const std::string_view text :
         {"t1: lhs, t2: rhs, t3: count",
          "t3: elements to bytes (4 bytes/element)",
          "t0: left value/result, t4: right value",
          "t5: end of words, t3: tail bytes", "stop at first mismatch",
          "compare 4-byte words", "compare optional 2-byte tail",
          "compare optional final byte", "all matched or empty: true",
          "mismatch: false"}) {
        assert(assembly.contains(std::format("# {}\n", text)));
    }
}

// x86_64: a negated 'arrays_equal' condition uses 'sete' and 'setne' directly,
// without inverting or comparing the result and without an extra scratch
// register
auto check_arrays_equal_condition_x86() -> void {
    const std::string assembly{compiled_x86_64(R"baz(
func assert(ok bool) { if not ok exit(1) }
func main() {
    var left = i8[2]{1, 2}
    var right = i8[2]{1, 2}
    assert(arrays_equal(left, right, 2))
    right[1] = 3
    assert(not arrays_equal(left, right, 2))
}
)baz")};

    assert(assembly.contains("sete r15b\n"));
    assert(not assembly.contains("xor r15b, 1\n"));
    assert(assembly.contains("setne r15b\n"));
    assert(not assembly.contains("\n    cmp r15b, 0\n"));
    assert(not assembly.contains("allocate scratch register -> r14\n"));
}

// 'arrays_equal' and 'equal' stored in a variable: both targets store the
// flag result directly, without a normalizing instruction
auto check_equal_results_stored_directly() -> void {
    const std::string_view source{R"baz(
func main() {
    var left = i8[2]{1, 2}
    var right = i8[2]{1, 2}
    var same = arrays_equal(left, right, 2)
    same = not arrays_equal(left, right, 2)
    same = left == right
    same = left != right
}
)baz"};

    const std::string x86{compiled_x86_64(source)};

    assert(x86.contains("sete byte [rbp + 4]\n"));
    assert(x86.contains("setne byte [rbp + 4]\n"));
    assert(not x86.contains("sete r15b\n"));
    assert(not x86.contains("setne r15b\n"));
    assert(not x86.contains("cmp r15b, 0\n"));
    assert(not x86.contains("xor r15b, 1\n"));

    const std::string rv32i{compiled_rv32i(source)};

    assert(rv32i.contains("sb t3, -2028(s0)\n"));
    assert(not rv32i.contains("sltu "));
    assert(not rv32i.contains("sltiu "));
    assert(not rv32i.contains("xori "));
}

// ============================================================================
// 4. host checks: the x86_64 backend
// ============================================================================

// the registers handed out first are the ones no instruction needs: 'rdx' and
// 'rax' come late and 'rsi', 'rdi' and 'rcx', which every copy and compare of
// memory needs, last, also after a register was freed again
auto check_x86_scratch_registers() -> void {
    captured_x86_64 x86;
    machine_x86_64& backend{x86.backend};

    constexpr std::array<std::string_view, 14> order{
        "r15", "r14", "r13", "r12", "r10", "r9",  "r8",
        "r11", "rbx", "rdx", "rax", "rsi", "rdi", "rcx",
    };

    // the order holds again after every register was freed
    for (size_t pass{}; pass < 2; ++pass) {
        std::vector<operand> registers;
        for (const std::string_view expected : order) {
            registers.push_back(
                backend.alloc_scratch_register(source_tk, 0, integer64));
            assert(registers.back().base_register() == expected);
        }

        // scratch registers are exhausted after the last one
        assert(rejected_with([&] {
            std::ignore =
                backend.alloc_scratch_register(source_tk, 0, integer64);
        }));

        // a register needed by an instruction is rejected, not overwritten
        assert(rejected_with(
            [&] {
                std::ignore = backend.alloc_named_register(source_tk, 0, "rcx",
                                                           integer64);
            },
            "holds a scratch value"));

        // freed registers are handed out again in the same order
        for (size_t count{}; count < 2; ++count) {
            backend.free_scratch_register(token{}, 0, registers.back());
            registers.pop_back();
        }

        registers.push_back(
            backend.alloc_scratch_register(token{}, 0, integer64));
        assert(registers.back().base_register() == "rdi");

        registers.push_back(
            backend.alloc_scratch_register(token{}, 0, integer64));
        assert(registers.back().base_register() == "rcx");

        backend.free_scratch_registers(token{}, 0, registers);
        backend.finish();
    }
}

// 'read', 'write' and 'invoke_syscall' save and restore 'rcx' and 'r11' (which
// the syscall instruction clobbers) exactly when they are in use, around the
// syscall and without touching 'rsp'
auto check_x86_syscall_register_saving() -> void {
    captured_x86_64 x86;
    machine_x86_64& backend{x86.backend};

    // note: 8 scratch registers reach 'r11', the eighth of the order
    constexpr size_t scratch_count_to_r11{8};

    for (const bool r11_allocated : {false, true}) {
        for (const bool rcx_allocated : {false, true}) {
            operand live_rcx;
            if (rcx_allocated) {
                live_rcx = backend.alloc_named_register(token{}, 0, "rcx", byte);
            }

            std::vector<operand> scratch;
            if (r11_allocated) {
                scratch =
                    hold_scratch_registers(backend, scratch_count_to_r11, byte);
            }

            const machine::builtin_function_registers contract{
                backend.registers_for_builtin_function(
                    machine::builtin_function::write),
            };

            std::vector<operand> args;
            for (const std::string_view name : contract.arguments) {
                args.push_back(
                    backend.alloc_named_register(token{}, 0, name, integer64));
            }

            const operand result{
                backend.alloc_named_register(token{}, 0, contract.result,
                                             integer64),
            };

            const std::array<std::function<void()>, 3> system_calls{
                [&] {
                    backend.read(token{}, 0, result, args.at(0), args.at(1),
                                 args.at(2));
                },
                [&] {
                    backend.write(token{}, 0, result, args.at(0), args.at(1),
                                  args.at(2));
                },
                [&] { backend.invoke_syscall(0); },
            };

            for (const std::function<void()>& system_call : system_calls) {
                x86.output.str({});
                system_call();

                const std::string assembly{x86.output.str()};

                assert(assembly.contains("push rcx") == rcx_allocated);
                assert(assembly.contains("pop rcx") == rcx_allocated);
                assert(assembly.contains("push r11") == r11_allocated);
                assert(assembly.contains("pop r11") == r11_allocated);
                assert(not assembly.contains("rsp"));
                assert(not assembly.contains("push rax"));
                assert(not assembly.contains("push rdi"));

                // a saved register is pushed before and popped after the call
                for (const std::string_view name : {"rcx", "r11"}) {
                    const std::string push{std::format("push {}", name)};
                    if (assembly.contains(push)) {
                        assert(assembly.find(push) < assembly.find("syscall"));
                        assert(assembly.find(std::format("pop {}", name)) >
                               assembly.find("syscall"));
                    }
                }
            }

            // 'exit' does not return, so nothing is saved
            x86.output.str({});
            backend.exit(token{}, 0, args.at(0));
            assert(not x86.output.str().contains("push "));
            assert(not x86.output.str().contains("pop "));

            backend.free_named_register(token{}, 0, result);
            backend.free_named_registers(token{}, 0, args);
            backend.free_scratch_registers(token{}, 0, scratch);
            if (rcx_allocated) {
                backend.free_named_register(token{}, 0, live_rcx);
            }
            backend.finish();
        }
    }
}

// a compiled 'write' in 'main' needs no saves and no scratch register
auto check_x86_write_in_program() -> void {
    const std::string assembly{
        compiled_x86_64("func main() { var b = i8[1]{} "
                        "var value = write(1, b, 0) exit(value) }"),
    };

    const size_t main_start{assembly.find("main:")};
    assert(main_start != std::string::npos);

    const std::string main_body{assembly.substr(main_start)};

    assert(main_body.contains("mov rdi, 1"));
    assert(not main_body.contains("push "));
    assert(not main_body.contains("pop "));
    assert(not main_body.contains("allocate scratch register"));
}

// a system call inside the arguments of another needs 'rdi' twice, which is
// rejected instead of overwriting it
auto check_x86_nested_syscalls_rejected() -> void {
    for (const std::string_view source :
         {"func main() { var b = i8[1]{} write(1, b, write(1, b, 0)) }",
          "func main() { var b = i8[1]{} exit(write(1, b, 0)) }"}) {
        assert(rejected_with([&] { std::ignore = compiled_x86_64(source); },
                             "register rdi is always used by the operation"));
    }
}

// ============================================================================
// 5. host checks: instruction shapes of the RV32I backend
//
// Each check drives 'machine_rv32i' directly and compares the text it emits.
// The scratch registers of the pool are held where a check must prove that no
// temporary register is used.
// ============================================================================

// the index scales that address lowering accepts: powers of two that a shift
// reaches within 32 bits
auto check_index_scale_support() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    assert(&backend.default_type() == &integer);
    assert(backend.address_size_bytes() == 4);
    assert(backend.can_lower_index_scale(1));
    assert(not backend.can_lower_index_scale(3));
    assert(backend.can_lower_index_scale(256));
    assert(backend.can_lower_index_scale(UINT64_C(2147483648)));
    assert(not backend.can_lower_index_scale(UINT32_MAX));
    assert(not backend.can_lower_index_scale(0));
    assert(not backend.can_lower_index_scale(UINT64_C(4294967296)));
}

// comparing memory with few free scratch registers: the result register
// doubles as the temporary, a known size takes its tail at offsets and a
// run-time count advances the pointers
auto check_memory_comparison_tail() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    // note: 25 held registers leave the comparison 5 of the pool
    constexpr size_t held_count{25};

    // note: 7 bytes are a word, a halfword and a byte
    constexpr int64_t size_bytes{7};

    for (const bool counted : {false, true}) {
        const std::vector<operand> held{
            hold_scratch_registers(backend, held_count, boolean),
        };

        const operand result{held.front()};
        const operand buffer{operand::mem(held.back(), byte)};

        if (counted) {
            backend.arrays_equal(
                token{}, 0, 1,
                [&](const operand& count) -> void {
                    backend.copy_value(token{}, 0, count,
                                       operand::imm(size_bytes, integer));
                },
                {
                    .alignment{4},
                    .lhs{fixed_address(buffer)},
                    // note: the text before the right address sets up the
                    //       count and is not checked
                    .rhs{[&](const operand&, const operand&,
                             const machine::address_use use) -> void {
                        use(buffer);
                        rv32i.output.str({});
                    }},
                    .dst{result},
                    .inverted{false},
                });
        } else {
            backend.memory_equal(token{}, 0, size_bytes,
                                 {
                                     .alignment{4},
                                     .lhs{fixed_address(buffer)},
                                     .rhs{fixed_address(buffer)},
                                     .dst{result},
                                     .inverted{false},
                                 });
        }

        const std::string assembly{rv32i.output.str()};
        const std::string_view reg{result.base_register()};

        for (const std::string_view load : {"lw", "lhu", "lbu"}) {
            assert(assembly.contains(std::format("{} {}, ", load, reg)));
        }
        assert(assembly.contains(std::format("li {}, 1", reg)));
        assert(assembly.contains(std::format("li {}, 0", reg)));
        assert(not assembly.contains(std::format("addi {},", reg)));
        assert(not assembly.contains("slli"));

        backend.free_scratch_registers(token{}, 0, held);
        backend.finish();
        rv32i.output.str({});
    }
}

// 'copy' of a known size: loads and stores of the widest width first, the
// tail by width, and a loop from 17 bytes on
auto check_known_size_copy() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    // note: 16 bytes are the most copied without a loop
    constexpr size_t unrolled_limit_bytes{16};

    // the unsigned loads of the copy, they move bits only
    struct access_pair {
        size_t width;
        std::string_view load;
        std::string_view store;
    };

    constexpr std::array<access_pair, 3> accesses{{
        {.width{4}, .load{"lw "}, .store{"sw "}},
        {.width{2}, .load{"lhu "}, .store{"sh "}},
        {.width{1}, .load{"lbu "}, .store{"sb "}},
    }};

    for (size_t size_bytes{}; size_bytes <= 24; ++size_bytes) {
        const std::string assembly{
            rv32i.emitted([&] {
                backend.copy(token{}, 0, operand::mem("a1", {}, 1, 0, byte),
                             operand::mem("a0", {}, 1, 0, byte), size_bytes,
                             4);
            }),
        };

        assert(assembly.contains("bne ") == (size_bytes > unrolled_limit_bytes));

        if (size_bytes == 0) {
            assert(assembly.empty());
        } else if (size_bytes <= unrolled_limit_bytes) {
            // the words, then at most one halfword and one byte
            for (const access_pair& access : accesses) {
                size_t loads{};
                size_t stores{};
                std::istringstream lines{assembly};
                for (std::string line; std::getline(lines, line);) {
                    loads += line.starts_with(access.load);
                    stores += line.starts_with(access.store);
                }

                const size_t expected{
                    access.width == 4
                        ? size_bytes / 4
                        : (size_bytes % (access.width * 2)) / access.width,
                };

                assert(loads == expected and stores == expected);
            }
            assert(not assembly.contains("beqz"));
        } else {
            // the known size decides the tail at compile time
            assert(assembly.contains("lw ") and assembly.contains("sw "));
            assert((assembly.contains("lhu ") and assembly.contains("sh ")) ==
                   ((size_bytes & 2U) != 0));
            assert((assembly.contains("lbu ") and assembly.contains("sb ")) ==
                   ((size_bytes & 1U) != 0));
            assert(not assembly.contains("srli ") and
                   not assembly.contains("andi "));
        }

        backend.finish();
    }
}

// string data: the escapes of .baz strings become assembler escapes
auto check_string_data_escapes() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    const auto string_data = [&](const std::string_view text) -> std::string {
        return rv32i.emitted([&] { backend.emit_string_data(text); });
    };

    assert(string_data({}) == ".ascii \"\"\n");

    assert(
        string_data(R"baz(\0\a\b\t\n\v\f\r\e\"\'`\\\x00\x7F\x80\xff\x41B)baz") ==
        R"baz(.ascii "\000\007\010\t\n\013\014\r\033\"'`\\\000\177\200\377AB")baz"
        "\n");

    // the bytes of utf-8 are written as they are
    assert(string_data("\xc3\xa9\n") == R"baz(.ascii "\303\251\n")baz"
                                        "\n");

    // a digit after '\x00' is not read as part of the octal escape
    assert(string_data(R"baz(hello\n\x007)baz") ==
           R"baz(.ascii "hello\n\0007")baz"
           "\n");
}

// ============================================================================
// 6. host checks: arithmetic on registers only (no scratch register is free)
// ============================================================================

// shifts: register counts and immediate counts of 0 to 31, the front end
// rejects other constant counts (tests 0713 and 0750)
auto check_shifts() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    const std::vector<operand> held{
        hold_scratch_registers(backend, rv32i_scratch_count, integer),
    };

    const operand value{operand::reg("a0", integer)};

    const auto shifted = [&](const char operation,
                             const operand& count) -> std::string {
        return rv32i.emitted([&] {
            backend.shift(token{}, 0, op_of(operation), value, count,
                          machine::runtime_check{});
        });
    };

    for (const char operation : {'<', '>'}) {
        const std::string_view by_register{operation == '<' ? "sll" : "sra"};
        const std::string_view by_constant{operation == '<' ? "slli" : "srai"};

        assert(shifted(operation, operand::reg("a1", integer)) ==
               std::format("{} a0, a0, a1\n", by_register));

        // a shift by 0 changes nothing
        assert(shifted(operation, operand::imm(0, integer)).empty());

        for (const unsigned count : {2U, 31U}) {
            assert(shifted(operation, operand::imm(count, integer)) ==
                   std::format("{} a0, a0, {}\n", by_constant, count));
        }
    }

    backend.free_scratch_registers(token{}, 0, held);
    backend.finish();
}

// 'add_subtract' and 'bitwise' with register and constant operands, a
// subtracted constant is added negated
auto check_add_subtract_bitwise() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    const std::vector<operand> held{
        hold_scratch_registers(backend, rv32i_scratch_count, integer),
    };

    struct operation_form {
        char operation;
        std::string_view instruction;
    };

    constexpr std::array<operation_form, 5> forms{{
        {.operation{'+'}, .instruction{"add"}},
        {.operation{'-'}, .instruction{"sub"}},
        {.operation{'&'}, .instruction{"and"}},
        {.operation{'|'}, .instruction{"or"}},
        {.operation{'^'}, .instruction{"xor"}},
    }};

    const operand dst{operand::reg("a0", integer)};

    for (const operation_form& form : forms) {
        const bool is_additive{form.operation == '+' or form.operation == '-'};

        const auto computed = [&](const operand& src) -> std::string {
            return rv32i.emitted([&] {
                if (is_additive) {
                    backend.add_subtract(token{}, 0, op_of(form.operation), dst,
                                         src);
                } else {
                    backend.bitwise(token{}, 0, op_of(form.operation), dst,
                                    src);
                }
            });
        };

        assert(computed(operand::reg("a1", integer)) ==
               std::format("{} a0, a0, a1\n", form.instruction));

        // note: 2047 is the largest 12-bit immediate
        for (const int64_t constant : {7, -7, 2047, -2047}) {
            const std::string_view immediate_form{
                form.operation == '-' ? "add" : form.instruction,
            };

            const int64_t immediate{
                form.operation == '-' ? -constant : constant,
            };

            assert(computed(operand::imm(constant, integer)) ==
                   std::format("{}i a0, a0, {}\n", immediate_form, immediate));
        }
    }

    backend.free_scratch_registers(token{}, 0, held);
    backend.finish();
}

// 'x & 0' stores or loads its known result for every value width, a shift by
// 0 emits nothing, '~' is an 'xori' and '-' a 'sub' from zero
auto check_zero_operations_and_zero_shifts() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    const std::vector<operand> held{
        hold_scratch_registers(backend, rv32i_scratch_count, integer),
    };

    for (const type* value_type : {&integer, &half, &byte}) {
        for (const bool in_memory : {false, true}) {
            const operand dst{
                in_memory ? operand::mem("a0", {}, 1, 0, *value_type)
                          : operand::reg("a0", *value_type),
            };

            const std::string zero_shifts{
                rv32i.emitted([&] {
                    for (const char operation : {'<', '>'}) {
                        backend.shift(token{}, 0, op_of(operation), dst,
                                      operand::imm(0, integer),
                                      machine::runtime_check{});
                    }
                }),
            };

            assert(zero_shifts.empty());

            const std::string masked_out{
                rv32i.emitted([&] {
                    backend.bitwise(token{}, 0, op_of('&'), dst,
                                    operand::imm(0, integer));
                }),
            };

            assert(masked_out ==
                   (in_memory ? std::format("{} zero, 0(a0)\n",
                                            store_of(*value_type))
                              : "li a0, 0\n"));
        }

        assert(rv32i.emitted([&] {
            backend.unary(token{}, 0, unary_of('~'),
                          operand::reg("a0", *value_type));
        }) == "xori a0, a0, -1\n");
    }

    assert(rv32i.emitted([&] {
        backend.unary(token{}, 0, unary_of('-'), operand::reg("a0", integer));
    }) == "sub a0, zero, a0\n");

    backend.free_scratch_registers(token{}, 0, held);
    backend.finish();
}

// byte values: an operation with a known result loads it, a shift extends the
// sign again, and constants beyond 12 bits take two instructions
auto check_narrow_values_and_large_constants() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    const std::vector<operand> held{
        hold_scratch_registers(backend, rv32i_scratch_count, integer),
    };

    const operand value{operand::reg("a0", byte)};

    const auto shifted_left = [&](const int64_t count) -> std::string {
        return rv32i.emitted([&] {
            backend.shift(token{}, 0, op_of('<'), value,
                          operand::imm(count, integer),
                          machine::runtime_check{});
        });
    };

    // every bit of a byte set is -1
    assert(rv32i.emitted([&] {
        backend.bitwise(token{}, 0, op_of('|'), value,
                        operand::imm(255, integer));
    }) == "li a0, -1\n");

    // note: 24 more shifts the byte to the top of the register, 'srai' brings
    //       its sign back
    assert(shifted_left(7) == "slli a0, a0, 31\nsrai a0, a0, 24\n");

    assert(shifted_left(1) == "slli a0, a0, 25\nsrai a0, a0, 24\n");

    // note: 2048 and -2049 are one past the 12-bit immediates
    assert(rv32i.emitted([&] {
        backend.add_subtract(token{}, 0, op_of('+'),
                             operand::reg("a0", integer),
                             operand::imm(2048, integer));
    }) == "addi a0, a0, 2047\naddi a0, a0, 1\n");

    assert(rv32i.emitted([&] {
        backend.add_subtract(token{}, 0, op_of('-'),
                             operand::reg("a0", integer),
                             operand::imm(2049, integer));
    }) == "addi a0, a0, -2048\naddi a0, a0, -1\n");

    backend.free_scratch_registers(token{}, 0, held);
    backend.finish();
}

// ============================================================================
// 7. host checks: copies, comparisons, addresses and zeroing
// ============================================================================

// copies between a register and memory use the load and store of the width,
// a copy of a register to itself emits nothing
auto check_copy_widths() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    for (const type* value_type : {&integer, &half, &byte}) {
        const operand memory{operand::mem("a0", {}, 1, 0, *value_type)};
        const operand reg{operand::reg("a1", *value_type)};

        assert(rv32i.emitted([&] {
            backend.copy_value(token{}, 0, memory, reg);
        }) == std::format("{} a1, 0(a0)\n", store_of(*value_type)));

        assert(rv32i.emitted([&] {
            backend.copy_value(token{}, 0, reg, memory);
        }) == std::format("{} a1, 0(a0)\n", load_of(*value_type)));

        assert(rv32i.emitted([&] { backend.copy_value(token{}, 0, reg, reg); })
                   .empty());

        backend.finish();
    }
}

// bitwise operations on byte registers need one instruction, the bits above
// the byte are the sign of both operands already
auto check_byte_bitwise() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    for (const char operation : {'&', '|', '^'}) {
        const std::string assembly{
            rv32i.emitted([&] {
                backend.bitwise(token{}, 0, op_of(operation),
                                operand::reg("a0", byte),
                                operand::reg("a1", byte));
            }),
        };

        assert(std::ranges::count(assembly, '\n') == 1);

        backend.finish();
    }
}

// a shift of variables compiles without a copy of the count
auto check_shift_of_variables() -> void {
    const std::string assembly{
        compiled_rv32i(
            "func main() { var a = 3 var b = 2 var x = a << b exit(x) }"),
    };

    assert(assembly.contains("sll t0, t0, t1"));
    assert(not assembly.contains("addi t1, t0, 0"));
}

// an address and the text that loads from it into 'a1'
struct address_form {
    operand address;
    std::string_view expected;
};

// addresses of the form base + index * scale + offset with free registers:
// an offset beyond 12 bits takes a temporary, a scale is a shift
auto check_address_lowering() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    const operand dst{operand::reg("a1", integer)};

    const auto loaded = [&](const operand& address) -> std::string {
        return rv32i.emitted(
            [&] { backend.copy_value(token{}, 0, dst, address); });
    };

    // note: 8196 is 2 << 12 plus 4, 'lui' loads the 2
    assert(loaded(operand::mem("a2", "a3", 1, 8196, integer)) ==
           "add a1, a2, a3\nlui t0, 2\nadd a1, a1, t0\nlw a1, 4(a1)\n");

    for (const uint64_t scale :
         {UINT64_C(2), UINT64_C(256), UINT64_C(2147483648)}) {
        assert(loaded(operand::mem("a2", "a3", scale, 40, integer)) ==
               std::format("slli a1, a3, {}\nadd a1, a1, a2\nlw a1, 40(a1)\n",
                           std::countr_zero(scale)));
    }

    backend.finish();
}

// with every scratch register but 'a0' held, an address is built in the
// destination register: the register terms first, then the offset
auto check_address_lowering_in_destination() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    std::vector<operand> held{
        hold_scratch_registers(backend, rv32i_scratch_count, integer),
    };

    assert(held.back().base_register() == "a0");
    backend.free_scratch_register(token{}, 0, held.back());
    held.pop_back();

    const operand dst{operand::reg("a1", integer)};

    const std::array forms{
        // the destination as the base keeps its value
        address_form{
            .address{operand::mem("a1", {}, 1, 4, integer)},
            .expected{"lw a1, 4(a1)\n"},
        },
        address_form{
            .address{operand::mem("a2", "a3", 1, 4, integer)},
            .expected{"add a1, a2, a3\nlw a1, 4(a1)\n"},
        },
        // note: 8196 is 2 << 12 plus 4
        address_form{
            .address{operand::mem("a2", {}, 1, 8196, integer)},
            .expected{"lui a1, 2\nadd a1, a1, a2\nlw a1, 4(a1)\n"},
        },
        // note: -2049 is -1 << 12 plus 2047, the upper part wraps
        address_form{
            .address{operand::mem("a2", {}, 1, -2049, integer)},
            .expected{"lui a1, 1048575\nadd a1, a1, a2\nlw a1, 2047(a1)\n"},
        },
        address_form{
            .address{operand::mem("zero", {}, 1, 8196, integer)},
            .expected{"lui a1, 2\nadd a1, a1, zero\nlw a1, 4(a1)\n"},
        },
        // an offset of the size of the address space wraps to its low bits
        address_form{
            .address{operand::mem("a1", {}, 1, INT64_C(-4294967295), integer)},
            .expected{"lw a1, 1(a1)\n"},
        },
        address_form{
            .address{
                operand::mem("zero", {}, 1, INT64_C(-4294967295), integer)},
            .expected{"lw a1, 1(zero)\n"},
        },
        address_form{
            .address{operand::mem("a1", {}, 1, INT64_C(4294967295), integer)},
            .expected{"lw a1, -1(a1)\n"},
        },
        address_form{
            .address{operand::mem("zero", {}, 1, INT64_C(4294967295), integer)},
            .expected{"lw a1, -1(zero)\n"},
        },
        address_form{
            .address{operand::mem("a1", "a3", 1, 4, integer)},
            .expected{"add a1, a1, a3\nlw a1, 4(a1)\n"},
        },
    };

    for (const address_form& form : forms) {
        assert(rv32i.emitted([&] {
            backend.copy_value(token{}, 0, dst, form.address);
        }) == form.expected);
    }

    backend.free_scratch_registers(token{}, 0, held);
    backend.finish();
}

// with every scratch register held, an address is built in the destination
// register, whichever of base and index it is
auto check_address_forms_without_temporaries() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    const std::vector<operand> held{
        hold_scratch_registers(backend, rv32i_scratch_count, integer),
    };

    const operand dst{operand::reg("a1", integer)};

    const std::array forms{
        // the destination as the index
        address_form{
            .address{operand::mem("a2", "a1", 1, 4, integer)},
            .expected{"add a1, a2, a1\nlw a1, 4(a1)\n"},
        },
        // the destination as base and index
        address_form{
            .address{operand::mem("a1", "a1", 1, 4, integer)},
            .expected{"add a1, a1, a1\nlw a1, 4(a1)\n"},
        },
        // the destination as the base
        address_form{
            .address{operand::mem("a1", "a2", 1, 4, integer)},
            .expected{"add a1, a1, a2\nlw a1, 4(a1)\n"},
        },
        // a scaled destination index is shifted in place
        address_form{
            .address{operand::mem("a2", "a1", 4, 4, integer)},
            .expected{"slli a1, a1, 2\nadd a1, a1, a2\nlw a1, 4(a1)\n"},
        },
        address_form{
            .address{operand::mem("zero", "a2", 1, 4, integer)},
            .expected{"add a1, zero, a2\nlw a1, 4(a1)\n"},
        },
    };

    for (const address_form& form : forms) {
        assert(rv32i.emitted([&] {
            backend.copy_value(token{}, 0, dst, form.address);
        }) == form.expected);
    }

    // the address of the destination itself is already in it
    assert(rv32i
               .emitted([&] {
                   backend.address_of(token{}, 0, dst,
                                      operand::mem("a1", {}, 1, 0, integer));
               })
               .empty());

    backend.free_scratch_registers(token{}, 0, held);
    backend.finish();
}

// stores of constants and zeroing of small ranges need no temporary register:
// zero is the register 'zero'
auto check_constant_stores_and_small_zeroing() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    const std::vector<operand> held{
        hold_scratch_registers(backend, rv32i_scratch_count, integer),
    };

    assert(rv32i.emitted([&] {
        backend.copy_value(token{}, 0, operand::mem("a2", {}, 1, 0, integer),
                           operand::imm(0, integer));
    }) == "sw zero, 0(a2)\n");

    // 255 in a byte is -1
    assert(rv32i.emitted([&] {
        backend.copy_value(token{}, 0, operand::reg("a1", byte),
                           operand::imm(255, integer));
    }) == "li a1, -1\n");

    const auto zeroed = [&](const int64_t offset,
                            const size_t size_bytes) -> std::string {
        return rv32i.emitted([&] {
            backend.zero(token{}, 0, operand::mem("a2", {}, 1, offset, byte),
                         size_bytes, 4);
        });
    };

    assert(zeroed(0, 1) == "sb zero, 0(a2)\n");
    assert(zeroed(208, 16) == "sw zero, 208(a2)\nsw zero, 212(a2)\n"
                              "sw zero, 216(a2)\nsw zero, 220(a2)\n");
    assert(zeroed(0, 4) == "sw zero, 0(a2)\n");

    backend.free_scratch_registers(token{}, 0, held);
    backend.finish();
}

// a comparison, its operands and the text it emits with every scratch
// register held
struct comparison_form {
    operand lhs;
    operand rhs;
    machine::comparison_action action;
    std::string_view expected;
};

// comparisons that produce a boolean or branch, with every scratch register
// held: loads go to the destination, constants use the immediate forms
auto check_comparison_selection() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    const std::vector<operand> held{
        hold_scratch_registers(backend, rv32i_scratch_count, integer),
    };

    using comparison = machine::comparison_operator;

    const operand a0{operand::reg("a0", integer)};
    const operand a1{operand::reg("a1", integer)};

    const auto into = [](const comparison operation,
                         const std::string_view reg)
        -> machine::comparison_action {
        return {
            .operation{operation},
            .destination{operand::reg(reg, boolean)},
        };
    };

    const std::array forms{
        comparison_form{
            .lhs{a0},
            .rhs{a1},
            .action{into(comparison::less, "a2")},
            .expected{"slt a2, a0, a1\n"},
        },
        // a memory operand is loaded into the destination
        comparison_form{
            .lhs{operand::mem("a0", {}, 1, 0, integer)},
            .rhs{a1},
            .action{into(comparison::less, "a2")},
            .expected{"lw a2, 0(a0)\nslt a2, a2, a1\n"},
        },
        comparison_form{
            .lhs{a0},
            .rhs{operand::mem("a1", {}, 1, 0, integer)},
            .action{into(comparison::equal, "a1")},
            .expected{"lw a1, 0(a1)\nxor a1, a0, a1\nsltiu a1, a1, 1\n"},
        },
        // the wider operand is narrowed to the byte of the other
        comparison_form{
            .lhs{operand::reg("a0", byte)},
            .rhs{a1},
            .action{into(comparison::less, "a1")},
            .expected{"slli a1, a1, 24\nsrai a1, a1, 24\nslt a1, a0, a1\n"},
        },
        // every scratch register is held here, so no far jump register is
        // named
        comparison_form{
            .lhs{a0},
            .rhs{a1},
            .action{machine::comparison_action{
                .operation{comparison::equal},
                .target{"comparison_target"},
                .branch_on_true{true},
            }},
            .expected{"beq a0, a1, comparison_target\n"},
        },
        comparison_form{
            .lhs{a0},
            .rhs{operand::imm(7, integer)},
            .action{into(comparison::less, "a0")},
            .expected{"slti a0, a0, 7\n"},
        },
        // a comparison with zero is a test of the value
        comparison_form{
            .lhs{a0},
            .rhs{operand::imm(0, integer)},
            .action{into(comparison::equal, "a0")},
            .expected{"sltiu a0, a0, 1\n"},
        },
        comparison_form{
            .lhs{a0},
            .rhs{operand::imm(0, integer)},
            .action{into(comparison::not_equal, "a0")},
            .expected{"sltu a0, zero, a0\n"},
        },
        // 255 is -1 at the width of the byte
        comparison_form{
            .lhs{operand::reg("a0", byte)},
            .rhs{operand::imm(255, integer)},
            .action{into(comparison::less, "a1")},
            .expected{"slti a1, a0, -1\n"},
        },
    };

    for (const comparison_form& form : forms) {
        assert(rv32i.emitted([&] {
            backend.compare_and_branch(token{}, 0, form.lhs, form.rhs,
                                       form.action, {});
        }) == form.expected);
    }

    backend.free_scratch_registers(token{}, 0, held);
    backend.finish();
}

// multiplication by constants: 1 changes nothing, a power of two is a shift,
// -1 a negation and 0 a store of zero
auto check_multiply_by_constants() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    const std::vector<operand> held{
        hold_scratch_registers(backend, rv32i_scratch_count, integer),
    };

    const auto multiplied = [&](const operand& product,
                                const int64_t factor) -> std::string {
        return rv32i.emitted([&] {
            backend.multiply(token{}, 0, product, operand::imm(factor, integer),
                             machine::factor_use::read_only);
        });
    };

    const operand reg{operand::reg("a1", integer)};

    assert(multiplied(reg, 1).empty());
    assert(multiplied(reg, 8) == "slli a1, a1, 3\n");
    assert(multiplied(reg, -1) == "sub a1, zero, a1\n");
    assert(multiplied(operand::mem("a2", {}, 1, 0, integer), 0) ==
           "sw zero, 0(a2)\n");

    backend.free_scratch_registers(token{}, 0, held);
    backend.finish();
}

// a known-size copy takes 't0' as its temporary, and any free register when
// 't0' is held
auto check_copy_temporaries() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    const auto copied = [&](const int64_t src_offset, const int64_t dst_offset,
                            const size_t size_bytes) -> std::string {
        return rv32i.emitted([&] {
            backend.copy(token{}, 0, operand::mem("a2", {}, 1, dst_offset, byte),
                         operand::mem("a1", {}, 1, src_offset, byte),
                         size_bytes, 4);
        });
    };

    assert(copied(208, 240, 4) == "lw t0, 208(a1)\nsw t0, 240(a2)\n");

    // note: 7 bytes are a word, a halfword and a byte
    assert(copied(-16, 16, 7) ==
           "lw t0, -16(a1)\nsw t0, 16(a2)\nlhu t0, -12(a1)\nsh t0, 20(a2)\n"
           "lbu t0, -10(a1)\nsb t0, 22(a2)\n");

    // note: -1 leaves 'a0', the last of the pool, free
    const std::vector<operand> held{
        hold_scratch_registers(backend, rv32i_scratch_count - 1, integer),
    };

    assert(copied(208, 240, 4) == "lw a0, 208(a1)\nsw a0, 240(a2)\n");

    backend.free_scratch_registers(token{}, 0, held);
    backend.finish();
}

// zeroing: a base offset beyond 12 bits moves into a temporary, and more than
// 16 bytes become a loop with a tail
auto check_zeroing_ranges() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    const auto zeroed = [&](const int64_t offset,
                            const size_t size_bytes) -> std::string {
        return rv32i.emitted([&] {
            backend.zero(token{}, 0, operand::mem("a2", {}, 1, offset, byte),
                         size_bytes, 4);
        });
    };

    // note: 2047 plus the 12 of the last word is beyond the 12-bit offset
    assert(zeroed(2047, 16) == "addi t0, a2, 2047\nsw zero, 0(t0)\n"
                               "sw zero, 4(t0)\nsw zero, 8(t0)\n"
                               "sw zero, 12(t0)\n");

    // note: 67 bytes are 16 words and a tail of 3
    assert(zeroed(0, 67) ==
           "# zero loop of 4-byte accesses: start word aligned\n"
           "addi t0, a2, 0\n# zero 4-byte words\naddi t1, t0, 64\n1:\n"
           "sw zero, 0(t0)\naddi t0, t0, 4\nbne t0, t1, 1b\n"
           "# zero 3 B tail\nsh zero, 0(t0)\nsb zero, 2(t0)\n");

    backend.finish();
}

// an address that cannot be lowered is rejected: a scale beyond the 32-bit
// address range, or an intermediate address without a free scratch register
auto check_invalid_addresses_rejected() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    for (const uint64_t scale :
         {UINT64_C(4294967296), UINT64_C(9223372036854775808)}) {
        assert(not backend.can_lower_index_scale(scale));
    }

    // note: -1 leaves 'a0' free, it is also the destination
    const std::vector<operand> held{
        hold_scratch_registers(backend, rv32i_scratch_count - 1, integer),
    };

    assert(rejected_with([&] {
        backend.address_of(source_tk, 1, operand::reg("a0", integer),
                           operand::mem("a0", {}, 1, 8196, integer));
    }));

    backend.free_scratch_registers(token{}, 0, held);
    backend.finish();
}

// the scratch register pool: which registers it hands out, in which order,
// with 0, 1 and 2 registers reserved for the variables and frame bases
auto check_scratch_register_pool() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    // note: the 8 argument registers come last in the order
    constexpr size_t argument_register_count{8};

    for (const size_t reserved_count : {size_t{}, size_t{1}, size_t{2}}) {
        // the argument registers can be allocated by name and freed again
        for (const std::string_view name :
             {"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"}) {
            const operand reg{
                backend.alloc_named_register(token{}, 0, name, integer),
            };
            backend.free_named_register(token{}, 0, reg);
        }

        if (reserved_count >= 1) {
            backend.reserve_variables_base();
        }
        if (reserved_count == 2) {
            backend.reserve_frame_base();
        }

        const size_t available{rv32i_scratch_count - reserved_count};

        std::vector<operand> held;
        bool has_ra{};
        bool has_gp{};
        bool has_tp{};
        for (size_t count{}; count < available; ++count) {
            const operand reg{
                backend.alloc_scratch_register(token{}, 0, integer),
            };
            const std::string_view name{reg.base_register()};

            assert(name != "zero" and name != "sp");
            assert(reserved_count == 0 or name != "s0");
            assert(reserved_count != 2 or name != "s1");
            assert(name.starts_with("a") ==
                   (count >= available - argument_register_count));

            has_ra = has_ra or name == "ra";
            has_gp = has_gp or name == "gp";
            has_tp = has_tp or name == "tp";
            held.push_back(reg);
        }

        // 'ra', 'gp' and 'tp' are ordinary scratch registers
        assert(has_ra and has_gp and has_tp);

        assert(rejected_with([&] {
            std::ignore = backend.alloc_scratch_register(source_tk, 0, integer);
        }));

        backend.free_scratch_registers(token{}, 0, held);
        if (reserved_count == 2) {
            backend.release_frame_base();
        }
        if (reserved_count >= 1) {
            backend.release_variables_base();
        }

        const operand named{
            backend.alloc_named_register(token{}, 0, "ra", integer),
        };
        assert(named.base_register() == "ra");
        backend.free_named_register(token{}, 0, named);
        backend.finish();
    }
}

// ============================================================================
// 8. host checks: helper routines for multiplication and division
// ============================================================================

// '*', '/' and '%' call a shared routine ('.Lbaz_multiply' or '.Lbaz_divide')
// that is emitted once, and only when something calls it; a multiplication
// by a constant ('+' here) never needs the routine
auto check_multiply_divide_routines() -> void {
    for (const char operation : {'*', '/', '%', '+'}) {
        std::ostringstream output;
        machine_rv32i_linux backend{&output, test_builtins};

        const auto computed = [&](const operand& dst, const operand& src) {
            if (operation == '*') {
                backend.multiply(token{}, 0, dst, src,
                                 machine::factor_use::read_only);
            } else {
                backend.divide(token{}, 0, op_of(operation), dst, src,
                               machine::runtime_check{});
            }
        };

        if (operation != '+') {
            // the operands go to the argument registers and the result comes
            // back in 'a0', the remainder in 'a1'
            const operand dst{operand::reg("s2", integer)};
            const operand src{operand::reg("s3", integer)};

            computed(dst, src);

            assert(output.str() ==
                   std::format("addi a0, s2, 0\naddi a1, s3, 0\ncall {}\n"
                               "addi s2, {}, 0\n",
                               operation == '*' ? ".Lbaz_multiply"
                                                : ".Lbaz_divide",
                               operation == '%' ? "a1" : "a0"));

            // the call is made also with every scratch register held
            const std::vector<operand> held{
                hold_scratch_registers(backend, rv32i_scratch_count, integer),
            };
            computed(dst, src);
            backend.free_scratch_registers(token{}, 0, held);

            output.str({});
        }

        for (size_t count{}; count < 2; ++count) {
            if (operation == '+') {
                backend.multiply(token{}, 0, operand::reg("a0", integer),
                                 operand::imm(3, integer),
                                 machine::factor_use::read_only);
            } else {
                computed(operand::reg("a0", integer),
                         operand::reg("a1", integer));
            }
        }

        // the routines follow the code of 'main'
        assert(not output.str().contains(".Lbaz_multiply:"));
        assert(not output.str().contains(".Lbaz_divide:"));

        backend.end_main();
        backend.begin_data(4);

        const std::string assembly{output.str()};

        for (const std::string_view label :
             {".Lbaz_multiply:", ".Lbaz_divide:"}) {
            const bool is_called{
                label == ".Lbaz_multiply:" ? operation == '*'
                                           : operation == '/' or
                                                 operation == '%',
            };

            const size_t first{assembly.find(label)};

            assert((first != std::string::npos) == is_called);

            if (is_called) {
                assert(assembly.find(label, first + label.size()) ==
                       std::string::npos);
            }
        }

        backend.finish();
    }
}

// ============================================================================
// 9. generated programs with hand-written lines between the backend's output
// ============================================================================

constexpr std::string_view hand_written_marker{"@hand "};

// the hand-written assembly between the backend's lines travels as comments of
// the backend and is written as it is after the backend wrote its lines
auto hand_written(machine_rv32i& backend, const std::string_view text) -> void {
    for (const auto line : std::views::split(text, '\n')) {
        backend.comment(token{}, 0, "{}{}", hand_written_marker,
                        std::string_view{line});
    }
}

auto write_with_hand_written(machine_rv32i& backend) -> void {
    std::ostringstream assembly;
    backend.write_assembly(assembly);

    std::istringstream lines{assembly.str()};
    for (std::string line; std::getline(lines, line);) {
        const size_t marker_at{line.find(hand_written_marker)};

        // the marker is the first thing on a comment line
        const bool is_hand_written{
            marker_at != std::string::npos and
                line.find_first_not_of(' ') == marker_at - 2,
        };
        // note: -2 because the comment starts with '# '

        std::println("{}",
                     is_hand_written
                         ? line.substr(marker_at + hand_written_marker.size())
                         : line);
    }
}

// a hand-written check that 's1', the frame base, is 'offset' past 'dat'
auto expect_frame_base(machine_rv32i& backend, const size_t offset) -> void {
    hand_written(backend,
                 std::format("    la t0, dat\n    li t1, {}\n    add t0, t0, t1\n"
                             "    beq s1, t0, 1f\n    j call_failure\n1:",
                             offset));
}

// 'noninline': a function call preserves exactly the allocated registers
// and callees never write the variables base
auto generate_noninline() -> void {
    // the registers 'x1' to 'x31' by number, 'x2' is 'sp' and 'x8' the
    // variables base 's0'
    constexpr size_t register_count{32};
    constexpr size_t sp_index{2};
    constexpr size_t s0_index{8};

    // note: 2032 is where 'start' puts the variables base past 'dat'
    constexpr size_t variables_base_offset{2032};

    machine_rv32i_linux backend{&std::cout, test_builtins};
    backend.start();

    hand_written(backend, "    addi sp, sp, -128\n    sw sp, 124(sp)");

    // a call keeps only allocated registers, the variables base s0 is
    // already allocated
    std::vector<operand> live;
    for (size_t index{1}; index < register_count; ++index) {
        if (index != sp_index and index != s0_index) {
            live.push_back(backend.alloc_named_register(
                token{}, 0, assembler_rv32i::register_names.at(index),
                integer));
            hand_written(backend,
                         std::format("    li x{}, {}", index, 100 + index));
        }
    }

    backend.call_function(token{}, 1, "outer",
                          operand::mem("s0", {}, 1, 4096, integer), operand{});

    // every register after the call is saved and compared with what it held
    for (size_t index{1}; index < register_count; ++index) {
        if (index != sp_index) {
            hand_written(backend, std::format("    sw x{}, {}(sp)", index,
                                              (index - 1) * 4));
        }
    }

    hand_written(backend, "    lw t0, 124(sp)\n    beq t0, sp, 1f\n"
                          "    j call_failure\n1:");

    for (size_t index{1}; index < register_count; ++index) {
        if (index == sp_index) {
            continue;
        }

        hand_written(backend,
                     std::format("    lw t0, {}(sp)", (index - 1) * 4));

        if (index == s0_index) {
            hand_written(backend,
                         std::format("    la t1, dat\n    addi t1, t1, {}",
                                     variables_base_offset));
        } else {
            hand_written(backend, std::format("    li t1, {}", 100 + index));
        }

        hand_written(backend, "    beq t0, t1, 1f\n    j call_failure\n1:");
    }

    hand_written(backend, "    addi sp, sp, 128");

    for (const operand& reg : live | std::views::reverse) {
        backend.free_named_register(token{}, 0, reg);
    }

    backend.end_main();
    backend.label(0, "call_failure");
    backend.exit(token{}, 1, operand::imm(1, integer));

    // note: 6128 is the variables base plus the 4096 of the frame of 'outer'
    backend.label(0, "outer");
    backend.reserve_frame_base();
    expect_frame_base(backend, 6128);
    backend.call_function(token{}, 1, "inner",
                          operand::mem("s1", {}, 1, 8192, integer), operand{});

    // the frame base is live here, so the call restores it
    expect_frame_base(backend, 6128);
    backend.return_function(1);
    backend.release_frame_base();

    // note: 14320 is the frame base of 'outer' plus 8192
    backend.label(0, "inner");
    backend.reserve_frame_base();
    expect_frame_base(backend, 14320);

    // callees never write the variables base s0
    for (size_t index{1}; index < register_count; ++index) {
        if (index != sp_index and index != s0_index) {
            hand_written(backend, std::format("    li x{}, -1", index));
        }
    }

    backend.return_function(1);
    backend.release_frame_base();
    backend.finish();
    write_with_hand_written(backend);
    std::println(".data\ndat:\nvars:\n    .zero 16384");
}

// 'frame-checks': 'check_frame_capacity' jumps to the overflow label exactly
// when the check is enabled and the frame at 'vars + offset' does not fit in
// the 256 bytes of 'vars', for every offset and size around the limit
auto generate_frame_checks() -> void {
    constexpr int vars_size_bytes{256};

    machine_rv32i_linux backend{&std::cout, test_builtins};
    backend.start();

    // 's3' is where the overflow handler continues
    const operand continuation{
        backend.alloc_named_register(token{}, 0, "s3", integer),
    };

    // 'a3' is set to 1 by the handler, it is compared with the expectation
    const auto expect_failed = [&](const size_t case_index,
                                   const bool failed) -> void {
        hand_written(backend,
                     std::format("    li a3, 0\nframe_result_{}:\n    li a4, {}\n"
                                 "    beq a3, a4, 1f\n    j frame_failure\n1:",
                                 case_index, failed ? 1 : 0));
    };

    size_t case_index{};
    for (const bool enabled : {false, true}) {
        for (const int offset : {-1, 0, 1, 255, 256, 257}) {
            for (const uint32_t size : {0U, 1U, 256U, 257U, UINT32_MAX}) {
                const bool failed{
                    enabled and
                        (offset < 0 or offset > vars_size_bytes or
                         size > static_cast<uint32_t>(vars_size_bytes - offset)),
                };

                const std::string size_label{
                    std::format("frame_size_{}", case_index),
                };

                hand_written(backend,
                             std::format("    la a0, vars\n    addi a0, a0, {}\n"
                                         "    la s3, frame_result_{}",
                                         offset, case_index));

                if (enabled) {
                    backend.check_frame_capacity(
                        token{}, 1, operand::mem("a0", {}, 1, 0, integer),
                        operand::symbol(size_label, integer));
                }

                expect_failed(case_index++, failed);
                backend.define_constant(size_label, size);
            }
        }
    }

    backend.free_named_register(token{}, 0, continuation);
    backend.end_main();

    // the backend jumps to the label, so it defines it
    backend.label(0, machine::frame_overflow_handler_label);
    hand_written(backend, "    li a3, 1\n    jr s3");
    backend.label(0, "frame_failure");
    backend.exit(token{}, 1, operand::imm(1, integer));
    backend.finish();
    write_with_hand_written(backend);
    std::println(".data\ndat:\nvars:\n    .zero 256\nvars.end:");
}

// 'long-loop': an array iteration whose body is larger than a branch reaches
// (8 KiB) still repeats, for strides on both sides of the immediate limits
auto generate_long_loop() -> void {
    // note: 3 iterations of 'foo'
    constexpr int64_t iteration_count{3};

    machine_rv32i_linux backend{&std::cout, test_builtins};
    backend.start();
    hand_written(backend, "    addi sp, sp, -16");

    for (const size_t stride : {4U, 2047U, 2048U, 4094U, 4095U, 8192U}) {
        const std::string loop_label{std::format("long_loop_{}", stride)};

        hand_written(backend, "    sw zero, 0(sp)\n    li s2, 0");
        backend.label(0, loop_label);

        // note: 2048 instructions are 8 KiB
        hand_written(backend,
                     "    .rept 2048\n    nop\n    .endr\n    addi s2, s2, 1");

        backend.foo_advance_iteration(token{}, 1, operand::reg("s2", integer),
                                      operand::mem("sp", {}, 1, 0, integer),
                                      stride,
                                      operand::imm(iteration_count, integer),
                                      loop_label);

        // the iterator advanced by the stride and the body's 1, the counter
        // reached the count
        hand_written(backend,
                     std::format("    li t0, {}\n    beq s2, t0, 1f\n"
                                 "    j long_loop_failure\n1:\n"
                                 "    lw t1, 0(sp)\n    li t0, 3\n"
                                 "    beq t1, t0, 1f\n    j long_loop_failure\n1:",
                                 iteration_count * (stride + 1)));
    }

    hand_written(backend, "    addi sp, sp, 16");
    backend.end_main();
    backend.label(0, "long_loop_failure");
    backend.exit(token{}, 1, operand::imm(1, integer));
    backend.finish();
    write_with_hand_written(backend);
    std::println(".data\ndat:\nvars:\n    .word 0");
}

// 'far-jumps' and 'far-jumps-optimized': loops, taken branches and skipped
// jumps around padding of 8 KiB (needs 'j') and 1.08 MiB (needs 'jump'),
// executed in every direction
auto generate_far_jumps(const assembler::jump_mode jumps) -> void {
    machine_rv32i_linux backend{&std::cout, test_builtins, {}, jumps};
    backend.start();

    // buffered output must come from the backend, so pad with 'xori' on a
    // register nothing reads
    const auto padding = [&](const size_t count) -> void {
        for (size_t i{}; i < count; ++i) {
            backend.unary(token{}, 1, unary_of('~'),
                          operand::reg("s4", integer));
        }
    };

    const auto branch_if = [&](const machine::comparison_operator operation,
                               const std::string_view target) -> void {
        // note: 15 is the iterator after 3 iterations of 4 and 1
        backend.compare_and_branch(token{}, 1, operand::reg("s2", integer),
                                   operand::imm(15, integer),
                                   {
                                       .operation{operation},
                                       .target{target},
                                       .branch_on_true{true},
                                   },
                                   {});
    };

    const operand stack{operand::reg("sp", integer)};
    const operand iterator{operand::reg("s2", integer)};
    const operand counter{operand::mem("sp", {}, 1, 0, integer)};

    backend.add_subtract(token{}, 1, op_of('-'), stack,
                         operand::imm(16, integer));

    // note: 2048 instructions are 8 KiB, 270000 are 1.08 MiB
    for (const size_t count : {2048U, 270000U}) {
        const std::string loop_label{std::format("far_loop_{}", count)};
        backend.copy_value(token{}, 1, counter, operand::imm(0, integer));
        backend.copy_value(token{}, 1, iterator, operand::imm(0, integer));

        backend.label(0, loop_label);
        padding(count);
        backend.add_subtract(token{}, 1, op_of('+'), iterator,
                             operand::imm(1, integer));

        backend.foo_advance_iteration(token{}, 1, iterator, counter, 4,
                                      operand::imm(3, integer), loop_label);

        branch_if(machine::comparison_operator::not_equal, "far_failure");

        const std::string taken_label{std::format("far_taken_{}", count)};

        branch_if(machine::comparison_operator::equal, taken_label);
        padding(count);
        backend.exit(token{}, 1, operand::imm(2, integer));
        backend.label(0, taken_label);

        const std::string skipped_label{std::format("far_skipped_{}", count)};

        backend.branch(1, skipped_label);
        padding(count);
        backend.exit(token{}, 1, operand::imm(3, integer));
        backend.label(0, skipped_label);
    }

    backend.add_subtract(token{}, 1, op_of('+'), stack,
                         operand::imm(16, integer));

    backend.end_main();
    padding(270000);
    backend.label(0, "far_failure");
    backend.exit(token{}, 1, operand::imm(1, integer));
    backend.finish();
    backend.write_assembly(std::cout);
    std::println(".data\ndat:\nvars:\n    .word 0");
}

// 'far-foo' and 'far-foo-optimized': a .baz program whose 'foo', 'if',
// 'break' and 'continue' jump over bodies of 8 KiB and 1.08 MiB
auto generate_far_foo(const assembler::jump_mode jumps) -> void {
    std::string source{"dat values = []{1, 2, 3, 4, 5}\nfunc main() {\n"
                       "    var pad = 0\n    var visits = 0\n"
                       "    var sum = 0\n"};

    // each 'pad = pad + 1' is 12 bytes, so 700 needs 'j' and 90000 needs
    // 'jump' for the jumps of 'foo', 'if', 'break' and 'continue'
    for (const size_t count : {700U, 90000U}) {
        std::string padding;
        for (size_t i{}; i < count; ++i) {
            padding += "        pad = pad + 1\n";
        }

        // 'continue' at 2 runs the padding and 'break' at 4 skips it, so
        // it runs for 1, 2 and 3
        source += std::format("    pad = 0\n    visits = 0\n    sum = 0\n"
                              "    foo values {{\n"
                              "        visits = visits + 1\n"
                              "        if e == 2 {{\n{}"
                              "            continue\n        }}\n"
                              "        if e == 4 {{\n"
                              "            break\n        }}\n"
                              "{}        sum = sum + e\n    }}\n"
                              "    if visits != 4 exit(1)\n"
                              "    if sum != 4 exit(2)\n"
                              "    if pad != {} exit(3)\n",
                              padding, padding, 3 * count);
    }
    source += "}\n";

    source_files files;
    files.add("test.baz", std::string{source});

    machine_rv32i_linux compiler{&std::cout, test_builtins, {}, jumps};

    program prg{compiler, files, vars_capacity_bytes, check_options{}};
    prg.build(std::cout);
}

// ============================================================================
// 10. the bounds check for every combination of its parameters
// ============================================================================

// one combination of the parameters of 'check_bounds'
struct bounds_case {
    bool upper;
    bool lower;
    bool allow_end;
    uint32_t size;
    int32_t index;
    int32_t count;
    bool slice;

    // a negative index (or slice count) violates the lower bound, a position
    // at the size (or past it when the end is allowed) the upper bound
    [[nodiscard]] auto violated() const -> bool {
        const int64_t top{int64_t{index} + (slice ? count : 0)};
        const bool below{index < 0 or (slice and count < 0)};
        const bool above{allow_end ? top > size : top >= size};

        return (lower and below) or (upper and above);
    }
};

// prints the code of one case: 'a0' holds the index and 'a1' the count, 's3'
// the address to continue at after the check, and 'a3' tells whether the
// check jumped to the panic label, which is compared with the expectation
auto emit_bounds_case(machine_rv32i& backend, const bounds_case& parameters,
                      const size_t case_index) -> void {
    std::println("    li a0, {}\n    li a1, {}\n    la s3, bounds_result_{}",
                 parameters.index, parameters.count, case_index);

    // a count found non-negative is remembered until allocated again
    const operand count_register{
        backend.alloc_named_register(token{}, 0, "a1", integer),
    };

    // the matrix also checks combinations the front end does not make
    backend.check_bounds(
        token{}, 1,
        {
            .checked{operand::reg("a0", integer)},
            .count{parameters.slice ? operand::reg("a1", integer) : operand{}},
            .array_count{parameters.size},
            .allow_end{parameters.allow_end},
        },
        {.upper{parameters.upper}, .lower{parameters.lower}, .with_line{}});

    backend.free_named_register(token{}, 0, count_register);

    std::println("    li a3, 0\nbounds_result_{}:\n    li a4, {}\n"
                 "    beq a3, a4, 1f\n    j bounds_failure\n1:",
                 case_index, parameters.violated() ? 1 : 0);

    // the check must not change its operands
    std::println("    li a4, {}\n    beq a0, a4, 1f\n    j bounds_failure\n1:\n"
                 "    li a4, {}\n    beq a1, a4, 1f\n    j bounds_failure\n1:",
                 parameters.index, parameters.count);
}

// 'bounds-matrix': 'check_bounds' reports a violation exactly when the index
// (or the slice end) is outside the size, for every combination of enabled
// checks, size, index and count
auto generate_bounds_matrix(machine_rv32i& backend) -> void {
    const operand continuation{
        backend.alloc_named_register(token{}, 0, "s3", integer),
    };

    size_t case_index{};
    for (const bool upper : {false, true}) {
        for (const bool lower : {false, true}) {
            for (const bool allow_end : {false, true}) {
                for (const uint32_t size :
                     {0U, 4U, uint32_t{INT32_MAX}, UINT32_MAX}) {
                    for (const int32_t index :
                         {INT32_MIN, -1, 0, 3, 4, 5, INT32_MAX}) {
                        for (const int32_t count :
                             {INT32_MIN, -1, 0, 1, 4, INT32_MAX}) {
                            for (const bool slice : {false, true}) {
                                emit_bounds_case(backend,
                                                 {
                                                     .upper{upper},
                                                     .lower{lower},
                                                     .allow_end{allow_end},
                                                     .size{size},
                                                     .index{index},
                                                     .count{count},
                                                     .slice{slice},
                                                 },
                                                 case_index++);
                            }
                        }
                    }
                }
            }
        }
    }

    backend.free_named_register(token{}, 0, continuation);
    backend.end_main();
    std::println("baz_bounds_panic:\n    li a3, 1\n    jr s3\nbounds_failure:");
    backend.exit(token{}, 1, operand::imm(1, integer));
}

auto generate_bounds() -> void {
    machine_rv32i_linux backend{&std::cout, test_builtins};
    std::println(".option norvc\n.option norelax\n.text\n.globl _start\n_start:");

    generate_bounds_matrix(backend);

    backend.finish();
}

// ============================================================================
// 11. runtime test: the program 'test-rv32i.sh' runs under QEMU
//
// The driver calls the backend to emit code for each case, and prints before
// and after it the hand-written lines that load the inputs and compare the
// results with values computed here. A mismatch jumps to 'failure'.
// ============================================================================

// prints the check of one result: continues when 'reg' holds 'value', jumps
// to 'failure' otherwise
auto emit_expect(const std::string_view reg, const int64_t value,
                 const std::string_view scratch = "a3") -> void {
    std::println("    li {}, {}\n    beq {}, {}, 1f\n    j failure\n1:",
                 scratch, value, reg, scratch);
}

// where the second operand of a runtime case is: 'a1', the word at
// 'buffer + 4' that holds the same value, or a constant
enum class source_place : uint8_t { reg, memory, constant };

constexpr std::array source_places{
    source_place::reg,
    source_place::memory,
    source_place::constant,
};

// the second operand of a runtime case of 'value' at 'place'; 'a2' holds
// 'buffer'
auto source_at(const source_place place, const int32_t value,
               const type& reg_type) -> operand {
    if (place == source_place::memory) {
        return operand::mem("a2", {}, 1, 4, reg_type);
    }

    if (place == source_place::constant) {
        return operand::imm(value, integer);
    }

    return operand::reg("a1", reg_type);
}

// the destination of a runtime case of 'value_type': 'a0' or the word at
// 'buffer'
auto destination_at(const bool in_memory, const type& value_type) -> operand {
    return in_memory ? operand::mem("a2", {}, 1, 0, value_type)
                     : operand::reg("a0", value_type);
}

// copies of 0 to 24 bytes, with a known size and a run-time count, between
// addresses at every alignment: only the copied bytes change, and both
// pointers are left unchanged
auto emit_copy_tests(machine_rv32i& backend) -> void {
    // note: the source bytes are 128 and up at 'sp', the destination bytes 85
    //       at 'sp + 32', the copy starts 4 bytes into each
    constexpr size_t area_bytes{32};
    constexpr int fill_byte{85};

    for (const bool counted : {false, true}) {
        for (size_t size_bytes{}; size_bytes <= 24; ++size_bytes) {
            for (size_t alignment{}; alignment < 4; ++alignment) {
                // displacements on both sides of the 12-bit offsets
                const int displacement{
                    std::array{-2048, 0, 2047, 2048}.at(alignment),
                };

                std::println("    addi sp, sp, -64");

                for (size_t offset{}; offset < area_bytes; ++offset) {
                    std::println("    li a2, {}\n    sb a2, {}(sp)\n"
                                 "    li a2, {}\n    sb a2, {}(sp)",
                                 128 + offset, offset, fill_byte,
                                 area_bytes + offset);
                }

                // note: +4 is the start of the copy in the destination
                std::println("    addi a0, sp, {}\n    addi a1, sp, {}",
                             alignment, area_bytes + 4 + alignment);

                // both addresses are 'alignment' bytes past a word boundary
                const size_t known_alignment{offset_alignment(alignment, 4)};

                if (counted) {
                    backend.copy_elements(
                        token{}, 1, 1,
                        [&](const operand& count) -> void {
                            backend.copy_value(
                                token{}, 1, count,
                                operand::imm(size_bytes, integer));
                        },
                        {
                            .alignment{known_alignment},
                            .src{fixed_address(
                                operand::mem("a0", {}, 1, 0, byte))},
                            .dst{fixed_address(
                                operand::mem("a1", {}, 1, 0, byte))},
                        });
                } else {
                    std::println(
                        "    li a2, {}\n    sub a0, a0, a2\n    sub a1, a1, a2",
                        displacement);
                    backend.copy(token{}, 1,
                                 operand::mem("a1", {}, 1, displacement, byte),
                                 operand::mem("a0", {}, 1, displacement, byte),
                                 size_bytes, known_alignment);
                    std::println(
                        "    li a2, {}\n    add a0, a0, a2\n    add a1, a1, a2",
                        displacement);
                }

                for (size_t offset{}; offset < area_bytes; ++offset) {
                    const size_t start{4 + alignment};
                    const bool is_copied{
                        offset >= start and offset < start + size_bytes,
                    };

                    // note: -4 because the copy starts 4 bytes into the
                    //       destination
                    const int expected{
                        is_copied ? static_cast<int>(128 + offset - 4)
                                  : fill_byte,
                    };

                    std::println("    lbu a2, {}(sp)", area_bytes + offset);
                    emit_expect("a2", expected);
                }

                std::println("    addi a2, sp, {}\n    beq a0, a2, 1f\n"
                             "    j failure\n1:\n    addi a2, sp, {}\n"
                             "    beq a1, a2, 1f\n    j failure\n1:\n"
                             "    addi sp, sp, 64",
                             alignment, area_bytes + 4 + alignment);
                backend.finish();
            }
        }
    }
}

// '/' and '%' on every value width, destination in a register or memory, and
// source in a register, memory or constant: the result is the C++ one at the
// width, and a register source is left unchanged
auto emit_division_tests(machine_rv32i& backend) -> void {
    for (const type* value_type : {&integer, &half, &byte}) {
        for (const bool in_memory : {false, true}) {
            for (const int32_t initial :
                 {0, 1, -1, 7, -7, -128, -32768, 32767, INT32_MIN, INT32_MAX}) {
                for (const int32_t divisor :
                     {1, -1, 2, -2, 3, -7, 255, 65536, INT32_MIN, INT32_MAX}) {
                    for (const source_place place : source_places) {
                        for (const char operation : {'/', '%'}) {
                            std::println("    la a2, buffer\n    li a1, {}\n"
                                         "    sw a1, 4(a2)",
                                         divisor);

                            const operand dst{
                                destination_at(in_memory, *value_type),
                            };

                            backend.copy_value(token{}, 1, dst,
                                               operand::imm(initial, integer));
                            backend.divide(token{}, 1, op_of(operation), dst,
                                           source_at(place, divisor, integer),
                                           machine::runtime_check{});
                            backend.copy_value(token{}, 1,
                                               operand::reg("a0", integer),
                                               dst);

                            const int64_t dividend{
                                wrapped(static_cast<uint32_t>(initial),
                                        *value_type),
                            };

                            const int64_t result{
                                operation == '/' ? dividend / divisor
                                                 : dividend % divisor,
                            };

                            emit_expect("a0",
                                        wrapped(static_cast<uint32_t>(result),
                                                *value_type));

                            if (place == source_place::reg) {
                                emit_expect("a1", divisor);
                            }

                            backend.finish();
                        }
                    }
                }
            }
        }
    }
}

// the helper call of '*', '/' and '%' on memory operands keeps every
// register the program holds, including 'ra', and the stack pointer
auto emit_helper_call_register_preservation(machine_rv32i& backend) -> void {
    for (const char operation : {'*', '/', '%'}) {
        std::vector<operand> live;
        for (const std::string_view name :
             {"ra", "a0", "a1", "t0", "t1", "t2", "t3", "t4"}) {
            live.push_back(
                backend.alloc_named_register(token{}, 0, name, integer));
            std::println("    li {}, {}", name, 100 + live.size());
        }

        // note: -17 and 3 at 'sp' and 'sp + 4'
        std::println("    addi sp, sp, -16\n    mv s2, sp\n    li a2, -17\n"
                     "    sw a2, 0(sp)\n    li a2, 3\n    sw a2, 4(sp)");

        const operand dst{operand::mem("s2", {}, 1, 0, integer)};
        const operand src{operand::mem("s2", {}, 1, 4, integer)};

        if (operation == '*') {
            backend.multiply(token{}, 1, dst, src,
                             machine::factor_use::read_only);
        } else {
            backend.divide(token{}, 1, op_of(operation), dst, src,
                           machine::runtime_check{});
        }

        for (const auto [index, reg] : std::views::enumerate(live)) {
            emit_expect(reg.base_register(), 101 + index, "a2");
        }

        // -17 * 3, -17 / 3 and -17 % 3
        const int expected{operation == '*' ? -51 : operation == '/' ? -5 : -2};

        std::println("    lw a2, 0(sp)\n    li a3, {}\n    beq a2, a3, 1f\n"
                     "    j failure\n1:\n    beq sp, s2, 1f\n    j failure\n1:\n"
                     "    addi sp, sp, 16",
                     expected);

        backend.free_named_registers(token{}, 0, live);
        backend.finish();
    }
}

// '/' and '%' at the same address: a value by itself is 1 with no remainder
auto emit_division_same_address(machine_rv32i& backend) -> void {
    for (const char operation : {'/', '%'}) {
        std::println("    la t0, buffer\n    li a0, -17\n    sw a0, 0(t0)");

        const operand address{operand::mem("t0", {}, 1, 0, integer)};

        backend.divide(token{}, 1, op_of(operation), address, address,
                       machine::runtime_check{});
        std::println("    lw a0, 0(t0)");
        emit_expect("a0", operation == '/' ? 1 : 0);
        backend.finish();
    }
}

// a factor of a runtime case: where it is, and whether the multiplication
// may overwrite its register
struct factor_form {
    source_place place;
    machine::factor_use use;
};

// '*' on every value width, product in a register or memory, and factor in
// a register, memory, constant or a register the multiplication may reuse
auto emit_multiplication_tests(machine_rv32i& backend) -> void {
    constexpr std::array factors{
        factor_form{
            .place{source_place::reg},
            .use{machine::factor_use::read_only},
        },
        factor_form{
            .place{source_place::memory},
            .use{machine::factor_use::read_only},
        },
        factor_form{
            .place{source_place::constant},
            .use{machine::factor_use::read_only},
        },
        factor_form{
            .place{source_place::reg},
            .use{machine::factor_use::may_overwrite},
        },
    };

    for (const type* value_type : {&integer, &half, &byte}) {
        for (const bool in_memory : {false, true}) {
            for (const int32_t initial :
                 {0, 1, -1, 7, -128, 32767, INT32_MIN, INT32_MAX}) {
                for (const int32_t multiplier :
                     {0, 1, -1, 2, 3, 12, -7, 255, INT32_MIN, INT32_MAX}) {
                    for (const factor_form& factor : factors) {
                        std::println("    la a2, buffer\n    li a1, {}\n"
                                     "    sw a1, 4(a2)",
                                     multiplier);

                        const operand product{
                            destination_at(in_memory, *value_type),
                        };

                        backend.copy_value(token{}, 1, product,
                                           operand::imm(initial, integer));
                        backend.multiply(
                            token{}, 1, product,
                            source_at(factor.place, multiplier, integer),
                            factor.use);
                        backend.copy_value(token{}, 1,
                                           operand::reg("a0", integer),
                                           product);

                        const uint32_t expected{
                            static_cast<uint32_t>(initial) *
                                static_cast<uint32_t>(multiplier),
                        };

                        emit_expect("a0", wrapped(expected, *value_type));

                        if (factor.place == source_place::reg and
                            factor.use == machine::factor_use::read_only) {
                            emit_expect("a1", multiplier);
                        }

                        backend.finish();
                    }
                }
            }
        }
    }
}

// '*' of a value by itself at the same address, with and without the
// permission to overwrite the operand
auto emit_multiplication_aliased_operands(machine_rv32i& backend) -> void {
    for (const machine::factor_use use :
         {machine::factor_use::read_only, machine::factor_use::may_overwrite}) {
        std::println("    la a2, buffer\n    li a0, -7\n    sw a0, 0(a2)");

        const operand address{operand::mem("a2", {}, 1, 0, integer)};

        backend.multiply(token{}, 1, address, address, use);
        std::println("    lw a0, 0(a2)");
        emit_expect("a0", 49);
        backend.finish();
    }
}

// 'scale_index' multiplies an index register by a constant scale, wrapping
// at 32 bits
auto emit_index_scaling_tests(machine_rv32i& backend) -> void {
    // note: -7 as the bits of a register
    constexpr uint32_t minus_seven{0xfffffff9};

    for (const size_t scale :
         {size_t{0}, size_t{1}, size_t{2}, size_t{3}, size_t{7}, size_t{12},
          size_t{256}, size_t{4097}}) {
        std::println("    li a0, -7");
        backend.scale_index(token{}, 1, operand::reg("a0", integer), scale);

        emit_expect("a0", std::bit_cast<int32_t>(
                              minus_seven * static_cast<uint32_t>(scale)));

        backend.finish();
    }
}

// '<' on a memory left operand, with the right operand in a register or at
// the same base as the left one
auto emit_memory_comparison_tests(machine_rv32i& backend) -> void {
    for (const bool shared_address : {false, true}) {
        std::println("    la a0, buffer\n    li a1, -257\n    sw a1, 0(a0)\n"
                     "    li a1, 257\n    sw a1, 4(a0)");

        backend.compare_and_branch(
            token{}, 1, operand::mem("a0", {}, 1, 0, integer),
            shared_address ? operand::mem("a0", {}, 1, 4, integer)
                           : operand::reg("a1", integer),
            {
                .operation{machine::comparison_operator::less},
                .destination{operand::reg("a0", boolean)},
            },
            {});

        emit_expect("a0", 1);
        backend.finish();
    }
}

// whether 'operation' holds between 'lhs' and 'rhs'
auto holds(const machine::comparison_operator operation, const int32_t lhs,
           const int32_t rhs) -> bool {
    using comparison = machine::comparison_operator;

    if (operation == comparison::equal) {
        return lhs == rhs;
    }
    if (operation == comparison::not_equal) {
        return lhs != rhs;
    }
    if (operation == comparison::less) {
        return lhs < rhs;
    }
    if (operation == comparison::greater_equal) {
        return lhs >= rhs;
    }
    if (operation == comparison::greater) {
        return lhs > rhs;
    }

    assert(operation == comparison::less_equal);

    return lhs <= rhs;
}

// where the boolean result of a comparison of the matrix goes
enum class result_place : uint8_t { none, a0, a1, memory };

// the places of the operands and the result of a comparison of the matrix
struct comparison_layout {
    // both operands at 'buffer', else in 'a0' and 'a1'
    bool memory_operands;
    // the right operand a constant
    bool constant_right;
    result_place result;
    bool branches;
};

// every comparison operator against the value -7, inverted or not, branching
// on true or on false, in every layout of operands and result
auto emit_comparison_matrix(machine_rv32i& backend) -> void {
    using comparison = machine::comparison_operator;

    constexpr std::array layouts{
        comparison_layout{
            .memory_operands{},
            .constant_right{},
            .result{result_place::none},
            .branches{true},
        },
        comparison_layout{
            .memory_operands{},
            .constant_right{},
            .result{result_place::a0},
            .branches{true},
        },
        // the result in the register of the right operand
        comparison_layout{
            .memory_operands{},
            .constant_right{},
            .result{result_place::a1},
            .branches{true},
        },
        comparison_layout{
            .memory_operands{},
            .constant_right{},
            .result{result_place::memory},
            .branches{true},
        },
        comparison_layout{
            .memory_operands{},
            .constant_right{},
            .result{result_place::a0},
            .branches{},
        },
        comparison_layout{
            .memory_operands{true},
            .constant_right{},
            .result{result_place::memory},
            .branches{true},
        },
        comparison_layout{
            .memory_operands{},
            .constant_right{true},
            .result{result_place::a0},
            .branches{true},
        },
        comparison_layout{
            .memory_operands{},
            .constant_right{true},
            .result{result_place::memory},
            .branches{true},
        },
    };

    constexpr int32_t left_value{-7};

    size_t comparison_index{};
    for (const comparison operation :
         {comparison::equal, comparison::not_equal, comparison::less,
          comparison::greater_equal, comparison::greater,
          comparison::less_equal}) {
        for (const bool inverted : {false, true}) {
            for (const bool branch_on_true : {false, true}) {
                // values on both sides of the 12-bit immediates
                for (const int32_t right_value :
                     {INT32_MIN, -2049, -2048, -9, -7, 0, 3, 2046, 2047, 2048,
                      INT32_MAX}) {
                    for (const comparison_layout& layout : layouts) {
                        const std::string target{
                            std::format("comparison_{}", comparison_index++),
                        };

                        const bool expected{
                            holds(operation, left_value, right_value) !=
                                inverted,
                        };

                        operand result;
                        if (layout.result == result_place::a0) {
                            result = operand::reg("a0", boolean);
                        } else if (layout.result == result_place::a1) {
                            result = operand::reg("a1", boolean);
                        } else if (layout.result == result_place::memory) {
                            result = operand::mem("a2", {}, 1, 8, boolean);
                        }

                        std::println("    li a0, {}\n    li a1, {}\n"
                                     "    la a2, buffer\n    sw a0, 0(a2)\n"
                                     "    sw a1, 4(a2)",
                                     left_value, right_value);

                        const operand lhs{
                            layout.memory_operands
                                ? operand::mem("a2", {}, 1, 0, integer)
                                : operand::reg("a0", integer),
                        };

                        operand rhs{operand::reg("a1", integer)};
                        if (layout.memory_operands) {
                            rhs = operand::mem("a2", {}, 1, 4, integer);
                        } else if (layout.constant_right) {
                            rhs = operand::imm(right_value, integer);
                        }

                        backend.compare_and_branch(
                            token{}, 1, lhs, rhs,
                            {
                                .operation{operation},
                                .inverted{inverted},
                                .destination{result},
                                .target{layout.branches ? std::string_view{target}
                                                        : std::string_view{}},
                                .branch_on_true{branch_on_true},
                            },
                            {});

                        // the branch is taken exactly when expected
                        if (layout.branches) {
                            if (expected == branch_on_true) {
                                std::println("    j failure\n{}:", target);
                            } else {
                                std::println("    j {}_done\n{}:\n"
                                             "    j failure\n{}_done:",
                                             target, target, target);
                            }
                        }

                        if (not result.is_empty()) {
                            backend.copy_value(token{}, 1,
                                               operand::reg("a3", integer),
                                               result);
                            emit_expect("a3", expected ? 1 : 0, "a4");
                        }

                        backend.finish();
                    }
                }
            }
        }
    }
}

// 'zero' of 1 to 35 bytes at every alignment and at base offsets on both
// sides of the 12-bit limit: exactly the range becomes zero
auto emit_zeroing_tests(machine_rv32i& backend) -> void {
    constexpr size_t area_bytes{64};

    for (size_t count{1}; count <= 35; ++count) {
        for (size_t alignment{}; alignment < 4; ++alignment) {
            std::println("    addi sp, sp, -64\n    li a0, -1");
            for (size_t offset{}; offset < area_bytes; offset += 4) {
                std::println("    sw a0, {}(sp)", offset);
            }

            // note: +4 keeps the zeroing inside the area at every alignment
            const size_t start{4 + alignment};
            const size_t displacement{count % 2 == 0 ? 2047U : 2048U};

            std::println("    li a2, {}\n    sub a2, sp, a2",
                         displacement - start);

            backend.zero(token{}, 1,
                         operand::mem("a2", {}, 1,
                                      static_cast<int64_t>(displacement), byte),
                         count, offset_alignment(start, 4));

            for (size_t offset{}; offset < area_bytes; ++offset) {
                const bool is_zeroed{offset >= start and offset < start + count};
                std::println("    lbu a0, {}(sp)", offset);
                emit_expect("a0", is_zeroed ? 0 : 255, "a1");
            }

            std::println("    addi sp, sp, 64");
            backend.finish();
        }
    }
}

// unary '-' and '~' on every integer width, in a register or memory
auto emit_unary_tests(machine_rv32i& backend) -> void {
    for (const type* value_type : {&integer, &half, &byte}) {
        for (const bool in_memory : {false, true}) {
            for (const int32_t initial : {0, 1, -1, -128, -32768, INT32_MIN}) {
                for (const char operation : {'-', '~'}) {
                    std::println("    la a2, buffer");

                    const operand dst{destination_at(in_memory, *value_type)};

                    backend.copy_value(token{}, 1, dst,
                                       operand::imm(initial, integer));
                    backend.unary(token{}, 1, unary_of(operation), dst);
                    backend.copy_value(token{}, 1, operand::reg("a0", integer),
                                       dst);

                    const uint32_t bits{static_cast<uint32_t>(initial)};
                    const uint32_t result{
                        operation == '-' ? uint32_t{} - bits : ~bits,
                    };

                    emit_expect("a0", wrapped(result, *value_type));

                    backend.finish();
                }
            }
        }
    }
}

// what 'operation' gives for 'lhs' and 'rhs', as the bits of 32-bit values
auto applied(const char operation, const uint32_t lhs, const uint32_t rhs)
    -> uint32_t {
    if (operation == '+') {
        return lhs + rhs;
    }
    if (operation == '-') {
        return lhs - rhs;
    }
    if (operation == '&') {
        return lhs & rhs;
    }
    if (operation == '|') {
        return lhs | rhs;
    }

    assert(operation == '^');

    return lhs ^ rhs;
}

// '+', '-', '&', '|' and '^' on every value width, destination in a register
// or memory, and source in a register, memory or constant around the 12-bit
// immediate limits: the result wraps at the width of the destination
auto emit_add_subtract_bitwise_tests(machine_rv32i& backend) -> void {
    constexpr int32_t initial{127};

    for (const type* value_type : {&integer, &half, &byte}) {
        for (const char operation : {'+', '-', '&', '|', '^'}) {
            for (const bool in_memory : {false, true}) {
                for (const source_place place : source_places) {
                    for (const int32_t source_value :
                         {-2049, -2048, -1, 0, 1, 2047, 2048}) {
                        const size_t bits{value_type->size_bytes() * 8};
                        const uint32_t mask{UINT32_MAX >> (32 - bits)};

                        // folding removes the constants that keep the
                        // destination
                        const uint32_t kept{operation == '&' ? mask : 0U};
                        if (place == source_place::constant and
                            (static_cast<uint32_t>(source_value) & mask) ==
                                kept) {
                            continue;
                        }

                        std::println("    la a2, buffer\n    li a1, {}\n"
                                     "    sw a1, 4(a2)",
                                     source_value);

                        const operand dst{
                            destination_at(in_memory, *value_type),
                        };
                        const operand src{
                            source_at(place, source_value, integer),
                        };

                        backend.copy_value(token{}, 1, dst,
                                           operand::imm(initial, integer));

                        if (operation == '+' or operation == '-') {
                            backend.add_subtract(token{}, 1, op_of(operation),
                                                 dst, src);
                        } else {
                            backend.bitwise(token{}, 1, op_of(operation), dst,
                                            src);
                        }

                        backend.copy_value(token{}, 1,
                                           operand::reg("a0", integer), dst);

                        const uint32_t result{
                            applied(operation, initial,
                                    static_cast<uint32_t>(source_value)),
                        };

                        emit_expect("a0", wrapped(result, *value_type));

                        backend.finish();
                    }
                }
            }
        }
    }
}

// '<<' and '>>' on every value width (and 'bool'), with the count 35 in a
// register or memory, which the hardware masks to 3, or the constant 3
auto emit_shift_tests(machine_rv32i& backend) -> void {
    for (const type* value_type : {&integer, &half, &byte, &boolean}) {
        for (const char operation : {'<', '>'}) {
            for (const bool in_memory : {false, true}) {
                for (const source_place place : source_places) {
                    std::println(
                        "    la a2, buffer\n    li a1, 35\n    sw a1, 4(a2)");

                    const operand dst{destination_at(in_memory, *value_type)};

                    backend.copy_value(token{}, 1, dst,
                                       operand::imm(-16, integer));
                    backend.shift(token{}, 1, op_of(operation), dst,
                                  source_at(place, 3, byte),
                                  machine::runtime_check{});
                    backend.copy_value(token{}, 1, operand::reg("a0", integer),
                                       dst);

                    // -16 is 240 in a 'bool', which does not extend its sign
                    const int expected{
                        value_type == &boolean ? (operation == '<' ? 128 : 30)
                                               : (operation == '<' ? -128 : -2),
                    };

                    emit_expect("a0", expected);

                    backend.finish();
                }
            }
        }
    }

    // the count is the shifted register itself
    std::println("    li a0, 3");
    backend.shift(token{}, 1, op_of('<'), operand::reg("a0", integer),
                  operand::reg("a0", integer), machine::runtime_check{});
    emit_expect("a0", 24);
    backend.finish();
}

// loads, stores and 'address_of' on memory at offsets around the 12-bit and
// 32-bit limits, with and without a scaled index register: the address is
// always 'buffer'
auto emit_address_range_tests(machine_rv32i& backend) -> void {
    // note: 5 in the index register 't5'
    constexpr uint64_t index_value{5};

    for (const int64_t offset :
         {INT64_C(-4294967295), INT64_C(-2147483648), INT64_C(-2049),
          INT64_C(-2048), INT64_C(2047), INT64_C(2048), INT64_C(8196),
          INT64_C(2147483648), INT64_C(4294967295)}) {
        // the base is 'buffer - offset'
        std::println("    la t6, buffer\n    li a3, {}\n    sub t6, t6, a3",
                     static_cast<uint32_t>(offset));

        const operand direct{operand::mem("t6", {}, 1, offset, integer)};

        backend.copy_value(token{}, 1, direct, operand::imm(42, integer));
        backend.copy_value(token{}, 1, operand::reg("a0", integer), direct);
        std::println("    li a3, 42\n    bne a0, a3, failure");
        backend.address_of(token{}, 1, operand::reg("t6", integer), direct);
        std::println("    la a3, buffer\n    bne t6, a3, failure");
        backend.finish();

        for (const uint64_t scale :
             std::array<uint64_t, 11>{1, 2, 4, 8, 16, 32, 64, 128, 256, 65536,
                                      UINT64_C(2147483648)}) {
            // the base is 'buffer - offset - index * scale'
            const std::string base_setup{
                std::format("    la t6, buffer\n    li a3, {}\n"
                            "    sub t6, t6, a3\n    li t5, {}\n    li a3, {}\n"
                            "    sub t6, t6, a3",
                            static_cast<uint32_t>(offset), index_value,
                            static_cast<uint32_t>(index_value * scale)),
            };

            std::println("{}\n    li t4, 42", base_setup);

            const operand address{
                operand::mem("t6", "t5", scale, offset, integer),
            };

            backend.copy_value(token{}, 1, address,
                               operand::reg("t4", integer));
            backend.copy_value(token{}, 1, operand::reg("t3", integer),
                               address);
            std::println("    li a3, 42\n    bne t3, a3, failure");
            backend.address_of(token{}, 1, operand::reg("t3", integer),
                               address);
            std::println("    la a3, buffer\n    bne t3, a3, failure");

            // a copy from memory to memory
            std::println("    la a4, buffer_copy");
            backend.copy_value(token{}, 1,
                               operand::mem("a4", {}, 1, 0, integer), address);
            backend.copy_value(token{}, 1, operand::reg("t3", integer),
                               operand::mem("a4", {}, 1, 0, integer));
            std::println("    li a3, 42\n    bne t3, a3, failure");

            // the address stored in memory
            std::println("{}", base_setup);
            std::println("    la a4, pointer");
            backend.address_of(token{}, 1,
                               operand::mem("a4", {}, 1, 0, integer), address);
            backend.copy_value(token{}, 1, operand::reg("t3", integer),
                               operand::mem("a4", {}, 1, 0, integer));
            std::println("    la a3, buffer\n    bne t3, a3, failure");
            backend.finish();
        }
    }
}

// 'address_of' with base, scaled index and offset in every combination of
// destination and base register, including a destination that is the base or
// the index and bases that are 'zero'
auto emit_address_of_register_tests(machine_rv32i& backend) -> void {
    // note: 'a2' holds 100, 'a3' the index 5 and 'a4' 'buffer'
    constexpr uint32_t a2_value{100};
    constexpr uint32_t index_value{5};
    constexpr uint32_t offset{40};

    for (const uint64_t scale :
         {UINT64_C(1), UINT64_C(2), UINT64_C(4), UINT64_C(256), UINT64_C(65536),
          UINT64_C(2147483648)}) {
        for (const std::string_view base : {"a2", "a3", "a4", "zero"}) {
            for (const std::string_view dst : {"a2", "a3", "a4"}) {
                std::println("    li a2, {}\n    li a3, {}\n    la a4, buffer",
                             a2_value, index_value);

                backend.address_of(
                    token{}, 1, operand::reg(dst, integer),
                    operand::mem(base, "a3", scale, offset, integer));

                const uint32_t displacement{
                    static_cast<uint32_t>(index_value * scale + offset),
                };

                if (base == "a4") {
                    std::println(
                        "    la a5, buffer\n    li a6, {}\n    add a5, a5, a6",
                        displacement);
                } else {
                    const uint32_t base_value{
                        base == "a2"   ? a2_value
                        : base == "a3" ? index_value
                                       : 0U,
                    };

                    std::println("    li a5, {}", displacement + base_value);
                }

                std::println("    beq {}, a5, 1f\n    j failure\n1:", dst);
                backend.finish();
            }
        }
    }
}

// stores of -1 to narrow memory cells read back as -1 (as 255 for 'bool'), and
// an address that uses 'sp' as base and index
auto emit_narrow_memory_tests(machine_rv32i& backend) -> void {
    for (const type* value_type : {&byte, &half, &boolean}) {
        std::println("    la a2, buffer");
        backend.copy_value(token{}, 1,
                           operand::mem("a2", {}, 1, 0, *value_type),
                           operand::imm(-1, integer));
        backend.copy_value(token{}, 1, operand::reg("a0", integer),
                           operand::mem("a2", {}, 1, 0, *value_type));
        std::println("    li a1, {}\n    bne a0, a1, failure",
                     value_type == &boolean ? 255 : -1);
    }

    backend.address_of(token{}, 1, operand::reg("t6", integer),
                       operand::mem("sp", "sp", 4, 2048, integer));
    std::println("    slli a0, sp, 2\n    add a0, a0, sp\n    li a1, 2048\n"
                 "    add a0, a0, a1\n    bne t6, a0, failure");
}

// 'read' and 'write' system calls: results, argument registers, the stack
// pointer, and the rejection of a held syscall register. Input comes from
// stdin and is written to stdout, which 'test-rv32i.sh' compares.
auto emit_io_tests(machine_rv32i& backend) -> void {
    // note: -9 is '-EBADF', the result for the descriptor -1
    constexpr int bad_descriptor_result{-9};

    std::vector<operand> io_args;
    for (const std::string_view name :
         backend.registers_for_builtin_function(machine::builtin_function::read)
             .arguments) {
        io_args.push_back(
            backend.alloc_named_register(token{}, 0, name, integer));
    }

    const auto read = [&] {
        backend.read(token{}, 1, io_args.at(0), io_args.at(0), io_args.at(1),
                     io_args.at(2));
    };

    const auto write = [&] {
        backend.write(token{}, 1, io_args.at(0), io_args.at(0), io_args.at(1),
                      io_args.at(2));
    };

    // 6 bytes from stdin to 'buffer' and back to stdout
    std::println("    mv s2, sp\n    li a0, 0\n    la a1, buffer\n    li a2, 6");
    read();
    std::println("    li t0, 6\n    bne a0, t0, failure\n"
                 "    bne a2, t0, failure\n    la t0, buffer\n"
                 "    bne a1, t0, failure\n    bne sp, s2, failure\n"
                 "    li a0, 1");
    write();
    std::println("    li t0, 6\n    bne a0, t0, failure\n"
                 "    bne a2, t0, failure\n    la t0, buffer\n"
                 "    bne a1, t0, failure\n    li a0, -1");

    // a bad descriptor is reported in the result
    read();
    std::println("    li t0, {}\n    bne a0, t0, failure\n    li a0, -1",
                 bad_descriptor_result);
    write();
    std::println("    li t0, {}\n    bne a0, t0, failure\n"
                 "    bne sp, s2, failure",
                 bad_descriptor_result);

    backend.free_named_registers(token{}, 0, io_args);

    // 'a7' selects the system call, it cannot be held by the caller
    const operand held_syscall_register{
        backend.alloc_named_register(source_tk, 0, "a7", integer),
    };

    assert(rejected_with([&] {
        backend.read(source_tk, 1, operand::reg("a0", integer),
                     operand::reg("a0", integer), operand::reg("a1", integer),
                     operand::reg("a2", integer));
    }));

    backend.free_named_register(token{}, 0, held_syscall_register);
}

// the whole program: the header, the cases of the sections above in order,
// the end of 'main', and the entry points that 'test-rv32i.sh' starts
// separately (division by zero, bounds failures) with the failure path
auto generate_runtime_program() -> void {
    machine_rv32i_linux backend{&std::cout, test_builtins};

    std::println(".option norvc\n.option norelax\n.text\n.globl _start\n_start:");

    emit_copy_tests(backend);
    emit_division_tests(backend);
    emit_helper_call_register_preservation(backend);
    emit_division_same_address(backend);
    emit_multiplication_tests(backend);
    emit_multiplication_aliased_operands(backend);
    emit_index_scaling_tests(backend);
    emit_memory_comparison_tests(backend);
    emit_comparison_matrix(backend);
    emit_zeroing_tests(backend);
    emit_unary_tests(backend);
    emit_add_subtract_bitwise_tests(backend);
    emit_shift_tests(backend);
    emit_address_range_tests(backend);
    emit_address_of_register_tests(backend);
    emit_narrow_memory_tests(backend);
    emit_io_tests(backend);

    // the end of the straight-line test; the symbols below are entered by
    // 'test-rv32i.sh' as separate programs
    backend.end_main();

    std::println(".globl divide_by_zero\ndivide_by_zero:\n    li a0, 17");
    backend.divide(token{}, 1, op_of('/'), operand::reg("a0", integer),
                   operand::imm(0, integer), machine::runtime_check{});
    backend.exit(token{}, 1, operand::imm(0, integer));

    // the line of a failed check is printed, also the largest one
    for (const uint32_t line : {0U, 9U, 123U, UINT32_MAX}) {
        std::println(".globl bounds_line_{}\nbounds_line_{}:\n    li a0, -1",
                     line, line);

        const token location{0, {}, 0, {}, 0, {}, line, false};

        backend.check_bounds(
            location, 1,
            machine::bounds_range::index(operand::reg("a0", integer), 4),
            {.upper{true}, .lower{true}, .with_line{true}});
        backend.exit(token{}, 1, operand::imm(0, integer));
    }

    backend.emit_bounds_failure_handler(true);
    std::println("failure:\n    li a0, 1\n    li a7, 93\n    ecall");
    backend.begin_data(4);
    std::println("buffer: .zero 16\nbuffer_copy: .word 0\npointer: .word 0");
    backend.finish();
}

// ============================================================================
// 12. host checks: the front end tables (scopes and storage)
// ============================================================================

// a 'toc' over an x86_64 backend with the builtin types, compiling, the global
// frame is entered
struct front_end {
    std::ostringstream output;
    source_files files;
    machine_x86_64 backend{&output, test_builtins, &files};
    toc tc;

    // errors need a token with a line
    token at{0, {}, 0, {}, 0, {}, 1, false};

    explicit front_end(const size_t vars_capacity_bytes_in)
        : tc{backend, files, vars_capacity_bytes_in, check_options{}} {

        files.add("test.baz", "");
        tc.begin_compile();
        tc.enter_block();
    }

    // a scalar or an array of 'array_len' elements of 'value_type', its
    // offset in the variables section
    auto add_var(const std::string_view name, const type& value_type,
                 const size_t array_len = 0) -> int64_t {

        tc.add_var(at, 0,
                   {
                       .name{name},
                       .type_ptr{&value_type},
                       .src_loc_tk{at},
                       .is_array{array_len != 0},
                       .array_len{array_len},
                   },
                   var_kind::var);

        return tc.make_ident_info(at, name).operand().displacement();
    }
};

// variables are placed in declaration order with the alignment of their type,
// and the usage keeps the most that was in use after the scope ends
auto check_storage_placement() -> void {
    front_end fe{64};

    fe.tc.enter_block();
    const int64_t first{fe.add_var("a", byte)};
    const int64_t second{fe.add_var("b", integer)};
    // 'a' takes one byte and 'b' starts at the next multiple of four
    assert(second - first == 4);
    fe.tc.exit_block();

    assert(fe.tc.usage().max_vars_size_bytes == 8);

    // the space is reused by the next scope
    fe.tc.enter_block();
    const int64_t reused{fe.add_var("c", integer)};
    assert(reused == first);
    fe.tc.exit_block();
    assert(fe.tc.usage().max_vars_size_bytes == 8);

    fe.tc.exit_block();
    fe.tc.assert_released();
}

// a variable beyond the capacity of the variables section is rejected, except
// in a dry run, which leaves no trace in the usage, the depth or the data
auto check_dry_run_leaves_no_trace() -> void {
    front_end fe{64};

    fe.tc.enter_block();
    fe.add_var("a", integer);
    const usage_statistics before{fe.tc.usage()};
    const size_t constants_before{fe.tc.data().constants().size()};

    // note: 16 elements of 8 bytes are twice the capacity
    assert(rejected_with([&] -> void { fe.add_var("big", integer64, 16); },
                         "does not fit in the variable storage"));

    fe.tc.check_only([&] -> void {
        fe.tc.enter_block();
        fe.tc.enter_block();
        fe.add_var("big", integer64, 16);
        std::ignore = fe.tc.add_bytes_constant(token{}, "dry run only");
        fe.tc.exit_block();
        fe.tc.exit_block();
    });

    const usage_statistics after{fe.tc.usage()};
    assert(after.max_vars_size_bytes == before.max_vars_size_bytes);
    assert(after.max_frame_count == before.max_frame_count);
    assert(fe.tc.data().constants().size() == constants_before);

    // the capacity is enforced again after the dry run
    assert(rejected_with([&] -> void { fe.add_var("big", integer64, 16); },
                         "does not fit in the variable storage"));

    fe.tc.exit_block();
    fe.tc.exit_block();
    fe.tc.assert_released();
}

// a constant is visible in its function and in the global frame, not in the
// function that called it
auto check_constant_scopes() -> void {
    front_end fe{64};

    fe.tc.add_const(token{}, 0, "global", folded_value::untyped(1));
    fe.tc.enter_func("outer");
    fe.tc.add_const(token{}, 0, "outer_only", folded_value::untyped(2));
    fe.tc.enter_func("inner", "path", "");
    assert(fe.tc.scopes().lookup("global").is_constant());
    assert(not fe.tc.scopes().lookup("outer_only").is_constant());
    fe.tc.exit_func("inner");
    assert(fe.tc.scopes().lookup("outer_only").is_constant());
    fe.tc.exit_func("outer");

    fe.tc.exit_block();
    fe.tc.assert_released();
}

// the host checks in the order of the parts
auto run_host_checks() -> void {
    // 1. the assembler
    check_removed_and_inverted_jumps();
    check_every_branch_inverts();
    check_jumps_kept_at_entries();
    check_far_branch_folds_and_grows();
    check_jumps_across_data();
    check_branch_reach();
    check_jump_reach();
    check_far_jump_needs_scratch();
    check_growth_moves_other_jumps();
    check_data_takes_no_code_offset();
    check_smaller_version_kept();
    check_nested_version_choice();
    check_jump_in_kept_version_grows();

    // 2. the machine
    check_line_sizes();
    check_equal_size_choice();
    check_version_state_kept();
    check_backend_jump_optimization();
    check_copy_to_same_address();
    check_variable_comments();
    check_comment_positions();
    check_allocation_comments();
    check_memory_operation_comments();
    check_rv32i_scratch_order();
    check_copy_helper_comments();

    // 3. compiled programs
    check_array_copy_pointers();
    check_arrays_equal_registers_rv32i();
    check_arrays_equal_condition_x86();
    check_equal_results_stored_directly();

    // 4. x86_64
    check_x86_scratch_registers();
    check_x86_syscall_register_saving();
    check_x86_write_in_program();
    check_x86_nested_syscalls_rejected();

    // 5. instruction shapes
    check_index_scale_support();
    check_memory_comparison_tail();
    check_known_size_copy();
    check_string_data_escapes();

    // 6. arithmetic on registers only
    check_shifts();
    check_add_subtract_bitwise();
    check_zero_operations_and_zero_shifts();
    check_narrow_values_and_large_constants();

    // 7. copies, comparisons, addresses and zeroing
    check_copy_widths();
    check_byte_bitwise();
    check_shift_of_variables();
    check_address_lowering();
    check_address_lowering_in_destination();
    check_address_forms_without_temporaries();
    check_constant_stores_and_small_zeroing();
    check_comparison_selection();
    check_multiply_by_constants();
    check_copy_temporaries();
    check_zeroing_ranges();
    check_invalid_addresses_rejected();
    check_scratch_register_pool();

    // 8. helper routines
    check_multiply_divide_routines();

    // 12. the front end tables
    check_storage_placement();
    check_dry_run_leaves_no_trace();
    check_constant_scopes();
}

} // namespace

auto main(const int argc, const char* argv[]) -> int {
    const std::string_view mode{argc > 1 ? argv[1] : ""};

    // special modes print one program and need no host checks
    if (mode == "noninline") {
        generate_noninline();
    } else if (mode == "frame-checks") {
        generate_frame_checks();
    } else if (mode == "long-loop") {
        generate_long_loop();
    } else if (mode == "far-jumps" or mode == "far-jumps-optimized") {
        generate_far_jumps(mode == "far-jumps"
                               ? assembler::jump_mode::resolved
                               : assembler::jump_mode::optimized);
    } else if (mode == "far-foo" or mode == "far-foo-optimized") {
        generate_far_foo(mode == "far-foo" ? assembler::jump_mode::resolved
                                           : assembler::jump_mode::optimized);
    } else if (mode == "bounds-matrix") {
        generate_bounds();
    } else {
        run_host_checks();
        generate_runtime_program();
    }

    return 0;
}
