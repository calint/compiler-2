#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <format>
#include <functional>
#include <ostream>
#include <print>
#include <regex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "token.hpp"
#include "type.hpp"
#include "utils.hpp"

class token;
class type;

class x86 final {
    static constexpr size_t size_qword{8};
    static constexpr size_t size_dword{4};
    static constexpr size_t size_word{2};
    static constexpr size_t size_byte{1};

    static constexpr std::string_view data_qword{"dq"};
    static constexpr std::string_view data_dword{"dd"};
    static constexpr std::string_view data_word{"dw"};
    static constexpr std::string_view data_byte{"db"};

    static constexpr size_t threshold_for_rep_stos{32};
    static constexpr size_t threshold_for_rep_movs{16};

    struct allocated_register {
        std::string source_location;
        std::string name;
        const type* type_ptr;
    };

    std::vector<std::string> all_registers_{
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
        "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"};
    size_t all_registers_initial_size_{all_registers_.size()};
    std::vector<std::string> named_registers_{"rax", "rbx", "rcx", "rdx",
                                              "rsi", "rdi", "rbp"};
    size_t named_registers_initial_size_{named_registers_.size()};
    std::vector<std::string> scratch_registers_{"r8",  "r9",  "r10", "r11",
                                                "r12", "r13", "r14", "r15"};
    size_t scratch_registers_initial_size_{scratch_registers_.size()};
    std::vector<allocated_register> allocated_registers_;
    size_t usage_max_scratch_regs_{};

    std::regex regex_nasm_number_register_{R"(r(\d+))"};

    std::string_view source_;

    const type* default_type_{};
    const type* type_bool_{};
    const type* type_i64_{};
    const type* type_i32_{};
    const type* type_i16_{};
    const type* type_i8_{};

  public:
    // the assembler output stream for the current compile pass; rebindable
    // via 'use_stream' so trial-compiles can target a scratch buffer while
    // keeping this same instance (and its register-allocation state)
    // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    std::reference_wrapper<std::ostream> os;

    explicit x86(std::ostream& os_ref, std::string_view source)
        : source_{source}, os{os_ref} {}

    auto set_type_default(const type& tpe) -> void { default_type_ = &tpe; }

    auto set_builtin_types(const type& t_i64, const type& t_i32,
                           const type& t_i16, const type& t_i8,
                           const type& t_bool) -> void {
        type_i64_ = &t_i64;
        type_i32_ = &t_i32;
        type_i16_ = &t_i16;
        type_i8_ = &t_i8;
        type_bool_ = &t_bool;
    }

    // redirects output to 'new_stream', returning the previously used stream
    // so the caller can restore it later
    auto use_stream(std::ostream& new_stream) -> std::ostream& {
        std::ostream& prev{os.get()};
        os = new_stream;
        return prev;
    }

    // member form: uses this instance's own print/os/source instead of
    // taking them
    auto comment_start(const token& source_location, const size_t indent)
        -> void {
        const auto [line, column]{utils::line_and_col_num_for_char_index(
            source_location.at_line(), source_location.start_index(), source_)};
        comment_indent(indent);
        print("[{}:{}] ", line, column);
    }

    auto comment_token(const token& token, const size_t indent) -> void {
        comment_start(token, indent);
        println("{}", token.text());
    }

    template <typename... args_t>
    auto comment_line(const token& source_location, const size_t indent,
                      const std::format_string<args_t...> format,
                      args_t&&... args) -> void {
        comment_start(source_location, indent);
        println(format, std::forward<args_t>(args)...);
    }

    template <typename... args_t>
    auto comment_line(const size_t indent,
                      const std::format_string<args_t...> format,
                      args_t&&... args) -> void {
        comment_indent(indent);
        println(format, std::forward<args_t>(args)...);
    }

    auto comment_indent(const size_t indent) -> void {
        print(";");

        if (indent != 0) {
            print("   ");
        }
        for (size_t index{1}; index < indent; ++index) {
            print("    ");
        }
    }

    template <typename... args_t>
    auto comment(const std::format_string<args_t...> format, args_t&&... args)
        -> void {
        print("; ");
        println(format, std::forward<args_t>(args)...);
    }

    auto emit_buffer(const std::string_view text) const -> void {
        std::print(os.get(), "{}", text);
    }

    template <typename... args_t>
    auto asm_line(const size_t indent,
                  const std::format_string<args_t...> format, args_t&&... args)
        -> void {

        for (size_t index{}; index < indent; ++index) {
            print("    ");
        }
        println(format, std::forward<args_t>(args)...);
    }

