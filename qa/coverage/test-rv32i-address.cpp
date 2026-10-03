#include <iostream>
#include <sstream>

#include "../../src/assembler_rv32i.hpp"
#include "../../src/decouple_impl.hpp" // IWYU pragma: keep
#include "../../src/machine_rv32i.hpp"
#include "../../src/machine_x86_64.hpp"
#include "../../src/program.hpp"

// ============================================================================
// test driver of the RV32I backend ('test-rv32i.sh' builds and runs it)
//
// The driver calls the backend API directly, so it also covers calls that no
// .baz source can produce. It runs in one of these ways, chosen by the first
// command line argument of 'main' at the end of the file:
//
//   (none)     the host checks (parts 1 to 8) assert on the emitted text, then
//              the runtime program (part 11) is printed on stdout, which the
//              script assembles and runs under QEMU; a wrong result jumps to
//              the label 'failure'
//   <mode>     print one special program on stdout instead (parts 9 and 10):
//              'noninline', 'frame-checks', 'long-loop', 'far-jumps',
//              'far-jumps-optimized', 'far-foo', 'far-foo-optimized', 'bulk',
//              'strings-syscall', 'bounds-matrix' and 'bounds-silent'
//
// Every check is a function named after what it tests, with a comment that
// says what the expected output shows.
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
//   10 special programs from .baz sources, and bounds checks
//   11 the runtime program
// ============================================================================

// instruction-shape tests must not depend on register diagnostic comments
class assembly_output : public std::ostringstream {
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

const type integer64{"i64", 8, true};
const type integer{"i32", 4, true};
const type half{"i16", 2, true};
const type byte{"i8", 1, true};
const type boolean{"bool", 1, true};

// the diagnostics of the rejected calls need a real position
const token source_tk{token::position(0, 1)};

// whether 'action' is rejected with a compiler error that contains 'text'
template <typename action_t>
auto rejected_with(const action_t& action, const std::string_view text = {})
    -> bool {
    try {
        action();
    } catch (const compiler_exception& error) {
        return std::string_view{error.what()}.contains(text);
    }

    return false;
}

// an RV32I backend whose output is captured, for checks of emitted text
struct captured_rv32i {
    assembly_output output;
    std::ostream stream{output.rdbuf()};
    machine_rv32i backend{stream};

