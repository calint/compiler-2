#include <iostream>
#include <sstream>

#include "../../src/decouple_impl.hpp"
#include "../../src/jump_optimizer.hpp"
#include "../../src/machine_rv32i.hpp"
#include "../../src/machine_x86.hpp"
#include "../../src/program.hpp"

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

auto main(const int argc, const char* argv[]) -> int {
    if (argc > 1 and std::string_view{argv[1]} == "optimize-jumps") {
        jump_optimizer::rv32i::optimize(std::cin, std::cout);

        return 0;
    }
    {
        const auto optimize = [](const std::string& assembly) -> std::string {
            std::istringstream input{assembly};
            std::ostringstream output;
            jump_optimizer::rv32i::optimize(input, output);

            return output.str();
        };
        assert(optimize("    j bool_end_139_12\n    1:\n    j "
                        "bool_end_139_12\n    bool_end_139_12:\n") ==
               "    1:\n    bool_end_139_12:\n");
        assert(optimize("beq a0, a1, 1f\nj end\n1:\naddi a0, a0, 1\nend:\n") ==
               "bne a0, a1, end\n1:\naddi a0, a0, 1\nend:\n");
        assert(
            optimize("1:\naddi a0, a0, 1\nbnez a0, 1b\nj 1f\n# next\n1:\n") ==
            "1:\naddi a0, a0, 1\nbnez a0, 1b\n# next\n1:\n");
        const std::string barrier{
            "beqz a0, 1f\nj end\n1:\n.space 8192\nend:\n"};
        assert(optimize(barrier) == barrier);
        std::string distant{"beqz a0, 1f\nj end\n1:\n"};
        for (size_t count{}; count < 1024; ++count) {
            distant += "addi a0, a0, 1\n";
        }
        distant += "end:\n";
        assert(optimize(distant) == distant);
        // every supported inverse must preserve operand order in both
        // directions
        for (const auto& [first, second] :
             std::array<std::pair<std::string_view, std::string_view>, 8>{{
                 {"beq", "bne"},
                 {"blt", "bge"},
                 {"bltu", "bgeu"},
                 {"bgt", "ble"},
                 {"bgtu", "bleu"},
                 {"beqz", "bnez"},
                 {"bltz", "bgez"},
                 {"bgtz", "blez"},
             }}) {
            for (const bool reverse : {false, true}) {
                const std::string_view mnemonic{reverse ? second : first};
                const std::string_view inverted{reverse ? first : second};
                const std::string_view operands{
                    first.ends_with('z') ? "a0," : "a0, a1,"};
                const std::string input{std::format(
                    "{} {} 1f\n# keep\nj 2f\n1:\naddi a0, a0, 1\n2:\n",
                    mnemonic, operands)};
                const std::string expected{
                    std::format("{} {} 2f\n# keep\n1:\naddi a0, a0, 1\n2:\n",
                                inverted, operands)};
                assert(optimize(input) == expected);
                assert(optimize(expected) == expected);
            }
        }
        assert(optimize("beqz a0, end\nj end\naddi a0, a0, 1\nend:\n") ==
               "j end\naddi a0, a0, 1\nend:\n");
        assert(
            optimize("j end\nj other\naddi a0, a0, 1\nother:\necall\nend:\n") ==
            "j end\naddi a0, a0, 1\nother:\necall\nend:\n");
        assert(optimize("1:\naddi a0, a0, 1\nbeqz a0, 2f\nj 1b\n2:\n") ==
               "1:\naddi a0, a0, 1\nbnez a0, 1b\n2:\n");
        for (const std::string unchanged :
             {"1:\nj 1b\n", "call end\nend:\n", "jal ra, end\nend:\n",
              "beqz a0, 1f\nentry:\nj end\n1:\necall\nend:\n",
              "beqz a0, 1f\nj end\n1:\nunknown_instruction\nend:\n",
              "j end\n.balign 16\nend:\n", "j missing\n"}) {
            assert(optimize(unchanged) == unchanged);
        }
        // pseudo-instructions can exceed branch reach with fewer than 1024
        // lines
        std::string expanded{"beqz a0, 1f\nj end\n1:\n"};
        for (size_t count{}; count < 512; ++count) {
            expanded += "li a0, 1234567\n";
        }
        expanded += "end:\n";
        assert(optimize(expanded) == expanded);
    }
    const type integer64{"i64", 8, true};
    const type integer{"i32", 4, true};
    const type half{"i16", 2, true};
    const type byte{"i8", 1, true};
    const type boolean{"bool", 1, true};
    const type empty{"void", 0, true};
    if (argc > 1 and std::string_view{argv[1]} == "x86-scales") {
        machine_x86 backend{std::cout, {}};
        backend.set_builtin_types(integer64, integer, half, byte, boolean,
                                  empty);
        std::println("bits 64\nsection .text\nglobal _start\n_start:");
        for (const uint64_t scale :
             {UINT64_C(256), UINT64_C(4294967296),
              UINT64_C(9223372036854775808)}) {
            std::println("mov rax, 5\nmov rbx, 100\ncmp rax, rax");
            backend.address_of(
                token{}, 0, operand::reg("rcx", integer64),
                operand::mem("rbx", "rax", scale, 40, integer64));
            std::println("jnz failure\nmov rdx, {}\ncmp rcx, rdx\njne failure",
                         uint64_t{140} + 5 * scale);
        }
        std::println("mov rax, 60\nxor rdi, rdi\nsyscall\nfailure:\nmov rax, "
                     "60\nmov rdi, 1\nsyscall");
        backend.finish();

        return 0;
    }
    {
        machine_rv32i backend;
        std::ostringstream output;
        backend.use_stream(output);
        // equal expanded costs must retain the version without scratch
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
            const std::string candidate{
                std::format("  # comment\n\t.option norelax\n.Lcandidate: \t# "
                            "label\n\t{}  # instruction\n",
                            instruction)};
            const std::string alternative{
                cost == 1 ? "addi a0, a1, 0\n"
                          : "addi a0, a1, 0\naddi a0, a0, 1\n"};
            backend.emit_most_efficient(token{}, 0, candidate, alternative);
            assert(output.str() == candidate);
            output.str({});
            backend.emit_most_efficient(token{}, 0, candidate,
                                        "addi a0, a1, 0\n");
            assert(output.str() ==
                   (cost == 1 ? candidate : "addi a0, a1, 0\n"));
            output.str({});
        }
    }
    {
        machine_rv32i backend;
        assembly_output copies;
        backend.set_builtin_types(integer64, integer, half, byte, boolean,
                                  empty);
        backend.use_stream(copies);
        const operand address{operand::mem("s0", {}, 1, 24, integer)};
        backend.copy_value(token{}, 0, address, address);
        assert(copies.str().empty());
        backend.copy_value(token{}, 0,
                           operand::mem("s0", {}, 1, 28, integer), address);
        assert(copies.str().contains("lw t0, 24(s0)\n"));
        assert(copies.str().contains("sw t0, 28(s0)\n"));

        copies.str({});
        backend.copy_value(token{}, 0,
                           operand::mem("s0", {}, 1, 24, byte), address);
        assert(copies.str().contains("lw t0, 24(s0)\n"));
        assert(copies.str().contains("sb t0, 24(s0)\n"));

        std::ostringstream comments;
        backend.use_stream(comments);
        backend.comment_variable(token{}, 0, "arr: i32[4]", 16,
                                 operand::mem("s0", {}, 1, 208, integer));
        backend.comment_variable(token{}, 0, "indexed", 4,
                                 operand::mem("s1", "t0", 4, -16, integer));
        backend.comment_variable(token{}, 0, "first", 4,
                                 operand::mem("s0", {}, 1, 0, integer));
        assert(comments.str() == "# arr: i32[4] (16 B @ [s0 + 208])\n"
                                 "# indexed (4 B @ [s1 + t0 * 4 - 16])\n"
                                 "# first (4 B @ [s0])\n");
        // columns must be relative to the source line rather than the file
        machine_rv32i located{"first\n    value"};
        located.use_stream(comments);
        comments.str({});
        const token location{{}, 10, "value", 15, {}, 2, false};
        located.comment(location, 1, "assignment");
        located.comment(token{}, 0, "generated");
        assert(comments.str() == "    # [2:5] assignment\n# generated\n");
        located.set_builtin_types(integer64, integer, half, byte, boolean,
                                  empty);
        comments.str({});
        const operand scratch{
            located.alloc_scratch_register(location, 1, integer)};
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
            ordered.push_back(
                located.alloc_scratch_register(location, 1, integer));
            assert(ordered.back().base_register() == name);
        }
        located.free_scratch_registers(location, 1, ordered);
        comments.str({});
        const operand count{located.begin_array_copy(location, 1)};
        located.copy_value(location, 1, count, operand::imm("2", integer));
        located.set_array_copy_source(1,
                                      operand::mem("s0", {}, 1, 216, integer));
        located.set_array_copy_destination(
            1, operand::mem("s0", {}, 1, 208, integer));
        located.end_array_copy(location, 1, 4);
        for (const std::string_view text :
             {"t0: source, t1: destination, t2: count",
              "t2: elements to bytes (4 bytes/element)",
              "t3: copy value, t4: words, t2: tail bytes", "copy 4-byte words",
              "copy optional 2-byte tail", "copy optional final byte"}) {
            assert(comments.str().contains(std::format("# [2:5] {}\n", text)));
        }
        located.finish();
    }
    if (argc > 1 and std::string_view{argv[1]} == "noninline") {
        machine_rv32i backend;
        backend.set_builtin_types(integer64, integer, half, byte, boolean, empty);
        backend.use_stream(std::cout);
        backend.program_start();
        std::println("    addi sp, sp, -128\n    sw sp, 124(sp)");
        for (size_t index{1}; index < 32; ++index) {
            if (index != 2 and index != 8) {
                std::println("    li x{}, {}", index, 100 + index);
            }
        }
        backend.call_function(1, "outer", operand::mem("s0", {}, 1, 4096, integer));
        for (size_t index{1}; index < 32; ++index) {
            if (index != 2) {
                std::println("    sw x{}, {}(sp)", index, (index - 1) * 4);
            }
        }
        std::println("    lw t0, 124(sp)\n    beq t0, sp, 1f\n    j call_failure\n1:");
        for (size_t index{1}; index < 32; ++index) {
            if (index == 2) {
                continue;
            }
            std::println("    lw t0, {}(sp)", (index - 1) * 4);
            if (index == 8) {
                std::println("    la t1, dat");
            } else {
                std::println("    li t1, {}", 100 + index);
            }
            std::println("    beq t0, t1, 1f\n    j call_failure\n1:");
        }
        std::println("    addi sp, sp, 128");
        backend.program_end();
        backend.label(0, "call_failure");
        backend.exit(token{}, 1, operand::imm("1", integer));
        backend.label(0, "outer");
        backend.reserve_frame_base();
        std::println("    la t0, dat\n    li t1, 4096\n    add t0, t0, t1\n"
                     "    beq s1, t0, 1f\n    j call_failure\n1:");
        backend.call_function(1, "inner", operand::mem("s1", {}, 1, 8192, integer));
        backend.return_function(1);
        backend.release_frame_base();
        backend.label(0, "inner");
        backend.reserve_frame_base();
        std::println("    la t0, dat\n    li t1, 12288\n    add t0, t0, t1\n"
                     "    beq s1, t0, 1f\n    j call_failure\n1:");
        for (size_t index{1}; index < 32; ++index) {
            if (index != 2) {
                std::println("    li x{}, -1", index);
            }
        }
        backend.return_function(1);
        backend.release_frame_base();
        backend.finish();
        std::println(".data\ndat:\n    .zero 16384");

        return 0;
    }
    if (argc > 1 and std::string_view{argv[1]} == "frame-checks") {
        machine_rv32i backend;
        backend.set_builtin_types(integer64, integer, half, byte, boolean, empty);
        backend.use_stream(std::cout);
        backend.program_start();
        const operand continuation{backend.alloc_named_register(token{}, 0, "s3", integer)};
        size_t case_index{};
        for (const bool enabled : {false, true}) {
            for (const int offset : {-1, 0, 1, 255, 256, 257}) {
                for (const uint32_t size : {0U, 1U, 256U, 257U, UINT32_MAX}) {
                    const bool failed{enabled and (offset < 0 or offset > 256 or
                        size > static_cast<uint32_t>(256 - offset))};
                    const std::string size_label{std::format("frame_size_{}", case_index)};
                    std::println("    la a0, vars\n    addi a0, a0, {}\n"
                                 "    la s3, frame_result_{}", offset, case_index);
                    backend.check_frame_capacity(token{}, 1, operand::mem("a0", {}, 1, 0, integer),
                        operand::imm(size_label, integer), "frame_overflow", enabled);
                    std::println("    li a3, 0\nframe_result_{}:\n    li a4, {}\n"
                                 "    beq a3, a4, 1f\n    j frame_failure\n1:", case_index++, failed ? 1 : 0);
                    backend.define_constant(size_label, size);
                }
            }
        }
        for (const bool positive : {false, true}) {
            std::println("    la a0, vars\n    addi a0, a0, {}\n    la s3, frame_result_{}",
                         positive ? 1 : -1, case_index);
            backend.check_frame_capacity(token{}, 1,
                operand::mem("a0", {}, 1, positive ? int64_t{UINT32_MAX} : -int64_t{UINT32_MAX}, integer),
                operand::imm("0", integer), "frame_overflow", true);
            std::println("    li a3, 0\nframe_result_{}:\n    li a4, 1\n"
                         "    beq a3, a4, 1f\n    j frame_failure\n1:", case_index++);
        }
        backend.free_named_register(token{}, 0, continuation);
        backend.program_end();
        std::println("frame_overflow:\n    li a3, 1\n    jr s3\nframe_failure:");
        backend.exit(token{}, 1, operand::imm("1", integer));
        backend.finish();
        std::println(".data\ndat:\nvars:\n    .zero 256\nvars.end:");

        return 0;
    }
    if (argc > 1 and std::string_view{argv[1]} == "long-loop") {
        machine_rv32i backend;
        backend.set_builtin_types(integer64, integer, half, byte, boolean, empty);
        backend.use_stream(std::cout);
        backend.program_start();
        std::println("    addi sp, sp, -16");
        for (const size_t stride : {4U, 2047U, 2048U, 4094U, 4095U, 8192U}) {
            const std::string loop_label{std::format("long_loop_{}", stride)};
            std::println("    sw zero, 0(sp)\n    li s2, 0");
            backend.label(0, loop_label);
            std::println("    .rept 2048\n    nop\n    .endr\n    addi s2, s2, 1");
            backend.advance_array_iteration(1, operand::reg("s2", integer),
                                            operand::mem("sp", {}, 1, 0, integer),
                                            stride, 3, loop_label);
            std::println("    li t0, {}\n    beq s2, t0, 1f\n"
                         "    j long_loop_failure\n1:\n"
                         "    lw t1, 0(sp)\n    li t0, 3\n"
                         "    beq t1, t0, 1f\n    j long_loop_failure\n1:",
                         3 * (stride + 1));
        }
        std::println("    addi sp, sp, 16");
        backend.program_end();
        backend.label(0, "long_loop_failure");
        backend.exit(token{}, 1, operand::imm("1", integer));
        backend.finish();
        std::println(".data\ndat:\n    .word 0");

        return 0;
    }
    {
        const std::string_view source{R"baz(
func main() {
    var source[4] i32
    var destination[4] i32
    array_copy(source[2], destination[1], 2)
}
)baz"};
        machine_rv32i compiler;
        program prg{compiler, source, 4096, false, false, false};
        std::ostringstream output;
        prg.build(output);
        // reserved pointers must hold the address throughout index arithmetic
        assert(output.str().contains("slli t0, t3, 2\n"));
        assert(output.str().contains("add t0, t0, s0\n"));
        assert(output.str().contains("slli t1, t3, 2\n"));
        assert(output.str().contains("add t1, t1, s0\n"));
        assert(not output.str().contains("addi t0, t3, 0\n"));
        assert(not output.str().contains("addi t1, t3, 0\n"));
    }
    {
        const std::string_view source{R"baz(
func assert(ok bool) { if not ok exit(1) }
func main() {
    var source[4] i32
    var destination[4] i32
    assert(arrays_equal(source[2], destination[1], 2))
}
)baz"};
        machine_rv32i compiler;
        program prg{compiler, source, 4096, false, false, false};
        std::ostringstream output;
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
              "t0: left value/result, t5: right value, t4: words, t3: tail "
              "bytes",
              "stop at first mismatch", "compare 4-byte words",
              "compare optional 2-byte tail", "compare optional final byte",
              "all matched or empty: true", "mismatch: false"}) {
            assert(output.str().contains(std::format("# {}\n", text)));
        }
    }
    {
        const std::string_view source{R"baz(
func assert(ok bool) { if not ok exit(1) }
func main() {
    var left[2] i8 = {1, 2}
    var right[2] i8 = {1, 2}
    assert(arrays_equal(left, right, 2))
    right[1] = 3
    assert(not arrays_equal(left, right, 2))
}
)baz"};
        std::ostringstream output;
        machine_x86 compiler{output, source};
        program prg{compiler, source, 4096, false, false, false};
        prg.build(output);
        assert(output.str().contains("sete r15b\n"));
        assert(not output.str().contains("xor r15b, 1\n"));
        assert(output.str().contains("setne r15b\n"));
        assert(not output.str().contains("\n    cmp r15b, 0\n"));
        assert(not output.str().contains("allocate scratch register -> r14\n"));
    }
    {
        const std::string_view source{R"baz(
func main() {
    var left[2] i8 = {1, 2}
    var right[2] i8 = {1, 2}
    var same bool = arrays_equal(left, right, 2)
    same = not arrays_equal(left, right, 2)
    same = equal(left, right)
    same = not equal(left, right)
}
)baz"};
        std::ostringstream x86_output;
        machine_x86 x86_compiler{x86_output, source};
        program x86_program{x86_compiler, source, 4096, false, false, false};
        x86_program.build(x86_output);
        assert(x86_output.str().contains("sete byte [rbp + 4]\n"));
        assert(x86_output.str().contains("setne byte [rbp + 4]\n"));
        assert(not x86_output.str().contains("sete r15b\n"));
        assert(not x86_output.str().contains("setne r15b\n"));
        assert(not x86_output.str().contains("cmp r15b, 0\n"));
        assert(not x86_output.str().contains("xor r15b, 1\n"));

        std::ostringstream rv32i_output;
        machine_rv32i rv32i_compiler;
        program rv32i_program{rv32i_compiler, source, 4096,
                              false,          false,  false};
        rv32i_program.build(rv32i_output);
        assert(rv32i_output.str().contains("sb t3, 4(s0)\n"));
        assert(not rv32i_output.str().contains("sltu "));
        assert(not rv32i_output.str().contains("sltiu "));
        assert(not rv32i_output.str().contains("xori "));
    }
    if (argc > 1 and std::string_view{argv[1]} == "bulk") {
        const std::string_view source{R"baz(
func assert(ok bool) { if not ok exit(1) }
type packed { first i8, second i16 }
func nested(source[] packed, destination[] packed) count i32 {
    array_copy(source, destination, 1)
    count = 2
}
func main() {
    var source[3] packed = {{1, 300}, {2, -400}, {3, 500}}
    var destination[3] packed
    var single packed = source[1]
    assert(equal(single, source[1]))
    single.second = 12
    assert(not equal(single, source[1]))
    array_copy(source, destination, nested(source, destination))
    assert(arrays_equal(source, destination, 2))
    assert(not arrays_equal(source, destination, 3))
    var index i32 = 2
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
        machine_rv32i compiler;
        program prg{compiler, source, 4096, true, true, true};
        prg.build(std::cout);

        return 0;
    }
    if (argc > 1 and std::string_view{argv[1]} == "strings-syscall") {
        const std::string_view source{R"baz(
dat text[] i8 = "A\0\a\b\t\n\v\f\r\e\"'`\\\x00\x7f\x80\xff\x41B"
func main() {
    mov(a0, 1)
    mov(a1, address_of(text))
    mov(a2, array_size_of(text))
    mov(a7, 64)
    syscall()
    if a0 != 20 exit(1)
    mov(a0, -1)
    mov(a7, 64)
    syscall()
    if a0 != -9 exit(2)
    mov(a0, 0)
    mov(a7, 93)
    syscall()
}
)baz"};
        machine_rv32i compiler;
        program prg{compiler, source, 4096, false, false, false};
        prg.build(std::cout);

        return 0;
    }
    if (argc > 1) {
        machine_rv32i bounds_backend;
        bounds_backend.set_builtin_types(integer64, integer, half, byte, boolean, empty);
        bounds_backend.use_stream(std::cout);
        std::println(".option norvc\n.option norelax\n.text\n.globl _start\n_start:");
        if (std::string_view{argv[1]} == "bounds-silent") {
            std::println("    li a0, -1");
            bounds_backend.check_bounds(token{}, 1, operand::reg("a0", integer), 4, false, {},
                                        {.upper{true}, .lower{true}, .with_line{}});
            bounds_backend.program_end();
            bounds_backend.emit_bounds_failure_handler(false);
        } else {
            const operand continuation{bounds_backend.alloc_named_register(token{}, 0, "s3", integer)};
            size_t case_index{};
            for (const bool upper : {false, true}) {
                for (const bool lower : {false, true}) {
                    for (const bool allow_end : {false, true}) {
                        for (const uint32_t size : {0U, 4U, uint32_t{INT32_MAX}, UINT32_MAX}) {
                            for (const int32_t index : {INT32_MIN, -1, 0, 3, 4, 5, INT32_MAX}) {
                                for (const int32_t count : {INT32_MIN, -1, 0, 1, 4, INT32_MAX}) {
                                    for (const bool slice : {false, true}) {
                                        const int64_t top{int64_t{index} + (slice ? count : 0)};
                                        const bool expected{(lower and index < 0) or
                                            (upper and (allow_end ? top > size : top >= size))};
                                        std::println("    li a0, {}\n    li a1, {}\n    la s3, bounds_result_{}", index, count, case_index);
                                        bounds_backend.check_bounds(token{}, 1, operand::reg("a0", integer), size, allow_end,
                                            slice ? operand::reg("a1", integer) : operand{},
                                            {.upper{upper}, .lower{lower}, .with_line{}});
                                        std::println("    li a3, 0\nbounds_result_{}:\n    li a4, {}\n    beq a3, a4, 1f\n    j bounds_failure\n1:", case_index++, expected ? 1 : 0);
                                        std::println("    li a4, {}\n    beq a0, a4, 1f\n    j bounds_failure\n1:\n    li a4, {}\n    beq a1, a4, 1f\n    j bounds_failure\n1:", index, count);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            bounds_backend.free_named_register(token{}, 0, continuation);
            bounds_backend.program_end();
            std::println("baz_bounds_panic:\n    li a3, 1\n    jr s3\nbounds_failure:");
            bounds_backend.exit(token{}, 1, operand::imm("1", integer));
        }
        bounds_backend.finish();

        return 0;
    }
    std::ostringstream x86_output;
    machine_x86 x86_backend{x86_output, {}};
    x86_backend.set_builtin_types(integer64, integer, half, byte, boolean, empty);
    for (size_t pass{}; pass < 2; ++pass) {
        std::vector<operand> registers;
        for (size_t count{}; count < 8; ++count) {
            const operand reg{x86_backend.alloc_scratch_register(token{}, 0, integer64)};
            assert((reg.base_register() == "r11") == (count == 7));
            registers.push_back(reg);
        }
        bool x86_exhausted{};
        try {
            static_cast<void>(x86_backend.alloc_scratch_register(token{}, 0, integer64));
        } catch (const compiler_exception&) {
            x86_exhausted = true;
        }
        assert(x86_exhausted);
        for (size_t count{}; count < 2; ++count) {
            x86_backend.free_scratch_register(token{}, 0, registers.back());
            registers.pop_back();
        }
        const operand ordinary{x86_backend.alloc_scratch_register(token{}, 0, integer64)};
        assert(ordinary.base_register() == "r8");
        registers.push_back(ordinary);
        const operand special{x86_backend.alloc_scratch_register(token{}, 0, integer64)};
        assert(special.base_register() == "r11");
        registers.push_back(special);
        x86_backend.free_scratch_registers(token{}, 0, registers);
        x86_backend.finish();
    }
    for (const unsigned live_mask : {0U, 1U, 2U, 3U}) {
        const bool rcx_allocated{(live_mask & 1U) != 0};
        const bool r11_allocated{(live_mask & 2U) != 0};
        std::vector<operand> scratch;
        operand live_rcx;
        if (rcx_allocated) {
            live_rcx = x86_backend.alloc_named_register(token{}, 0, "rcx", byte);
        }
        if (r11_allocated) {
            for (size_t count{}; count < 8; ++count) {
                scratch.push_back(x86_backend.alloc_scratch_register(token{}, 0, byte));
            }
        }
        const machine::builtin_registers contract{
            x86_backend.registers_for_builtin(machine::builtin_function::write)};

        std::vector<operand> args;
        for (const std::string_view name : contract.arguments) {
            args.push_back(x86_backend.alloc_named_register(token{}, 0, name, integer64));
        }
        const operand result{x86_backend.alloc_named_register(token{}, 0, contract.result, integer64)};
        for (const unsigned operation : {0U, 1U, 2U}) {
            x86_output.str({});
            if (operation == 0) {
                x86_backend.read(token{}, 0, result, args.at(0), args.at(1), args.at(2));
            } else if (operation == 1) {
                x86_backend.write(token{}, 0, result, args.at(0), args.at(1), args.at(2));
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
                    assert(assembly.find(std::format("push {}", name)) < assembly.find("syscall"));
                    assert(assembly.find(std::format("pop {}", name)) > assembly.find("syscall"));
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
    {
        const std::string_view source{
            "func main() { var value = write(1, 0, 0) exit(value) }"};

        std::ostringstream output;
        machine_x86 compiler{output, source};
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
    for (const std::string_view source : {
             "func main() { write(1, 0, write(1, 0, 0)) }",
             "func main() { exit(write(1, 0, 0)) }"}) {
        std::ostringstream output;
        machine_x86 compiler{output, source};
        program prg{compiler, source, 4096, false, false, false};
        bool rejected{};
        try {
            prg.build(output);
        } catch (const compiler_exception& error) {
            rejected = std::string_view{error.what()}.contains("cannot allocate register rdi");
        }
        assert(rejected);
    }
    machine_rv32i backend;
    backend.set_builtin_types(integer64, integer, half, byte, boolean, empty);
    assert(&backend.default_type() == &integer);
    assert(backend.address_size_bytes() == 4);
    assert(backend.can_lower_index_scale(1));
    assert(not backend.can_lower_index_scale(3));
    assert(backend.can_lower_index_scale(256));
    assert(backend.can_lower_index_scale(UINT64_C(2147483648)));
    assert(not backend.can_lower_index_scale(UINT32_MAX));
    assert(not backend.can_lower_index_scale(0));
    assert(not backend.can_lower_index_scale(UINT64_C(4294967296)));

    assembly_output shift_output;
    backend.use_stream(shift_output);
    for (const bool counted : {false, true}) {
        std::vector<operand> held;
        for (size_t count{}; count < 25; ++count) {
            held.push_back(backend.alloc_scratch_register(token{}, 0, boolean));
        }
        const operand result{held.front()};
        const operand count{backend.begin_memory_equal(token{}, 0)};
        backend.copy_value(token{}, 0, count, operand::imm("7", integer));
        std::println(shift_output, "la {}, buffer", held.back().base_register());
        backend.set_memory_equal_left(0, operand::mem(held.back(), byte));
        backend.set_memory_equal_right(0, operand::mem(held.back(), byte));
        shift_output.str({});
        if (counted) {
            backend.end_arrays_equal(token{}, 0, 1, result);
        } else {
            backend.end_memory_equal(token{}, 0, 7, result);
        }
        const std::string assembly{shift_output.str()};
        for (const std::string_view instruction : {"lw", "lhu", "lbu"}) {
            assert(assembly.contains(std::format("{} {}, 0(", instruction, result.base_register())));
        }
        assert(assembly.contains(std::format("li {}, 1", result.base_register())));
        assert(assembly.contains(std::format("li {}, 0", result.base_register())));
        assert(not assembly.contains(std::format("addi {},", result.base_register())));
        assert(not assembly.contains("slli"));
        backend.free_scratch_registers(token{}, 0, held);
        backend.finish();
        shift_output.str({});
    }
    for (size_t size_bytes{}; size_bytes <= 24; ++size_bytes) {
        backend.copy(token{}, 0, operand::mem("a0", {}, 1, 0, byte),
                     operand::mem("a1", {}, 1, 0, byte), size_bytes);
        const std::string assembly{shift_output.str()};
        assert(assembly.contains("bnez") == (size_bytes > 16));
        if (size_bytes == 0) {
            assert(assembly.empty());
        } else if (size_bytes <= 16) {
            for (const size_t width : {size_t{4}, size_t{2}, size_t{1}}) {
                const std::string load{width == 4 ? "lw " : width == 2 ? "lhu " : "lbu "};
                const std::string store{width == 4 ? "sw " : width == 2 ? "sh " : "sb "};
                size_t loads{};
                size_t stores{};
                std::istringstream lines{assembly};
                for (std::string line; std::getline(lines, line);) {
                    loads += line.starts_with(load);
                    stores += line.starts_with(store);
                }
                const size_t expected{width == 4 ? size_bytes / 4 : (size_bytes % (width * 2)) / width};
                assert(loads == expected and stores == expected);
            }
            assert(not assembly.contains("beqz"));
        } else {
            assert(assembly.contains("lw ") and assembly.contains("sw "));
            assert(assembly.contains("lhu ") and assembly.contains("sh "));
            assert(assembly.contains("lbu ") and assembly.contains("sb "));
            assert(assembly.contains("srli ") and assembly.contains("andi "));
        }
        backend.finish();
        shift_output.str({});
    }
    backend.invoke_syscall(1);
    assert(shift_output.str() == "    ecall\n");
    shift_output.str({});
    backend.emit_string_data({});
    assert(shift_output.str() == ".ascii \"\"\n");
    shift_output.str({});
    backend.emit_string_data(R"baz(\0\a\b\t\n\v\f\r\e\"\'`\\\x00\x7F\x80\xff\x41B)baz");
    assert(shift_output.str() == R"baz(.ascii "\000\007\010\t\n\013\014\r\033\"'`\\\000\177\200\377AB")baz" "\n");
    shift_output.str({});
    backend.emit_string_data("\xc3\xa9\n");
    assert(shift_output.str() == R"baz(.ascii "\303\251\n")baz" "\n");
    shift_output.str({});
    backend.emit_string_data(R"baz(hello\n\x007)baz");
    assert(shift_output.str() == R"baz(.ascii "hello\n\0007")baz" "\n");
    for (const std::string_view text : {"\\", "\\x", "\\x1", "\\xGG", "\\q"}) {
        bool rejected{};
        try {
            backend.emit_string_data(text);
        } catch (const compiler_exception&) {
            rejected = true;
        }
        assert(rejected);
    }
    shift_output.str({});
    std::vector<operand> shift_registers;
    for (size_t count{}; count < 30; ++count) {
        shift_registers.push_back(backend.alloc_scratch_register(token{}, 0, integer));
    }
    for (const char operation : {'<', '>'}) {
        shift_output.str({});

        backend.shift(token{}, 0, operation, operand::reg("a0", integer),
                      operand::reg("a1", integer));

        assert(shift_output.str() == (operation == '<' ? "sll a0, a0, a1\n" : "sra a0, a0, a1\n"));
        shift_output.str({});

        for (const unsigned count : {0U, 2U, 31U, 32U, 34U, 35U, 64U}) {
            shift_output.str({});
            backend.shift(token{}, 0, operation, operand::reg("a0", integer),
                          operand::imm(std::format("{}", count), integer));
            const unsigned masked{count & 31U};
            assert(shift_output.str() ==
                   (masked == 0
                        ? std::string{}
                        : std::format("{} a0, a0, {}\n",
                                      operation == '<' ? "slli" : "srai",
                                      masked)));
        }
        for (const operand& destination :
             {operand::reg("a0", integer),
              operand::mem("a0", {}, 1, 0, integer)}) {
            for (const std::string_view count :
                 {"shift_amount + 1", "2 + 1", "-~"}) {
                shift_output.str({});
                bool rejected{};
                try {
                    backend.shift(token{}, 0, operation, destination,
                                  operand::imm(std::string{count}, integer));
                } catch (const compiler_exception&) {
                    rejected = true;
                }
                assert(rejected);
                assert(shift_output.str().empty());
            }
        }
    }
    for (const char operation : {'+', '-', '&', '|', '^'}) {
        shift_output.str({});
        std::string_view instruction;
        if (operation == '+' or operation == '-') {
            instruction = operation == '+' ? "add" : "sub";

            backend.add_subtract(token{}, 0, operation, operand::reg("a0", integer),
                                 operand::reg("a1", integer));

        } else {
            instruction = operation == '&' ? "and" : operation == '|' ? "or" : "xor";

            backend.bitwise(token{}, 0, operation, operand::reg("a0", integer),
                            operand::reg("a1", integer));

        }
        assert(shift_output.str() == std::format("{} a0, a0, a1\n", instruction));
        for (const std::string_view constant : {"7", "-7", "~-8", "--7", "2047", "-2047"}) {
            shift_output.str({});

            const operand source{operand::imm(std::string{constant}, integer)};

            if (operation == '+' or operation == '-') {
                backend.add_subtract(token{}, 0, operation, operand::reg("a0", integer), source);
            } else {
                backend.bitwise(token{}, 0, operation, operand::reg("a0", integer), source);
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

            assert(shift_output.str() == std::format("{}i a0, a0, {}\n",
                operation == '-' ? "add" : instruction, expected));

        }
    }
    for (const type* value_type : {&integer, &half, &byte}) {
        for (const bool memory_destination : {false, true}) {

            const operand destination{memory_destination
                ? operand::mem("a0", {}, 1, 0, *value_type)
                : operand::reg("a0", *value_type)};

            shift_output.str({});
            backend.add_subtract(token{}, 0, '+', destination, operand::imm("0", integer));
            backend.add_subtract(token{}, 0, '-', destination, operand::imm("0", integer));
            backend.bitwise(token{}, 0, '|', destination, operand::imm("0", integer));
            backend.bitwise(token{}, 0, '^', destination, operand::imm("0", integer));
            backend.bitwise(token{}, 0, '&', destination, operand::imm("-1", integer));
            backend.bitwise(token{}, 0, '|', destination, destination);
            backend.bitwise(token{}, 0, '&', destination, destination);
            backend.shift(token{}, 0, '<', destination, operand::imm("32", integer));
            backend.shift(token{}, 0, '>', destination, operand::imm("0", integer));
            assert(shift_output.str().empty());
            shift_output.str({});
            backend.bitwise(token{}, 0, '&', destination, operand::imm("0", integer));

            const std::string_view store{value_type == &integer ? "sw" : value_type == &half ? "sh" : "sb"};
            const std::string zero_result{memory_destination
                ? std::format("{} zero, 0(a0)\n", store) : "li a0, 0\n"};

            assert(shift_output.str() == zero_result);
            shift_output.str({});
            backend.add_subtract(token{}, 0, '-', destination, destination);
            assert(shift_output.str() == zero_result);
            shift_output.str({});
            backend.bitwise(token{}, 0, '^', destination, destination);
            assert(shift_output.str() == zero_result);
        }
        shift_output.str({});
        backend.unary(0, '~', operand::reg("a0", *value_type));
        assert(shift_output.str() == "xori a0, a0, -1\n");
    }
    shift_output.str({});
    backend.unary(0, '-', operand::reg("a0", integer));
    assert(shift_output.str() == "sub a0, zero, a0\n");
    shift_output.str({});
    backend.bitwise(token{}, 0, '&', operand::reg("a0", byte), operand::imm("255", integer));
    backend.add_subtract(token{}, 0, '+', operand::reg("a0", byte), operand::imm("256", integer));
    assert(shift_output.str().empty());
    shift_output.str({});
    backend.bitwise(token{}, 0, '|', operand::reg("a0", byte), operand::imm("255", integer));
    assert(shift_output.str() == "li a0, -1\n");
    shift_output.str({});
    backend.shift(token{}, 0, '<', operand::reg("a0", byte), operand::imm("8", integer));
    assert(shift_output.str() == "li a0, 0\n");
    shift_output.str({});
    backend.shift(token{}, 0, '<', operand::reg("a0", byte), operand::imm("1", integer));
    assert(shift_output.str() == "slli a0, a0, 25\nsrai a0, a0, 24\n");
    shift_output.str({});
    backend.add_subtract(token{}, 0, '+', operand::reg("a0", integer), operand::imm("2048", integer));
    assert(shift_output.str() == "addi a0, a0, 2047\naddi a0, a0, 1\n");
    shift_output.str({});
    backend.add_subtract(token{}, 0, '-', operand::reg("a0", integer), operand::imm("2049", integer));
    assert(shift_output.str() == "addi a0, a0, -2048\naddi a0, a0, -1\n");
    backend.free_scratch_registers(token{}, 0, shift_registers);
    backend.finish();
    for (const type* value_type : {&integer, &half, &byte}) {
        shift_output.str({});

        backend.copy_value(token{}, 0, operand::mem("a0", {}, 1, 0, *value_type),
                           operand::reg("a1", *value_type));

        const std::string_view store{value_type == &integer ? "sw" : value_type == &half ? "sh" : "sb"};
        assert(shift_output.str() == std::format("{} a1, 0(a0)\n", store));
        shift_output.str({});

        backend.copy_value(token{}, 0, operand::reg("a1", *value_type),
                           operand::mem("a0", {}, 1, 0, *value_type));

        const std::string_view load{value_type == &integer ? "lw" : value_type == &half ? "lh" : "lb"};
        assert(shift_output.str() == std::format("{} a1, 0(a0)\n", load));
        shift_output.str({});

        backend.copy_value(token{}, 0, operand::reg("a1", *value_type),
                           operand::reg("x11", *value_type));

        assert(shift_output.str().empty());
        backend.finish();
    }
    for (const char operation : {'&', '|', '^'}) {
        shift_output.str({});

        backend.bitwise(token{}, 0, operation, operand::reg("a0", byte),
                        operand::reg("a1", byte));

        assert(std::ranges::count(shift_output.str(), '\n') == 1);
        backend.finish();
    }
    shift_output.str({});

    backend.add_subtract(token{}, 0, '+', operand::mem("a0", {}, 1, 0, integer),
                         operand::mem("x10", {}, 1, 0, integer));

    assert(shift_output.str() ==
           "lw t0, 0(a0)\nadd t0, t0, t0\nsw t0, 0(a0)\n");
    backend.finish();
    shift_output.str({});

    backend.shift(token{}, 0, '<', operand::mem("a0", {}, 1, 0, integer),
                  operand::mem("x10", {}, 1, 0, integer));

    assert(shift_output.str() ==
           "lw t0, 0(a0)\nsll t0, t0, t0\nsw t0, 0(a0)\n");
    backend.finish();
    {
        const std::string_view source{
            "func main() { var a = 3 var b = 2 var x = a << b exit(x) }"};

        std::ostringstream output;
        machine_rv32i compiler;
        program prg{compiler, source, 4096, false, false, false};
        prg.build(output);
        assert(output.str().contains("sll t0, t0, t1"));
        assert(not output.str().contains("addi t1, t0, 0"));
    }

    assembly_output address_output;
    backend.use_stream(address_output);
        backend.copy_value(token{}, 0, operand::reg("a1", integer),
                  operand::mem("a2", "a3", 1, 8196, integer));

        assert(address_output.str() ==
            "add a1, a2, a3\nlui t0, 2\nadd a1, a1, t0\nlw a1, 4(a1)\n");
        address_output.str({});

    for (const uint64_t scale : {UINT64_C(2), UINT64_C(256), UINT64_C(2147483648)}) {
        backend.copy_value(token{}, 0, operand::reg("a1", integer),
                           operand::mem("a2", "a3", scale, 40, integer));
        assert(address_output.str() == std::format(
            "slli a1, a3, {}\nadd a1, a1, a2\nlw a1, 40(a1)\n",
            std::countr_zero(scale)));
        address_output.str({});
    }
    std::vector<operand> address_registers;
    for (size_t count{}; count < 30; ++count) {
        address_registers.push_back(backend.alloc_scratch_register(token{}, 0, integer));
    }
    assert(address_registers.back().base_register() == "a0");
    backend.free_scratch_register(token{}, 0, address_registers.back());
    address_registers.pop_back();
    address_output.str({});

    std::println(address_output, "la a1, buffer");
    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("a1", {}, 1, 4, integer));

    assert(address_output.str() == "la a1, buffer\nlw a1, 4(a1)\n");
    address_output.str({});

    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("a2", "a3", 1, 4, integer));

    assert(address_output.str() == "add a1, a2, a3\nlw a1, 4(a1)\n");
    address_output.str({});

    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("a2", {}, 1, 8196, integer));

    assert(address_output.str() == "lui a1, 2\nadd a1, a1, a2\nlw a1, 4(a1)\n");
    address_output.str({});

    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("a2", {}, 1, -2049, integer));

    assert(address_output.str() ==
           "lui a1, 1048575\nadd a1, a1, a2\nlw a1, 2047(a1)\n");
    address_output.str({});

    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("zero", {}, 1, 8196, integer));

    assert(address_output.str() == "lui a1, 2\nadd a1, a1, zero\nlw a1, 4(a1)\n");
    address_output.str({});

    for (const int64_t offset : {INT64_C(-4294967295), INT64_C(4294967295)}) {
        const int32_t low{offset < 0 ? 1 : -1};
        for (const std::string_view base : {"x11", "zero", "x0"}) {
            backend.copy_value(token{}, 0, operand::reg("a1", integer),
                               operand::mem(base, {}, 1, offset, integer));

            assert(address_output.str() ==
                   std::format("lw a1, {}({})\n", low, base));
            address_output.str({});
        }
    }

    std::println(address_output, "la a1, buffer");
    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("a1", "a3", 1, 4, integer));

    assert(address_output.str() == "la a1, buffer\nadd a1, a1, a3\nlw a1, 4(a1)\n");
    address_registers.push_back(backend.alloc_scratch_register(token{}, 0, integer));
    address_output.str({});

    for (const std::string_view base : {"a2", "x11"}) {
        backend.copy_value(token{}, 0, operand::reg("a1", integer),
                           operand::mem(base, "x11", 1, 4, integer));
        assert(address_output.str() ==
               std::format("add a1, {}, x11\nlw a1, 4(a1)\n", base));
        address_output.str({});
    }
    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("x11", "a2", 1, 4, integer));
    assert(address_output.str() == "add a1, x11, a2\nlw a1, 4(a1)\n");
    address_output.str({});
    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("a2", "x11", 4, 4, integer));
    assert(address_output.str() ==
           "slli a1, x11, 2\nadd a1, a1, a2\nlw a1, 4(a1)\n");
    address_output.str({});

    std::println(address_output, "la a1, buffer");
    backend.address_of(token{}, 0, operand::reg("a1", integer),
                       operand::mem("a1", {}, 1, 0, integer));

    assert(address_output.str() == "la a1, buffer\n");
    address_output.str({});

    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("zero", "a2", 1, 4, integer));

    assert(address_output.str() == "add a1, zero, a2\nlw a1, 4(a1)\n");
    address_output.str({});

    backend.copy_value(token{}, 0, operand::mem("a2", {}, 1, 0, integer),
                       operand::imm("0", integer));

    assert(address_output.str() == "sw zero, 0(a2)\n");
    address_output.str({});
    backend.copy_value(token{}, 0, operand::reg("a1", byte), operand::imm("255", integer));
    assert(address_output.str() == "li a1, -1\n");
    address_output.str({});
    backend.zero(token{}, 0, operand::mem("a2", {}, 1, 0, byte), 1);
    assert(address_output.str() == "sb zero, 0(a2)\n");
    address_output.str({});
    backend.zero(token{}, 0, operand::mem("a2", {}, 1, 208, byte), 16);
    assert(address_output.str() == "sw zero, 208(a2)\nsw zero, 212(a2)\nsw "
                                   "zero, 216(a2)\nsw zero, 220(a2)\n");
    address_output.str({});
    backend.zero(token{}, 0, operand::mem("a2", {}, 1, 0, byte), 4);
    assert(address_output.str() == "sw zero, 0(a2)\n");
    address_output.str({});

    backend.compare_and_branch(token{}, 0, operand::reg("a0", integer),
        operand::reg("a1", integer), {
            .operation{"<"},
            .destination{operand::reg("a2", boolean)},
        }, {});

    assert(address_output.str() == "slt a2, a0, a1\n");
    address_output.str({});

    backend.compare_and_branch(token{}, 0,
                               operand::mem("a0", {}, 1, 0, integer),
                               operand::reg("a1", integer),
                               {
                                   .operation{"<"},
                                   .destination{operand::reg("a2", boolean)},
                               },
                               {});
    assert(address_output.str() == "lw a2, 0(a0)\nslt a2, a2, a1\n");
    address_output.str({});
    backend.compare_and_branch(token{}, 0, operand::reg("a0", integer),
                               operand::mem("x11", {}, 1, 0, integer),
                               {
                                   .operation{"=="},
                                   .destination{operand::reg("a1", boolean)},
                               },
                               {});
    assert(address_output.str() ==
           "lw a1, 0(x11)\nxor a1, a0, a1\nsltiu a1, a1, 1\n");
    address_output.str({});
    backend.compare_and_branch(token{}, 0, operand::reg("a0", byte),
                               operand::reg("a1", integer),
                               {
                                   .operation{"<"},
                                   .destination{operand::reg("a1", boolean)},
                               },
                               {});
    assert(address_output.str() ==
           "slli a1, a1, 24\nsrai a1, a1, 24\nslt a1, a0, a1\n");
    address_output.str({});

    backend.compare_and_branch(token{}, 0, operand::reg("a0", integer),
        operand::reg("a1", integer), {
            .operation{"=="},
            .target{"comparison_target"},
            .branch_on_true{true},
        }, {});

    assert(address_output.str() == "bne a0, a1, 1f\nj comparison_target\n1:\n");
    for (const std::string_view operation : {"<", "==", "!="}) {
        address_output.str({});

        backend.compare_and_branch(token{}, 0, operand::reg("a0", integer),
            operand::imm(operation == "<" ? "7" : "0", integer), {
                .operation{operation},
                .destination{operand::reg("a0", boolean)},
            }, {});

        if (operation == "<") {
            assert(address_output.str() == "slti a0, a0, 7\n");
        } else if (operation == "==") {
            assert(address_output.str() == "sltiu a0, a0, 1\n");
        } else {
            assert(address_output.str() == "sltu a0, zero, a0\n");
        }
    }
    address_output.str({});

    backend.compare_and_branch(token{}, 0, operand::reg("a0", byte),
        operand::imm("255", integer), {
            .operation{"<"},
            .destination{operand::reg("a1", boolean)},
        }, {});

    assert(address_output.str() == "slti a1, a0, -1\n");
    address_output.str({});
    backend.multiply(token{}, 0, operand::reg("a1", integer), operand::imm("1", integer));
    assert(address_output.str().empty());
    backend.multiply(token{}, 0, operand::reg("a1", integer), operand::imm("8", integer));
    assert(address_output.str() == "slli a1, a1, 3\n");
    address_output.str({});
    backend.multiply(token{}, 0, operand::reg("a1", integer), operand::imm("-1", integer));
    assert(address_output.str() == "sub a1, zero, a1\n");
    address_output.str({});
    backend.multiply(token{}, 0, operand::mem("a2", {}, 1, 0, integer), operand::imm("0", integer));
    assert(address_output.str() == "sw zero, 0(a2)\n");
    backend.free_scratch_registers(token{}, 0, address_registers);
    address_registers.clear();
    address_output.str({});
    backend.copy(token{}, 0, operand::mem("a1", {}, 1, 208, byte),
                 operand::mem("a2", {}, 1, 240, byte), 4);
    assert(address_output.str() == "lw t0, 208(a1)\nsw t0, 240(a2)\n");
    address_output.str({});
    backend.copy(token{}, 0, operand::mem("a1", {}, 1, -16, byte),
                 operand::mem("a2", {}, 1, 16, byte), 7);
    assert(address_output.str() ==
           "lw t0, -16(a1)\nsw t0, 16(a2)\nlhu t0, -12(a1)\nsh t0, 20(a2)\n"
           "lbu t0, -10(a1)\nsb t0, 22(a2)\n");
    address_output.str({});
    for (size_t count{}; count < 29; ++count) {
        address_registers.push_back(
            backend.alloc_scratch_register(token{}, 0, integer));
    }
    backend.copy(token{}, 0, operand::mem("a1", {}, 1, 208, byte),
                 operand::mem("a2", {}, 1, 240, byte), 4);
    assert(address_output.str() == "lw a0, 208(a1)\nsw a0, 240(a2)\n");
    backend.free_scratch_registers(token{}, 0, address_registers);
    address_output.str({});
    backend.zero(token{}, 0, operand::mem("a2", {}, 1, 2047, byte), 16);
    assert(address_output.str() ==
           "addi t0, a2, 2047\nsw zero, 0(t0)\nsw zero, 4(t0)\nsw zero, "
           "8(t0)\nsw zero, 12(t0)\n");
    address_output.str({});
    backend.zero(token{}, 0, operand::mem("a2", {}, 1, 0, byte), 19);
    assert(address_output.str() ==
           "addi t0, a2, 0\nli t1, 4\n1:\nsw zero, 0(t0)\naddi t0, t0, 4\naddi "
           "t1, t1, -1\nbnez t1, 1b\nsh zero, 0(t0)\nsb zero, 2(t0)\n");
    address_output.str({});
    backend.finish();

    std::ostringstream rejected_output;
    backend.use_stream(rejected_output);
    for (const std::string_view base : {"buffer", "x32", "not_a_register", ""}) {
        for (const std::string_view index : {"", "a2"}) {
            const operand address{operand::mem(base, index, 1, 4, integer)};
            for (const int operation : {0, 1, 2, 3}) {
                bool rejected{};
                try {
                    switch (operation) {
                    case 0:
                        backend.address_of(token{}, 0, operand::reg("a0", integer), address);
                        break;

                    case 1:
                        backend.copy_value(token{}, 0, operand::reg("a0", integer), address);
                        break;

                    case 2:
                        backend.copy_value(token{}, 0, address, operand::reg("a0", integer));
                        break;

                    default:
                        backend.copy_value(token{}, 0, address, address);
                        break;
                    }
                } catch (const compiler_exception& error) {
                    rejected = std::string_view{error.what()}.contains("invalid RV32I base register");
                }
                assert(rejected);
                backend.finish();
            }
        }
    }
    for (const int64_t offset : {INT64_MIN, INT64_C(-4294967296), INT64_C(4294967296), INT64_MAX}) {
        bool rejected{};
        try {
            backend.address_of(token{}, 1, operand::reg("a0", integer),
                               operand::mem("a1", {}, 1, offset, integer));
        } catch (const compiler_exception&) {
            rejected = true;
        }
        assert(rejected);
        backend.finish();
    }
    for (const uint64_t scale :
            {UINT64_C(4294967296), UINT64_C(9223372036854775808)}) {
        assert(not backend.can_lower_index_scale(scale));
        bool rejected{};
        try {
            backend.address_of(token{}, 1, operand::reg("a0", integer),
                               operand::mem("a1", "a2", scale, 0, integer));
        } catch (const compiler_exception& error) {
            rejected = std::string_view{error.what()}.contains("index scale exceeds RV32I address range");
        }
        assert(rejected);
        backend.finish();
    }
    assert(rejected_output.str().empty());
    bool rejected_i64{};
    try {
        static_cast<void>(backend.make_register_operand("a0", integer64));
    } catch (const compiler_exception&) {
        rejected_i64 = true;
    }
    assert(rejected_i64);
    std::vector<operand> held_registers;
    for (size_t count{}; count < 29; ++count) {
        held_registers.push_back(backend.alloc_scratch_register(token{}, 0, integer));
    }
    bool exhausted{};
    try {
        backend.address_of(token{}, 1, operand::reg("a0", integer),
                           operand::mem("a0", {}, 1, 8196, integer));
    } catch (const compiler_exception&) {
        exhausted = true;
    }
    assert(exhausted);
    backend.free_scratch_registers(token{}, 0, held_registers);
    backend.finish();
    for (const size_t reserved_count : {size_t{}, size_t{1}, size_t{2}}) {
        for (const std::string_view name : {"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"}) {
            const operand reg{backend.alloc_named_register(token{}, 0, name, integer)};
            backend.free_named_register(token{}, 0, reg);
        }
        if (reserved_count >= 1) {
            backend.reserve_variables_base();
            assert(backend.is_variables_base(operand::reg("fp", integer)));
            assert(backend.is_variables_base(operand::reg("x8", integer)));
        }
        if (reserved_count == 2) {
            backend.reserve_frame_base();
        }
        held_registers.clear();
        uint32_t register_mask{};
        for (size_t count{}; count < 30 - reserved_count; ++count) {
            const operand reg{backend.alloc_scratch_register(token{}, 0, integer)};
            assert(reg.base_register() != "zero" and reg.base_register() != "sp");
            assert(reserved_count == 0 or reg.base_register() != "s0");
            assert(reserved_count != 2 or reg.base_register() != "s1");
            assert(reg.base_register().starts_with("a") == (count >= 22 - reserved_count));
            assert(backend.allocated_register_type(reg.base_register()) == &integer);
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
        bool pool_exhausted{};
        try {
            static_cast<void>(backend.alloc_scratch_register(token{}, 0, integer));
        } catch (const compiler_exception&) {
            pool_exhausted = true;
        }
        assert(pool_exhausted);
        for (const std::string_view name : {"zero", "x0", "sp", "x2", "fp", "x8", "s1", "x9"}) {
            bool rejected_named{};
            try {
                static_cast<void>(backend.alloc_named_register(token{}, 0, name, integer));
            } catch (const compiler_exception&) {
                rejected_named = true;
            }
            assert(rejected_named);
        }
        backend.free_scratch_registers(token{}, 0, held_registers);
        if (reserved_count == 2) {
            backend.release_frame_base();
        }
        if (reserved_count >= 1) {
            backend.release_variables_base();
        }
        const operand named{backend.alloc_named_register(token{}, 0, "x1", integer)};
        assert(named.base_register() == "ra");
        backend.free_named_register(token{}, 0, named);
        backend.finish();
    }
    backend.use_stream(std::cout);

    for (const char operation : {'*', '/', '%', '+'}) {
        machine_rv32i helper_backend;
        helper_backend.set_builtin_types(integer64, integer, half, byte, boolean, empty);
        std::ostringstream output;
        helper_backend.use_stream(output);
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
                    operation == '+' ? operand::imm("3", integer) : operand::reg("a1", integer));
            } else {
                helper_backend.divide(token{}, 0, operation, operand::reg("a0", integer), operand::reg("a1", integer));
            }
        }
        assert(not output.str().contains(".Lbaz_multiply:"));
        assert(not output.str().contains(".Lbaz_divide:"));
        helper_backend.program_end();
        helper_backend.begin_data(4);
        const std::string assembly{output.str()};
        for (const std::string_view label : {".Lbaz_multiply:", ".Lbaz_divide:"}) {
            const bool expected{label == ".Lbaz_multiply:" ? operation == '*' : operation == '/' or operation == '%'};
            const size_t first{assembly.find(label)};
            assert((first != std::string::npos) == expected);
            if (expected) {
                assert(assembly.find(label, first + label.size()) == std::string::npos);
            }
        }
        helper_backend.finish();
    }

    std::println(".option norvc\n.option norelax\n.text\n.globl _start\n_start:");
    for (const bool counted : {false, true}) {
    for (size_t size_bytes{}; size_bytes <= 24; ++size_bytes) {
        for (size_t alignment{}; alignment < 4; ++alignment) {
            const int displacement{
                std::array{-2048, 0, 2047, 2048}.at(alignment)};
            std::println("    addi sp, sp, -64");
            for (size_t offset{}; offset < 32; ++offset) {
                std::println("    li a2, {}\n    sb a2, {}(sp)\n    li a2, 85\n    sb a2, {}(sp)",
                             128 + offset, offset, 32 + offset);
            }
            std::println("    addi a0, sp, {}\n    addi a1, sp, {}", alignment, 36 + alignment);
            if (counted) {
                const operand count{backend.begin_array_copy(token{}, 1)};
                backend.copy_value(token{}, 1, count, operand::imm(std::format("{}", size_bytes), integer));
                backend.set_array_copy_source(1, operand::mem("a0", {}, 1, 0, byte));
                backend.set_array_copy_destination(1, operand::mem("a1", {}, 1, 0, byte));
                backend.end_array_copy(token{}, 1, 1);
            } else {
                std::println(
                    "    li a2, {}\n    sub a0, a0, a2\n    sub a1, a1, a2",
                    displacement);
                backend.copy(
                    token{}, 1, operand::mem("a0", {}, 1, displacement, byte),
                    operand::mem("a1", {}, 1, displacement, byte), size_bytes);
                std::println(
                    "    li a2, {}\n    add a0, a0, a2\n    add a1, a1, a2",
                    displacement);
            }
            for (size_t offset{}; offset < 32; ++offset) {
                const size_t start{4 + alignment};
                const size_t expected{offset >= start and offset < start + size_bytes ? 128 + offset - 4 : 85};
                std::println("    lbu a2, {}(sp)\n    li a3, {}\n    beq a2, a3, 1f\n    j failure\n1:", 32 + offset, expected);
            }
            std::println("    addi a2, sp, {}\n    beq a0, a2, 1f\n    j failure\n1:\n    addi a2, sp, {}\n    beq a1, a2, 1f\n    j failure\n1:\n    addi sp, sp, 64", alignment, 36 + alignment);
            backend.finish();
        }
    }
    }
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
            for (const int32_t initial : {0, 1, -1, 7, -7, -128, -32768, 32767, INT32_MIN, INT32_MAX}) {
                for (const int32_t divisor : {1, -1, 2, -2, 3, -7, 255, 65536, INT32_MIN, INT32_MAX}) {
                    for (const unsigned source_kind : {0U, 1U, 2U}) {
                        for (const char operation : {'/', '%'}) {
                            std::println("    la a2, buffer\n    li a1, {}\n    sw a1, 4(a2)", divisor);
                            const operand destination{memory_destination
                                ? operand::mem("a2", {}, 1, 0, *value_type)
                                : operand::reg("a0", *value_type)};

                            operand source{operand::reg("a1", integer)};
                            if (source_kind == 1) {
                                source = operand::mem("a2", {}, 1, 4, integer);
                            } else if (source_kind == 2) {
                                source = operand::imm(std::format("{}", divisor), integer);
                            }
                            backend.copy_value(token{}, 1, destination, operand::imm(std::format("{}", initial), integer));
                            backend.divide(token{}, 1, operation, destination, source);
                            backend.copy_value(token{}, 1, operand::reg("a0", integer), destination);
                            const int64_t dividend{normalize(initial)};
                            const int32_t expected{normalize(operation == '/' ? dividend / divisor : dividend % divisor)};
                            std::println("    li a3, {}\n    beq a0, a3, 1f\n    j failure\n1:", expected);
                            if (source_kind == 0) {
                                std::println("    li a3, {}\n    beq a1, a3, 1f\n    j failure\n1:", divisor);
                            }
                            backend.finish();
                        }
                    }
                }
            }
        }
    }
    for (const char operation : {'*', '/', '%'}) {
        std::vector<operand> live;
        for (const std::string_view name : {"ra", "a0", "a1", "t0", "t1", "t2", "t3", "t4"}) {
            live.push_back(backend.alloc_named_register(token{}, 0, name, integer));
            std::println("    li {}, {}", name, 100 + live.size());
        }
        std::println("    addi sp, sp, -16\n    mv s2, sp\n    li a2, -17\n    sw a2, 0(sp)\n    li a2, 3\n    sw a2, 4(sp)");
        const operand destination{operand::mem("sp", {}, 1, 0, integer)};
        const operand source{operand::mem("sp", {}, 1, 4, integer)};
        if (operation == '*') {
            backend.multiply(token{}, 1, destination, source);
        } else {
            backend.divide(token{}, 1, operation, destination, source);
        }
        for (const auto [index, reg] : std::views::enumerate(live)) {
            std::println("    li a2, {}\n    beq {}, a2, 1f\n    j failure\n1:", 101 + index, reg.base_register());
        }
        const int expected{operation == '*' ? -51 : operation == '/' ? -5 : -2};
        std::println("    lw a2, 0(sp)\n    li a3, {}\n    beq a2, a3, 1f\n    j failure\n1:\n    beq sp, s2, 1f\n    j failure\n1:\n    addi sp, sp, 16", expected);
        backend.free_named_registers(token{}, 0, live);
        backend.finish();
    }
    for (const char operation : {'*', '/', '%'}) {
        std::println("    la a1, buffer\n    addi a0, a1, 4\n    li a2, -17\n  "
                     "  sw a2, 0(a1)\n    li a2, 3\n    sw a2, 0(a0)");
        const operand destination{operand::mem("a1", {}, 1, 0, integer)};
        const operand source{operand::mem("a0", {}, 1, 0, integer)};
        if (operation == '*') {
            backend.multiply(token{}, 1, destination, source);
        } else {
            backend.divide(token{}, 1, operation, destination, source);
        }
        const int expected{operation == '*' ? -51 : operation == '/' ? -5 : -2};
        std::println(
            "    la a2, buffer\n    beq a1, a2, 1f\n    j failure\n1:\n    "
            "addi a2, a2, 4\n    beq a0, a2, 1f\n    j failure\n1:\n    lw a2, "
            "0(a1)\n    li a3, {}\n    beq a2, a3, 1f\n    j failure\n1:",
            expected);
        backend.finish();
    }
    for (const char operation : {'/', '%'}) {
        std::println("    li a1, -17\n    li a0, 3");
        backend.divide(token{}, 1, operation, operand::reg("a1", integer), operand::reg("a0", integer));
        std::println("    li a3, {}\n    beq a1, a3, 1f\n    j failure\n1:\n    li a3, 3\n    beq a0, a3, 1f\n    j failure\n1:", operation == '/' ? -5 : -2);
        std::println("    li a0, -17");
        backend.divide(token{}, 1, operation, operand::reg("a0", integer), operand::reg("x10", integer));
        std::println("    li a3, {}\n    beq a0, a3, 1f\n    j failure\n1:", operation == '/' ? 1 : 0);
        std::println("    la t0, buffer\n    li a0, -17\n    sw a0, 0(t0)");
        const operand address{operand::mem("t0", {}, 1, 0, integer)};
        backend.divide(token{}, 1, operation, address, address);
        std::println("    lw a0, 0(t0)\n    li a3, {}\n    beq a0, a3, 1f\n    j failure\n1:", operation == '/' ? 1 : 0);
        backend.finish();
    }
    for (const type* value_type : {&integer, &half, &byte}) {
        for (const bool memory_product : {false, true}) {
            for (const int32_t initial : {0, 1, -1, 7, -128, 32767, INT32_MIN, INT32_MAX}) {
                for (const int32_t multiplier : {0, 1, -1, 2, 3, 12, -7, 255, INT32_MIN, INT32_MAX}) {
                    for (const unsigned source_kind : {0U, 1U, 2U, 3U}) {
                        std::println("    la a2, buffer\n    li a1, {}\n    sw a1, 4(a2)", multiplier);

                        const operand product{memory_product
                            ? operand::mem("a2", {}, 1, 0, *value_type)
                            : operand::reg("a0", *value_type)};

                        operand factor{operand::reg("a1", integer)};
                        if (source_kind == 1) {
                            factor = operand::mem("a2", {}, 1, 4, integer);
                        } else if (source_kind == 2) {
                            factor = operand::imm(std::format("{}", multiplier), integer);
                        }
                        backend.copy_value(token{}, 1, product, operand::imm(std::format("{}", initial), integer));
                        backend.multiply(token{}, 1, product, factor, source_kind == 3);
                        backend.copy_value(token{}, 1, operand::reg("a0", integer), product);
                        const size_t bits{value_type->size_bytes() * 8};
                        const uint32_t mask{UINT32_MAX >> (32 - bits)};
                        uint32_t expected{(static_cast<uint32_t>(initial) * static_cast<uint32_t>(multiplier)) & mask};
                        if ((expected & (uint32_t{1} << (bits - 1))) != 0) {
                            expected |= ~mask;
                        }

                        std::println("    li a3, {}\n    beq a0, a3, 1f\n    j failure\n1:", std::bit_cast<int32_t>(expected));

                        if (source_kind == 0) {
                            std::println("    li a3, {}\n    beq a1, a3, 1f\n    j failure\n1:", multiplier);
                        }
                        backend.finish();
                    }
                }
            }
        }
    }
    for (const bool reuse : {false, true}) {
        std::println("    li a0, -7");
        backend.multiply(token{}, 1, operand::reg("a0", integer), operand::reg("x10", integer), reuse);
        std::println("    li a3, 49\n    beq a0, a3, 1f\n    j failure\n1:");
        std::println("    la a2, buffer\n    li a0, -7\n    sw a0, 0(a2)");
        const operand address{operand::mem("a2", {}, 1, 0, integer)};
        backend.multiply(token{}, 1, address, address, reuse);
        std::println("    lw a0, 0(a2)\n    li a3, 49\n    beq a0, a3, 1f\n    j failure\n1:");
        backend.finish();
    }
    for (const size_t scale : {size_t{0}, size_t{1}, size_t{2}, size_t{3}, size_t{7}, size_t{12}, size_t{256}, size_t{4097}}) {
        std::println("    li a0, -7");
        backend.scale_index(token{}, 1, operand::reg("a0", integer), scale);

        std::println("    li a3, {}\n    beq a0, a3, 1f\n    j failure\n1:",
                     std::bit_cast<int32_t>(uint32_t{0xfffffff9} * static_cast<uint32_t>(scale)));

        backend.finish();
    }
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
        std::println("    li a3, 1\n    beq a0, a3, 1f\n    j failure\n1:");
        backend.finish();
    }
    size_t comparison_index{};
    for (const std::string_view operation : {"==", "!=", "<", ">=", ">", "<="}) {
        for (const bool inverted : {false, true}) {
            for (const bool branch_on_true : {false, true}) {
                for (const int32_t right_value : {INT32_MIN, -2049, -2048, -9, -7, 0, 3, 2046, 2047, 2048, INT32_MAX}) {
                    for (const unsigned mode : {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U}) {
                        const std::string target{std::format("comparison_{}", comparison_index++)};
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
                        std::println("    li a0, -7\n    li a1, {}\n    la a2, buffer\n    sw a0, 0(a2)\n    sw a1, 4(a2)", right_value);
                        const operand lhs{mode == 5 ? operand::mem("a2", {}, 1, 0, integer) : operand::reg("a0", integer)};
                        operand rhs{operand::reg("a1", integer)};
                        if (mode == 5) {
                            rhs = operand::mem("a2", {}, 1, 4, integer);
                        } else if (mode >= 6) {
                            rhs = operand::imm(std::format("{}", right_value), integer);
                        }

                        backend.compare_and_branch(token{}, 1, lhs, rhs, {
                            .operation{operation},
                            .inverted{inverted},
                            .destination{destination},
                            .target{mode == 4 ? std::string_view{} : target},
                            .branch_on_true{branch_on_true},
                        }, {});

                        if (mode != 4) {
                            if (expected == branch_on_true) {
                                std::println("    j failure\n{}:", target);
                            } else {
                                std::println("    j {}_done\n{}:\n    j failure\n{}_done:", target, target, target);
                            }
                        }
                        if (not destination.is_empty()) {
                            backend.copy_value(token{}, 1, operand::reg("a3", integer), destination);
                            std::println("    li a4, {}\n    beq a3, a4, 1f\n    j failure\n1:", expected ? 1 : 0);
                        }
                        backend.finish();
                    }
                }
            }
        }
    }
    for (size_t count{}; count <= 35; ++count) {
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
                         count);
            for (size_t offset{}; offset < 64; ++offset) {
                const int expected{
                    offset >= start and offset < start + count ? 0 : 255};
                std::println("    lbu a0, {}(sp)\n    li a1, {}\n    beq a0, "
                             "a1, 1f\n    j failure\n1:",
                             offset, expected);
            }
            std::println("    addi sp, sp, 64");
            backend.finish();
        }
    }
    for (const type* value_type : {&integer, &half, &byte, &boolean}) {
        for (const bool memory_destination : {false, true}) {
            for (const int32_t initial : {0, 1, -1, -128, -32768, INT32_MIN}) {
                for (const char operation : {'-', '~'}) {
                    std::println("    la a2, buffer");

                    const operand destination{memory_destination
                        ? operand::mem("a2", {}, 1, 0, *value_type)
                        : operand::reg("a0", *value_type)};

                    backend.copy_value(token{}, 1, destination, operand::imm(std::format("{}", initial), integer));
                    backend.unary(1, operation, destination);
                    backend.copy_value(token{}, 1, operand::reg("a0", integer), destination);
                    const uint32_t bits{static_cast<uint32_t>(initial)};
                    const size_t width{value_type->size_bytes() * 8};
                    const uint32_t mask{UINT32_MAX >> (32 - width)};
                    uint32_t expected{(operation == '-' ? uint32_t{} - bits : ~bits) & mask};
                    if (value_type != &boolean and (expected & (uint32_t{1} << (width - 1))) != 0) {
                        expected |= ~mask;
                    }

                    std::println("    li a3, {}\n    beq a0, a3, 1f\n    j failure\n1:",
                                 std::bit_cast<int32_t>(expected));

                    backend.finish();
                }
            }
        }
    }
    for (const type* value_type : {&integer, &half, &byte}) {
        for (const char operation : {'+', '-', '&', '|', '^'}) {
            for (const bool memory_destination : {false, true}) {
                for (const unsigned source_kind : {0U, 1U, 2U}) {
                    for (const int32_t source_value : {-2049, -2048, -1, 0, 1, 2047, 2048}) {
                        std::println("    la a2, buffer\n    li a1, {}\n    sw a1, 4(a2)", source_value);

                        const operand destination{memory_destination
                            ? operand::mem("a2", {}, 1, 0, *value_type)
                            : operand::reg("a0", *value_type)};

                        operand source{operand::reg("a1", integer)};
                        if (source_kind == 1) {
                            source = operand::mem("a2", {}, 1, 4, integer);
                        } else if (source_kind == 2) {
                            source = operand::imm(std::format("{}", source_value), integer);
                        }
                        backend.copy_value(token{}, 1, destination, operand::imm("127", integer));
                        uint32_t expected{127};
                        const uint32_t rhs{static_cast<uint32_t>(source_value)};
                        if (operation == '+' or operation == '-') {
                            backend.add_subtract(token{}, 1, operation, destination, source);
                            expected = operation == '+' ? expected + rhs : expected - rhs;
                        } else {
                            backend.bitwise(token{}, 1, operation, destination, source);
                            if (operation == '&') {
                                expected &= rhs;
                            } else if (operation == '|') {
                                expected |= rhs;
                            } else {
                                expected ^= rhs;
                            }
                        }
                        const size_t bits{value_type->size_bytes() * 8};
                        const uint32_t sign{uint32_t{1} << (bits - 1)};
                        const uint32_t mask{UINT32_MAX >> (32 - bits)};
                        expected &= mask;
                        if ((expected & sign) != 0) {
                            expected |= ~mask;
                        }
                        backend.copy_value(token{}, 1, operand::reg("a0", integer), destination);

                        std::println("    li a3, {}\n    beq a0, a3, 1f\n    j failure\n1:",
                                     std::bit_cast<int32_t>(expected));

                        backend.finish();
                    }
                }
            }
        }
    }
    for (const char operation : {'+', '-', '&', '|', '^'}) {
        std::println("    la a0, buffer\n    li a1, -7\n    sw a1, 0(a0)");
        const operand destination{operand::mem("a0", {}, 1, 0, integer)};
        const operand source{operand::mem("x10", {}, 1, 0, integer)};
        int expected{};
        if (operation == '+' or operation == '-') {
            backend.add_subtract(token{}, 1, operation, destination, source);
            expected = operation == '+' ? -14 : 0;
        } else {
            backend.bitwise(token{}, 1, operation, destination, source);
            expected = operation == '^' ? 0 : -7;
        }

        std::println("    lw a1, 0(a0)\n    li a3, {}\n    beq a1, a3, 1f\n    j failure\n1:", expected);

        backend.finish();
    }
    for (const type* value_type : {&integer, &half, &byte, &boolean}) {
        for (const char operation : {'<', '>'}) {
            for (const bool memory_destination : {false, true}) {
                for (const unsigned count_kind : {0U, 1U, 2U}) {
                    std::println("    la a2, buffer\n    li a1, 35\n    sw a1, 4(a2)");

                    const operand destination{memory_destination
                        ? operand::mem("a2", {}, 1, 0, *value_type)
                        : operand::reg("a0", *value_type)};

                    operand count{operand::reg("a1", byte)};
                    if (count_kind == 1) {
                        count = operand::mem("a2", {}, 1, 4, byte);
                    } else if (count_kind == 2) {
                        count = operand::imm("35", integer);
                    }

                    backend.copy_value(token{}, 1, destination, operand::imm("-16", integer));
                    backend.shift(token{}, 1, operation, destination, count);
                    backend.copy_value(token{}, 1, operand::reg("a0", integer), destination);

                    std::println("    li a3, {}\n    beq a0, a3, 1f\n    j failure\n1:",
                                 value_type == &boolean ? (operation == '<' ? 128 : 30)
                                                        : (operation == '<' ? -128 : -2));

                    backend.finish();
                }
            }
        }
    }
    std::println("    li a0, 3");
    backend.shift(token{}, 1, '<', operand::reg("a0", integer), operand::reg("x10", integer));
    std::println("    li a3, 24\n    beq a0, a3, 1f\n    j failure\n1:");
    std::println("    la a0, buffer\n    li a1, 2\n    sw a1, 0(a0)");
    backend.shift(token{}, 1, '<', operand::mem("a0", {}, 1, 0, integer), operand::mem("a0", {}, 1, 0, integer));
    std::println("    lw a1, 0(a0)\n    li a3, 8\n    beq a1, a3, 1f\n    j failure\n1:");
    backend.finish();
    for (const int64_t offset : {INT64_C(-4294967295), INT64_C(-2147483648), INT64_C(-2049),
                                 INT64_C(-2048), INT64_C(2047), INT64_C(2048), INT64_C(8196),
                                 INT64_C(2147483648), INT64_C(4294967295)}) {
                    std::println("    la t6, buffer\n    li a3, {}\n    sub t6, t6, a3", static_cast<uint32_t>(offset));
                    const operand direct{operand::mem("x31", {}, 1, offset, integer)};
                    backend.copy_value(token{}, 1, direct, operand::imm("42", integer));
                    backend.copy_value(token{}, 1, operand::reg("a0", integer), direct);
                    std::println("    li a3, 42\n    bne a0, a3, failure");
                    backend.address_of(token{}, 1, operand::reg("t6", integer), direct);
                    std::println("    la a3, buffer\n    bne t6, a3, failure");
                    backend.finish();
                    for (const uint64_t scale : std::array<uint64_t, 11>{
                             1, 2, 4, 8, 16, 32, 64, 128, 256, 65536,
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
                        backend.copy_value(
                            token{}, 1, operand::reg("t3", integer), address);
                        std::println("    li a3, 42\n    bne t3, a3, failure");
                        backend.address_of(
                            token{}, 1, operand::reg("t3", integer), address);
                        std::println(
                            "    la a3, buffer\n    bne t3, a3, failure");
                        std::println("    la a4, buffer_copy");
                        backend.copy_value(
                            token{}, 1,
                            operand::mem("a4", {}, 1, 0, integer),
                            address);
                        backend.copy_value(
                            token{}, 1, operand::reg("t3", integer),
                            operand::mem("a4", {}, 1, 0, integer));
                        std::println("    li a3, 42\n    bne t3, a3, failure");
                        std::println("    la t6, buffer\n    li a3, {}\n    "
                                     "sub t6, t6, a3\n    li t5, 5\n    li a3, "
                                     "{}\n    sub t6, t6, a3",
                                     static_cast<uint32_t>(offset),
                                     static_cast<uint32_t>(5 * scale));
                        std::println("    la a4, pointer");
                        backend.address_of(
                            token{}, 1,
                            operand::mem("a4", {}, 1, 0, integer),
                            address);
                        backend.copy_value(
                            token{}, 1, operand::reg("t3", integer),
                            operand::mem("a4", {}, 1, 0, integer));
                        std::println(
                            "    la a3, buffer\n    bne t3, a3, failure");
                        backend.finish();
                    }
    }
    for (const uint64_t scale :
           {UINT64_C(1), UINT64_C(2), UINT64_C(4), UINT64_C(256),
            UINT64_C(65536), UINT64_C(2147483648)}) {
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
    for (const type* value_type : {&byte, &half, &boolean}) {
        std::println("    la a2, buffer");
        backend.copy_value(token{}, 1, operand::mem("a2", {}, 1, 0, *value_type), operand::imm("-1", integer));
        backend.copy_value(token{}, 1, operand::reg("a0", integer), operand::mem("a2", {}, 1, 0, *value_type));
        std::println("    li a1, {}\n    bne a0, a1, failure", value_type == &boolean ? 255 : -1);
    }
    backend.address_of(token{}, 1, operand::reg("t6", integer), operand::mem("sp", "sp", 4, 2048, integer));
    std::println("    slli a0, sp, 2\n    add a0, a0, sp\n    li a1, 2048\n    add a0, a0, a1\n    bne t6, a0, failure");
    std::vector<operand> io_args;
    for (const std::string_view name : backend.registers_for_builtin(machine::builtin_function::read).arguments) {
        io_args.push_back(backend.alloc_named_register(token{}, 0, name, integer));
    }
    std::println("    mv s2, sp\n    li a0, 0\n    la a1, buffer\n    li a2, 6");
    backend.read(token{}, 1, io_args.at(0), io_args.at(0), io_args.at(1), io_args.at(2));
    std::println("    li t0, 6\n    bne a0, t0, failure\n    bne a2, t0, failure\n    la t0, buffer\n    bne a1, t0, failure\n    bne sp, s2, failure\n    li a0, 1");
    backend.write(token{}, 1, io_args.at(0), io_args.at(0), io_args.at(1), io_args.at(2));
    std::println("    li t0, 6\n    bne a0, t0, failure\n    bne a2, t0, failure\n    la t0, buffer\n    bne a1, t0, failure\n    li a0, -1");
    backend.read(token{}, 1, io_args.at(0), io_args.at(0), io_args.at(1), io_args.at(2));
    std::println("    li t0, -9\n    bne a0, t0, failure\n    li a0, -1");
    backend.write(token{}, 1, io_args.at(0), io_args.at(0), io_args.at(1), io_args.at(2));
    std::println("    li t0, -9\n    bne a0, t0, failure\n    bne sp, s2, failure");
    backend.free_named_registers(token{}, 0, io_args);
    const operand held_syscall_register{backend.alloc_named_register(token{}, 0, "a7", integer)};
    bool syscall_conflict{};
    try {
        backend.read(token{}, 1, operand::reg("a0", integer), operand::reg("a0", integer),
                     operand::reg("a1", integer), operand::reg("a2", integer));
    } catch (const compiler_exception&) {
        syscall_conflict = true;
    }
    assert(syscall_conflict);
    backend.free_named_register(token{}, 0, held_syscall_register);
    backend.program_end();
    std::println(".globl divide_by_zero\ndivide_by_zero:\n    li a0, 17");
    backend.divide(token{}, 1, '/', operand::reg("a0", integer), operand::imm("0", integer));
    backend.exit(token{}, 1, operand::imm("0", integer));
    for (const uint32_t line : {0U, 9U, 123U, UINT32_MAX}) {
        std::println(".globl bounds_line_{}\nbounds_line_{}:\n    li a0, -1", line, line);
        const token location{{}, 0, {}, 0, {}, line, false};
        backend.check_bounds(location, 1, operand::reg("a0", integer), 4, false, {},
                             {.upper{true}, .lower{true}, .with_line{true}});
        backend.exit(token{}, 1, operand::imm("0", integer));
    }
    backend.emit_bounds_failure_handler(true);
    std::println("failure:\n    li a0, 1\n    li a7, 93\n    ecall");
    backend.begin_data(4);
    std::println("buffer: .zero 16\nbuffer_copy: .word 0\npointer: .word 0");
    backend.finish();

    return 0;
}