    auto imul(const token& src_loc_tk, const size_t indent,
              const std::string_view dst_op, const std::string_view src_op)
        -> void {
        op(src_loc_tk, indent, "imul", dst_op, src_op);
    }

    auto alloc_named_register(const token& src_loc_tk, const size_t indnt,
                              const std::string_view reg, const type& type_ref)
        -> void {

        comment_start(src_loc_tk, indnt);
        println("allocate named register '{}'", reg);

        auto reg_iter{std::ranges::find(named_registers_, reg)};
        if (reg_iter == named_registers_.end()) {
            // not found
            std::string loc;
            const auto allocated{std::ranges::find(allocated_registers_, reg,
                                                   &allocated_register::name)};
            if (allocated != allocated_registers_.end()) {
                loc = allocated->source_location;
            }
            throw compiler_exception{
                src_loc_tk, std::format("cannot allocate register '{}' because "
                                        "it was allocated at {}",
                                        reg, loc)};
        }

        allocated_registers_.emplace_back(source_location_hr(src_loc_tk),
                                          std::move(*reg_iter), &type_ref);

        named_registers_.erase(reg_iter);
    }

    [[nodiscard]] auto alloc_scratch_register(const token& src_loc_tk,
                                              const size_t indnt,
                                              const type& type_ref)
        -> std::string {

        if (scratch_registers_.empty()) {
            throw compiler_exception{src_loc_tk,
                                     "out of scratch registers. try to reduce "
                                     "expression complexity"};
        }

        std::string reg{std::move(scratch_registers_.back())};
        scratch_registers_.pop_back();

        comment_start(src_loc_tk, indnt);
        println("allocate scratch register -> {}", reg);

        const size_t n{scratch_registers_initial_size_ -
                       scratch_registers_.size()};
        usage_max_scratch_regs_ = std::max(n, usage_max_scratch_regs_);

        allocated_registers_.emplace_back(source_location_hr(src_loc_tk),
                                          std::move(reg), &type_ref);

        return allocated_registers_.back().name;
    }

    auto free_named_register(const token& src_loc_tk, const size_t indnt,
                             const std::string_view reg) -> void {

        comment_start(src_loc_tk, indnt);
        println("free named register '{}'", reg);

        assert(allocated_registers_.back().name == reg);

        named_registers_.emplace_back(
            std::move(allocated_registers_.back().name));
        allocated_registers_.pop_back();
    }

    auto free_scratch_register(const token& src_loc_tk, const size_t indnt,
                               const std::string_view reg) -> void {

        comment_start(src_loc_tk, indnt);
        println("free scratch register '{}'", reg);

        assert(allocated_registers_.back().name == reg);

        scratch_registers_.emplace_back(
            std::move(allocated_registers_.back().name));
        allocated_registers_.pop_back();
    }

    // returns the type a currently allocated register holds, falling back to
    // a builtin type inferred from the register's width if it isn't (or is
    // no longer) allocated
    [[nodiscard]] auto
    get_allocated_register_type(const std::string_view reg) const
        -> const type& {

        for (const allocated_register& allocated : allocated_registers_) {
            if (reg == allocated.name) {
                return *allocated.type_ptr;
            }
        }

        return get_builtin_type_for_size(utils::register_size(reg));
    }

    // asserts register pools are balanced and prints usage stats; called
    // once at the end of the compile pass
    auto finish() -> void {
        println("\n; max scratch registers in use: {}",
                usage_max_scratch_regs_);
        assert(all_registers_.size() == all_registers_initial_size_);
        assert(allocated_registers_.empty());
        assert(named_registers_.size() == named_registers_initial_size_);
        assert(scratch_registers_.size() == scratch_registers_initial_size_);
        usage_max_scratch_regs_ = 0;
    }

    auto mov(const token& src_loc_tk, const size_t indent,
             const std::string_view dst_op, const std::string_view src_op)
        -> void {
        op(src_loc_tk, indent, "mov", dst_op, src_op);
    }

