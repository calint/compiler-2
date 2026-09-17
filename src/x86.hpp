#pragma once

#include <cassert>
#include <cstddef>
#include <format>
#include <functional>
#include <ostream>
#include <print>
#include <string>
#include <string_view>
#include <utility>

#include "utils.hpp"

class toc;
class token;
class statement;

class x86 final {
    static constexpr size_t size_qword{8};
    static constexpr size_t size_dword{4};
    static constexpr size_t size_word{2};
    static constexpr size_t size_byte{1};
    static constexpr std::string_view data_qword{"dq"};
    static constexpr std::string_view data_dword{"dd"};
    static constexpr std::string_view data_word{"dw"};
    static constexpr std::string_view data_byte{"db"};

  public:
    // the assembler output stream for the current compile pass; rebindable
    // via 'use_stream' so trial-compiles can target a scratch buffer while
    // keeping this same instance (and its register-allocation state)
    std::reference_wrapper<std::ostream> os;

    explicit x86(std::ostream& os_ref) : os{os_ref} {}

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

    static auto comment_source(const toc& tc, const statement& statement,
                               std::ostream& os, size_t indent) -> void;

    static auto comment_source(const toc& tc, const statement& statement,
                               std::ostream& os, size_t indent,
                               std::string_view dst, std::string_view op)
        -> void;

    static auto comment_start(const toc& tc, const token& source_location,
                              std::ostream& os, size_t indent) -> void;

    // member form: uses this instance's own stream instead of taking one
    auto comment_start(const toc& tc, const token& source_location,
                       const size_t indent) -> void {
        x86::comment_start(tc, source_location, os.get(), indent);
    }

    static auto comment_token(const toc& tc, const token& token,
                              std::ostream& os, size_t indent) -> void;

    template <typename... args_t>
    auto comment_line(const toc& tc, const token& source_location,
                      const size_t indent,
                      const std::format_string<args_t...> format,
                      args_t&&... args) -> void {
        comment_start(tc, source_location, indent);
        println(format, std::forward<args_t>(args)...);
    }

    template <typename... args_t>
    auto comment_line(const size_t indent,
                      const std::format_string<args_t...> format,
                      args_t&&... args) -> void {
        comment_indent(indent);
        println(format, std::forward<args_t>(args)...);
    }

    // static form: needed by the still-static 'comment_source' overloads,
    // which are used directly by 'toc' (not yet converted to hold an 'x86&')
    template <typename... args_t>
    static auto comment_line(std::ostream& os, const size_t indent,
                             const std::format_string<args_t...> format,
                             args_t&&... args) -> void {
        comment_indent(os, indent);
        std::println(os, format, std::forward<args_t>(args)...);
    }

    auto comment_indent(const size_t indent) -> void {
        comment_indent(os.get(), indent);
    }

    // static form: needed by the static 'comment_start' overload, which is
    // still used directly by 'toc' (not yet converted to hold an 'x86&')
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

    auto imul(toc& tc, const token& src_loc_tk, size_t indent,
            std::string_view dst_op, std::string_view src_op) -> void;

    auto mov(toc& tc, const token& src_loc_tk, size_t indent,
            std::string_view dst_op, std::string_view src_op) -> void;

    auto op(toc& tc, const token& src_loc_tk, size_t indent,
          std::string_view op, std::string_view dst_op,
          std::string_view src_op) -> void;

    auto cmp(toc& tc, const token& src_loc_tk, size_t indent,
            std::string_view dst_op, std::string_view src_op) -> void;

    auto copy(toc& tc, const token& src_loc_tk, size_t indent,
            std::string_view src, std::string_view dst, size_t bytes_count)
        -> void;

    auto zero(toc& tc, const token& src_loc_tk, size_t indent,
            std::string_view dst, size_t bytes_count) -> void;

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

  private:
    [[nodiscard]] static auto operand_size(const toc& tc,
                                           std::string_view operand) -> size_t;

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
