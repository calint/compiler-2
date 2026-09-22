#include <iostream>
#include <sstream>

#include "../../src/decouple_impl.hpp"
#include "../../src/machine_rv32i.hpp"
#include "../../src/machine_x86.hpp"
#include "../../src/program.hpp"

auto main(const int argc, const char* argv[]) -> int {
    const type integer64{"i64", 8, true};
    const type integer{"i32", 4, true};
    const type half{"i16", 2, true};
    const type byte{"i8", 1, true};
    const type boolean{"bool", 1, true};
    const type empty{"void", 0, true};
    if (argc > 1 and std::string_view{argv[1]} == "bulk") {
        const std::string_view source{R"baz(
func assert(ok : bool) if not ok exit(1)
type packed { first : i8, second : i16 }
func nested(source : packed[], destination : packed[]) : i32 count {
    array_copy(source, destination, 1)
    count = 2
}
func main() {
    var source : packed[3] = {{1, 300}, {2, -400}, {3, 500}}
    var destination : packed[3]
    var single : packed = source[1]
    assert(equal(single, source[1]))
    single.second = 12
    assert(not equal(single, source[1]))
    array_copy(source, destination, nested(source, destination))
    assert(arrays_equal(source, destination, 2))
    assert(not arrays_equal(source, destination, 3))
    array_copy(source[2], destination[2], 1)
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
dat text : i8[] = "A\0\a\b\t\n\v\f\r\e\"'`\\\x00\x7f\x80\xff\x41B"
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
    assert(not backend.can_encode_index_scale(1));

    std::ostringstream shift_output;
    backend.use_stream(shift_output);
    for (const bool counted : {false, true}) {
        std::vector<operand> held;
        for (size_t count{}; count < 25; ++count) {
            held.push_back(backend.alloc_scratch_register(token{}, 0, boolean));
        }
        const operand result{held.front()};
        const operand count{backend.begin_memory_equal(token{}, 0)};
        backend.copy_value(token{}, 0, count, operand::imm("7", integer));
        backend.set_memory_equal_left(0, operand::mem("buffer", {}, 1, 0, byte));
        backend.set_memory_equal_right(0, operand::mem("buffer", {}, 1, 0, byte));
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

    std::ostringstream address_output;
    backend.use_stream(address_output);
    std::vector<operand> address_registers;
    for (size_t count{}; count < 30; ++count) {
        address_registers.push_back(backend.alloc_scratch_register(token{}, 0, integer));
    }
    assert(address_registers.back().base_register() == "a0");
    backend.free_scratch_register(token{}, 0, address_registers.back());
    address_registers.pop_back();
    address_output.str({});

    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("buffer", {}, 1, 4, integer));

    assert(address_output.str() == "la a1, buffer\nlw a1, 4(a1)\n");
    address_output.str({});

    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("a2", "a3", 1, 4, integer));

    assert(address_output.str() == "add a1, a2, a3\nlw a1, 4(a1)\n");
    address_output.str({});

    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("a2", {}, 1, 8196, integer));

    assert(address_output.str() == "li a1, 8196\nadd a1, a1, a2\nlw a1, 0(a1)\n");
    address_output.str({});

    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem("buffer", "a3", 1, 4, integer));

    assert(address_output.str() == "la a1, buffer\nadd a1, a1, a3\nlw a1, 4(a1)\n");
    address_registers.push_back(backend.alloc_scratch_register(token{}, 0, integer));
    address_output.str({});

    backend.address_of(token{}, 0, operand::reg("a1", integer),
                       operand::mem("buffer", {}, 1, 0, integer));

    assert(address_output.str() == "la a1, buffer\n");
    address_output.str({});

    backend.copy_value(token{}, 0, operand::reg("a1", integer),
                       operand::mem({}, "a2", 1, 4, integer));

    assert(address_output.str() == "lw a1, 4(a2)\n");
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

    backend.compare_and_branch(token{}, 0, operand::reg("a0", integer),
        operand::reg("a1", integer), {
            .operation{"<"},
            .destination{operand::reg("a2", boolean)},
        }, {});

    assert(address_output.str() == "slt a2, a0, a1\n");
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
    assert(address_output.str() == "slli a1, a1, ((3) & 31)\n");
    address_output.str({});
    backend.multiply(token{}, 0, operand::reg("a1", integer), operand::imm("-1", integer));
    assert(address_output.str() == "sub a1, zero, a1\n");
    address_output.str({});
    backend.multiply(token{}, 0, operand::mem("a2", {}, 1, 0, integer), operand::imm("0", integer));
    assert(address_output.str() == "sw zero, 0(a2)\n");
    backend.free_scratch_registers(token{}, 0, address_registers);
    backend.finish();

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
                backend.copy(token{}, 1, operand::mem("a0", {}, 1, 0, byte),
                             operand::mem("a1", {}, 1, 0, byte), size_bytes);
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
    for (const size_t count : {size_t{1}, size_t{2}, size_t{3}, size_t{4}}) {
        std::println("    la a2, buffer\n    li a0, -1\n    sw a0, 0(a2)\n    sw a0, 4(a2)");
        backend.zero(token{}, 1, operand::mem("a2", {}, 1, 1, byte), count);
        for (size_t offset{}; offset < 6; ++offset) {
            const int expected{offset >= 1 and offset <= count ? 0 : 255};
            std::println("    lbu a0, {}(a2)\n    li a1, {}\n    beq a0, a1, 1f\n    j failure\n1:", offset, expected);
        }
        backend.finish();
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