    captured_rv32i() {
        backend.set_builtin_types(integer64, integer, half, byte);
    }
};

// allocates scratch registers so that a check proves it needs no more
auto hold_scratch_registers(machine_rv32i& backend, const size_t count,
                            const type& value_type) -> std::vector<operand> {
    std::vector<operand> held;
    for (size_t i{}; i < count; ++i) {
        held.push_back(backend.alloc_scratch_register(token{}, 0, value_type));
    }

    return held;
}

// ----------------------------------------------------------------------------
// helpers of the jump checks
// ----------------------------------------------------------------------------

// the branches that 'resolved_branch' grows
auto branch_op(const std::string_view mnemonic) -> assembler_rv32i::op {
    using op = assembler_rv32i::op;
    constexpr std::array<std::pair<std::string_view, op>, 10> branches{{
        {"beq", op::beq},
        {"bne", op::bne},
        {"blt", op::blt},
        {"bge", op::bge},
        {"bltu", op::bltu},
        {"bgeu", op::bgeu},
        {"bgt", op::bgt},
        {"ble", op::ble},
        {"bgtu", op::bgtu},
        {"bleu", op::bleu},
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

// ============================================================================
// 1. host checks: the assembler (jump optimizer and jump resolution)
// ============================================================================

// 'optimize_jumps' on rv32i text: which jumps and branches are removed,
// inverted or kept
auto check_jump_optimizer() -> void {
    // 'name:' lines are labels, '# ' lines comments, 'j' and branches jump,
    // and 'addi', 'ecall' and 'call' are instructions
    const auto optimize = [&](const std::string_view assembly) -> std::string {
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
                split_operands(code.substr(split + 1))};

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

            assembler.resolved_branch(indent, branch_op(mnemonic),
                                      operands.at(0), operands.at(1),
                                      operands.at(2), "t0");
        }
        assembler.optimize_jumps();
        std::ostringstream output;
        assembler.resolve_jumps();
        assembler.write_resolved(output);

        return output.str();
    };

    // --- the patterns 'assembler' documents, in rv32i form

    // a jump to the next instruction is removed
    assert(optimize("    j cmp_13_26\n    cmp_13_26:\n") == "    cmp_13_26:\n");

    // a branch and a jump to the label right after them are both removed
    assert(optimize("    bne a0, a1, bool_end_15_9\n    j bool_end_15_9\n"
                    "    bool_end_15_9:\n") == "    bool_end_15_9:\n");

    // a branch over a jump becomes the inverted branch to the jump target
    assert(optimize("    bne a0, a1, cmp_14_26\n    j if_14_8_code\n"
                    "    cmp_14_26:\n    ecall\n    if_14_8_code:\n") ==
           "    beq a0, a1, if_14_8_code\n    cmp_14_26:\n    ecall\n"
           "    if_14_8_code:\n");

    // numeric labels are entries and jumps to the next instruction go
    assert(optimize("    j done\n    1:\n    j done\n    done:\n") ==
           "    1:\n    done:\n");

    // the same inversion without indentation
    assert(optimize("beq a0, a1, skip\nj end\nskip:\naddi a0, a0, 1\n"
                    "end:\n") ==
           "bne a0, a1, end\nskip:\naddi a0, a0, 1\nend:\n");

    // comments between a jump and its target do not keep the jump
    assert(optimize("top:\naddi a0, a0, 1\nbne a0, zero, top\nj next\n"
                    "# next\nnext:\n") ==
           "top:\naddi a0, a0, 1\nbne a0, zero, top\n# next\nnext:\n");

    // --- inversion of every branch kind

    // every supported inverse must preserve operand order in both
    // directions, also with a comment between the branch and the jump
    for (const auto& [first, second] :
         std::array<std::pair<std::string_view, std::string_view>, 5>{{
             {"beq", "bne"},
             {"blt", "bge"},
             {"bltu", "bgeu"},
             {"bgt", "ble"},
             {"bgtu", "bleu"},
         }}) {
        for (const bool reverse : {false, true}) {
            const std::string_view mnemonic{reverse ? second : first};
            const std::string_view inverted{reverse ? first : second};
            const std::string_view operands{"a0, a1"};

            const std::string input{std::format(
                "{} {}, skip\n# keep\nj done\nskip:\naddi a0, a0, 1\n"
                "done:\n",
                mnemonic, operands)};

            const std::string expected{std::format(
                "{} {}, done\n# keep\nskip:\naddi a0, a0, 1\ndone:\n", inverted,
                operands)};

            assert(optimize(input) == expected);
            assert(optimize(expected) == expected);
        }
    }

    // --- when a branch or jump stays or goes

    // both outcomes continue at the same place
    assert(optimize("beq a0, zero, end\nj end\naddi a0, a0, 1\nend:\n") ==
           "j end\naddi a0, a0, 1\nend:\n");

    // nothing reaches a jump right after another
    assert(optimize("j end\nj other\naddi a0, a0, 1\nother:\necall\nend:\n") ==
           "j end\naddi a0, a0, 1\nother:\necall\nend:\n");

    // a backward jump after a branch to the next label also inverts
    assert(optimize("top:\naddi a0, a0, 1\nbeq a0, zero, done\nj top\n"
                    "done:\n") ==
           "top:\naddi a0, a0, 1\nbne a0, zero, top\ndone:\n");

    // labels between the jumps let execution enter
    for (const std::string_view unchanged :
         {"top:\nj top\n", "call end\nend:\n",
          "call entry\nbeq a0, zero, skip\nentry:\nj end\nskip:\necall\n"
          "end:\n",
          "beq a0, zero, skip\n1:\nj end\nskip:\necall\nend:\n"}) {
        assert(optimize(unchanged) == unchanged);
    }

    // a label nothing names cannot be entered
    assert(optimize("beq a0, zero, skip\nentry:\nj end\nskip:\necall\n"
                    "end:\n") ==
           "bne a0, zero, end\nentry:\nskip:\necall\nend:\n");

    // folding ignores reach because resolving grows the branch again
    std::string distant{"beq a0, zero, skip\nj end\nskip:\n"};
    for (size_t count{}; count < 1024; ++count) {
        distant += "addi a0, a0, 1\n";
    }
    distant += "end:\n";

    assert(optimize(distant) ==
           "beq a0, zero, .Lbaz_jump.0\nj end\n.Lbaz_jump.0:\n" +
               distant.substr(distant.find("skip:")));

    {
        // code continues across data placed in another section
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
        std::ostringstream output;
        assembler.resolve_jumps();
        assembler.write_resolved(output);
        assert(output.str() == "bne a0, a1, end\n.data\n.word 1\n.text\n"
                               "skip:\necall\nend:\n");
    }
}

// jump resolution: the size of a branch or jump follows its distance to the
// target, the limits below are the exact byte counts of the encodings, and
// 'emit_smaller' keeps the shorter of two code versions
auto check_jump_resolution() -> void {
    const auto padding = [](const size_t count) -> std::string {
        std::string text;
        for (size_t i{}; i < count; ++i) {
            text += "addi a0, a0, 1\n";
        }

        return text;
    };

    const auto add_padding = [](assembler_rv32i& assembler,
                                const size_t count) -> void {
        for (size_t i{}; i < count; ++i) {
            assembler.addi(0, "a0", "a0", 1);
        }
    };

    const auto written = [](assembler_rv32i& assembler) -> std::string {
        std::ostringstream output;
        assembler.resolve_jumps();
        assembler.write_resolved(output);

        return output.str();
    };

    const auto forward =
        [&](const std::string_view mnemonic, const std::string_view operands,
            const size_t count, const std::string_view scratch) -> std::string {
        assembler_rv32i assembler;
        add_jump(assembler, mnemonic, operands, "end", scratch);
        add_padding(assembler, count);
        assembler.label(0, "end");

        return written(assembler);
    };

    const auto backward = [&](const std::string_view mnemonic,
                              const std::string_view operands,
                              const size_t count) -> std::string {
        assembler_rv32i assembler;
        assembler.label(0, "end");
        add_padding(assembler, count);
        add_jump(assembler, mnemonic, operands, "end", "t0");

        return written(assembler);
    };

    const auto rejects = [](const auto& action) -> bool {
        try {
            action();
        } catch (const panic_exception&) {
            return true;
        }

        return false;
    };

    // --- reach of conditional branches

    // branches reach 4094 bytes forward and 4096 bytes backward
    assert(forward("beq", "a0, a1", 1022, "t0") ==
           "beq a0, a1, end\n" + padding(1022) + "end:\n");

    assert(forward("beq", "a0, a1", 1023, "t0") ==
           "bne a0, a1, .Lbaz_jump.0\nj end\n.Lbaz_jump.0:\n" + padding(1023) +
               "end:\n");

    assert(backward("beq", "a0, a1", 1024) ==
           "end:\n" + padding(1024) + "beq a0, a1, end\n");

    assert(backward("beq", "a0, a1", 1025) ==
           "end:\n" + padding(1025) +
               "bne a0, a1, .Lbaz_jump.0\nj end\n.Lbaz_jump.0:\n");

    // --- reach of unconditional jumps

    // 'jal' reaches 1048574 bytes forward and 1048576 bytes backward
    assert(forward("j", {}, 262142, "t0") ==
           "j end\n" + padding(262142) + "end:\n");

    assert(forward("j", {}, 262143, "t0") ==
           "jump end, t0\n" + padding(262143) + "end:\n");

    assert(backward("j", {}, 262144) == "end:\n" + padding(262144) + "j end\n");

    assert(backward("j", {}, 262145) ==
           "end:\n" + padding(262145) + "jump end, t0\n");

    // a branch beyond 'jal' reach inverts around the long jump
    assert(forward("bne", "a0, zero", 262143, "t1") ==
           "beq a0, zero, .Lbaz_jump.0\njump end, t1\n.Lbaz_jump.0:\n" +
               padding(262143) + "end:\n");

    // --- interaction between jumps and sections

    {
        // growing the inner branch pushes the outer one out of reach
        assembler_rv32i assembler;
        add_jump(assembler, "beq", "a0, a1", "end", "t0");
        add_padding(assembler, 1021);
        add_jump(assembler, "beq", "a0, a1", "far", "t0");
        assembler.label(0, "end");
        add_padding(assembler, 1100);
        assembler.label(0, "far");
        const std::string chained{written(assembler)};
        assert(chained.starts_with("bne a0, a1, .Lbaz_jump.0\nj end\n"));
        assert(chained.contains("bne a0, a1, .Lbaz_jump.1\nj far\n"));
    }
    {
        // other sections do not count towards code offsets
        assembler_rv32i assembler;
        add_jump(assembler, "beq", "a0, a1", "end", "t0");
        assembler.switch_section(assembler_rv32i::section::data);
        const std::array<int64_t, 1> word{1};
        for (size_t i{}; i < 2000; ++i) {
            assembler.data(4, word);
        }
        assembler.switch_section(assembler_rv32i::section::text);
        assembler.label(0, "end");

        std::string words;
        for (size_t i{}; i < 2000; ++i) {
            words += ".word 1\n";
        }

        assert(written(assembler) ==
               "beq a0, a1, end\n.data\n" + words + ".text\nend:\n");
    }
    // --- choosing the smaller of two code versions ('emit_smaller')

    {
        // the smaller version is kept and ties keep the first
        assembler_rv32i assembler;

        assembler.emit_smaller([&] { add_padding(assembler, 2); },
                               [&] { assembler.addi(0, "a0", "a0", 1); });

        assembler.emit_smaller([&] { assembler.sw(0, "a0", 0, "sp"); },
                               [&] { add_padding(assembler, 1); });

        assert(written(assembler) == "addi a0, a0, 1\nsw a0, 0(sp)\n");
    }
    {
        // a nested choice lands inside the version that contains it
        assembler_rv32i assembler;

        assembler.emit_smaller(
            [&] {
                assembler.lw(0, "a0", 0, "sp");
                assembler.emit_smaller(
                    [&] { add_padding(assembler, 2); },
                    [&] { assembler.addi(0, "a0", "a0", 1); });
            },
            [&] { add_padding(assembler, 3); });

        assert(written(assembler) == "lw a0, 0(sp)\naddi a0, a0, 1\n");
    }
    {
        // a jump inside a kept version still grows
        assembler_rv32i assembler;

        assembler.emit_smaller(
            [&] { add_jump(assembler, "beq", "a0, a1", "end", "t0"); },
            [&] { add_padding(assembler, 2); });

        add_padding(assembler, 1100);
        assembler.label(0, "end");
        assert(written(assembler) ==
               "bne a0, a1, .Lbaz_jump.0\nj end\n.Lbaz_jump.0:\n" +
                   padding(1100) + "end:\n");
    }

    // --- invalid jumps are rejected

    assert(rejects([&] { static_cast<void>(forward("j", {}, 262143, {})); }));

    assert(rejects([&] {
        assembler_rv32i assembler;
        add_jump(assembler, "j", {}, "missing", "t0");
        static_cast<void>(written(assembler));
    }));
}

// ============================================================================
// 2. host checks: the machine (instruction selection and diagnostics)
// ============================================================================

// the byte size 'line_size_bytes' assigns to a line of assembly text, which
// the jump resolution relies on
auto check_line_sizes() -> void {
    // 'li' and 'la' grow to two instructions when the value needs them
    for (const auto [instruction, cost] :
         std::array<std::pair<std::string_view, size_t>, 13>{
             {{"li a0, 2047", 1},
              {"li a0, 2048", 2},
              {"li a0, -2048", 1},
              {"li a0, -2049", 2},
              {"li a0, 4096", 1},
              {"li a0, -4096", 1},
              {"li a0, 2147483647", 2},
              {"li a0, -2147483648", 1},
              {"li a0, 4294967295", 1},
              {"li a0, value + 1", 2},
              {"la a0, buffer", 2},
              {"call function", 2},
              {"mv a0, a1", 1}}}) {
        assert(assembler_rv32i::line_size_bytes(std::format(
                   "\t{}  # instruction", instruction)) == cost * 4);
    }

    // comments, directives and labels take no space
    for (const std::string_view sizeless :
         {"  # comment", "\t.option norelax", ".Lcandidate: \t# label"}) {
        assert(assembler_rv32i::line_size_bytes(sizeless) == 0);
    }
}

// 'emit_most_efficient' on versions of equal size, in every jump mode
auto check_equal_size_choice() -> void {
    // equal sizes keep the version without scratch in direct and buffered
    // output
    for (const assembler::jump_mode jumps :
         {assembler::jump_mode::as_emitted, assembler::jump_mode::resolved,
          assembler::jump_mode::optimized}) {
        std::ostringstream output;
        machine_rv32i backend{output, {}, jumps};
        backend.set_builtin_types(integer64, integer, half, byte);
        backend.start();

        const operand result{operand::reg("a0", integer)};

        const auto load = [&](const std::string_view value) -> void {
            backend.copy_value(token{}, 0, result,
                               operand::imm(std::string{value}, integer));
        };

        const auto copy = [&] {
            backend.copy_value(token{}, 0, result, operand::reg("a1", integer));
        };

        backend.emit_most_efficient(token{}, 0, [&] { load("2047"); }, copy);

        backend.emit_most_efficient(token{}, 0, [&] { load("2048"); }, copy);

        backend.finish();
        // direct output is already written
        if (jumps != assembler::jump_mode::as_emitted) {
            backend.write_assembly(output);
        }
        // buffered output is followed by the optimization counts
        assert(output.str().contains("li a0, 2047\naddi a0, a1, 0\n"));
    }
}

// the jump optimizer sees the jumps and labels that the backend emits
auto check_backend_jump_optimization() -> void {
    // the backend's jumps and labels reach the optimizer
    std::ostringstream output;
    machine_rv32i backend{output, {}, assembler::jump_mode::optimized};
    backend.set_builtin_types(integer64, integer, half, byte);
    backend.start();

    const operand left{operand::reg("a0", integer)};
    const operand right{operand::reg("a1", integer)};

    const auto branch_if_different =
        [&](const std::string_view target) -> void {
        backend.compare_and_branch(token{}, 0, left, right,
                                   {
                                       .operation{"!="},
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

// copies between equal addresses, and the comments that describe variables
auto check_copies_and_variable_comments() -> void {
    assembly_output copies;
    machine_rv32i backend{copies};
    backend.set_builtin_types(integer64, integer, half, byte);
    // --- a copy to the same address emits nothing, others load and store

    const operand address{operand::mem("s0", {}, 1, 24, integer)};
    backend.copy_value(token{}, 0, address, address);
    assert(copies.str().empty());
    backend.copy_value(token{}, 0, operand::mem("s0", {}, 1, 28, integer),
                       address);
    assert(copies.str().contains("lw t0, 24(s0)\n"));
    assert(copies.str().contains("sw t0, 28(s0)\n"));

    copies.str({});
    backend.copy_value(token{}, 0, operand::mem("s0", {}, 1, 24, byte),
                       address);
    assert(copies.str().contains("lw t0, 24(s0)\n"));
    assert(copies.str().contains("sb t0, 24(s0)\n"));

    // --- variable comments name size and place

    copies.str({});
    backend.comment_variable(token{}, 0, "arr: i32[4]", 16,
                             operand::mem("s0", {}, 1, 208, integer));
    backend.comment_variable(token{}, 0, "first", 4,
                             operand::mem("s0", {}, 1, 0, integer));
    assert(copies.str() == "# arr: i32[4] (16 B @ [s0 + 208])\n"
                           "# first (4 B @ [s0])\n");
}

// comments carry the source position of their token, and the register and
// array copy helpers describe their registers in comments
auto check_comments_with_source_positions() -> void {
    // columns must be relative to the source line rather than the file
    std::ostringstream comments;
    machine_rv32i located{comments, "first\n    value"};
    const token location{{}, 10, "value", 15, {}, 2, false};
    located.comment(location, 1, "assignment");
    located.comment(token{}, 0, "generated");
    assert(comments.str() == "    # [2:5] assignment\n# generated\n");
    located.set_builtin_types(integer64, integer, half, byte);
    comments.str({});
    const operand scratch{located.alloc_scratch_register(location, 1, integer)};
    const operand named{
        located.alloc_named_register(location, 1, "x10", integer)};
    located.free_named_register(location, 1, named);
    located.free_scratch_register(location, 1, scratch);
    assert(comments.str() == "    # [2:5] allocate scratch register -> t0\n"
                             "    # [2:5] allocate named register a0\n"
                             "    # [2:5] free named register a0\n"
                             "    # [2:5] free scratch register t0\n");
    comments.str({});
    located.add_subtract(location, 1, '+',
                         operand::mem("a0", {}, 1, 0, integer),
                         operand::imm("1", integer));
    assert(comments.str() ==
           "    # [2:5] allocate scratch register -> t0\n"
           "    lw t0, 0(a0)\n    addi t0, t0, 1\n    sw t0, 0(a0)\n"
           "    # [2:5] free scratch register t0\n");
    std::vector<operand> ordered;
    for (const std::string_view name :
         {"t0", "t1", "t2", "t3", "t4", "t5", "t6", "s0",  "s1",  "s2",
          "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "tp",
          "gp", "ra", "a1", "a2", "a3", "a4", "a5", "a6",  "a7",  "a0"}) {
        ordered.push_back(located.alloc_scratch_register(location, 1, integer));
        assert(ordered.back().base_register() == name);
    }
    located.free_scratch_registers(location, 1, ordered);
    comments.str({});
    const operand count{located.begin_array_copy(location, 1)};
    located.copy_value(location, 1, count, operand::imm("2", integer));
    located.set_array_copy_source(location, 1,
                                  operand::mem("s0", {}, 1, 216, integer));
    located.set_array_copy_destination(location, 1,
                                       operand::mem("s0", {}, 1, 208, integer));
    located.end_array_copy(location, 1, 4, 4);
    for (const std::string_view text :
         {"t0: source, t1: destination, t2: count",
          "t2: elements to bytes (4 bytes/element)",
          "t4: end of words, t2: tail bytes", "copy 4-byte words",
          "copy optional 2-byte tail", "copy optional final byte"}) {
        assert(comments.str().contains(std::format("# [2:5] {}\n", text)));
    }
    located.finish();
}

// ============================================================================
// 3. host checks: compiled .baz programs inspected as assembly text
// ============================================================================

// 'array_copy' on offset arrays: the pointer registers hold the address
// throughout the index arithmetic
auto check_array_copy_pointers() -> void {
    const std::string_view source{R"baz(
func main() {
    var source = i32[4]{}
    var destination = i32[4]{}
    var count = 2
    array_copy(source[2], destination[1], count)
}
)baz"};
    std::ostringstream output;
    machine_rv32i compiler{output};
    program prg{compiler, source, 4096, false, false, false};
    prg.build(output);
    // reserved pointers must hold the address throughout index arithmetic
    assert(output.str().contains("slli t0, t3, 2\n"));
    assert(output.str().contains("add t0, t0, s0\n"));
    assert(output.str().contains("slli t1, t3, 2\n"));
    assert(output.str().contains("add t1, t1, s0\n"));
    assert(not output.str().contains("addi t0, t3, 0\n"));
    assert(not output.str().contains("addi t1, t3, 0\n"));
}

// 'arrays_equal' on offset arrays: same pointer rule as 'array_copy', no
// extra scratch register or boolean normalization, and described registers
auto check_arrays_equal_registers_rv32i() -> void {
    const std::string_view source{R"baz(
func assert(ok bool) { if not ok exit(1) }
func main() {
    var source = i32[4]{}
    var destination = i32[4]{}
    assert(arrays_equal(source[2], destination[1], 2))
}
)baz"};
    std::ostringstream output;
    machine_rv32i compiler{output};
    program prg{compiler, source, 4096, false, false, false};
    prg.build(output);
    assert(output.str().contains("slli t1, t4, 2\n"));
    assert(output.str().contains("add t1, t1, s0\n"));
    assert(output.str().contains("slli t2, t4, 2\n"));
    assert(output.str().contains("add t2, t2, s0\n"));
    assert(not output.str().contains("addi t1, t4, 0\n"));
    assert(not output.str().contains("addi t2, t4, 0\n"));
    assert(not output.str().contains("sltu t0, zero, t0\n"));
    assert(not output.str().contains("allocate scratch register -> t6\n"));
    for (const std::string_view text :
         {"t1: source, t2: destination, t3: count",
          "t3: elements to bytes (4 bytes/element)",
          "t0: left value/result, t4: right value",
          "t5: end of words, t3: tail bytes", "stop at first mismatch",
          "compare 4-byte words", "compare optional 2-byte tail",
          "compare optional final byte", "all matched or empty: true",
          "mismatch: false"}) {
        assert(output.str().contains(std::format("# {}\n", text)));
    }
}

// x86_64: a negated 'arrays_equal' condition uses 'sete' and 'setne' directly,
// without inverting or comparing the result and without an extra scratch
// register
auto check_arrays_equal_condition_x86() -> void {
    const std::string_view source{R"baz(
func assert(ok bool) { if not ok exit(1) }
func main() {
    var left = i8[2]{1, 2}
    var right = i8[2]{1, 2}
    assert(arrays_equal(left, right, 2))
    right[1] = 3
    assert(not arrays_equal(left, right, 2))
}
)baz"};
    std::ostringstream output;
    machine_x86_64 compiler{output, source};
    program prg{compiler, source, 4096, false, false, false};
    prg.build(output);
    assert(output.str().contains("sete r15b\n"));
    assert(not output.str().contains("xor r15b, 1\n"));
    assert(output.str().contains("setne r15b\n"));
    assert(not output.str().contains("\n    cmp r15b, 0\n"));
    assert(not output.str().contains("allocate scratch register -> r14\n"));
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
    same = equal(left, right)
    same = not equal(left, right)
}
)baz"};
    std::ostringstream x86_output;
    machine_x86_64 x86_compiler{x86_output, source};
    program x86_program{x86_compiler, source, 4096, false, false, false};
    x86_program.build(x86_output);
    assert(x86_output.str().contains("sete byte [rbp + 4]\n"));
    assert(x86_output.str().contains("setne byte [rbp + 4]\n"));
    assert(not x86_output.str().contains("sete r15b\n"));
    assert(not x86_output.str().contains("setne r15b\n"));
    assert(not x86_output.str().contains("cmp r15b, 0\n"));
    assert(not x86_output.str().contains("xor r15b, 1\n"));

    std::ostringstream rv32i_output;
    machine_rv32i rv32i_compiler{rv32i_output};
    program rv32i_program{rv32i_compiler, source, 4096, false, false, false};
    rv32i_program.build(rv32i_output);
    assert(rv32i_output.str().contains("sb t3, -2028(s0)\n"));
    assert(not rv32i_output.str().contains("sltu "));
    assert(not rv32i_output.str().contains("sltiu "));
    assert(not rv32i_output.str().contains("xori "));
}

// ============================================================================
// 4. host checks: the x86_64 backend
// ============================================================================

// the registers handed out first are the ones no instruction needs: 'rcx',
// 'rdx' and 'rax' come last, also after a register was freed again
auto check_x86_scratch_registers() -> void {
    std::ostringstream x86_output;
    machine_x86_64 x86_backend{x86_output, {}};
    x86_backend.set_builtin_types(integer64, integer, half, byte);
    constexpr std::array<std::string_view, 14> x86_scratch_order{
        "r15", "r14", "r13", "r12", "r10", "r9",  "r8",
        "r11", "rbx", "rsi", "rdi", "rcx", "rdx", "rax"};

    for (size_t pass{}; pass < 2; ++pass) {
        std::vector<operand> registers;
        for (const std::string_view expected : x86_scratch_order) {
            const operand reg{
                x86_backend.alloc_scratch_register(token{}, 0, integer64)};
            assert(reg.base_register() == expected);
            registers.push_back(reg);
        }
        // scratch registers are exhausted after the last one
        assert(rejected_with([&] {
            static_cast<void>(
                x86_backend.alloc_scratch_register(source_tk, 0, integer64));
        }));

        // a register needed by an instruction is rejected, not overwritten
        assert(rejected_with(
            [&] {
                static_cast<void>(x86_backend.alloc_named_register(
                    source_tk, 0, "rcx", integer64));
            },
            "holds a scratch value"));

        // freed registers are handed out again in the same order
        for (size_t count{}; count < 2; ++count) {
            x86_backend.free_scratch_register(token{}, 0, registers.back());
            registers.pop_back();
        }
        const operand ordinary{
            x86_backend.alloc_scratch_register(token{}, 0, integer64)};
        assert(ordinary.base_register() == "rdx");
        registers.push_back(ordinary);
        const operand special{
            x86_backend.alloc_scratch_register(token{}, 0, integer64)};
        assert(special.base_register() == "rax");
        registers.push_back(special);
        x86_backend.free_scratch_registers(token{}, 0, registers);
        x86_backend.finish();
    }
}

// 'read', 'write' and 'invoke_syscall' save and restore 'rcx' and 'r11' (which
// the syscall instruction clobbers) exactly when they are in use, around the
// syscall and without touching 'rsp'
auto check_x86_syscall_register_saving() -> void {
    std::ostringstream x86_output;
    machine_x86_64 x86_backend{x86_output, {}};
    x86_backend.set_builtin_types(integer64, integer, half, byte);

    for (const unsigned live_mask : {0U, 1U, 2U, 3U}) {
        const bool rcx_allocated{(live_mask & 1U) != 0};
        const bool r11_allocated{(live_mask & 2U) != 0};
        std::vector<operand> scratch;
        operand live_rcx;
        if (rcx_allocated) {
            live_rcx =
                x86_backend.alloc_named_register(token{}, 0, "rcx", byte);
        }
        if (r11_allocated) {
            for (size_t count{}; count < 8; ++count) {
                scratch.push_back(
                    x86_backend.alloc_scratch_register(token{}, 0, byte));
            }
        }
        const machine::builtin_function_registers contract{
            x86_backend.registers_for_builtin_function(
                machine::builtin_function::write)};

        std::vector<operand> args;
        for (const std::string_view name : contract.arguments) {
            args.push_back(
                x86_backend.alloc_named_register(token{}, 0, name, integer64));
        }
        const operand result{x86_backend.alloc_named_register(
            token{}, 0, contract.result, integer64)};
        for (const unsigned operation : {0U, 1U, 2U}) {
            x86_output.str({});
            if (operation == 0) {
                x86_backend.read(token{}, 0, result, args.at(0), args.at(1),
                                 args.at(2));
            } else if (operation == 1) {
                x86_backend.write(token{}, 0, result, args.at(0), args.at(1),
                                  args.at(2));
            } else {
                x86_backend.invoke_syscall(0);
            }
            const std::string assembly{x86_output.str()};
            assert(assembly.contains("push rcx") == rcx_allocated);
            assert(assembly.contains("pop rcx") == rcx_allocated);
            assert(assembly.contains("push r11") == r11_allocated);
            assert(assembly.contains("pop r11") == r11_allocated);
            assert(not assembly.contains("rsp"));
            assert(not assembly.contains("push rax"));
            assert(not assembly.contains("push rdi"));
            for (const std::string_view name : {"rcx", "r11"}) {
                if (assembly.contains(std::format("push {}", name))) {
                    assert(assembly.find(std::format("push {}", name)) <
                           assembly.find("syscall"));
                    assert(assembly.find(std::format("pop {}", name)) >
                           assembly.find("syscall"));
                }
            }
        }
        x86_output.str({});
        x86_backend.exit(token{}, 0, args.at(0));
        assert(not x86_output.str().contains("push "));
        assert(not x86_output.str().contains("pop "));
        x86_backend.free_named_register(token{}, 0, result);
        x86_backend.free_named_registers(token{}, 0, args);
        x86_backend.free_scratch_registers(token{}, 0, scratch);
        if (rcx_allocated) {
            x86_backend.free_named_register(token{}, 0, live_rcx);
        }
        x86_backend.finish();
    }
}

// a compiled 'write' in 'main' needs no saves and no scratch register
auto check_x86_write_in_program() -> void {
    const std::string_view source{
        "func main() { var b = i8[1]{} var value = write(1, b, 0) "
        "exit(value) }"};

    std::ostringstream output;
    machine_x86_64 compiler{output, source};
    program prg{compiler, source, 4096, false, false, false};
    prg.build(output);
    const std::string assembly{output.str()};
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
        std::ostringstream output;
        machine_x86_64 compiler{output, source};
        program prg{compiler, source, 4096, false, false, false};
        assert(rejected_with([&] { prg.build(output); },
                             "cannot allocate register rdi"));
    }
}

// ============================================================================
// 5. host checks: instruction shapes of the RV32I backend
//
// Each check drives 'machine_rv32i' directly and compares the text it emits.
// The scratch registers of the pool are held where a check must prove that no
// temporary register is used.
// ============================================================================

// the index scales that address lowering accepts
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
    assembly_output& output{rv32i.output};

    for (const bool counted : {false, true}) {
        std::vector<operand> held;
        for (size_t count{}; count < 25; ++count) {
            held.push_back(backend.alloc_scratch_register(token{}, 0, boolean));
        }
        const operand result{held.front()};
        if (counted) {
            const operand count{backend.begin_memory_equal(token{}, 0)};
            backend.copy_value(token{}, 0, count, operand::imm("7", integer));
            std::println(output, "la {}, buffer", held.back().base_register());
            backend.set_memory_equal_left(token{}, 0,
                                          operand::mem(held.back(), byte));
            backend.set_memory_equal_right(token{}, 0,
                                           operand::mem(held.back(), byte));
            output.str({});
            backend.end_arrays_equal(token{}, 0, 1, 4, result);
        } else {
            std::println(output, "la {}, buffer", held.back().base_register());
            output.str({});
            backend.compare_memory(token{}, 0, operand::mem(held.back(), byte),
                                   operand::mem(held.back(), byte), 7, 4,
                                   result);
        }
        const std::string assembly{output.str()};
        // a known size takes its tail at offsets, a run-time count advances
        for (const std::string_view instruction : {"lw", "lhu", "lbu"}) {
            assert(assembly.contains(
                std::format("{} {}, ", instruction, result.base_register())));
        }
        assert(
            assembly.contains(std::format("li {}, 1", result.base_register())));
        assert(
            assembly.contains(std::format("li {}, 0", result.base_register())));
        assert(not assembly.contains(
            std::format("addi {},", result.base_register())));
        assert(not assembly.contains("slli"));
        backend.free_scratch_registers(token{}, 0, held);
        backend.finish();
        output.str({});
    }
}

// 'copy' of a known size: loads and stores of the widest width first, the
// tail by width, and a loop from 17 bytes on
auto check_known_size_copy() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};
    assembly_output& output{rv32i.output};

