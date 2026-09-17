#pragma once

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

    struct allocated_register {
        std::string name;
        std::string source_location;
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

    // cached data that would otherwise require a 'toc&' per call; set once
    // via the setters below before compiling starts
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
    std::reference_wrapper<std::ostream> os;

    explicit x86(std::ostream& os_ref) : os{os_ref} {}

    // called once by 'program' before compiling starts; provides the pieces
    // of 'toc' state 'x86' needs without holding a 'toc&'
    auto set_source(const std::string_view source) -> void { source_ = source; }

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

    auto println() -> void { std::println(os.get()); }

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

    auto comment_source(const token& source_location, size_t indent,
                       std::string_view text) -> void;

    static auto comment_start(std::string_view source,
                              const token& source_location, std::ostream& os,
                              size_t indent) -> void;

    // member form: uses this instance's own print/os/source instead of
    // taking them
    auto comment_start(const token& source_location, size_t indent) -> void;

    auto comment_token(const token& token, size_t indent) -> void;

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
        comment_indent(os.get(), indent);
    }

    // static form: needed by the static 'comment_start' overload, which is
    // used directly by 'toc' (which has no 'x86&' available in its own
    // 'add_const'/'add_var' methods)
    static auto comment_indent(std::ostream& os, const size_t indent) -> void {
        std::print(os, ";");
        if (indent != 0) {
            std::print(os, "   ");
        }
        for (size_t index{1}; index < indent; ++index) {
            std::print(os, "    ");
        }
    }

    template <typename... args_t>
    auto comment(const std::format_string<args_t...> format, args_t&&... args)
        -> void {
        print("; ");
        println(format, std::forward<args_t>(args)...);
    }

    auto emit_buffer(const std::string_view text) -> void {
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

    auto imul(const token& src_loc_tk, size_t indent, std::string_view dst_op,
            std::string_view src_op) -> void;

    auto alloc_named_register_or_throw(const token& src_loc_tk, size_t indnt,
                                       std::string_view reg,
                                       const type& type_ref) -> void;

    [[nodiscard]] auto alloc_scratch_register(const token& src_loc_tk,
                                              size_t indnt,
                                              const type& type_ref)
        -> std::string;

    auto free_named_register(const token& src_loc_tk, size_t indnt,
                             std::string_view reg) -> void;

    auto free_scratch_register(const token& src_loc_tk, size_t indnt,
                               std::string_view reg) -> void;

    // returns the type a currently allocated register holds, falling back to
    // a builtin type inferred from the register's width if it isn't (or is
    // no longer) allocated
    [[nodiscard]] auto get_allocated_register_type(std::string_view reg) const
        -> const type&;

    // asserts register pools are balanced and prints usage stats; called
    // once at the end of the compile pass
    auto finish() -> void;

    auto mov(const token& src_loc_tk, size_t indent, std::string_view dst_op,
            std::string_view src_op) -> void;

    auto op(const token& src_loc_tk, size_t indent, std::string_view op,
          std::string_view dst_op, std::string_view src_op) -> void;

    auto cmp(const token& src_loc_tk, size_t indent, std::string_view dst_op,
            std::string_view src_op) -> void;

    auto copy(const token& src_loc_tk, size_t indent, std::string_view src,
            std::string_view dst, size_t bytes_count)
        -> void;

    auto zero(const token& src_loc_tk, size_t indent, std::string_view dst,
            size_t bytes_count) -> void;

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

    auto div_reg_ext(const size_t indent, const size_t operand_size)
        -> void {
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

    auto idiv(const size_t indent, const std::string_view operand)
        -> void {
        asm_line(indent, "idiv {}", operand);
    }


    auto inc(const size_t indent, const std::string_view dst)
        -> void {
        asm_line(indent, "inc {}", dst);
    }

    auto jcc(const size_t indent, const std::string_view comparison,
            const std::string_view label) -> void {
        asm_line(indent, "j{} {}", comparison, label);
    }

    auto jmp(const size_t indent, const std::string_view label)
        -> void {
        asm_line(indent, "jmp {}", label);
    }

    auto jne(const size_t indent, const std::string_view label)
        -> void {
        asm_line(indent, "jne {}", label);
    }

    auto label(const size_t indent, const std::string_view label)
        -> void {
        asm_line(indent, "{}:", label);
    }

    auto lea(const size_t indent, const std::string_view dst,
            const std::string_view operand) -> void {
        asm_line(indent, "lea {}, [{}]", dst, operand);
    }

    auto neg(const size_t indent, const std::string_view operand)
        -> void {
        asm_line(indent, "neg {}", operand);
    }

    auto not_op(const size_t indent, const std::string_view operand)
        -> void {
        asm_line(indent, "not {}", operand);
    }

    auto rep_movs(const size_t indent, const char size_suffix)
        -> void {
        asm_line(indent, "rep movs{}", size_suffix);
    }

    auto rep_stos(const size_t indent, const char size_suffix)
        -> void {
        asm_line(indent, "rep stos{}", size_suffix);
    }

    auto repe_cmps(const size_t indent, const char size_suffix)
        -> void {
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

    auto syscall(const size_t indent) -> void {
        asm_line(indent, "syscall");
    }

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

    auto dat_end() -> void { println(); }

    auto dat_value(const std::string_view value) -> void {
        print("{}", value);
    }

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
              const std::string_view value) -> void {
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

  private:
    [[nodiscard]] auto operand_size(std::string_view operand) const -> size_t;

    // human-readable "line:col" for a token, using the cached source text
    [[nodiscard]] auto source_location_hr(const token& src_loc_tk) const
        -> std::string;

    // returns the cached builtin type (i64/i32/i16/i8) matching 'size'
    [[nodiscard]] auto get_builtin_type_for_size(size_t size) const
        -> const type&;

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