    auto op(const token& src_loc_tk, const size_t indent,
            const std::string_view op, const std::string_view dst_op,
            const std::string_view src_op) -> void {
        if (op == "mov" and dst_op == src_op) {
            return;
        }

        const size_t dst_size{operand_size(dst_op)};
        const size_t src_size{operand_size(src_op)};

        if (dst_size == src_size) {
            if (is_memory_operand(dst_op) and is_memory_operand(src_op)) {
                const std::string reg{
                    alloc_scratch_register(src_loc_tk, indent, *default_type_)};
                const std::string reg_sized{
                    get_sized_register_operand(reg, dst_size)};
                asm_line(indent, "mov {}, {}", reg_sized, src_op);
                asm_line(indent, "{} {}, {}", op, dst_op, reg_sized);
                free_scratch_register(src_loc_tk, indent, reg);
                return;
            }
            asm_line(indent, "{} {}, {}", op, dst_op, src_op);
            return;
        }

        if (dst_size > src_size) {
            if (is_memory_operand(dst_op) and is_memory_operand(src_op)) {
                const std::string reg{
                    alloc_scratch_register(src_loc_tk, indent, *default_type_)};
                const std::string reg_sized{
                    get_sized_register_operand(reg, dst_size)};
                asm_line(indent, "movsx {}, {}", reg_sized, src_op);
                asm_line(indent, "{} {}, {}", op, dst_op, reg_sized);
                free_scratch_register(src_loc_tk, indent, reg);
                return;
            }
            if (op == "mov") {
                asm_line(indent, "movsx {}, {}", dst_op, src_op);
                return;
            }
            if (op == "sal" or op == "sar") {
                asm_line(indent, "{} {}, {}", op, dst_op, src_op);
                return;
            }
            const std::string reg_sx{
                alloc_scratch_register(src_loc_tk, indent, *default_type_)};
            asm_line(indent, "movsx {}, {}", reg_sx, src_op);
            asm_line(indent, "{} {}, {}", op, dst_op, reg_sx);
            free_scratch_register(src_loc_tk, indent, reg_sx);
            return;
        }

        if (is_memory_operand(dst_op) and is_memory_operand(src_op)) {
            const std::string reg{
                alloc_scratch_register(src_loc_tk, indent, *default_type_)};
            const std::string reg_sized{
                get_sized_register_operand(reg, dst_size)};
            asm_line(indent, "mov {}, {}", reg_sized,
                     sized_memory_operand(src_op, dst_size));
            asm_line(indent, "{} {}, {}", op, dst_op, reg_sized);
            free_scratch_register(src_loc_tk, indent, reg);
            return;
        }

        const bool dst_is_reg{is_register_operand(dst_op)};
        const bool src_is_reg{is_register_operand(src_op)};
        if (dst_is_reg and src_is_reg) {
            asm_line(indent, "{} {}, {}", op, dst_op,
                     get_sized_register_operand(src_op, dst_size));
            return;
        }
        if (dst_is_reg) {
            asm_line(indent, "{} {}, {}", op, dst_op,
                     is_memory_operand(src_op)
                         ? sized_memory_operand(src_op, dst_size)
                         : src_op);
            return;
        }
        if (src_is_reg) {
            asm_line(indent, "{} {}, {}", op, dst_op,
                     get_sized_register_operand(src_op, dst_size));
            return;
        }
        asm_line(indent, "{} {}, {}", op, dst_op, src_op);
    }

    auto cmp(const token& src_loc_tk, const size_t indent,
             const std::string_view dst_op, const std::string_view src_op)
        -> void {
        op(src_loc_tk, indent, "cmp", dst_op, src_op);
    }

