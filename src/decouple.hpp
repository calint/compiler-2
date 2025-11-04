#pragma once
// solves circular references
// implemented in 'main.cpp'

// reviewed: 2025-09-28

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>

#include "token.hpp"

class toc;
class tokenizer;
class statement;
class stmt_identifier;
class type;
class expr_any;

struct operand {
    static constexpr size_t size_qword{8};
    static constexpr size_t size_dword{4};
    static constexpr size_t size_word{2};
    static constexpr size_t size_byte{1};

    std::string base_register;
    std::string index_register;
    int32_t displacement{};
    uint8_t scale{1};
    size_t size{};

    operand() = default;

    explicit operand(const std::string_view operand_sv) {
        if (operand_sv.empty()) {
            return;
        }
        // note: regex handles only "base + index * scale +/- displacement" with
        //       optional index, scale and displacement
        const std::regex pattern_with_brackets(
            // whitespace and size specifier
            R"(^\s*(?:(byte|word|dword|qword)\s*)?)"
            // opening bracket
            R"(\[)"
            // base register (must start with a letter)
            R"(([a-z][a-z0-9]*)?)"
            // +index (must start with a letter)
            R"((?:\s*\+\s*([a-z][a-z0-9]*?))?)"
            // *scale
            R"((?:\s*\*\s*([1248]))?)"
            // +/- displacement
            R"((?:\s*([+-])\s*(\d+))?)"
            // closing bracket
            R"(\]\s*$)");

        const std::regex pattern_without_brackets(
            // whitespace and size specifier
            R"(^\s*(?:(byte|word|dword|qword)\s*)?)"
            // base register (must start with a letter)
            R"(([a-z][a-z0-9]*)?)"
            // +index (must start with a letter)
            R"((?:\s*\+\s*([a-z][a-z0-9]*?))?)"
            // *scale
            R"((?:\s*\*\s*([1248]))?)"
            // +/- displacement
            R"((?:\s*([+-])\s*(\d+))?)"
            // whitespace
            R"(\s*$)");

        constexpr size_t match_size{1};
        constexpr size_t match_base_register{2};
        constexpr size_t match_index_register{3};
        constexpr size_t match_scale{4};
        constexpr size_t match_displacement_op{5};
        constexpr size_t match_displacement{6};

        std::smatch matches;
        const std::string operand_str{operand_sv};
        if (std::regex_match(operand_str, matches, pattern_with_brackets)) {
            // handle pattern with brackets
            if (matches[match_size].matched) {
                const auto size_str = matches[match_size].str();
                if (size_str == "byte") {
                    size = size_byte;
                } else if (size_str == "word") {
                    size = size_word;
                } else if (size_str == "dword") {
                    size = size_dword;
                } else if (size_str == "qword") {
                    size = size_qword;
                }
            }

            if (matches[match_base_register].matched) {
                base_register = matches[match_base_register].str();
            }

            if (matches[match_index_register].matched) {
                index_register = matches[match_index_register].str();
            }

            if (matches[match_scale].matched) {
                scale =
                    static_cast<uint8_t>(std::stoi(matches[match_scale].str()));
            }

            if (matches[match_displacement_op].matched and
                matches[match_displacement].matched) {

                const int disp_value =
                    std::stoi(matches[match_displacement].str());
                if (matches[match_displacement_op].str() == "-") {
                    displacement = -disp_value;
                } else {
                    displacement = disp_value;
                }
            }
        } else if (std::regex_match(operand_str, matches,
                                    pattern_without_brackets)) {
            // handle pattern without brackets
            if (matches[match_size].matched) {
                const auto size_str = matches[match_size].str();
                if (size_str == "byte") {
                    size = size_byte;
                } else if (size_str == "word") {
                    size = size_word;
                } else if (size_str == "dword") {
                    size = size_dword;
                } else if (size_str == "qword") {
                    size = size_qword;
                }
            }

            if (matches[match_base_register].matched) {
                if (matches[match_index_register].matched) {
                    base_register = matches[match_base_register].str();
                    index_register = matches[match_index_register].str();
                } else {
                    base_register = matches[match_base_register].str();
                }
            }

            if (matches[match_scale].matched) {
                scale =
                    static_cast<uint8_t>(std::stoi(matches[match_scale].str()));
            }

            if (matches[match_displacement_op].matched and
                matches[match_displacement].matched) {

                const int disp_value =
                    std::stoi(matches[match_displacement].str());
                if (matches[match_displacement_op].str() == "-") {
                    displacement = -disp_value;
                } else {
                    displacement = disp_value;
                }
            }
        } else {
            // try to match a standalone register
            const std::regex register_pattern(R"(^\s*([a-z][a-z0-9._]*)\s*$)");
            std::smatch register_match;
            if (std::regex_match(operand_str, register_match,
                                 register_pattern)) {
                base_register = register_match[1].str();
            } else {
                throw std::invalid_argument(std::format(
                    "invalid NASM operand format: {}", operand_str));
            }
        }
    }

