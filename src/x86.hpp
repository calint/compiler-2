#pragma once

#include <cassert>
#include <cstddef>
#include <format>
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

  public:
    static auto comment_source(const toc& tc, const statement& statement,
                               std::ostream& os, size_t indent) -> void;

    static auto comment_source(const toc& tc, const statement& statement,
                               std::ostream& os, size_t indent,
                               std::string_view dst, std::string_view op)
        -> void;

    static auto comment_start(const toc& tc, const token& source_location,
                              std::ostream& os, size_t indent) -> void;

    static auto comment_token(const toc& tc, const token& token,
                              std::ostream& os, size_t indent) -> void;

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

    static auto jmp(toc& tc, std::ostream& os, const size_t indent,
                    const std::string_view label) -> void {
        asm_line(tc, os, indent, "jmp {}", label);
    }

    static auto jne(toc& tc, std::ostream& os, const size_t indent,
                    const std::string_view label) -> void {
        asm_line(tc, os, indent, "jne {}", label);
    }

    static auto label(toc& tc, std::ostream& os, const size_t indent,
                      const std::string_view label) -> void {
        asm_line(tc, os, indent, "{}:", label);
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

    static auto syscall(toc& tc, std::ostream& os, const size_t indent)
        -> void {
        asm_line(tc, os, indent, "syscall");
    }

    static auto test(toc& tc, std::ostream& os, const size_t indent,
                     const std::string_view dst, const std::string_view src)
        -> void {
        asm_line(tc, os, indent, "test {}, {}", dst, src);
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