    auto copy(const token& src_loc_tk, const size_t indent,
              const std::string_view src, const std::string_view dst,
              const size_t bytes_count) -> void {
        if (bytes_count > threshold_for_rep_movs) {
            alloc_named_register(src_loc_tk, indent, "rsi", *default_type_);
            alloc_named_register(src_loc_tk, indent, "rdi", *default_type_);
            alloc_named_register(src_loc_tk, indent, "rcx", *default_type_);
            lea(indent, "rsi", src);
            lea(indent, "rdi", dst);
            mov(src_loc_tk, indent, "rcx", std::format("{}", bytes_count));
            rep_movs(indent, 'b');
            free_named_register(src_loc_tk, indent, "rcx");
            free_named_register(src_loc_tk, indent, "rdi");
            free_named_register(src_loc_tk, indent, "rsi");
            return;
        }

        comment_start(src_loc_tk, indent);
        std::println(os.get(), "size <= {} B, use mov", threshold_for_rep_movs);
        alloc_named_register(src_loc_tk, indent, "rax", *default_type_);
        size_t rest{bytes_count};
        const size_t qword_movs{rest / operand::size_qword};
        operand src_operand{src};
        operand dst_operand{dst};
        for (size_t index{}; index < qword_movs; ++index) {
            mov(src_loc_tk, indent, "rax",
                src_operand.str(operand::size_qword));
            mov(src_loc_tk, indent, dst_operand.str(operand::size_qword),
                "rax");
            src_operand.displacement += operand::size_qword;
            dst_operand.displacement += operand::size_qword;
            rest -= operand::size_qword;
        }
        if ((rest / operand::size_dword) != 0) {
            mov(src_loc_tk, indent, "eax",
                src_operand.str(operand::size_dword));
            mov(src_loc_tk, indent, dst_operand.str(operand::size_dword),
                "eax");
            src_operand.displacement += operand::size_dword;
            dst_operand.displacement += operand::size_dword;
            rest -= operand::size_dword;
        }
        if ((rest / operand::size_word) != 0) {
            mov(src_loc_tk, indent, "ax", src_operand.str(operand::size_word));
            mov(src_loc_tk, indent, dst_operand.str(operand::size_word), "ax");
            src_operand.displacement += operand::size_word;
            dst_operand.displacement += operand::size_word;
            rest -= operand::size_word;
        }
        if (rest != 0) {
            mov(src_loc_tk, indent, "al", src_operand.str(operand::size_byte));
            mov(src_loc_tk, indent, dst_operand.str(operand::size_byte), "al");
        }
        free_named_register(src_loc_tk, indent, "rax");
    }

    auto zero(const token& src_loc_tk, const size_t indent,
              const std::string_view dst, const size_t bytes_count) -> void {
        if (bytes_count > threshold_for_rep_stos) {
            alloc_named_register(src_loc_tk, indent, "rax", *default_type_);
            alloc_named_register(src_loc_tk, indent, "rdi", *default_type_);
            alloc_named_register(src_loc_tk, indent, "rcx", *default_type_);
            xor_op(indent, "al", "al");
            lea(indent, "rdi", dst);
            mov(src_loc_tk, indent, "rcx", std::format("{}", bytes_count));
            rep_stos(indent, 'b');
            free_named_register(src_loc_tk, indent, "rcx");
            free_named_register(src_loc_tk, indent, "rdi");
            free_named_register(src_loc_tk, indent, "rax");
            return;
        }

        comment_start(src_loc_tk, indent);
        std::println(os.get(), "size <= {} B, use mov", threshold_for_rep_stos);
        size_t rest{bytes_count};
        const size_t qword_movs{rest / operand::size_qword};
        operand dst_operand{dst};
        for (size_t index{}; index < qword_movs; ++index) {
            mov(src_loc_tk, indent, dst_operand.str(operand::size_qword), "0");
            dst_operand.displacement += operand::size_qword;
            rest -= operand::size_qword;
        }
        if ((rest / operand::size_dword) != 0) {
            mov(src_loc_tk, indent, dst_operand.str(operand::size_dword), "0");
            dst_operand.displacement += operand::size_dword;
            rest -= operand::size_dword;
        }
        if ((rest / operand::size_word) != 0) {
            mov(src_loc_tk, indent, dst_operand.str(operand::size_word), "0");
            dst_operand.displacement += operand::size_word;
            rest -= operand::size_word;
        }
        if (rest != 0) {
            mov(src_loc_tk, indent, dst_operand.str(operand::size_byte), "0");
        }
    }

    auto add(const size_t indent, const std::string_view dst,
             const std::string_view src) -> void {
        asm_line(indent, "add {}, {}", dst, src);
    }

    auto cmp(const size_t indent, const std::string_view dst,
             const std::string_view src) -> void {
        asm_line(indent, "cmp {}, {}", dst, src);
    }

    auto cmovs(const size_t indent, const std::string_view dst,
               const std::string_view src) -> void {
        asm_line(indent, "cmovs {}, {}", dst, src);
    }

    auto div_reg_ext(const size_t indent, const size_t operand_size) -> void {
        switch (operand_size) {
        case size_qword:
            asm_line(indent, "cqo");
            return;
        case size_dword:
            asm_line(indent, "cdq");
            return;
        case size_word:
            asm_line(indent, "cwde");
            return;
        case size_byte:
            asm_line(indent, "cbw");
            return;
        default:
            std::unreachable();
        }
    }

    auto idiv(const size_t indent, const std::string_view operand) -> void {
        asm_line(indent, "idiv {}", operand);
    }

    auto inc(const size_t indent, const std::string_view dst) -> void {
        asm_line(indent, "inc {}", dst);
    }

