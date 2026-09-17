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
    static auto comment_line(const toc& tc, const token& source_location,
                             std::ostream& os, const size_t indent,
                             const std::format_string<args_t...> format,
                             args_t&&... args) -> void {
        comment_start(tc, source_location, os, indent);
        std::println(os, format, std::forward<args_t>(args)...);
    }

    template <typename... args_t>
    static auto comment_line(std::ostream& os, const size_t indent,
                             const std::format_string<args_t...> format,
                             args_t&&... args) -> void {
        comment_indent(os, indent);
        std::println(os, format, std::forward<args_t>(args)...);
    }

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
    static auto comment(std::ostream& os,
                        const std::format_string<args_t...> format,
                        args_t&&... args) -> void {
        std::print(os, "; ");
        std::println(os, format, std::forward<args_t>(args)...);
    }

    static auto emit_buffer(std::ostream& os, const std::string_view text)
        -> void {
        std::print(os, "{}", text);
    }

    template <typename... args_t>
    static auto asm_line([[maybe_unused]] toc& tc, std::ostream& os,
                         const size_t indent,
                         const std::format_string<args_t...> format,
                         args_t&&... args) -> void {

        for (size_t index{}; index < indent; ++index) {
            std::print(os, "    ");
        }
        std::println(os, format, std::forward<args_t>(args)...);
    }

    static auto imul(toc& tc, const token& src_loc_tk, std::ostream& os,
                     size_t indent, std::string_view dst_op,
                     std::string_view src_op) -> void;

    static auto mov(toc& tc, const token& src_loc_tk, std::ostream& os,
                    size_t indent, std::string_view dst_op,
                    std::string_view src_op) -> void;

    static auto op(toc& tc, const token& src_loc_tk, std::ostream& os,
                   size_t indent, std::string_view op, std::string_view dst_op,
                   std::string_view src_op) -> void;

    static auto cmp(toc& tc, const token& src_loc_tk, std::ostream& os,
                    size_t indent, std::string_view dst_op,
                    std::string_view src_op) -> void;

    static auto copy(toc& tc, const token& src_loc_tk, std::ostream& os,
                     size_t indent, std::string_view src, std::string_view dst,
                     size_t bytes_count) -> void;

    static auto zero(toc& tc, const token& src_loc_tk, std::ostream& os,
                     size_t indent, std::string_view dst, size_t bytes_count)
        -> void;

    static auto add(toc& tc, std::ostream& os, const size_t indent,
                    const std::string_view dst, const std::string_view src)
        -> void {
        asm_line(tc, os, indent, "add {}, {}", dst, src);
    }

    static auto cmp(toc& tc, std::ostream& os, const size_t indent,
                    const std::string_view dst, const std::string_view src)
        -> void {
        asm_line(tc, os, indent, "cmp {}, {}", dst, src);
    }

    static auto cmovs(toc& tc, std::ostream& os, const size_t indent,
                      const std::string_view dst, const std::string_view src)
        -> void {
        asm_line(tc, os, indent, "cmovs {}, {}", dst, src);
    }

    static auto div_reg_ext(toc& tc, std::ostream& os, const size_t indent,
                            const size_t operand_size) -> void {
        switch (operand_size) {
        case size_qword:
            asm_line(tc, os, indent, "cqo");
            return;
        case size_dword:
            asm_line(tc, os, indent, "cdq");
            return;
        case size_word:
            asm_line(tc, os, indent, "cwde");
            return;
        case size_byte:
            asm_line(tc, os, indent, "cbw");
            return;
        default:
            std::unreachable();
        }
    }

    static auto idiv(toc& tc, std::ostream& os, const size_t indent,
                     const std::string_view operand) -> void {
        asm_line(tc, os, indent, "idiv {}", operand);
    }

    static auto imul(toc& tc, std::ostream& os, const size_t indent,
                     const std::string_view dst, const std::string_view src)
        -> void {
        asm_line(tc, os, indent, "imul {}, {}", dst, src);
    }

    static auto inc(toc& tc, std::ostream& os, const size_t indent,
                    const std::string_view dst) -> void {
        asm_line(tc, os, indent, "inc {}", dst);
    }

    static auto jcc(toc& tc, std::ostream& os, const size_t indent,
                    const std::string_view comparison,
                    const std::string_view label) -> void {
        asm_line(tc, os, indent, "j{} {}", comparison, label);
    }

    static auto je(toc& tc, std::ostream& os, const size_t indent,
                   const std::string_view label) -> void {
        asm_line(tc, os, indent, "je {}", label);
    }

    auto jmp(toc& tc, const size_t indent, const std::string_view label)
        -> void {
        asm_line(tc, os.get(), indent, "jmp {}", label);
    }

    static auto jne(toc& tc, std::ostream& os, const size_t indent,
                    const std::string_view label) -> void {
        asm_line(tc, os, indent, "jne {}", label);
    }

    auto label(toc& tc, const size_t indent, const std::string_view label)
        -> void {
        asm_line(tc, os.get(), indent, "{}:", label);
    }

    static auto lea(toc& tc, std::ostream& os, const size_t indent,
                    const std::string_view dst, const std::string_view operand)
        -> void {
        asm_line(tc, os, indent, "lea {}, [{}]", dst, operand);
    }

    static auto neg(toc& tc, std::ostream& os, const size_t indent,
                    const std::string_view operand) -> void {
        asm_line(tc, os, indent, "neg {}", operand);
    }

    static auto not_op(toc& tc, std::ostream& os, const size_t indent,
                       const std::string_view operand) -> void {
        asm_line(tc, os, indent, "not {}", operand);
    }

    static auto rep_movs(toc& tc, std::ostream& os, const size_t indent,
                         const char size) -> void {
        asm_line(tc, os, indent, "rep movs{}", size);
    }

    static auto rep_stos(toc& tc, std::ostream& os, const size_t indent,
                         const char size) -> void {
        asm_line(tc, os, indent, "rep stos{}", size);
    }

    static auto repe_cmps(toc& tc, std::ostream& os, const size_t indent,
                          const char size) -> void {
        asm_line(tc, os, indent, "repe cmps{}", size);
    }

    static auto setcc(toc& tc, std::ostream& os, const size_t indent,
                      const std::string_view comparison,
                      const std::string_view operand) -> void {
        asm_line(tc, os, indent, "set{} {}", comparison, operand);
    }

    static auto shl(toc& tc, std::ostream& os, const size_t indent,
                    const std::string_view dst, const std::string_view src)
        -> void {
        asm_line(tc, os, indent, "shl {}, {}", dst, src);
    }

    auto syscall(toc& tc, const size_t indent) -> void {
        asm_line(tc, os.get(), indent, "syscall");
    }

    static auto dat_begin(std::ostream& os, const size_t size) -> void {
        switch (size) {
        case size_qword:
            std::print(os, "dq ");
            return;
        case size_dword:
            std::print(os, "dd ");
            return;
        case size_word:
            std::print(os, "dw ");
            return;
        case size_byte:
            std::print(os, "db ");
            return;
        default:
            std::unreachable();
        }
    }

    static auto dat_separator(std::ostream& os) -> void {
        std::print(os, ", ");
    }

    static auto dat_end(std::ostream& os) -> void { std::println(os); }

    static auto dat_value(std::ostream& os, const std::string_view value)
        -> void {
        std::print(os, "{}", value);
    }

    static auto str_begin(std::ostream& os) -> void { std::print(os, "db `"); }

    static auto str_end(std::ostream& os) -> void { std::println(os, "`"); }

    static auto str_value(std::ostream& os, const std::string_view value)
        -> void {
        size_t position{};
        while (position < value.size()) {
            const size_t next{value.find('`', position)};
            if (next == std::string_view::npos) {
                dat_value(os, value.substr(position));
                return;
            }
            std::print(os, "{}\\`", value.substr(position, next - position));
            position = next + 1;
        }
    }

    static auto test(toc& tc, std::ostream& os, const size_t indent,
                     const std::string_view dst, const std::string_view src)
        -> void {
        asm_line(tc, os, indent, "test {}, {}", dst, src);
    }

    static auto times(std::ostream& os, const size_t count,
                      const std::string_view directive,
                      const std::string_view value) -> void {
        std::println(os, "times {} {} {}", count, directive, value);
    }

    static auto xor_op(toc& tc, std::ostream& os, const size_t indent,
                       const std::string_view dst, const std::string_view src)
        -> void {
        asm_line(tc, os, indent, "xor {}, {}", dst, src);
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