    for (size_t size_bytes{}; size_bytes <= 24; ++size_bytes) {
        backend.copy(token{}, 0, operand::mem("a0", {}, 1, 0, byte),
                     operand::mem("a1", {}, 1, 0, byte), size_bytes, 4);
        const std::string assembly{output.str()};
        assert(assembly.contains("bne ") == (size_bytes > 16));
        if (size_bytes == 0) {
            assert(assembly.empty());
        } else if (size_bytes <= 16) {
            for (const size_t width : {size_t{4}, size_t{2}, size_t{1}}) {
                const std::string load{width == 4   ? "lw "
                                       : width == 2 ? "lhu "
                                                    : "lbu "};
                const std::string store{width == 4   ? "sw "
                                        : width == 2 ? "sh "
                                                     : "sb "};
                size_t loads{};
                size_t stores{};
                std::istringstream lines{assembly};
                for (std::string line; std::getline(lines, line);) {
                    loads += line.starts_with(load);
                    stores += line.starts_with(store);
                }
                const size_t expected{width == 4
                                          ? size_bytes / 4
                                          : (size_bytes % (width * 2)) / width};
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
        output.str({});
    }
}

// string data: the escapes of .baz strings become assembler escapes
auto check_string_data_escapes() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};
    assembly_output& output{rv32i.output};

    backend.emit_string_data({});
    assert(output.str() == ".ascii \"\"\n");
    output.str({});
    backend.emit_string_data(
        R"baz(\0\a\b\t\n\v\f\r\e\"\'`\\\x00\x7F\x80\xff\x41B)baz");
    assert(
        output.str() ==
        R"baz(.ascii "\000\007\010\t\n\013\014\r\033\"'`\\\000\177\200\377AB")baz"
        "\n");
    output.str({});
    backend.emit_string_data("\xc3\xa9\n");
    assert(output.str() == R"baz(.ascii "\303\251\n")baz"
                           "\n");
    output.str({});
    backend.emit_string_data(R"baz(hello\n\x007)baz");
    assert(output.str() == R"baz(.ascii "hello\n\0007")baz"
                           "\n");
}

// ============================================================================
// 6. host checks: arithmetic on registers only (no scratch register is free)
// ============================================================================

// shifts: register counts, immediate counts of 0 to 31 and rejected others
auto check_shifts() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};
    assembly_output& output{rv32i.output};

    std::vector<operand> shift_registers{
        hold_scratch_registers(backend, 30, integer)};
    for (const char operation : {'<', '>'}) {
        output.str({});

        backend.shift(token{}, 0, operation, operand::reg("a0", integer),
                      operand::reg("a1", integer));

        assert(output.str() ==
               (operation == '<' ? "sll a0, a0, a1\n" : "sra a0, a0, a1\n"));
        output.str({});

        for (const unsigned count : {0U, 2U, 31U}) {
            output.str({});
            backend.shift(token{}, 0, operation, operand::reg("a0", integer),
                          operand::imm(std::format("{}", count), integer));
            assert(output.str() ==
                   (count == 0 ? std::string{}
                               : std::format("{} a0, a0, {}\n",
                                             operation == '<' ? "slli" : "srai",
                                             count)));
        }
        for (const std::string_view count : {"-1", "32", "35", "64"}) {
            output.str({});
            assert(rejected_with(
                [&] {
                    backend.shift(source_tk, 0, operation,
                                  operand::reg("a0", integer),
                                  operand::imm(std::string{count}, integer));
                },
                "RV32I shift count must be 0 to 31 for 32-bit values"));
            assert(output.str().empty());
        }
    }
    backend.free_scratch_registers(token{}, 0, shift_registers);
    backend.finish();
}

// 'add_subtract' and 'bitwise' with register and constant operands
auto check_add_subtract_bitwise() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};
    assembly_output& output{rv32i.output};

    std::vector<operand> shift_registers{
        hold_scratch_registers(backend, 30, integer)};
    for (const char operation : {'+', '-', '&', '|', '^'}) {
        output.str({});
        std::string_view instruction;
        if (operation == '+' or operation == '-') {
            instruction = operation == '+' ? "add" : "sub";

            backend.add_subtract(token{}, 0, operation,
                                 operand::reg("a0", integer),
                                 operand::reg("a1", integer));

        } else {
            instruction = operation == '&'   ? "and"
                          : operation == '|' ? "or"
                                             : "xor";

            backend.bitwise(token{}, 0, operation, operand::reg("a0", integer),
                            operand::reg("a1", integer));
        }
        assert(output.str() == std::format("{} a0, a0, a1\n", instruction));
        for (const std::string_view constant :
             {"7", "-7", "~-8", "--7", "2047", "-2047"}) {
            output.str({});

            const operand source{operand::imm(std::string{constant}, integer)};

            if (operation == '+' or operation == '-') {
                backend.add_subtract(token{}, 0, operation,
                                     operand::reg("a0", integer), source);
            } else {
                backend.bitwise(token{}, 0, operation,
                                operand::reg("a0", integer), source);
            }
            int expected{7};
            if (constant == "-7") {
                expected = -7;
            } else if (constant == "2047") {
                expected = 2047;
            } else if (constant == "-2047") {
                expected = -2047;
            }
            if (operation == '-') {
                expected = -expected;
            }

            assert(output.str() ==
                   std::format("{}i a0, a0, {}\n",
                               operation == '-' ? "add" : instruction,
                               expected));
        }
    }
    backend.free_scratch_registers(token{}, 0, shift_registers);
    backend.finish();
}

