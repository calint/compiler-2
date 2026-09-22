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
    for (const bool live_clobbers : {false, true}) {
        std::vector<operand> scratch;
        operand live_rcx;
        if (live_clobbers) {
            live_rcx = x86_backend.alloc_named_register(token{}, 0, "rcx", integer64);
            for (size_t count{}; count < 8; ++count) {
                scratch.push_back(x86_backend.alloc_scratch_register(token{}, 0, integer64));
            }
        }
        const machine::builtin_registers contract{
            x86_backend.registers_for_builtin(machine::builtin_function::write)};

        std::vector<operand> args;
        for (const std::string_view name : contract.arguments) {
            args.push_back(x86_backend.alloc_named_register(token{}, 0, name, integer64));
        }
        const operand result{x86_backend.alloc_named_register(token{}, 0, contract.result, integer64)};
        x86_output.str({});
        x86_backend.write(token{}, 0, result, args.at(0), args.at(1), args.at(2));
        const std::string assembly{x86_output.str()};
        assert(assembly.contains("push rcx") == live_clobbers);
        assert(assembly.contains("pop rcx") == live_clobbers);
        assert(assembly.contains("push r11") == live_clobbers);
        assert(assembly.contains("pop r11") == live_clobbers);
        assert(not assembly.contains("rsp"));
        assert(not assembly.contains("push rax"));
        assert(not assembly.contains("push rdi"));
        x86_backend.free_named_register(token{}, 0, result);
        x86_backend.free_named_registers(token{}, 0, args);
        x86_backend.free_scratch_registers(token{}, 0, scratch);
        if (live_clobbers) {
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