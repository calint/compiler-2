#include <cassert>
#include <cstdint>
#include <print>
#include <sstream>
#include <string_view>

#include "../../src/decouple_impl.hpp" // IWYU pragma: keep
#include "../../src/machine_x86.hpp"
#include "../../src/stmt_assign_var.hpp"
#include "../../src/stmt_def_dat.hpp"

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
    assert(not tc.is_frame_check());
    const toc checked_tc{backend, {}, 65536, false, false, false, true};
    assert(checked_tc.is_frame_check());
    tc.set_type_bool(boolean);
    tc.set_type_void(void_type);
    for (const type* const value_type : {&i64, &i32, &i16, &i8, &boolean}) {
        tc.add_type(token{}, *value_type);
    }

    for (const std::string_view source :
         {"func ordinary() {}", "func noinline() {}",
          "func  noinline\n  separate( ) { }",
          "func noinline recursive() { recursive() }"}) {
        toc parser_tc{backend, source, 65536, false, false, false};
        parser_tc.set_type_void(void_type);
        parser_tc.set_type_bool(boolean);
        parser_tc.enter_block();
        tokenizer function_tokens{source};
        const stmt_def_func function{parser_tc, function_tokens.next_token(),
                                     function_tokens};

        const bool expected_inline{source == "func ordinary() {}" or
                                   source == "func noinline() {}"};

        assert(function.is_inlined() == expected_inline);
        std::ostringstream reproduced;
        function.source_to(reproduced);
        assert(reproduced.str() == source);
        function.compile(parser_tc, 0, ident_info::make_empty());
        parser_tc.exit_block();
    }

    {
        const std::string_view source{
            "func noinline body(value: i16): i16 result { "
            "var local: i16 = value result = local value = 9 return }"};

        toc body_tc{backend, source, 65536, false, false, false};
        body_tc.set_type_void(void_type);
        body_tc.set_type_bool(boolean);
        body_tc.add_type(token{}, i16);
        body_tc.enter_block();
        body_tc.add_var(token{}, 0, {.name{"global"}, .type_ptr{&i64}}, false);
        tokenizer body_tokens{source};
        const stmt_def_func function{body_tc, body_tokens.next_token(),
                                     body_tokens};

        assert(function.body_label() == "body");
        assert(function.frame_size_label() == "body.size");
        backend.reserve_variables_base();
        output.str("");
        const size_t frame_size_bytes{function.compile_body(body_tc, 0)};
        assert(frame_size_bytes == 18);
        assert(output.str().starts_with(";   ["));
        assert(output.str().find("qword [r12 + 8]") != std::string::npos);
        assert(output.str().find("qword [r12]") != std::string::npos);
        assert(output.str().find("word [r12 + 16]") != std::string::npos);
        assert(output.str().ends_with("    ret\n"));
        assert(output.str().find("syscall") == std::string::npos);
        assert(backend.allocated_register_type("r12") == nullptr);
        assert(body_tc.next_frame_address().base_register() == "rbp");
        assert(body_tc.next_frame_address().displacement() == 8);
        backend.release_variables_base();
        body_tc.exit_block();
    }

    check_ident(tc, "al", i8);
    var_info parameter{
        .name{"parameter"},
        .type_ptr{&i16},
        .src_loc_tk{},
        .stack_idx{8},
        .reg{},
    };

    const ident_info direct_info{
        i16.accessor(token{}, "parameter", {"parameter"}, parameter, "r12")};

    assert(not parameter.is_pointer);
    assert(not direct_info.is_pointer);
    parameter.is_pointer = true;
    const ident_info pointer_info{
        i16.accessor(token{}, "parameter", {"parameter"}, parameter, "r12")};

    assert(pointer_info.is_pointer);
    assert(pointer_info.is_var());
    assert(pointer_info.validate_invariants());
    assert(&pointer_info.type_ref() == &i16);
    assert(pointer_info.operand.base_register() == "r12");
    assert(pointer_info.operand.displacement() == 8);
    assert(pointer_info.stack_idx == 8);

    tc.enter_func("pointer_access", {});
    tc.add_var(token{}, 0, parameter, false);
    const ident_info parameter_info{tc.make_ident_info(token{}, "parameter")};
    tokenizer parameter_tokens{"parameter"};
    const stmt_identifier parameter_stmt{
        tc, {}, parameter_tokens.next_token(), parameter_tokens};

    output.str("");
    std::vector<operand> address_registers;
    const operand resolved{tc.get_lea_operand(0, parameter_stmt, parameter_info,
                                              address_registers)};

    assert(address_registers.size() == 1);
    assert(resolved.base_register() ==
           address_registers.front().base_register());
    assert(resolved.displacement() == 0);
    assert(&resolved.type_ref() == &i16);
    assert(output.str().find(
               std::format("mov {}, qword [rbp", resolved.base_register())) !=
           std::string::npos);
    assert(tc.has_lea(parameter_stmt));
    tc.enter_func("inline_forward", {});
    tc.add_alias({
        .from{"forwarded"},
        .to{"parameter"},
        .lea{resolved},
        .type_ptr{&i16},
        .register_operand{},
    });

    tokenizer forwarded_tokens{"forwarded"};
    const stmt_identifier forwarded_stmt{
        tc, {}, forwarded_tokens.next_token(), forwarded_tokens};

    output.str("");
    forwarded_stmt.compile(
        tc, 0, toc::make_ident_info_from_register(backend.reg("rax", i64)));
    assert(output.str().find("movsx rax, word [") != std::string::npos);
    assert(output.str().find("qword [") == std::string::npos);
    tc.exit_func("inline_forward");
    backend.free_scratch_registers(token{}, 0, address_registers);

    output.str("");
    parameter_stmt.compile(
        tc, 0, toc::make_ident_info_from_register(backend.reg("rax", i64)));
    assert(output.str().find("qword [rbp") != std::string::npos);
    assert(output.str().find("movsx rax, word [") != std::string::npos);

    tokenizer assignment_tokens{"parameter = 9"};
    stmt_identifier assignment_target{
        tc, {}, assignment_tokens.next_token(), assignment_tokens};
    const token equals_tk{assignment_tokens.is_next_char_token('=')};
    const stmt_assign_var assignment{
        tc, assignment_tokens, std::move(assignment_target), equals_tk, false,
        0};

    output.str("");
    assignment.compile(tc, 0, ident_info::make_empty());
    assert(output.str().find("qword [rbp") != std::string::npos);
    assert(output.str().find("mov word [") != std::string::npos);
    assert(output.str().find("], 9") != std::string::npos);
    tc.exit_func("pointer_access");

    tc.enter_func("noninline_outer", {}, {}, {}, false);
    assert(not tc.is_inlined_func());
    tc.enter_block();
    tc.enter_loop("outer_loop");
    assert(not tc.is_inlined_func());
    tc.enter_func("inline_inner", {});
    assert(tc.is_inlined_func());
    tc.enter_block();
    assert(tc.is_inlined_func());
    tc.exit_block();
    tc.exit_func("inline_inner");
    assert(not tc.is_inlined_func());
    tc.exit_loop("outer_loop");
    tc.exit_block();
    tc.exit_func("noninline_outer");

    tc.enter_func("inline_outer", {});
    assert(tc.is_inlined_func());
    tc.enter_func("noninline_inner", {}, {}, {}, false);
    tc.enter_block();
    tc.enter_loop("inner_loop");
    assert(not tc.is_inlined_func());
    tc.exit_loop("inner_loop");
    tc.exit_block();
    tc.exit_func("noninline_inner");
    assert(tc.is_inlined_func());
    tc.exit_func("inline_outer");

    const stmt_return return_stmt{tc, token{}};
    tc.enter_func("return_outer", {}, {}, {}, false);
    tc.enter_block();
    tc.enter_loop("return_loop");
    output.str("");
    return_stmt.compile(tc, 0, ident_info::make_empty());
    assert(output.str().find("ret\n") != std::string::npos);
    assert(output.str().find("syscall") == std::string::npos);
    assert(not tc.is_inlined_func());
    tc.enter_func("return_inline", {}, {}, "inline_end");
    output.str("");
    return_stmt.compile(tc, 0, ident_info::make_empty());
    assert(output.str().find("jmp inline_end") != std::string::npos);
    assert(output.str().find("ret\n") == std::string::npos);
    tc.exit_func("return_inline");
    output.str("");
    return_stmt.compile(tc, 0, ident_info::make_empty());
    assert(output.str().find("ret\n") != std::string::npos);
    tc.exit_loop("return_loop");
    tc.exit_block();
    tc.exit_func("return_outer");
    tc.enter_func("main", {});
    output.str("");
    return_stmt.compile(tc, 0, ident_info::make_empty());
    assert(output.str().find("syscall") != std::string::npos);
    assert(output.str().find("ret\n") == std::string::npos);
    tc.exit_func("main");

    tc.enter_block();
    assert(tc.next_frame_address().base_register() == "rbp");
    assert(tc.next_frame_address().displacement() == 0);
    tc.add_var(token{}, 0, {.name{"global"}, .type_ptr{&i64}}, false);
    const ident_info global_info{tc.make_ident_info(token{}, "global")};
    assert(tc.next_frame_address().displacement() == 8);
    backend.reserve_frame_base();
    tc.enter_func("local_storage", {}, {}, {}, false,
                  backend.frame_base_register());

    assert(tc.next_frame_address().base_register() == "r12");
    assert(tc.next_frame_address().displacement() == 0);
    assert(tc.peak_frame_size_bytes() == 0);
    tc.add_var(token{}, 0,
               {.name{"argument"}, .type_ptr{&i16}, .is_pointer{true}}, false);

    tc.add_var(token{}, 0, {.name{"local"}, .type_ptr{&i16}}, false);
    assert(tc.next_frame_address().displacement() == 10);
    assert(tc.peak_frame_size_bytes() == 10);
    const ident_info argument_info{tc.make_ident_info(token{}, "argument")};
    const ident_info local_info{tc.make_ident_info(token{}, "local")};
    assert(argument_info.operand.base_register() == "r12");
    assert(argument_info.stack_idx == 0);
    assert(local_info.operand.base_register() == "r12");
    assert(local_info.stack_idx == 8);
    assert(tc.make_ident_info(token{}, "global").operand.base_register() ==
           "rbp");
    assert(tc.make_ident_info(token{}, "global").stack_idx ==
           global_info.stack_idx);
    tc.enter_block();
    tc.enter_func("inline_storage", {});
    tc.add_var(token{}, 0, {.name{"inner"}, .type_ptr{&i8}}, false);
    assert(tc.make_ident_info(token{}, "inner").operand.base_register() ==
           "r12");
    assert(tc.make_ident_info(token{}, "inner").stack_idx == 10);
    assert(tc.next_frame_address().base_register() == "r12");
    assert(tc.next_frame_address().displacement() == 11);
    tc.enter_loop("storage_loop");
    tc.add_var(token{}, 0, {.name{"loop_local"}, .type_ptr{&i32}}, false);
    assert(tc.next_frame_address().displacement() == 15);
    assert(tc.peak_frame_size_bytes() == 15);
    tc.enter_func("nested_storage", {}, {}, {}, false,
                  backend.frame_base_register());

    assert(tc.next_frame_address().displacement() == 0);
    assert(tc.peak_frame_size_bytes() == 0);
    tc.add_var(token{}, 0, {.name{"nested_local"}, .type_ptr{&i64}}, false);
    assert(tc.next_frame_address().displacement() == 8);
    tc.enter_block();
    tc.add_var(token{}, 0,
               {.name{"nested_buffer"},
                .type_ptr{&i64},
                .is_array{true},
                .array_count{4}},
               false);

    assert(tc.peak_frame_size_bytes() == 40);
    tc.exit_block();
    assert(tc.next_frame_address().displacement() == 8);
    assert(tc.peak_frame_size_bytes() == 40);
    tc.exit_func("nested_storage");
    assert(tc.next_frame_address().displacement() == 15);
    assert(tc.peak_frame_size_bytes() == 15);
    tc.exit_loop("storage_loop");
    assert(tc.next_frame_address().displacement() == 11);
    assert(tc.peak_frame_size_bytes() == 15);
    tc.exit_func("inline_storage");
    tc.exit_block();
    assert(tc.next_frame_address().displacement() == 10);
    assert(tc.peak_frame_size_bytes() == 15);
    tc.add_var(token{}, 0, {.name{"reused"}, .type_ptr{&i8}}, false);
    assert(tc.make_ident_info(token{}, "reused").stack_idx == 10);
    assert(tc.peak_frame_size_bytes() == 15);
    tc.add_var(
        token{}, 0,
        {.name{"buffer"}, .type_ptr{&i16}, .is_array{true}, .array_count{4}},
        false);

    tokenizer buffer_tokens{"buffer"};
    const stmt_identifier buffer_stmt{
        tc, {}, buffer_tokens.next_token(), buffer_tokens};
    const ident_info buffer_info{tc.make_ident_info(buffer_stmt)};
    std::vector<operand> buffer_registers;
    const operand buffer_address{buffer_stmt.compile_lea(
        tc, 0, token{}, buffer_registers, {}, buffer_info.lea_path)};

    assert(buffer_address.base_register() == "r12");
    assert(buffer_address.displacement() == 11);
    assert(buffer_registers.empty());

    tokenizer element_tokens{"buffer[2]"};
    const stmt_identifier element_stmt{
        tc, {}, element_tokens.next_token(), element_tokens};
    const operand element_address{tc.get_lea_operand(
        0, element_stmt, tc.make_ident_info(element_stmt), buffer_registers)};

    assert(element_address.base_register() == "r12");
    assert(element_address.displacement() == 11);
    assert(element_address.scale() == 2);
    assert(not element_address.index_register().empty());
    backend.free_scratch_registers(token{}, 0, buffer_registers);
    buffer_registers.clear();

    type triple{"triple", 0, false};
    triple.add_field(token{}, "first", i8, false, 0);
    triple.add_field(token{}, "second", i8, false, 0);
    triple.add_field(token{}, "third", i8, false, 0);
    tc.add_type(token{}, triple);
    tc.add_var(token{}, 0,
               {.name{"triples"},
                .type_ptr{&triple},
                .is_array{true},
                .array_count{4}},
               false);

    tokenizer triple_tokens{"triples[1].third"};
    const stmt_identifier triple_stmt{
        tc, {}, triple_tokens.next_token(), triple_tokens};

    output.str("");
    const operand triple_address{tc.get_lea_operand(
        0, triple_stmt, tc.make_ident_info(triple_stmt), buffer_registers)};

    assert(output.str().find("[r12 + 19]") != std::string::npos);
    assert(output.str().find("[rbp") == std::string::npos);
    assert(triple_address.base_register() != "r12");
    assert(triple_address.displacement() == 2);
    assert(&triple_address.type_ref() == &i8);
    backend.free_scratch_registers(token{}, 0, buffer_registers);
    assert(tc.peak_frame_size_bytes() == 31);
    tc.exit_func("local_storage");
    backend.release_frame_base();
    assert(tc.next_frame_address().base_register() == "rbp");
    assert(tc.next_frame_address().displacement() == 8);
    tc.add_var(token{}, 0, {.name{"root_local"}, .type_ptr{&i16}}, false);
    assert(tc.make_ident_info(token{}, "root_local").operand.base_register() ==
           "rbp");
    assert(tc.make_ident_info(token{}, "root_local").stack_idx == 8);
    assert(tc.next_frame_address().displacement() == 10);
    tc.exit_block();

    {
        toc padded_tc{backend, {}, 65536, false, false, false};
        padded_tc.add_type(token{}, i8);
        padded_tc.enter_block();
        tokenizer data_tokens{"dat marker: i8 = 1"};
        const stmt_def_dat data{padded_tc, data_tokens.next_token(),
                                data_tokens};

        assert(data.dat_size_bytes() == 1);
        const operand first_frame{padded_tc.next_frame_address()};
        assert(first_frame.base_register() == "rbp");
        assert(first_frame.displacement() ==
               static_cast<int32_t>(backend.data_alignment()));

        padded_tc.add_var(token{}, 0, {.name{"local"}, .type_ptr{&i16}}, false);
        assert(padded_tc.make_ident_info(token{}, "local").stack_idx ==
               first_frame.displacement());

        assert(padded_tc.next_frame_address().displacement() ==
               first_frame.displacement() + 2);

        padded_tc.enter_func("padded_frame", {}, {}, {}, false,
                             backend.frame_base_register());

        assert(padded_tc.next_frame_address().base_register() == "r12");
        assert(padded_tc.next_frame_address().displacement() == 0);
        assert(padded_tc.peak_frame_size_bytes() == 0);
        padded_tc.exit_func("padded_frame");
        assert(padded_tc.next_frame_address().displacement() ==
               first_frame.displacement() + 2);

        padded_tc.exit_block();
    }

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

    backend.reserve_variables_base();
    for (const bool reserve_frame : {true, false, true, false}) {
        if (reserve_frame) {
            backend.reserve_frame_base();
            check_ident(tc, "r12", i64);
            check_ident(tc, "r12b", i8);
        }

        std::vector<operand> pool;
        for (const std::string_view expected :
             {"r15", "r14", "r13", "r12", "r10", "r9", "r8"}) {
            if (reserve_frame and expected == backend.frame_base_register()) {
                continue;
            }
            pool.push_back(backend.alloc_scratch_register(token{}, 0, i64));
            assert(pool.back().base_register() == expected);
        }
        backend.free_scratch_registers(token{}, 0, pool);
        if (reserve_frame) {
            backend.release_frame_base();
            assert(backend.allocated_register_type("r12") == nullptr);
        }
    }
    backend.release_variables_base();

    const operand first{backend.alloc_scratch_register(token{}, 0, i64)};
    for (const std::string_view base : {"rbp", "r12"}) {
        output.str("");
        backend.call_function(0, "callee", operand::mem(base, {}, 1, 24, i64));
        backend.return_function(0);
        std::istringstream instructions{output.str()};
        std::string instruction;
        std::string argument;
        for (const std::string_view expected :
             {"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "r8", "r9",
              "r10", "r11", "r12", "r13", "r14", "r15"}) {
            instructions >> instruction >> argument;
            assert(instruction == "push");
            assert(argument == expected);
        }
        instructions >> instruction >> argument;
        assert(instruction == "lea");
        assert(argument == "r12,");
        std::getline(instructions, argument);
        assert(argument == std::format(" [{} + 24]", base));
        instructions >> instruction >> argument;
        assert(instruction == "call");
        assert(argument == "callee");
        for (const std::string_view expected :
             {"r15", "r14", "r13", "r12", "r11", "r10", "r9", "r8", "rbp",
              "rdi", "rsi", "rdx", "rcx", "rbx", "rax"}) {
            instructions >> instruction >> argument;
            assert(instruction == "pop");
            assert(argument == expected);
        }
        instructions >> instruction;
        assert(instruction == "ret");
        assert(not(instructions >> instruction));
        assert(backend.allocated_register_type(first.base_register()) == &i64);
    }

    output.str("");
    backend.check_frame_capacity(token{}, 0,
                                 operand::mem("r12", {}, 1, 24, i64),
                                 operand::imm("callee_size", i64), "overflow");

    assert(output.str().empty());
    backend.check_frame_capacity(
        token{}, 0, operand::mem("r12", {}, 1, 24, i64),
        operand::imm("callee_size", i64), "overflow", true);

    std::istringstream capacity_instructions{output.str()};
    std::string capacity_line;
    for (const std::string_view expected :
         {"lea r14, [r12 + 24]", "lea r13, [vars]", "cmp r14, r13",
          "jb overflow", "lea r13, [vars.end]", "cmp r14, r13", "ja overflow",
          "sub r13, r14", "mov r14, callee_size", "cmp r14, r13",
          "ja overflow"}) {
        do {
            assert(static_cast<bool>(
                std::getline(capacity_instructions, capacity_line)));

        } while (capacity_line.find(';') != std::string::npos or
                 capacity_line.find_first_not_of(' ') == std::string::npos);

        assert(capacity_line.substr(capacity_line.find_first_not_of(' ')) ==
               expected);
    }
    assert(backend.allocated_register_type("r14") == nullptr);
    assert(backend.allocated_register_type("r13") == nullptr);
    assert(backend.allocated_register_type(first.base_register()) == &i64);

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