// 'x & 0' loads its known result for every value width, and a shift by 0
// emits nothing
auto check_zero_operations_and_zero_shifts() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};
    assembly_output& output{rv32i.output};

    std::vector<operand> shift_registers{
        hold_scratch_registers(backend, 30, integer)};
    for (const type* value_type : {&integer, &half, &byte}) {
        for (const bool memory_destination : {false, true}) {
            const operand destination{
                memory_destination ? operand::mem("a0", {}, 1, 0, *value_type)
                                   : operand::reg("a0", *value_type)};

            output.str({});
            backend.shift(token{}, 0, '<', destination,
                          operand::imm("0", integer));
            backend.shift(token{}, 0, '>', destination,
                          operand::imm("0", integer));
            assert(output.str().empty());
            output.str({});
            backend.bitwise(token{}, 0, '&', destination,
                            operand::imm("0", integer));

            const std::string_view store{value_type == &integer ? "sw"
                                         : value_type == &half  ? "sh"
                                                                : "sb"};
            const std::string zero_result{
                memory_destination ? std::format("{} zero, 0(a0)\n", store)
                                   : "li a0, 0\n"};

            assert(output.str() == zero_result);
        }
        output.str({});
        backend.unary(token{}, 0, '~', operand::reg("a0", *value_type));
        assert(output.str() == "xori a0, a0, -1\n");
    }
    output.str({});
    backend.unary(token{}, 0, '-', operand::reg("a0", integer));
    assert(output.str() == "sub a0, zero, a0\n");
    backend.free_scratch_registers(token{}, 0, shift_registers);
    backend.finish();
}

// byte values: operations with a known result load it, a shift extends the
// sign again, and constants beyond 12 bits take two instructions
auto check_narrow_values_and_large_constants() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};
    assembly_output& output{rv32i.output};

    std::vector<operand> shift_registers{
        hold_scratch_registers(backend, 30, integer)};
    backend.bitwise(token{}, 0, '|', operand::reg("a0", byte),
                    operand::imm("255", integer));
    assert(output.str() == "li a0, -1\n");
    output.str({});
    backend.shift(token{}, 0, '<', operand::reg("a0", byte),
                  operand::imm("7", integer));
    assert(output.str() == "slli a0, a0, 31\nsrai a0, a0, 24\n");
    output.str({});
    for (const type* narrow : {&byte, &half, &boolean}) {
        const size_t bits{narrow->size_bytes() * 8};
        assert(rejected_with(
            [&] {
                backend.shift(source_tk, 0, '<', operand::reg("a0", *narrow),
                              operand::imm(std::format("{}", bits), integer));
            },
            std::format("RV32I shift count must be 0 to {} for {}-bit values",
                        bits - 1, bits)));
        assert(output.str().empty());
    }
    backend.shift(token{}, 0, '<', operand::reg("a0", byte),
                  operand::imm("1", integer));
    assert(output.str() == "slli a0, a0, 25\nsrai a0, a0, 24\n");
    output.str({});
    backend.add_subtract(token{}, 0, '+', operand::reg("a0", integer),
                         operand::imm("2048", integer));
    assert(output.str() == "addi a0, a0, 2047\naddi a0, a0, 1\n");
    output.str({});
    backend.add_subtract(token{}, 0, '-', operand::reg("a0", integer),
                         operand::imm("2049", integer));
    assert(output.str() == "addi a0, a0, -2048\naddi a0, a0, -1\n");
    backend.free_scratch_registers(token{}, 0, shift_registers);
    backend.finish();
}

// ============================================================================
// 7. host checks: copies, comparisons, addresses and zeroing
// ============================================================================

// copies between a register and memory use the load and store of the width
auto check_copy_widths() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};
    assembly_output& output{rv32i.output};

    for (const type* value_type : {&integer, &half, &byte}) {
        output.str({});

        backend.copy_value(token{}, 0,
                           operand::mem("a0", {}, 1, 0, *value_type),
                           operand::reg("a1", *value_type));

        const std::string_view store{value_type == &integer ? "sw"
                                     : value_type == &half  ? "sh"
                                                            : "sb"};
        assert(output.str() == std::format("{} a1, 0(a0)\n", store));
        output.str({});

        backend.copy_value(token{}, 0, operand::reg("a1", *value_type),
                           operand::mem("a0", {}, 1, 0, *value_type));

        const std::string_view load{value_type == &integer ? "lw"
                                    : value_type == &half  ? "lh"
                                                           : "lb"};
        assert(output.str() == std::format("{} a1, 0(a0)\n", load));
        output.str({});

        backend.copy_value(token{}, 0, operand::reg("a1", *value_type),
                           operand::reg("x11", *value_type));

        assert(output.str().empty());
        backend.finish();
    }
}

// bitwise operations on byte registers need one instruction
auto check_byte_bitwise() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};
    assembly_output& output{rv32i.output};

    for (const char operation : {'&', '|', '^'}) {
        output.str({});

        backend.bitwise(token{}, 0, operation, operand::reg("a0", byte),
                        operand::reg("a1", byte));

        assert(std::ranges::count(output.str(), '\n') == 1);
        backend.finish();
    }
}

// a shift of variables compiles without a copy of the count
auto check_shift_of_variables() -> void {
    const std::string_view source{
        "func main() { var a = 3 var b = 2 var x = a << b exit(x) }"};

    std::ostringstream output;
    machine_rv32i compiler{output};
    program prg{compiler, source, 4096, false, false, false};
    prg.build(output);
    assert(output.str().contains("sll t0, t0, t1"));
    assert(not output.str().contains("addi t1, t0, 0"));
}

// addresses of the form base + index * scale + offset: the shortest code for
// the offset and scale, and registers reused when they are free
auto check_address_lowering() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};
    assembly_output& output{rv32i.output};

    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("a2", "a3", 1, 8196, integer));

    assert(output.str() ==
           "add a1, a2, a3\nlui t0, 2\nadd a1, a1, t0\nlw a1, 4(a1)\n");
    output.str({});

    for (const uint64_t scale :
         {UINT64_C(2), UINT64_C(256), UINT64_C(2147483648)}) {
        backend.copy_value(token{}, 0, operand::reg("a1", integer),
                           operand::mem("a2", "a3", scale, 40, integer));
        assert(output.str() ==
               std::format("slli a1, a3, {}\nadd a1, a1, a2\nlw a1, 40(a1)\n",
                           std::countr_zero(scale)));
        output.str({});
    }
    std::vector<operand> address_registers{
        hold_scratch_registers(backend, 30, integer)};
    assert(address_registers.back().base_register() == "a0");
    backend.free_scratch_register(token{}, 0, address_registers.back());
    address_registers.pop_back();
    output.str({});

    std::println(output, "la a1, buffer");
    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("a1", {}, 1, 4, integer));

    assert(output.str() == "la a1, buffer\nlw a1, 4(a1)\n");
    output.str({});

    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("a2", "a3", 1, 4, integer));

    assert(output.str() == "add a1, a2, a3\nlw a1, 4(a1)\n");
    output.str({});

    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("a2", {}, 1, 8196, integer));

    assert(output.str() == "lui a1, 2\nadd a1, a1, a2\nlw a1, 4(a1)\n");
    output.str({});

    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("a2", {}, 1, -2049, integer));

    assert(output.str() ==
           "lui a1, 1048575\nadd a1, a1, a2\nlw a1, 2047(a1)\n");
    output.str({});

    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("zero", {}, 1, 8196, integer));

    assert(output.str() == "lui a1, 2\nadd a1, a1, zero\nlw a1, 4(a1)\n");
    output.str({});

    for (const int64_t offset : {INT64_C(-4294967295), INT64_C(4294967295)}) {
        const int32_t low{offset < 0 ? 1 : -1};
        for (const std::string_view base : {"x11", "zero", "x0"}) {
            backend.copy_value(token{}, 0, operand::reg("a1", integer),
                               operand::mem(base, {}, 1, offset, integer));

            assert(output.str() == std::format("lw a1, {}({})\n", low, base));
            output.str({});
        }
    }

    std::println(output, "la a1, buffer");
    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("a1", "a3", 1, 4, integer));

    assert(output.str() == "la a1, buffer\nadd a1, a1, a3\nlw a1, 4(a1)\n");
    backend.free_scratch_registers(token{}, 0, address_registers);
    backend.finish();
}

// with every scratch register held, an address is built in the destination
// register, whichever of base and index it is
auto check_address_forms_without_temporaries() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};
    assembly_output& output{rv32i.output};

    std::vector<operand> address_registers{
        hold_scratch_registers(backend, 30, integer)};

    for (const std::string_view base : {"a2", "x11"}) {
        backend.copy_value(token{}, 0, operand::reg("a1", integer),
                           operand::mem(base, "x11", 1, 4, integer));
        assert(output.str() ==
               std::format("add a1, {}, x11\nlw a1, 4(a1)\n", base));
        output.str({});
    }
    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("x11", "a2", 1, 4, integer));
    assert(output.str() == "add a1, x11, a2\nlw a1, 4(a1)\n");
    output.str({});
    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("a2", "x11", 4, 4, integer));
    assert(output.str() == "slli a1, x11, 2\nadd a1, a1, a2\nlw a1, 4(a1)\n");
    output.str({});

    std::println(output, "la a1, buffer");
    backend.address_of(token{}, 0, operand::reg("a1", integer),
                       operand::mem("a1", {}, 1, 0, integer));

    assert(output.str() == "la a1, buffer\n");
    output.str({});

    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("zero", "a2", 1, 4, integer));

    assert(output.str() == "add a1, zero, a2\nlw a1, 4(a1)\n");
    output.str({});
    backend.free_scratch_registers(token{}, 0, address_registers);
    backend.finish();
}

// stores of constants and zeroing of small ranges need no temporary register
auto check_constant_stores_and_small_zeroing() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};
    assembly_output& output{rv32i.output};

    std::vector<operand> address_registers{
        hold_scratch_registers(backend, 30, integer)};

    backend.copy_value(token{}, 0, operand::mem("a2", {}, 1, 0, integer),
                       operand::imm("0", integer));

    assert(output.str() == "sw zero, 0(a2)\n");
    output.str({});
    backend.copy_value(token{}, 0, operand::reg("a1", byte),
                       operand::imm("255", integer));
    assert(output.str() == "li a1, -1\n");
    output.str({});
    backend.zero(token{}, 0, operand::mem("a2", {}, 1, 0, byte), 1, 4);
    assert(output.str() == "sb zero, 0(a2)\n");
    output.str({});
    backend.zero(token{}, 0, operand::mem("a2", {}, 1, 208, byte), 16, 4);
    assert(output.str() == "sw zero, 208(a2)\nsw zero, 212(a2)\nsw "
                           "zero, 216(a2)\nsw zero, 220(a2)\n");
    output.str({});
    backend.zero(token{}, 0, operand::mem("a2", {}, 1, 0, byte), 4, 4);
    assert(output.str() == "sw zero, 0(a2)\n");
    output.str({});
    backend.free_scratch_registers(token{}, 0, address_registers);
    backend.finish();
}

// comparisons that produce a boolean or branch, with every scratch register
// held: loads go to the destination, constants use the immediate forms
auto check_comparison_selection() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};
    assembly_output& output{rv32i.output};

    std::vector<operand> address_registers{
        hold_scratch_registers(backend, 30, integer)};

    backend.compare_and_branch(token{}, 0, operand::reg("a0", integer),
                               operand::reg("a1", integer),
                               {
                                   .operation{"<"},
                                   .destination{operand::reg("a2", boolean)},
                               },
                               {});

    assert(output.str() == "slt a2, a0, a1\n");
    output.str({});

    backend.compare_and_branch(token{}, 0,
                               operand::mem("a0", {}, 1, 0, integer),
                               operand::reg("a1", integer),
                               {
                                   .operation{"<"},
                                   .destination{operand::reg("a2", boolean)},
                               },
                               {});
    assert(output.str() == "lw a2, 0(a0)\nslt a2, a2, a1\n");
    output.str({});
    backend.compare_and_branch(token{}, 0, operand::reg("a0", integer),
                               operand::mem("x11", {}, 1, 0, integer),
                               {
                                   .operation{"=="},
                                   .destination{operand::reg("a1", boolean)},
                               },
                               {});
    assert(output.str() == "lw a1, 0(x11)\nxor a1, a0, a1\nsltiu a1, a1, 1\n");
    output.str({});
    backend.compare_and_branch(token{}, 0, operand::reg("a0", byte),
                               operand::reg("a1", integer),
                               {
                                   .operation{"<"},
                                   .destination{operand::reg("a1", boolean)},
                               },
                               {});
    assert(output.str() ==
           "slli a1, a1, 24\nsrai a1, a1, 24\nslt a1, a0, a1\n");
    output.str({});

    backend.compare_and_branch(token{}, 0, operand::reg("a0", integer),
                               operand::reg("a1", integer),
                               {
                                   .operation{"=="},
                                   .target{"comparison_target"},
                                   .branch_on_true{true},
                               },
                               {});

    // every scratch register is held here, so no far jump register is named
    assert(output.str() == "beq a0, a1, comparison_target\n");
    for (const std::string_view operation : {"<", "==", "!="}) {
        output.str({});

        backend.compare_and_branch(
            token{}, 0, operand::reg("a0", integer),
            operand::imm(operation == "<" ? "7" : "0", integer),
            {
                .operation{operation},
                .destination{operand::reg("a0", boolean)},
            },
            {});

        if (operation == "<") {
            assert(output.str() == "slti a0, a0, 7\n");
        } else if (operation == "==") {
            assert(output.str() == "sltiu a0, a0, 1\n");
        } else {
            assert(output.str() == "sltu a0, zero, a0\n");
        }
    }
    output.str({});

    backend.compare_and_branch(token{}, 0, operand::reg("a0", byte),
                               operand::imm("255", integer),
                               {
                                   .operation{"<"},
                                   .destination{operand::reg("a1", boolean)},
                               },
                               {});

    assert(output.str() == "slti a1, a0, -1\n");
    output.str({});
    backend.free_scratch_registers(token{}, 0, address_registers);
    backend.finish();
}

