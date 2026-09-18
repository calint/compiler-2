#pragma once
// reviewed: 2025-09-28

// solves circular references
// implemented in 'decouple_impl.hpp'

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "token.hpp"
#include "utils.hpp"

class toc;
class tokenizer;
class statement;
class stmt_identifier;
class stmt_call;
class stmt_block;
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
    bool is_base_register{};
    bool is_memory{};

    operand() = default;

    operand(const std::string_view operand_sv,
            const bool operand_is_base_register)
        : is_base_register{operand_is_base_register} {

        if (operand_sv.empty()) {
            return;
        }

        if (operand_is_base_register) {
            base_register = operand_sv;
            size = utils::register_size(operand_sv);
            return;
        }

        size_t pos{};

        const auto skip_space = [&] -> void {
            while (pos < operand_sv.size() and
                   std::isspace(static_cast<unsigned char>(operand_sv[pos]))) {
                ++pos;
            }
        };

        const auto parse_identifier = [&](std::string& destination) -> bool {
            if (pos == operand_sv.size() or operand_sv[pos] < 'a' or
                operand_sv[pos] > 'z') {
                return false;
            }

            const size_t begin{pos++};
            while (pos < operand_sv.size() and
                   ((operand_sv[pos] >= 'a' and operand_sv[pos] <= 'z') or
                    (operand_sv[pos] >= '0' and operand_sv[pos] <= '9'))) {
                ++pos;
            }

            destination = operand_sv.substr(begin, pos - begin);
            return true;
        };

        const auto invalid_operand = [&] [[noreturn]] -> void {
            throw std::invalid_argument(
                std::format("invalid NASM operand format: {}", operand_sv));
        };

        skip_space();

        constexpr std::array<std::pair<std::string_view, size_t>, 4> sizes{
            {std::pair{"byte", size_byte}, std::pair{"word", size_word},
             std::pair{"dword", size_dword}, std::pair{"qword", size_qword}}};

        for (const auto [name, operand_size] : sizes) {
            if (operand_sv.substr(pos).starts_with(name)) {
                const size_t end{pos + name.size()};
                if (end == operand_sv.size() or
                    std::isspace(static_cast<unsigned char>(operand_sv[end])) or
                    operand_sv[end] == '[') {

                    size = operand_size;
                    is_memory = true;
                    pos = end;
                    break;
                }
            }
        }

        skip_space();

        const bool bracketed{pos < operand_sv.size() and
                             operand_sv[pos] == '['};

        if (bracketed) {
            ++pos;
            skip_space();
        }

        parse_identifier(base_register);

        skip_space();

        if (pos < operand_sv.size() and operand_sv[pos] == '+') {
            const size_t plus{pos};
            ++pos;

            skip_space();

            if (not parse_identifier(index_register)) {
                pos = plus;
            } else {
                skip_space();

                if (pos < operand_sv.size() and operand_sv[pos] == '*') {
                    ++pos;
                    skip_space();
                    if (pos == operand_sv.size() or
                        not std::string_view{"1248"}.contains(
                            operand_sv[pos])) {
                        invalid_operand();
                    }

                    scale = static_cast<uint8_t>(operand_sv[pos] - '0');
                    ++pos;
                }
            }
        }

        skip_space();

        if (pos < operand_sv.size() and
            (operand_sv[pos] == '+' or operand_sv[pos] == '-')) {
            const bool negative{operand_sv[pos++] == '-'};

            skip_space();

            const size_t begin{pos};
            while (pos < operand_sv.size() and operand_sv[pos] >= '0' and
                   operand_sv[pos] <= '9') {
                ++pos;
            }

            if (begin == pos) {
                invalid_operand();
            }

            int64_t magnitude{};
            for (size_t ix{begin}; ix < pos; ++ix) {
                const int64_t digit{operand_sv[ix] - '0'};
                constexpr int64_t decimal_radix{10};
                magnitude = (magnitude * decimal_radix) + digit;
            }

            displacement =
                static_cast<int32_t>(negative ? -magnitude : magnitude);
        }

        skip_space();

        if (bracketed) {
            if (pos == operand_sv.size() or operand_sv[pos] != ']') {
                invalid_operand();
            }
            ++pos;

            skip_space();
        }

        if (pos != operand_sv.size()) {
            invalid_operand();
        }
    }

    [[nodiscard]] auto is_empty() const -> bool {
        return base_register.empty() and index_register.empty() and
               displacement == 0;
    }

    [[nodiscard]] auto is_indexed() const -> bool {
        return not index_register.empty() or displacement != 0;
    }

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

    [[nodiscard]] auto str() const -> std::string {
        return is_base_register ? base_register : str(size);
    }

    [[nodiscard]] auto str(const size_t size_specifier) const -> std::string {
        std::string s;
        if (size_specifier != 0) {
            switch (size_specifier) {
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
        if (size_specifier != 0) {
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
    std::string reg;
};

struct ident_info {
    enum class ident_type : uint8_t { CONST, VAR, REGISTER, EMPTY };

    std::string id;
    std::vector<std::string> elem_path;
    std::vector<const type*> type_path;
    std::vector<std::string> lea_path;
    operand operand; // nasm valid source
    int32_t stack_ix{};
    int64_t const_value{};
    size_t array_size{};
    bool is_array{};
    bool use_operand{};
    ident_type ident_type{};

    [[nodiscard]] auto is_const() const -> bool {
        return ident_type == ident_type::CONST;
    }

    [[nodiscard]] auto is_register() const -> bool {
        return ident_type == ident_type::REGISTER;
    }

    [[nodiscard]] auto is_var() const -> bool {
        return ident_type == ident_type::VAR;
    }

    [[nodiscard]] auto is_empty() const -> bool {
        return ident_type == ident_type::EMPTY;
    }

    [[nodiscard]] auto has_lea() const -> bool {
        return std::ranges::any_of(lea_path, [](const std::string& s) -> bool {
            return not s.empty();
        });
    }

    [[nodiscard]] auto type_ref() const -> const type& {
        assert(not type_path.empty());
        return *type_path.back();
    }

    void push(std::string path_elem, const type* tp, std::string lea) {
        id += "." + path_elem;
        elem_path.emplace_back(path_elem);
        type_path.emplace_back(tp);
        lea_path.emplace_back(lea);
    }

    void pop() {
        id.resize(id.rfind('.'));
        elem_path.pop_back();
        type_path.pop_back();
        lea_path.pop_back();
    }

    void increment_offset(const int32_t n) {
        assert(stack_ix + n <= 0);
        stack_ix += n;
        operand.displacement += n;
    }

    void replace_back(std::string path_elem, const type* tp, std::string lea) {
        elem_path.back() = path_elem;
        type_path.back() = tp;
        lea_path.back() = lea;
    }
};

//
// functions necessary to solve circular references, implemented in
// 'decouple_impl.hpp'
//

[[nodiscard]] auto create_statement_in_expr_ops_list(toc& tc, tokenizer& tz)
    -> std::unique_ptr<statement>;

[[nodiscard]] auto create_statement_in_stmt_block(toc& tc, tokenizer& tz,
                                                  token tk)
    -> std::unique_ptr<statement>;

[[nodiscard]] auto create_stmt_call(toc& tc, tokenizer& tz,
                                    const stmt_identifier& si,
                                    token open_paren_tk)
    -> std::unique_ptr<statement>;