    auto jcc(const size_t indent, const std::string_view comparison,
             const std::string_view label) -> void {
        asm_line(indent, "j{} {}", comparison, label);
    }

    auto jmp(const size_t indent, const std::string_view label) -> void {
        asm_line(indent, "jmp {}", label);
    }

    auto jne(const size_t indent, const std::string_view label) -> void {
        asm_line(indent, "jne {}", label);
    }

    auto label(const size_t indent, const std::string_view label) -> void {
        asm_line(indent, "{}:", label);
    }

    auto lea(const size_t indent, const std::string_view dst,
             const std::string_view operand) -> void {
        asm_line(indent, "lea {}, [{}]", dst, operand);
    }

    auto neg(const size_t indent, const std::string_view operand) -> void {
        asm_line(indent, "neg {}", operand);
    }

    auto not_op(const size_t indent, const std::string_view operand) -> void {
        asm_line(indent, "not {}", operand);
    }

    auto rep_movs(const size_t indent, const char size_suffix) -> void {
        asm_line(indent, "rep movs{}", size_suffix);
    }

    auto rep_stos(const size_t indent, const char size_suffix) -> void {
        asm_line(indent, "rep stos{}", size_suffix);
    }

    auto repe_cmps(const size_t indent, const char size_suffix) -> void {
        asm_line(indent, "repe cmps{}", size_suffix);
    }

    auto setcc(const size_t indent, const std::string_view comparison,
               const std::string_view operand) -> void {
        asm_line(indent, "set{} {}", comparison, operand);
    }

    auto shl(const size_t indent, const std::string_view dst,
             const std::string_view src) -> void {
        asm_line(indent, "shl {}, {}", dst, src);
    }

    auto syscall(const size_t indent) -> void { asm_line(indent, "syscall"); }

    auto dat_begin(const size_t size) -> void {
        switch (size) {
        case size_qword:
            print("dq ");
            return;
        case size_dword:
            print("dd ");
            return;
        case size_word:
            print("dw ");
            return;
        case size_byte:
            print("db ");
            return;
        default:
            std::unreachable();
        }
    }

    auto dat_separator() -> void { print(", "); }

    auto dat_end() const -> void { println(); }

    auto dat_value(const std::string_view value) -> void { print("{}", value); }

    auto str_begin() -> void { print("db `"); }

    auto str_end() -> void { println("`"); }

    auto str_value(const std::string_view value) -> void {
        size_t position{};
        while (position < value.size()) {
            const size_t next{value.find('`', position)};
            if (next == std::string_view::npos) {
                dat_value(value.substr(position));
                return;
            }
            print("{}\\`", value.substr(position, next - position));
            position = next + 1;
        }
    }

    auto test(const size_t indent, const std::string_view dst,
              const std::string_view src) -> void {
        asm_line(indent, "test {}, {}", dst, src);
    }

    auto times(const size_t count, const std::string_view directive,
               const std::string_view value) const -> void {
        std::println(os.get(), "times {} {} {}", count, directive, value);
    }

    auto xor_op(const size_t indent, const std::string_view dst,
                const std::string_view src) -> void {
        asm_line(indent, "xor {}, {}", dst, src);
    }