// multiplication by constants: no-op, shift, negation and zero
auto check_multiply_by_constants() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};
    assembly_output& output{rv32i.output};

    std::vector<operand> address_registers{
        hold_scratch_registers(backend, 30, integer)};

    backend.multiply(token{}, 0, operand::reg("a1", integer),
                     operand::imm("1", integer));
    assert(output.str().empty());
    backend.multiply(token{}, 0, operand::reg("a1", integer),
                     operand::imm("8", integer));
    assert(output.str() == "slli a1, a1, 3\n");
    output.str({});
    backend.multiply(token{}, 0, operand::reg("a1", integer),
                     operand::imm("-1", integer));
    assert(output.str() == "sub a1, zero, a1\n");
    output.str({});
    backend.multiply(token{}, 0, operand::mem("a2", {}, 1, 0, integer),
                     operand::imm("0", integer));
    assert(output.str() == "sw zero, 0(a2)\n");
    backend.free_scratch_registers(token{}, 0, address_registers);
    backend.finish();
}

// a known-size copy takes 't0' as its temporary, and any free register when
// 't0' is held
auto check_copy_temporaries() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};
    assembly_output& output{rv32i.output};

    backend.copy(token{}, 0, operand::mem("a1", {}, 1, 208, byte),
                 operand::mem("a2", {}, 1, 240, byte), 4, 4);
    assert(output.str() == "lw t0, 208(a1)\nsw t0, 240(a2)\n");
    output.str({});
    backend.copy(token{}, 0, operand::mem("a1", {}, 1, -16, byte),
                 operand::mem("a2", {}, 1, 16, byte), 7, 4);
    assert(output.str() ==
           "lw t0, -16(a1)\nsw t0, 16(a2)\nlhu t0, -12(a1)\nsh t0, 20(a2)\n"
           "lbu t0, -10(a1)\nsb t0, 22(a2)\n");
    output.str({});
    std::vector<operand> address_registers{
        hold_scratch_registers(backend, 29, integer)};
    backend.copy(token{}, 0, operand::mem("a1", {}, 1, 208, byte),
                 operand::mem("a2", {}, 1, 240, byte), 4, 4);
    assert(output.str() == "lw a0, 208(a1)\nsw a0, 240(a2)\n");
    backend.free_scratch_registers(token{}, 0, address_registers);
    backend.finish();
}

// zeroing: a base offset beyond 12 bits moves into a temporary, and more than
// 16 bytes become a loop with a tail
auto check_zeroing_ranges() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};
    assembly_output& output{rv32i.output};

    backend.zero(token{}, 0, operand::mem("a2", {}, 1, 2047, byte), 16, 4);
    assert(output.str() ==
           "addi t0, a2, 2047\nsw zero, 0(t0)\nsw zero, 4(t0)\nsw zero, "
           "8(t0)\nsw zero, 12(t0)\n");
    output.str({});
    // above 16 stores the loop keeps the code small and stores the tail
    backend.zero(token{}, 0, operand::mem("a2", {}, 1, 0, byte), 67, 4);
    assert(output.str() ==
           "# zero loop of 4-byte accesses: start word aligned\n"
           "addi t0, a2, 0\n# zero 4-byte words\naddi t1, t0, 64\n1:\nsw "
           "zero, 0(t0)\naddi t0, t0, 4\nbne t0, t1, 1b\n"
           "# zero 3 B tail\nsh zero, 0(t0)\nsb zero, 2(t0)\n");
    output.str({});
    backend.finish();
}

// addresses that cannot be lowered are rejected: offsets beyond the 32-bit
// address range, and no free scratch register for the intermediate address
auto check_invalid_addresses_rejected() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};
    assembly_output& output{rv32i.output};

    for (const int64_t offset :
         {INT64_MIN, INT64_C(-4294967296), INT64_C(4294967296), INT64_MAX}) {
        assert(rejected_with([&] {
            backend.address_of(source_tk, 1, operand::reg("a0", integer),
                               operand::mem("a1", {}, 1, offset, integer));
        }));
        backend.finish();
    }
    for (const uint64_t scale :
         {UINT64_C(4294967296), UINT64_C(9223372036854775808)}) {
        assert(not backend.can_lower_index_scale(scale));
    }

    // 'finish' reports scratch usage even when nothing was emitted
    std::istringstream rejected_lines{output.str()};
    std::string rejected_line;
    while (std::getline(rejected_lines, rejected_line)) {
        assert(rejected_line.empty() or
               rejected_line == "# max scratch registers in use: 0");
    }

    std::vector<operand> held_registers{
        hold_scratch_registers(backend, 29, integer)};
    assert(rejected_with([&] {
        backend.address_of(source_tk, 1, operand::reg("a0", integer),
                           operand::mem("a0", {}, 1, 8196, integer));
    }));
    backend.free_scratch_registers(token{}, 0, held_registers);
    backend.finish();
}

// the scratch register pool: which registers it hands out, in which order,
// with 0, 1 and 2 registers reserved for the variables and frame bases, and
// which named registers can never be allocated
auto check_scratch_register_pool() -> void {
    captured_rv32i rv32i;
    machine_rv32i& backend{rv32i.backend};

    std::vector<operand> held_registers;
    for (const size_t reserved_count : {size_t{}, size_t{1}, size_t{2}}) {
        for (const std::string_view name :
             {"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"}) {
            const operand reg{
                backend.alloc_named_register(token{}, 0, name, integer)};
            backend.free_named_register(token{}, 0, reg);
        }
        if (reserved_count >= 1) {
            backend.reserve_variables_base();
        }
        if (reserved_count == 2) {
            backend.reserve_frame_base();
        }
        held_registers.clear();
        uint32_t register_mask{};
        for (size_t count{}; count < 30 - reserved_count; ++count) {
            const operand reg{
                backend.alloc_scratch_register(token{}, 0, integer)};
            assert(reg.base_register() != "zero" and
                   reg.base_register() != "sp");
            assert(reserved_count == 0 or reg.base_register() != "s0");
            assert(reserved_count != 2 or reg.base_register() != "s1");
            assert(reg.base_register().starts_with("a") ==
                   (count >= 22 - reserved_count));
            if (reg.base_register() == "ra") {
                register_mask |= 1;
            }
            if (reg.base_register() == "gp") {
                register_mask |= 2;
            }
            if (reg.base_register() == "tp") {
                register_mask |= 4;
            }
            held_registers.push_back(reg);
        }
        assert(register_mask == 7);
        assert(rejected_with([&] {
            static_cast<void>(
                backend.alloc_scratch_register(source_tk, 0, integer));
        }));
        for (const std::string_view name :
             {"zero", "x0", "sp", "x2", "fp", "x8", "s1", "x9"}) {
            assert(rejected_with([&] {
                static_cast<void>(
                    backend.alloc_named_register(source_tk, 0, name, integer));
            }));
        }
        backend.free_scratch_registers(token{}, 0, held_registers);
        if (reserved_count == 2) {
            backend.release_frame_base();
        }
        if (reserved_count >= 1) {
            backend.release_variables_base();
        }
        const operand named{
            backend.alloc_named_register(token{}, 0, "x1", integer)};
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
        machine_rv32i helper_backend{output};
        helper_backend.set_builtin_types(integer64, integer, half, byte);
        if (operation != '+') {
            const auto emit = [&]() {
                if (operation == '*') {
                    helper_backend.multiply(token{}, 0,
                                            operand::reg("s2", integer),
                                            operand::reg("s3", integer));
                } else {
                    helper_backend.divide(token{}, 0, operation,
                                          operand::reg("s2", integer),
                                          operand::reg("s3", integer));
                }
            };
            emit();
            assert(
                output.str() ==
                std::format(
                    "addi a0, s2, 0\naddi a1, s3, 0\ncall {}\naddi s2, {}, 0\n",
                    operation == '*' ? ".Lbaz_multiply" : ".Lbaz_divide",
                    operation == '%' ? "a1" : "a0"));
            std::vector<operand> occupied;
            for (size_t count{}; count < 30; ++count) {
                occupied.push_back(
                    helper_backend.alloc_scratch_register(token{}, 0, integer));
            }
            emit();
            helper_backend.free_scratch_registers(token{}, 0, occupied);
            output.str({});
        }
        for (size_t count{}; count < 2; ++count) {
            if (operation == '*' or operation == '+') {
                helper_backend.multiply(token{}, 0, operand::reg("a0", integer),
                                        operation == '+'
                                            ? operand::imm("3", integer)
                                            : operand::reg("a1", integer));
            } else {
                helper_backend.divide(token{}, 0, operation,
                                      operand::reg("a0", integer),
                                      operand::reg("a1", integer));
            }
        }
        assert(not output.str().contains(".Lbaz_multiply:"));
        assert(not output.str().contains(".Lbaz_divide:"));
        helper_backend.end_main();
        helper_backend.begin_data(4);
        const std::string assembly{output.str()};
        for (const std::string_view label :
             {".Lbaz_multiply:", ".Lbaz_divide:"}) {
            const bool expected{label == ".Lbaz_multiply:"
                                    ? operation == '*'
                                    : operation == '/' or operation == '%'};
            const size_t first{assembly.find(label)};
            assert((first != std::string::npos) == expected);
            if (expected) {
                assert(assembly.find(label, first + label.size()) ==
                       std::string::npos);
            }
        }
        helper_backend.finish();
    }
}

// ============================================================================
// 9. generated programs with hand-written lines between the backend's output
// ============================================================================

// 'noninline': a function call preserves exactly the allocated registers
// and callees never write the variables base
auto generate_noninline() -> void {
    // hand-written lines are interleaved with the backend's output
    machine_rv32i backend{std::cout, {}, assembler::jump_mode::as_emitted};
    backend.set_builtin_types(integer64, integer, half, byte);
    backend.start();
    std::println("    addi sp, sp, -128\n    sw sp, 124(sp)");
    // a call keeps only allocated registers, the variables base s0 is
    // already allocated
    std::vector<operand> live;
    for (size_t index{1}; index < 32; ++index) {
        if (index != 2 and index != 8) {
            live.push_back(backend.alloc_named_register(
                token{}, 0, std::format("x{}", index), integer));
            std::println("    li x{}, {}", index, 100 + index);
        }
    }
    backend.call_function(token{}, 1, "outer",
                          operand::mem("s0", {}, 1, 4096, integer));
    for (size_t index{1}; index < 32; ++index) {
        if (index != 2) {
            std::println("    sw x{}, {}(sp)", index, (index - 1) * 4);
        }
    }
    std::println(
        "    lw t0, 124(sp)\n    beq t0, sp, 1f\n    j call_failure\n1:");
    for (size_t index{1}; index < 32; ++index) {
        if (index == 2) {
            continue;
        }
        std::println("    lw t0, {}(sp)", (index - 1) * 4);
        if (index == 8) {
            std::println("    la t1, dat\n    addi t1, t1, 2032");
        } else {
            std::println("    li t1, {}", 100 + index);
        }
        std::println("    beq t0, t1, 1f\n    j call_failure\n1:");
    }
    std::println("    addi sp, sp, 128");
    for (const operand& reg : live | std::views::reverse) {
        backend.free_named_register(token{}, 0, reg);
    }
    backend.end_main();
    backend.label(0, "call_failure");
    backend.exit(token{}, 1, operand::imm("1", integer));
    backend.label(0, "outer");
    backend.reserve_frame_base();
    std::println("    la t0, dat\n    li t1, 6128\n    add t0, t0, t1\n"
                 "    beq s1, t0, 1f\n    j call_failure\n1:");
    backend.call_function(token{}, 1, "inner",
                          operand::mem("s1", {}, 1, 8192, integer));
    // the frame base is live here, so the call restores it
    std::println("    la t0, dat\n    li t1, 6128\n    add t0, t0, t1\n"
                 "    beq s1, t0, 1f\n    j call_failure\n1:");
    backend.return_function(1);
    backend.release_frame_base();
    backend.label(0, "inner");
    backend.reserve_frame_base();
    std::println("    la t0, dat\n    li t1, 14320\n    add t0, t0, t1\n"
                 "    beq s1, t0, 1f\n    j call_failure\n1:");
    // callees never write the variables base s0
    for (size_t index{1}; index < 32; ++index) {
        if (index != 2 and index != 8) {
            std::println("    li x{}, -1", index);
        }
    }
    backend.return_function(1);
    backend.release_frame_base();
    backend.finish();
    std::println(".data\ndat:\nvars:\n    .zero 16384");
}

