#include <cassert>
#include <cstdint>
#include <print>
#include <sstream>
#include <string_view>

#include "../../src/decouple_impl.hpp" // IWYU pragma: keep
#include "../../src/machine_x86.hpp"

static auto check_ident(const toc& tc, const std::string_view name,
                        const type& expected_type) -> void {
    const ident_info info{tc.make_ident_info(token{}, name)};
    assert(&info.type_ref() == &expected_type);
    assert(&info.operand.type_ref() == &expected_type);
    assert(info.operand.base_register() == name);
}

auto main() -> int {
    const type i64{"i64", sizeof(int64_t), true};
    const type i32{"i32", sizeof(int32_t), true};
    const type i16{"i16", sizeof(int16_t), true};
    const type i8{"i8", sizeof(int8_t), true};
    const type boolean{"bool", i8.size_bytes(), true};
    const type void_type{"void", 0, true};
    std::ostringstream output;
    machine_x86 backend{output, {}};
    backend.set_builtin_types(i64, i32, i16, i8, boolean, void_type);
    toc tc{backend, {}, 65536, false, false, false};
    for (const type* const value_type : {&i64, &i32, &i16, &i8, &boolean}) {
        tc.add_type(token{}, *value_type);
    }

    check_ident(tc, "al", i8);
    const operand named{
        backend.alloc_named_register(token{}, 0, "rax", boolean)};
    assert(&named.type_ref() == &boolean);
    assert(named.base_register() == "al");
    assert(named.allocation_register() == "rax");
    check_ident(tc, "rax", i64);
    check_ident(tc, "eax", i32);
    check_ident(tc, "ax", i16);
    check_ident(tc, "al", boolean);
    check_ident(tc, "ah", boolean);
    backend.free_named_register(token{}, 0, named);
    check_ident(tc, "al", i8);
    check_ident(tc, "ah", i8);
    assert(backend.allocated_register_type("al") == nullptr);

    backend.reserve_variables_base();
    check_ident(tc, "rbp", i64);
    check_ident(tc, "ebp", i32);
    check_ident(tc, "bp", i16);
    check_ident(tc, "bpl", i8);
    backend.release_variables_base();
    check_ident(tc, "bp", i16);

    const operand first{backend.alloc_scratch_register(token{}, 0, i64)};
    const operand second{backend.alloc_scratch_register(token{}, 0, i64)};
    const operand third{backend.alloc_scratch_register(token{}, 0, i64)};
    assert(third.allocation_register() == "r13");
    check_ident(tc, "r13b", i8);
    backend.free_scratch_register(token{}, 0, third);
    const operand scratch{backend.alloc_scratch_register(token{}, 0, boolean)};
    assert(&scratch.type_ref() == &boolean);
    check_ident(tc, "r13b", boolean);
    check_ident(tc, "r13w", i16);
    check_ident(tc, "r13d", i32);
    check_ident(tc, "r13", i64);
    backend.free_scratch_register(token{}, 0, scratch);
    check_ident(tc, "r13b", i8);
    assert(backend.allocated_register_type("r13b") == nullptr);
    backend.free_scratch_register(token{}, 0, second);
    backend.free_scratch_register(token{}, 0, first);

    operand memory{operand::mem("rbp", {}, 1, 0, i64)};
    memory.increment_offset(static_cast<int32_t>(i64.size_bytes()));
    assert(memory.displacement() == static_cast<int32_t>(i64.size_bytes()));
    const operand byte_view{operand::mem(memory, boolean)};
    assert(&byte_view.type_ref() == &boolean);
    assert(&memory.type_ref() == &i64);
    assert(byte_view.displacement() == memory.displacement());

    for (const type* const value_type : {&i8, &i16, &i32, &i64, &boolean}) {
        const operand immediate{operand::imm("1", *value_type)};
        assert(&immediate.type_ref() == value_type);
        for (const type* const dst_type : {&i8, &i16, &i32, &i64}) {
            const operand dst{backend.reg("rax", *dst_type)};
            const operand dst_memory{operand::mem(memory, *dst_type)};
            output.str("");
            backend.copy_value(token{}, 0, dst, immediate);
            backend.copy_value(token{}, 0, dst_memory, immediate);
            assert(output.str().find("movsx") == std::string::npos);
            assert(output.str().find("mov ") != std::string::npos);
        }
    }

    output.str("");
    backend.copy_value(token{}, 0, backend.reg("rax", i64),
                       operand::imm("-7", i8));
    assert(output.str().find("mov rax, -7") != std::string::npos);
    backend.finish();
    std::println("operand type contracts: ok");
}