    [[nodiscard]] auto
    get_sized_register_operand(const std::string_view operand,
                               const size_t size) -> std::string {

        //? sort of ugly
        if (operand == "rax") {
            switch (size) {
            case size_qword:
                return "rax";
            case size_dword:
                return "eax";
            case size_word:
                return "ax";
            case size_byte:
                return "al";
            default:
                std::unreachable();
            }
        }
        if (operand == "rbx") {
            switch (size) {
            case size_qword:
                return "rbx";
            case size_dword:
                return "ebx";
            case size_word:
                return "bx";
            case size_byte:
                return "bl";
            default:
                std::unreachable();
            }
        }
        if (operand == "rcx") {
            switch (size) {
            case size_qword:
                return "rcx";
            case size_dword:
                return "ecx";
            case size_word:
                return "cx";
            case size_byte:
                return "cl";
            default:
                std::unreachable();
            }
        }
        if (operand == "rdx") {
            switch (size) {
            case size_qword:
                return "rdx";
            case size_dword:
                return "edx";
            case size_word:
                return "dx";
            case size_byte:
                return "dl";
            default:
                std::unreachable();
            }
        }
        if (operand == "rbp") {
            switch (size) {
            case size_qword:
                return "rbp";
            case size_dword:
                return "ebp";
            case size_word:
                return "bp";
            case size_byte:
                return "bpl";
            default:
                std::unreachable();
            }
        }
        if (operand == "rsi") {
            switch (size) {
            case size_qword:
                return "rsi";
            case size_dword:
                return "esi";
            case size_word:
                return "si";
            case size_byte:
                return "sil";
            default:
                std::unreachable();
            }
        }
        if (operand == "rdi") {
            switch (size) {
            case size_qword:
                return "rdi";
            case size_dword:
                return "edi";
            case size_word:
                return "di";
            case size_byte:
                return "dil";
            default:
                std::unreachable();
            }
        }
        if (operand == "rsp") {
            switch (size) {
            case size_qword:
                return "rsp";
            case size_dword:
                return "esp";
            case size_word:
                return "sp";
            case size_byte:
                return "spl";
            default:
                std::unreachable();
            }
        }

        std::smatch match;
        const std::string operand_str{operand};
        if (not std::regex_search(operand_str, match,
                                  regex_nasm_number_register_)) {
            std::unreachable();
        }
        const std::string rnbr{match[1]};
        switch (size) {
        case size_qword:
            return std::format("r{}", rnbr);
        case size_dword:
            return std::format("r{}d", rnbr);
        case size_word:
            return std::format("r{}w", rnbr);
        case size_byte:
            return std::format("r{}b", rnbr);
        default:
            std::unreachable();
        }
    }

    [[nodiscard]] static auto get_data_def(const size_t size)
        -> std::string_view {
        switch (size) {
        case size_qword:
            return data_qword;
        case size_dword:
            return data_dword;
        case size_word:
            return data_word;
        case size_byte:
            return data_byte;
        default:
            std::unreachable();
        }
    }

    auto comment_source(const token& source_location, const size_t indent,
                        const std::string_view text) -> void {
        comment_start(source_location, indent);
        println("{}", text);
    }

    auto comment_start(const std::string_view source,
                       const token& source_location, const size_t indent)
        -> void {

        const auto [line, column]{utils::line_and_col_num_for_char_index(
            source_location.at_line(), source_location.start_index(), source)};

        comment_indent(indent);
        std::print(os, "[{}:{}] ", line, column);
    }

    template <typename... args_t>
    auto print(const std::format_string<args_t...> format, args_t&&... args)
        -> void {
        std::print(os.get(), format, std::forward<args_t>(args)...);
    }

    template <typename... args_t>
    auto println(const std::format_string<args_t...> format, args_t&&... args)
        -> void {
        std::println(os.get(), format, std::forward<args_t>(args)...);
    }

    auto println() const -> void { std::println(os.get()); }

  private:
    [[nodiscard]] auto operand_size(const std::string_view operand) const
        -> size_t {
        if (operand.starts_with("qword")) {
            return size_qword;
        }
        if (operand.starts_with("dword")) {
            return size_dword;
        }
        if (operand.starts_with("word")) {
            return size_word;
        }
        if (operand.starts_with("byte")) {
            return size_byte;
        }
        if (const size_t size{utils::register_size(operand)}) {
            return size;
        }
        return default_type_->size();
    }

    // human-readable "line:col" for a token, using the cached source text
    [[nodiscard]] auto source_location_hr(const token& src_loc_tk) const
        -> std::string {
        const auto [line, col]{utils::line_and_col_num_for_char_index(
            src_loc_tk.at_line(), src_loc_tk.start_index(), source_)};
        return std::format("{}:{}", line, col);
    }

    // returns the cached builtin type (i64/i32/i16/i8) matching 'size'
    [[nodiscard]] auto get_builtin_type_for_size(const size_t size) const
        -> const type& {
        switch (size) {
        case size_qword:
            return *type_i64_;
        case size_dword:
            return *type_i32_;
        case size_word:
            return *type_i16_;
        case size_byte:
            return *type_i8_;
        default:
            std::unreachable();
        }
    }

    [[nodiscard]] static auto is_memory_operand(const std::string_view operand)
        -> bool {
        return operand.contains('[');
    }

    [[nodiscard]] static auto is_register_operand(std::string_view operand)
        -> bool {
        return utils::register_size(operand) != 0;
    }

    [[nodiscard]] static auto
    sized_memory_operand(const std::string_view operand, const size_t size)
        -> std::string {
        const size_t bracket{operand.find('[')};
        assert(bracket != std::string_view::npos);
        return std::format("{} {}", utils::get_size_specifier(size),
                           operand.substr(bracket));
    }
};