// 'frame-checks': 'check_frame_capacity' jumps to the overflow label exactly
// when the check is enabled and the frame at 'vars + offset' does not fit in
// the 256 bytes of 'vars', for every offset and size around the limit
auto generate_frame_checks() -> void {
    machine_rv32i backend{std::cout, {}, assembler::jump_mode::as_emitted};
    backend.set_builtin_types(integer64, integer, half, byte);
    backend.start();
    const operand continuation{
        backend.alloc_named_register(token{}, 0, "s3", integer)};
    size_t case_index{};
    for (const bool enabled : {false, true}) {
        for (const int offset : {-1, 0, 1, 255, 256, 257}) {
            for (const uint32_t size : {0U, 1U, 256U, 257U, UINT32_MAX}) {
                const bool failed{enabled and
                                  (offset < 0 or offset > 256 or
                                   size > static_cast<uint32_t>(256 - offset))};
                const std::string size_label{
                    std::format("frame_size_{}", case_index)};
                std::println("    la a0, vars\n    addi a0, a0, {}\n"
                             "    la s3, frame_result_{}",
                             offset, case_index);
                backend.check_frame_capacity(
                    token{}, 1, operand::mem("a0", {}, 1, 0, integer),
                    operand::imm(size_label, integer), "frame_overflow",
                    enabled);
                std::println("    li a3, 0\nframe_result_{}:\n    li a4, {}\n"
                             "    beq a3, a4, 1f\n    j frame_failure\n1:",
                             case_index++, failed ? 1 : 0);
                backend.define_constant(size_label, size);
            }
        }
    }
    for (const bool positive : {false, true}) {
        std::println("    la a0, vars\n    addi a0, a0, {}\n    la s3, "
                     "frame_result_{}",
                     positive ? 1 : -1, case_index);
        backend.check_frame_capacity(
            token{}, 1,
            operand::mem("a0", {}, 1,
                         positive ? int64_t{UINT32_MAX} : -int64_t{UINT32_MAX},
                         integer),
            operand::imm("0", integer), "frame_overflow", true);
        std::println("    li a3, 0\nframe_result_{}:\n    li a4, 1\n"
                     "    beq a3, a4, 1f\n    j frame_failure\n1:",
                     case_index++);
    }
    backend.free_named_register(token{}, 0, continuation);
    backend.end_main();
    std::println("frame_overflow:\n    li a3, 1\n    jr s3\nframe_failure:");
    backend.exit(token{}, 1, operand::imm("1", integer));
    backend.finish();
    std::println(".data\ndat:\nvars:\n    .zero 256\nvars.end:");
}

// 'long-loop': an array iteration whose body is larger than a branch reaches
// (8 KiB) still repeats, for strides on both sides of the immediate limits
auto generate_long_loop() -> void {
    machine_rv32i backend{std::cout, {}, assembler::jump_mode::as_emitted};
    backend.set_builtin_types(integer64, integer, half, byte);
    backend.start();
    std::println("    addi sp, sp, -16");
    for (const size_t stride : {4U, 2047U, 2048U, 4094U, 4095U, 8192U}) {
        const std::string loop_label{std::format("long_loop_{}", stride)};
        std::println("    sw zero, 0(sp)\n    li s2, 0");
        backend.label(0, loop_label);
        std::println("    .rept 2048\n    nop\n    .endr\n    addi s2, s2, 1");
        backend.advance_array_iteration(token{}, 1, operand::reg("s2", integer),
                                        operand::mem("sp", {}, 1, 0, integer),
                                        stride, operand::imm("3", integer),
                                        loop_label);
        std::println("    li t0, {}\n    beq s2, t0, 1f\n"
                     "    j long_loop_failure\n1:\n"
                     "    lw t1, 0(sp)\n    li t0, 3\n"
                     "    beq t1, t0, 1f\n    j long_loop_failure\n1:",
                     3 * (stride + 1));
    }
    std::println("    addi sp, sp, 16");
    backend.end_main();
    backend.label(0, "long_loop_failure");
    backend.exit(token{}, 1, operand::imm("1", integer));
    backend.finish();
    std::println(".data\ndat:\nvars:\n    .word 0");
}

// 'far-jumps' and 'far-jumps-optimized': loops, taken branches and skipped
// jumps around padding of 8 KiB (needs 'j') and 1.08 MiB (needs 'jump'),
// executed in every direction
auto generate_far_jumps(const assembler::jump_mode jumps) -> void {
    machine_rv32i backend{std::cout, {}, jumps};
    backend.set_builtin_types(integer64, integer, half, byte);
    backend.start();

    // buffered output must come from the backend, so pad with 'xori' on a
    // register nothing reads
    const auto padding = [&](const size_t count) -> void {
        for (size_t i{}; i < count; ++i) {
            backend.unary(token{}, 1, '~', operand::reg("s4", integer));
        }
    };

    const operand stack{operand::reg("sp", integer)};
    const operand iterator{operand::reg("s2", integer)};
    const operand counter{operand::mem("sp", {}, 1, 0, integer)};

    // 8 KiB needs 'j' and 1.08 MiB needs 'jump' in every direction
    backend.add_subtract(token{}, 1, '-', stack, operand::imm("16", integer));

    for (const size_t count : {2048U, 270000U}) {
        const std::string loop_label{std::format("far_loop_{}", count)};
        backend.copy_value(token{}, 1, counter, operand::imm("0", integer));
        backend.copy_value(token{}, 1, iterator, operand::imm("0", integer));

        backend.label(0, loop_label);
        padding(count);
        backend.add_subtract(token{}, 1, '+', iterator,
                             operand::imm("1", integer));

        backend.advance_array_iteration(token{}, 1, iterator, counter, 4,
                                        operand::imm("3", integer), loop_label);

        backend.compare_and_branch(token{}, 1, operand::reg("s2", integer),
                                   operand::imm("15", integer),
                                   {
                                       .operation{"!="},
                                       .target{"far_failure"},
                                       .branch_on_true{true},
                                   },
                                   {});

        const std::string taken_label{std::format("far_taken_{}", count)};

        backend.compare_and_branch(token{}, 1, operand::reg("s2", integer),
                                   operand::imm("15", integer),
                                   {
                                       .operation{"=="},
                                       .target{taken_label},
                                       .branch_on_true{true},
                                   },
                                   {});

        padding(count);
        backend.exit(token{}, 1, operand::imm("2", integer));
        backend.label(0, taken_label);
        const std::string skipped_label{std::format("far_skipped_{}", count)};
        backend.branch(1, skipped_label);
        padding(count);
        backend.exit(token{}, 1, operand::imm("3", integer));
        backend.label(0, skipped_label);
    }
    backend.add_subtract(token{}, 1, '+', stack, operand::imm("16", integer));

    backend.end_main();
    padding(270000);
    backend.label(0, "far_failure");
    backend.exit(token{}, 1, operand::imm("1", integer));
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

    machine_rv32i compiler{std::cout, {}, jumps};

    program prg{compiler, source, 4096, false, false, false};
    prg.build(std::cout);
}

// ============================================================================
// 10. generated programs from .baz sources, and bounds checks
// ============================================================================

// 'bulk': 'equal', 'arrays_equal' and 'array_copy' on arrays of structs,
// overlapping ranges and empty ranges, with all runtime checks enabled
auto generate_bulk() -> void {
    const std::string_view source{R"baz(
func assert(ok bool) { if not ok exit(1) }
type packed { first i8, second i16 }
func nested(source packed[], destination mut packed[]) count i32 {
    array_copy(source, destination, 1)
    count = 2
}
func main() {
    var source = packed[3]{{1, 300}, {2, -400}, {3, 500}}
    var destination = packed[3]{}
    var single = source[1]
    assert(equal(single, source[1]))
    single.second = 12
    assert(not equal(single, source[1]))
    array_copy(source, destination, nested(source, destination))
    assert(arrays_equal(source, destination, 2))
    assert(not arrays_equal(source, destination, 3))
    var index = i32(2)
    array_copy(source[index], destination[index], 1)
    assert(arrays_equal(source[index], destination[index], 1))
    destination[index].second = 501
    assert(not arrays_equal(source[index], destination[index], 1))
    destination[index].second = 500
    assert(equal(source, destination))
    array_copy(source, destination, 0)
    assert(arrays_equal(source, destination, 0))
    array_copy(source, source, 3)
    assert(equal(source, destination))
    array_copy(destination[1], destination, 2)
    assert(equal(destination[0], source[1]))
    assert(equal(destination[1], source[2]))
    destination[0].first = 7
    assert(not arrays_equal(source[1], destination, 1))
    destination[0] = source[1]
    destination[0].second = 10
    assert(not arrays_equal(source[1], destination, 1))
}
)baz"};
    machine_rv32i compiler{std::cout};
    program prg{compiler, source, 4096, true, true, true};
    prg.build(std::cout);
}