    [[nodiscard]] auto is_empty() const -> bool {
        return base_register.empty() and index_register.empty() and
               displacement == 0;
    }

    [[nodiscard]] auto is_indexed() const -> bool {
        return not index_register.empty() or displacement != 0;
    }

    [[nodiscard]] auto is_memory() const -> bool { return size != 0; }

    [[nodiscard]] auto address_str() const -> std::string {
        std::string s;

        if (not base_register.empty()) {
            s += base_register;
        }

        if (not index_register.empty()) {
            if (not s.empty()) {
                s += " + ";
            }
            s += index_register;
            if (scale > 1) {
                s += " * " + std::to_string(scale);
            }
        }

        if (displacement != 0) {
            if (not s.empty()) {
                if (displacement > 0) {
                    s += " + ";
                } else {
                    s += " - ";
                }
            }
            s +=
                std::to_string(displacement < 0 ? -displacement : displacement);
        }

        return s;
    }

    [[nodiscard]] auto str() const -> std::string { return str(size); }

    [[nodiscard]] auto str(const size_t opsize) const -> std::string {
        std::string s;
        if (opsize != 0) {
            switch (opsize) {
            case size_byte:
                s.append("byte");
                break;
            case size_word:
                s.append("word");
                break;
            case size_dword:
                s.append("dword");
                break;
            case size_qword:
                s.append("qword");
                break;
            default:
                std::unreachable();
            }
            s.append(" [");
        }
        s.append(address_str());
        if (opsize != 0) {
            s.append("]");
        }
        return s;
    }
};

struct var_info {
    std::string name;
    const type* type_ptr{};
    token declared_at_tk; // token for position in the source
    int32_t stack_idx{};  // location relative to register rsp
    bool is_array{};
    size_t array_size{};
};

struct ident_info {
    enum class ident_type : uint8_t { CONST, VAR, REGISTER, FIELD, IMPLIED };

    std::string id;
    operand operand; // NASM valid source
    int64_t const_value{};
    const type* type_ptr{};
    int32_t stack_ix{};
    size_t array_size{};
    bool is_array{};
    std::vector<std::string> elem_path;
    std::vector<const type*> type_path;
    std::vector<std::string> lea_path;
    std::string lea;
    ident_type ident_type{ident_type::CONST};

    [[nodiscard]] auto is_const() const -> bool {
        return ident_type == ident_type::CONST;
    }

    [[nodiscard]] auto is_register() const -> bool {
        return ident_type == ident_type::REGISTER;
    }

    [[nodiscard]] auto is_var() const -> bool {
        return ident_type == ident_type::VAR;
    }

    [[nodiscard]] auto is_memory_operand() const -> bool {
        return operand.is_memory();
    }

    [[nodiscard]] auto has_lea() const -> bool {
        return std::ranges::any_of(lea_path, [](const std::string& s) -> bool {
            return not s.empty();
        });
    }
};

//
// functions necessary to solve circular references implemented in 'main.cpp'
//

inline auto create_statement_in_expr_ops_list(toc& tc, tokenizer& tz)
    -> std::unique_ptr<statement>;

inline auto create_statement_in_stmt_block(toc& tc, tokenizer& tz, token tk)
    -> std::unique_ptr<statement>;

inline auto create_stmt_call(toc& tc, tokenizer& tz, const stmt_identifier& si)
    -> std::unique_ptr<statement>;

inline auto create_expr_any(toc& tc, tokenizer& tz, const type& tp,
                            bool in_args) -> std::unique_ptr<expr_any>;
