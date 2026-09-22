#include <iostream>
#include <sstream>

#include "../../src/decouple_impl.hpp"
#include "../../src/machine_rv32i.hpp"
#include "../../src/machine_x86.hpp"
#include "../../src/program.hpp"

auto main() -> int {
    const type integer64{"i64", 8, true};
    const type integer{"i32", 4, true};
    const type half{"i16", 2, true};
    const type byte{"i8", 1, true};
    const type boolean{"bool", 1, true};
    const type empty{"void", 0, true};
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
    assert(not backend.can_encode_index_scale(1));

    std::ostringstream shift_output;
    backend.use_stream(shift_output);
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

        backend.shift(token{}, 0, operation, operand::reg("a0", integer),
                      operand::imm("35", integer));

        assert(shift_output.str() == (operation == '<' ? "slli a0, a0, ((35) & 31)\n" : "srai a0, a0, ((35) & 31)\n"));
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

    assert(shift_output.str() == "lw t6, 0(a0)\nadd t6, t6, t6\nsw t6, 0(a0)\n");
    backend.finish();
    shift_output.str({});

    backend.shift(token{}, 0, '<', operand::mem("a0", {}, 1, 0, integer),
                  operand::mem("x10", {}, 1, 0, integer));

    assert(shift_output.str() == "lw t6, 0(a0)\nsll t6, t6, t6\nsw t6, 0(a0)\n");
    backend.finish();
    {
        const std::string_view source{
            "func main() { var a = 3 var b = 2 var x = a << b exit(x) }"};

        std::ostringstream output;
        machine_rv32i compiler;
        program prg{compiler, source, 4096, false, false, false};
        prg.build(output);
        assert(output.str().contains("sll t6, t6, t5"));
        assert(not output.str().contains("addi t5, t6, 0"));
    }

    std::ostringstream rejected_output;
    backend.use_stream(rejected_output);
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
    for (const uint8_t scale : std::array<uint8_t, 3>{0, 3, 255}) {
        bool rejected{};
        try {
            backend.address_of(token{}, 1, operand::reg("a0", integer),
                               operand::mem("a1", "a2", scale, 0, integer));
        } catch (const compiler_exception&) {
            rejected = true;
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
                           operand::mem("a1", {}, 1, 8196, integer));
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

    std::println(".option norvc\n.option norelax\n.text\n.globl _start\n_start:");
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
        for (const uint8_t scale : std::array<uint8_t, 8>{1, 2, 4, 8, 16, 32, 64, 128}) {
            std::println("    la t6, buffer\n    li a3, {}\n    sub t6, t6, a3\n    li t5, 5\n    li a3, {}\n    sub t6, t6, a3\n    li t4, 42", static_cast<uint32_t>(offset), 5 * scale);
            const operand address{operand::mem("x31", "x30", scale, offset, integer)};
            backend.copy_value(token{}, 1, address, operand::reg("x29", integer));
            backend.copy_value(token{}, 1, operand::reg("t3", integer), address);
            std::println("    li a3, 42\n    bne t3, a3, failure");
            backend.address_of(token{}, 1, operand::reg("t3", integer), address);
            std::println("    la a3, buffer\n    bne t3, a3, failure");
            backend.copy_value(token{}, 1, operand::mem("buffer_copy", {}, 1, 0, integer), address);
            backend.copy_value(token{}, 1, operand::reg("t3", integer), operand::mem("buffer_copy", {}, 1, 0, integer));
            std::println("    li a3, 42\n    bne t3, a3, failure");
            std::println("    la t6, buffer\n    li a3, {}\n    sub t6, t6, a3\n    li t5, 5\n    li a3, {}\n    sub t6, t6, a3", static_cast<uint32_t>(offset), 5 * scale);
            backend.address_of(token{}, 1, operand::mem("pointer", {}, 1, 0, integer), address);
            backend.copy_value(token{}, 1, operand::reg("t3", integer), operand::mem("pointer", {}, 1, 0, integer));
            std::println("    la a3, buffer\n    bne t3, a3, failure");
            backend.finish();
        }
    }
    for (const type* value_type : {&byte, &half, &boolean}) {
        backend.copy_value(token{}, 1, operand::mem("buffer", {}, 1, 0, *value_type), operand::imm("-1", integer));
        backend.copy_value(token{}, 1, operand::reg("a0", integer), operand::mem("buffer", {}, 1, 0, *value_type));
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
    std::println("    li a0, 0\n    li a7, 93\n    ecall\nfailure:\n    li a0, 1\n    li a7, 93\n    ecall\n.data\n.balign 4\nbuffer: .zero 16\nbuffer_copy: .word 0\npointer: .word 0");
    backend.finish();

    return 0;
}