// 'strings-syscall': every string escape reaches the output byte for byte
// through 'write', and 'write' reports a bad file descriptor
auto generate_strings_syscall() -> void {
    const std::string_view source{R"baz(
dat text = "A\0\a\b\t\n\v\f\r\e\"'`\\\x00\x7f\x80\xff\x41B"
func main() {
    var count = write(1, text, array_length(text))
    if count != 20 exit(1)
    var bad = write(-1, text, 1)
    if bad != -9 exit(2)
    exit(0)
}
)baz"};
    machine_rv32i compiler{std::cout};
    program prg{compiler, source, 4096, false, false, false};
    prg.build(std::cout);
}

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
        backend.alloc_named_register(token{}, 0, "a1", integer)};

    backend.check_bounds(
        token{}, 1, operand::reg("a0", integer), parameters.size,
        parameters.allow_end,
        parameters.slice ? operand::reg("a1", integer) : operand{},
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
// checks, size, index and count; 'bounds-silent': the failure handler without
// a line exits with status 255 and prints nothing
auto generate_bounds(const bool silent) -> void {
    machine_rv32i bounds_backend{std::cout};
    bounds_backend.set_builtin_types(integer64, integer, half, byte);
    std::println(
        ".option norvc\n.option norelax\n.text\n.globl _start\n_start:");
    if (silent) {
        std::println("    li a0, -1");
        bounds_backend.check_bounds(token{}, 1, operand::reg("a0", integer), 4,
                                    false, {},
                                    {.upper{true}, .lower{true}, .with_line{}});
        bounds_backend.end_main();
        bounds_backend.emit_bounds_failure_handler(false);
    } else {
        const operand continuation{
            bounds_backend.alloc_named_register(token{}, 0, "s3", integer)};
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
                                    emit_bounds_case(bounds_backend,
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
        bounds_backend.free_named_register(token{}, 0, continuation);
        bounds_backend.end_main();
        std::println(
            "baz_bounds_panic:\n    li a3, 1\n    jr s3\nbounds_failure:");
        bounds_backend.exit(token{}, 1, operand::imm("1", integer));
    }
    bounds_backend.finish();
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

// copies of 0 to 24 bytes, with a known size and a run-time count, between
// addresses at every alignment: only the copied bytes change, and both
// pointers are left unchanged
auto emit_copy_tests(machine_rv32i& backend) -> void {
    for (const bool counted : {false, true}) {
        for (size_t size_bytes{}; size_bytes <= 24; ++size_bytes) {
            for (size_t alignment{}; alignment < 4; ++alignment) {
                const int displacement{
                    std::array{-2048, 0, 2047, 2048}.at(alignment)};
                std::println("    addi sp, sp, -64");
                for (size_t offset{}; offset < 32; ++offset) {
                    std::println("    li a2, {}\n    sb a2, {}(sp)\n    li a2, "
                                 "85\n    sb a2, {}(sp)",
                                 128 + offset, offset, 32 + offset);
                }
                std::println("    addi a0, sp, {}\n    addi a1, sp, {}",
                             alignment, 36 + alignment);
                // both addresses are 'alignment' bytes past a word boundary
                const size_t known_alignment{offset_alignment(alignment, 4)};
                if (counted) {
                    const operand count{backend.begin_array_copy(token{}, 1)};
                    backend.copy_value(
                        token{}, 1, count,
                        operand::imm(std::format("{}", size_bytes), integer));
                    backend.set_array_copy_source(
                        token{}, 1, operand::mem("a0", {}, 1, 0, byte));
                    backend.set_array_copy_destination(
                        token{}, 1, operand::mem("a1", {}, 1, 0, byte));
                    backend.end_array_copy(token{}, 1, 1, known_alignment);
                } else {
                    std::println(
                        "    li a2, {}\n    sub a0, a0, a2\n    sub a1, a1, a2",
                        displacement);
                    backend.copy(token{}, 1,
                                 operand::mem("a0", {}, 1, displacement, byte),
                                 operand::mem("a1", {}, 1, displacement, byte),
                                 size_bytes, known_alignment);
                    std::println(
                        "    li a2, {}\n    add a0, a0, a2\n    add a1, a1, a2",
                        displacement);
                }
                for (size_t offset{}; offset < 32; ++offset) {
                    const size_t start{4 + alignment};
                    const int expected{offset >= start and
                                               offset < start + size_bytes
                                           ? static_cast<int>(128 + offset - 4)
                                           : 85};
                    std::println("    lbu a2, {}(sp)", 32 + offset);
                    emit_expect("a2", expected);
                }
                std::println("    addi a2, sp, {}\n    beq a0, a2, 1f\n    j "
                             "failure\n1:\n    addi a2, sp, {}\n    beq a1, "
                             "a2, 1f\n    j failure\n1:\n    addi sp, sp, 64",
                             alignment, 36 + alignment);
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
        const size_t bits{value_type->size_bytes() * 8};
        const uint32_t mask{UINT32_MAX >> (32 - bits)};
        const auto normalize{[mask, bits](const int64_t value) -> int32_t {
            uint32_t narrowed{static_cast<uint32_t>(value) & mask};
            if ((narrowed & (uint32_t{1} << (bits - 1))) != 0) {
                narrowed |= ~mask;
            }

            return std::bit_cast<int32_t>(narrowed);
        }};
        for (const bool memory_destination : {false, true}) {
            for (const int32_t initial :
                 {0, 1, -1, 7, -7, -128, -32768, 32767, INT32_MIN, INT32_MAX}) {
                for (const int32_t divisor :
                     {1, -1, 2, -2, 3, -7, 255, 65536, INT32_MIN, INT32_MAX}) {
                    for (const unsigned source_kind : {0U, 1U, 2U}) {
                        for (const char operation : {'/', '%'}) {
                            std::println("    la a2, buffer\n    li a1, {}\n   "
                                         " sw a1, 4(a2)",
                                         divisor);
                            const operand destination{
                                memory_destination
                                    ? operand::mem("a2", {}, 1, 0, *value_type)
                                    : operand::reg("a0", *value_type)};

                            operand source{operand::reg("a1", integer)};
                            if (source_kind == 1) {
                                source = operand::mem("a2", {}, 1, 4, integer);
                            } else if (source_kind == 2) {
                                source = operand::imm(
                                    std::format("{}", divisor), integer);
                            }
                            backend.copy_value(
                                token{}, 1, destination,
                                operand::imm(std::format("{}", initial),
                                             integer));
                            backend.divide(token{}, 1, operation, destination,
                                           source);
                            backend.copy_value(token{}, 1,
                                               operand::reg("a0", integer),
                                               destination);
                            const int64_t dividend{normalize(initial)};
                            const int32_t expected{normalize(
                                operation == '/' ? dividend / divisor
                                                 : dividend % divisor)};
                            emit_expect("a0", expected);
                            if (source_kind == 0) {
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
        std::println("    addi sp, sp, -16\n    mv s2, sp\n    li a2, -17\n    "
                     "sw a2, 0(sp)\n    li a2, 3\n    sw a2, 4(sp)");
        const operand destination{operand::mem("s2", {}, 1, 0, integer)};
        const operand source{operand::mem("s2", {}, 1, 4, integer)};
        if (operation == '*') {
            backend.multiply(token{}, 1, destination, source);
        } else {
            backend.divide(token{}, 1, operation, destination, source);
        }
        for (const auto [index, reg] : std::views::enumerate(live)) {
            emit_expect(reg.base_register(), 101 + index, "a2");
        }
        const int expected{operation == '*' ? -51 : operation == '/' ? -5 : -2};
        std::println("    lw a2, 0(sp)\n    li a3, {}\n    beq a2, a3, 1f\n    "
                     "j failure\n1:\n    beq sp, s2, 1f\n    j failure\n1:\n   "
                     " addi sp, sp, 16",
                     expected);
        backend.free_named_registers(token{}, 0, live);
        backend.finish();
    }
}

// '/' and '%' at the same address
auto emit_division_same_address(machine_rv32i& backend) -> void {
    for (const char operation : {'/', '%'}) {
        std::println("    la t0, buffer\n    li a0, -17\n    sw a0, 0(t0)");
        const operand address{operand::mem("t0", {}, 1, 0, integer)};
        backend.divide(token{}, 1, operation, address, address);
        std::println("    lw a0, 0(t0)");
        emit_expect("a0", operation == '/' ? 1 : 0);
        backend.finish();
    }
}

// '*' on every value width, product in a register or memory, and factor in
// a register, memory, constant or the product itself (as 'reuse')
auto emit_multiplication_tests(machine_rv32i& backend) -> void {
    for (const type* value_type : {&integer, &half, &byte}) {
        for (const bool memory_product : {false, true}) {
            for (const int32_t initial :
                 {0, 1, -1, 7, -128, 32767, INT32_MIN, INT32_MAX}) {
                for (const int32_t multiplier :
                     {0, 1, -1, 2, 3, 12, -7, 255, INT32_MIN, INT32_MAX}) {
                    for (const unsigned source_kind : {0U, 1U, 2U, 3U}) {
                        std::println("    la a2, buffer\n    li a1, {}\n    sw "
                                     "a1, 4(a2)",
                                     multiplier);

                        const operand product{
                            memory_product
                                ? operand::mem("a2", {}, 1, 0, *value_type)
                                : operand::reg("a0", *value_type)};

                        operand factor{operand::reg("a1", integer)};
                        if (source_kind == 1) {
                            factor = operand::mem("a2", {}, 1, 4, integer);
                        } else if (source_kind == 2) {
                            factor = operand::imm(std::format("{}", multiplier),
                                                  integer);
                        }
                        backend.copy_value(
                            token{}, 1, product,
                            operand::imm(std::format("{}", initial), integer));
                        backend.multiply(token{}, 1, product, factor,
                                         source_kind == 3);
                        backend.copy_value(
                            token{}, 1, operand::reg("a0", integer), product);
                        const size_t bits{value_type->size_bytes() * 8};
                        const uint32_t mask{UINT32_MAX >> (32 - bits)};
                        uint32_t expected{(static_cast<uint32_t>(initial) *
                                           static_cast<uint32_t>(multiplier)) &
                                          mask};
                        if ((expected & (uint32_t{1} << (bits - 1))) != 0) {
                            expected |= ~mask;
                        }

                        emit_expect("a0", std::bit_cast<int32_t>(expected));

                        if (source_kind == 0) {
                            emit_expect("a1", multiplier);
                        }
                        backend.finish();
                    }
                }
            }
        }
    }
}

// '*' of a value by itself at the same address, with and without the 'reuse'
// of the operand
auto emit_multiplication_aliased_operands(machine_rv32i& backend) -> void {
    for (const bool reuse : {false, true}) {
        std::println("    la a2, buffer\n    li a0, -7\n    sw a0, 0(a2)");
        const operand address{operand::mem("a2", {}, 1, 0, integer)};
        backend.multiply(token{}, 1, address, address, reuse);
        std::println("    lw a0, 0(a2)");
        emit_expect("a0", 49);
        backend.finish();
    }
}

// 'scale_index' multiplies an index register by a constant scale, wrapping
// at 32 bits
auto emit_index_scaling_tests(machine_rv32i& backend) -> void {
    for (const size_t scale :
         {size_t{0}, size_t{1}, size_t{2}, size_t{3}, size_t{7}, size_t{12},
          size_t{256}, size_t{4097}}) {
        std::println("    li a0, -7");
        backend.scale_index(token{}, 1, operand::reg("a0", integer), scale);

        emit_expect("a0", std::bit_cast<int32_t>(uint32_t{0xfffffff9} *
                                                 static_cast<uint32_t>(scale)));

        backend.finish();
    }
}

// '<' on a memory left operand, with the right operand in a register or at
// the same base as the left one
auto emit_memory_comparison_tests(machine_rv32i& backend) -> void {
    for (const bool shared_address : {false, true}) {
        std::println("    la a0, buffer\n    li a1, -257\n    sw a1, 0(a0)\n   "
                     " li a1, 257\n    sw a1, 4(a0)");
        backend.compare_and_branch(
            token{}, 1, operand::mem("x10", {}, 1, 0, integer),
            shared_address ? operand::mem("a0", {}, 1, 4, integer)
                           : operand::reg("a1", integer),
            {
                .operation{"<"},
                .destination{operand::reg("a0", boolean)},
            },
            {});
        emit_expect("a0", 1);
        backend.finish();
    }
}

// every comparison operator against the value -7, inverted or not, branching
// on true or on false, with the operands and the boolean destination in the
// places 'mode' selects:
//
//   mode  operands                 boolean result      branch
//   0     register, register       none                yes
//   1     register, register       a0                  yes
//   2     register, register       a1 (right operand)  yes
//   3     register, register       memory              yes
//   4     register, register       a0                  no
//   5     memory, memory           memory              yes
//   6     register, constant       a0                  yes
//   7     register, constant       memory              yes
auto emit_comparison_matrix(machine_rv32i& backend) -> void {
    size_t comparison_index{};
    for (const std::string_view operation :
         {"==", "!=", "<", ">=", ">", "<="}) {
        for (const bool inverted : {false, true}) {
            for (const bool branch_on_true : {false, true}) {
                for (const int32_t right_value :
                     {INT32_MIN, -2049, -2048, -9, -7, 0, 3, 2046, 2047, 2048,
                      INT32_MAX}) {
                    for (const unsigned mode :
                         {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U}) {
                        const std::string target{
                            std::format("comparison_{}", comparison_index++)};
                        bool expected{};
                        if (operation == "==") {
                            expected = -7 == right_value;
                        } else if (operation == "!=") {
                            expected = -7 != right_value;
                        } else if (operation == "<") {
                            expected = -7 < right_value;
                        } else if (operation == ">=") {
                            expected = -7 >= right_value;
                        } else if (operation == ">") {
                            expected = -7 > right_value;
                        } else {
                            expected = -7 <= right_value;
                        }
                        expected = expected != inverted;
                        operand destination;
                        if (mode == 1 or mode == 4 or mode == 6) {
                            destination = operand::reg("a0", boolean);
                        } else if (mode == 2) {
                            destination = operand::reg("a1", boolean);
                        } else if (mode == 3 or mode == 5 or mode == 7) {
                            destination = operand::mem("a2", {}, 1, 8, boolean);
                        }
                        std::println(
                            "    li a0, -7\n    li a1, {}\n    la a2, buffer\n "
                            "   sw a0, 0(a2)\n    sw a1, 4(a2)",
                            right_value);
                        const operand lhs{
                            mode == 5 ? operand::mem("a2", {}, 1, 0, integer)
                                      : operand::reg("a0", integer)};
                        operand rhs{operand::reg("a1", integer)};
                        if (mode == 5) {
                            rhs = operand::mem("a2", {}, 1, 4, integer);
                        } else if (mode >= 6) {
                            rhs = operand::imm(std::format("{}", right_value),
                                               integer);
                        }

                        backend.compare_and_branch(
                            token{}, 1, lhs, rhs,
                            {
                                .operation{operation},
                                .inverted{inverted},
                                .destination{destination},
                                .target{mode == 4 ? std::string_view{}
                                                  : target},
                                .branch_on_true{branch_on_true},
                            },
                            {});

                        if (mode != 4) {
                            if (expected == branch_on_true) {
                                std::println("    j failure\n{}:", target);
                            } else {
                                std::println("    j {}_done\n{}:\n    j "
                                             "failure\n{}_done:",
                                             target, target, target);
                            }
                        }
                        if (not destination.is_empty()) {
                            backend.copy_value(token{}, 1,
                                               operand::reg("a3", integer),
                                               destination);
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
    for (size_t count{1}; count <= 35; ++count) {
        for (size_t alignment{}; alignment < 4; ++alignment) {
            std::println("    addi sp, sp, -64\n    li a0, -1");
            for (size_t offset{}; offset < 64; offset += 4) {
                std::println("    sw a0, {}(sp)", offset);
            }
            const size_t start{4 + alignment};
            const size_t displacement{count % 2 == 0 ? 2047U : 2048U};
            std::println("    li a2, {}\n    sub a2, sp, a2",
                         displacement - start);
            backend.zero(token{}, 1,
                         operand::mem("a2", {}, 1,
                                      static_cast<int64_t>(displacement), byte),
                         count, offset_alignment(start, 4));
            for (size_t offset{}; offset < 64; ++offset) {
                const int expected{
                    offset >= start and offset < start + count ? 0 : 255};
                std::println("    lbu a0, {}(sp)", offset);
                emit_expect("a0", expected, "a1");
            }
            std::println("    addi sp, sp, 64");
            backend.finish();
        }
    }
}

// unary '-' and '~' on every value width (and 'bool'), in a register or
// memory
auto emit_unary_tests(machine_rv32i& backend) -> void {
    for (const type* value_type : {&integer, &half, &byte, &boolean}) {
        for (const bool memory_destination : {false, true}) {
            for (const int32_t initial : {0, 1, -1, -128, -32768, INT32_MIN}) {
                for (const char operation : {'-', '~'}) {
                    std::println("    la a2, buffer");

                    const operand destination{
                        memory_destination
                            ? operand::mem("a2", {}, 1, 0, *value_type)
                            : operand::reg("a0", *value_type)};

                    backend.copy_value(
                        token{}, 1, destination,
                        operand::imm(std::format("{}", initial), integer));
                    backend.unary(token{}, 1, operation, destination);
                    backend.copy_value(token{}, 1, operand::reg("a0", integer),
                                       destination);
                    const uint32_t bits{static_cast<uint32_t>(initial)};
                    const size_t width{value_type->size_bytes() * 8};
                    const uint32_t mask{UINT32_MAX >> (32 - width)};
                    uint32_t expected{
                        (operation == '-' ? uint32_t{} - bits : ~bits) & mask};
                    if (value_type != &boolean and
                        (expected & (uint32_t{1} << (width - 1))) != 0) {
                        expected |= ~mask;
                    }

                    emit_expect("a0", std::bit_cast<int32_t>(expected));

                    backend.finish();
                }
            }
        }
    }
}

// '+', '-', '&', '|' and '^' on every value width, destination in a register
// or memory, and source in a register, memory or constant around the 12-bit
// immediate limits: the result wraps at the width of the destination
auto emit_add_subtract_bitwise_tests(machine_rv32i& backend) -> void {
    for (const type* value_type : {&integer, &half, &byte}) {
        for (const char operation : {'+', '-', '&', '|', '^'}) {
            for (const bool memory_destination : {false, true}) {
                for (const unsigned source_kind : {0U, 1U, 2U}) {
                    for (const int32_t source_value :
                         {-2049, -2048, -1, 0, 1, 2047, 2048}) {
                        const size_t bits{value_type->size_bytes() * 8};
                        const uint32_t mask{UINT32_MAX >> (32 - bits)};

                        // folding removes the constants that keep the
                        // destination
                        const uint32_t kept{operation == '&' ? mask : 0U};
                        if (source_kind == 2 and
                            (static_cast<uint32_t>(source_value) & mask) ==
                                kept) {
                            continue;
                        }

                        std::println("    la a2, buffer\n    li a1, {}\n    sw "
                                     "a1, 4(a2)",
                                     source_value);

                        const operand destination{
                            memory_destination
                                ? operand::mem("a2", {}, 1, 0, *value_type)
                                : operand::reg("a0", *value_type)};

                        operand source{operand::reg("a1", integer)};
                        if (source_kind == 1) {
                            source = operand::mem("a2", {}, 1, 4, integer);
                        } else if (source_kind == 2) {
                            source = operand::imm(
                                std::format("{}", source_value), integer);
                        }
                        backend.copy_value(token{}, 1, destination,
                                           operand::imm("127", integer));
                        uint32_t expected{127};
                        const uint32_t rhs{static_cast<uint32_t>(source_value)};
                        if (operation == '+' or operation == '-') {
                            backend.add_subtract(token{}, 1, operation,
                                                 destination, source);
                            expected = operation == '+' ? expected + rhs
                                                        : expected - rhs;
                        } else {
                            backend.bitwise(token{}, 1, operation, destination,
                                            source);
                            if (operation == '&') {
                                expected &= rhs;
                            } else if (operation == '|') {
                                expected |= rhs;
                            } else {
                                expected ^= rhs;
                            }
                        }
                        const uint32_t sign{uint32_t{1} << (bits - 1)};
                        expected &= mask;
                        if ((expected & sign) != 0) {
                            expected |= ~mask;
                        }
                        backend.copy_value(token{}, 1,
                                           operand::reg("a0", integer),
                                           destination);

                        emit_expect("a0", std::bit_cast<int32_t>(expected));

                        backend.finish();
                    }
                }
            }
        }
    }
}

// '<<' and '>>' on every value width (and 'bool'), with the count (35 in a
// register or memory, whose hardware masks it to 3, or the constant 3)
auto emit_shift_tests(machine_rv32i& backend) -> void {
    for (const type* value_type : {&integer, &half, &byte, &boolean}) {
        for (const char operation : {'<', '>'}) {
            for (const bool memory_destination : {false, true}) {
                for (const unsigned count_kind : {0U, 1U, 2U}) {
                    std::println(
                        "    la a2, buffer\n    li a1, 35\n    sw a1, 4(a2)");

                    const operand destination{
                        memory_destination
                            ? operand::mem("a2", {}, 1, 0, *value_type)
                            : operand::reg("a0", *value_type)};

                    operand count{operand::reg("a1", byte)};
                    if (count_kind == 1) {
                        count = operand::mem("a2", {}, 1, 4, byte);
                    } else if (count_kind == 2) {
                        count = operand::imm("3", integer);
                    }

                    backend.copy_value(token{}, 1, destination,
                                       operand::imm("-16", integer));
                    backend.shift(token{}, 1, operation, destination, count);
                    backend.copy_value(token{}, 1, operand::reg("a0", integer),
                                       destination);

                    emit_expect("a0", value_type == &boolean
                                          ? (operation == '<' ? 128 : 30)
                                          : (operation == '<' ? -128 : -2));

                    backend.finish();
                }
            }
        }
    }

    // the count is the shifted register itself
    std::println("    li a0, 3");
    backend.shift(token{}, 1, '<', operand::reg("a0", integer),
                  operand::reg("x10", integer));
    emit_expect("a0", 24);
    backend.finish();
}

// loads, stores and 'address_of' on memory at offsets around the 12-bit and
// 32-bit limits, with and without a scaled index register: the address is
// always 'buffer'
auto emit_address_range_tests(machine_rv32i& backend) -> void {
    for (const int64_t offset :
         {INT64_C(-4294967295), INT64_C(-2147483648), INT64_C(-2049),
          INT64_C(-2048), INT64_C(2047), INT64_C(2048), INT64_C(8196),
          INT64_C(2147483648), INT64_C(4294967295)}) {
        std::println("    la t6, buffer\n    li a3, {}\n    sub t6, t6, a3",
                     static_cast<uint32_t>(offset));
        const operand direct{operand::mem("x31", {}, 1, offset, integer)};
        backend.copy_value(token{}, 1, direct, operand::imm("42", integer));
        backend.copy_value(token{}, 1, operand::reg("a0", integer), direct);
        std::println("    li a3, 42\n    bne a0, a3, failure");
        backend.address_of(token{}, 1, operand::reg("t6", integer), direct);
        std::println("    la a3, buffer\n    bne t6, a3, failure");
        backend.finish();
        for (const uint64_t scale :
             std::array<uint64_t, 11>{1, 2, 4, 8, 16, 32, 64, 128, 256, 65536,
                                      UINT64_C(2147483648)}) {
            std::println("    la t6, buffer\n    li a3, {}\n    "
                         "sub t6, t6, a3\n    li t5, 5\n    li a3, "
                         "{}\n    sub t6, t6, a3\n    li t4, 42",
                         static_cast<uint32_t>(offset),
                         static_cast<uint32_t>(5 * scale));
            const operand address{
                operand::mem("x31", "x30", scale, offset, integer)};
            backend.copy_value(token{}, 1, address,
                               operand::reg("x29", integer));
            backend.copy_value(token{}, 1, operand::reg("t3", integer),
                               address);
            std::println("    li a3, 42\n    bne t3, a3, failure");
            backend.address_of(token{}, 1, operand::reg("t3", integer),
                               address);
            std::println("    la a3, buffer\n    bne t3, a3, failure");
            std::println("    la a4, buffer_copy");
            backend.copy_value(token{}, 1,
                               operand::mem("a4", {}, 1, 0, integer), address);
            backend.copy_value(token{}, 1, operand::reg("t3", integer),
                               operand::mem("a4", {}, 1, 0, integer));
            std::println("    li a3, 42\n    bne t3, a3, failure");
            std::println("    la t6, buffer\n    li a3, {}\n    "
                         "sub t6, t6, a3\n    li t5, 5\n    li a3, "
                         "{}\n    sub t6, t6, a3",
                         static_cast<uint32_t>(offset),
                         static_cast<uint32_t>(5 * scale));
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
    for (const uint64_t scale :
         {UINT64_C(1), UINT64_C(2), UINT64_C(4), UINT64_C(256), UINT64_C(65536),
          UINT64_C(2147483648)}) {
        for (const std::string_view base : {"a2", "x13", "a4", "zero", "x0"}) {
            for (const std::string_view destination : {"a2", "a3", "a4"}) {
                std::println("    li a2, 100\n    li a3, 5\n    la a4, buffer");
                backend.address_of(
                    token{}, 1, operand::reg(destination, integer),
                    operand::mem(base, "a3", scale, 40, integer));
                const uint32_t offset{static_cast<uint32_t>(5 * scale + 40)};
                if (base == "a4") {
                    std::println(
                        "    la a5, buffer\n    li a6, {}\n    add a5, a5, a6",
                        offset);
                } else {
                    const uint32_t base_value{base == "a2"    ? 100U
                                              : base == "x13" ? 5U
                                                              : 0U};
                    std::println("    li a5, {}", offset + base_value);
                }
                std::println("    beq {}, a5, 1f\n    j failure\n1:",
                             destination);
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
                           operand::imm("-1", integer));
        backend.copy_value(token{}, 1, operand::reg("a0", integer),
                           operand::mem("a2", {}, 1, 0, *value_type));
        std::println("    li a1, {}\n    bne a0, a1, failure",
                     value_type == &boolean ? 255 : -1);
    }
    backend.address_of(token{}, 1, operand::reg("t6", integer),
                       operand::mem("sp", "sp", 4, 2048, integer));
    std::println("    slli a0, sp, 2\n    add a0, a0, sp\n    li a1, 2048\n    "
                 "add a0, a0, a1\n    bne t6, a0, failure");
}

// 'read' and 'write' system calls: results, argument registers, the stack
// pointer, and the rejection of a held syscall register. Input comes from
// stdin and is written to stdout, which 'test-rv32i.sh' compares.
auto emit_io_tests(machine_rv32i& backend) -> void {
    std::vector<operand> io_args;
    for (const std::string_view name :
         backend.registers_for_builtin_function(machine::builtin_function::read)
             .arguments) {
        io_args.push_back(
            backend.alloc_named_register(token{}, 0, name, integer));
    }
    std::println(
        "    mv s2, sp\n    li a0, 0\n    la a1, buffer\n    li a2, 6");
    backend.read(token{}, 1, io_args.at(0), io_args.at(0), io_args.at(1),
                 io_args.at(2));
    std::println("    li t0, 6\n    bne a0, t0, failure\n    bne a2, t0, "
                 "failure\n    la t0, buffer\n    bne a1, t0, failure\n    bne "
                 "sp, s2, failure\n    li a0, 1");
    backend.write(token{}, 1, io_args.at(0), io_args.at(0), io_args.at(1),
                  io_args.at(2));
    std::println(
        "    li t0, 6\n    bne a0, t0, failure\n    bne a2, t0, failure\n    "
        "la t0, buffer\n    bne a1, t0, failure\n    li a0, -1");
    backend.read(token{}, 1, io_args.at(0), io_args.at(0), io_args.at(1),
                 io_args.at(2));
    std::println("    li t0, -9\n    bne a0, t0, failure\n    li a0, -1");
    backend.write(token{}, 1, io_args.at(0), io_args.at(0), io_args.at(1),
                  io_args.at(2));
    std::println(
        "    li t0, -9\n    bne a0, t0, failure\n    bne sp, s2, failure");
    backend.free_named_registers(token{}, 0, io_args);
    const operand held_syscall_register{
        backend.alloc_named_register(token{}, 0, "a7", integer)};
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
    machine_rv32i backend{std::cout};
    backend.set_builtin_types(integer64, integer, half, byte);

    std::println(
        ".option norvc\n.option norelax\n.text\n.globl _start\n_start:");

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
    backend.divide(token{}, 1, '/', operand::reg("a0", integer),
                   operand::imm("0", integer));
    backend.exit(token{}, 1, operand::imm("0", integer));
    for (const uint32_t line : {0U, 9U, 123U, UINT32_MAX}) {
        std::println(".globl bounds_line_{}\nbounds_line_{}:\n    li a0, -1",
                     line, line);
        const token location{{}, 0, {}, 0, {}, line, false};
        backend.check_bounds(location, 1, operand::reg("a0", integer), 4, false,
                             {},
                             {.upper{true}, .lower{true}, .with_line{true}});
        backend.exit(token{}, 1, operand::imm("0", integer));
    }
    backend.emit_bounds_failure_handler(true);
    std::println("failure:\n    li a0, 1\n    li a7, 93\n    ecall");
    backend.begin_data(4);
    std::println("buffer: .zero 16\nbuffer_copy: .word 0\npointer: .word 0");
    backend.finish();
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
    } else if (mode == "bulk") {
        generate_bulk();
    } else if (mode == "strings-syscall") {
        generate_strings_syscall();
    } else if (mode == "bounds-matrix" or mode == "bounds-silent") {
        generate_bounds(mode == "bounds-silent");
    } else {
        // no argument: host checks, then the runtime program on stdout
        check_jump_optimizer();
        check_jump_resolution();
        check_line_sizes();
        check_equal_size_choice();
        check_backend_jump_optimization();
        check_copies_and_variable_comments();
        check_comments_with_source_positions();
        check_array_copy_pointers();
        check_arrays_equal_registers_rv32i();
        check_arrays_equal_condition_x86();
        check_equal_results_stored_directly();
        check_x86_scratch_registers();
        check_x86_syscall_register_saving();
        check_x86_write_in_program();
        check_x86_nested_syscalls_rejected();
        check_index_scale_support();
        check_memory_comparison_tail();
        check_known_size_copy();
        check_string_data_escapes();
        check_shifts();
        check_add_subtract_bitwise();
        check_zero_operations_and_zero_shifts();
        check_narrow_values_and_large_constants();
        check_copy_widths();
        check_byte_bitwise();
        check_shift_of_variables();
        check_address_lowering();
        check_address_forms_without_temporaries();
        check_constant_stores_and_small_zeroing();
        check_comparison_selection();
        check_multiply_by_constants();
        check_copy_temporaries();
        check_zeroing_ranges();
        check_invalid_addresses_rejected();
        check_scratch_register_pool();
        check_multiply_divide_routines();
        generate_runtime_program();
    }

    return 0;
}
