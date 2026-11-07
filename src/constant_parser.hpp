#pragma once

#include <cassert>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <string_view>
#include <system_error>

#include "compile_error.hpp"
#include "token.hpp"

// the literals and their values as the tokenizer keeps their text: a number,
// a character, 'true' and 'false'
class constant_parser final {
    static constexpr int decimal_base_{10};

  public:
    // the value of a literal, 'is_bool' for 'true' and 'false'
    struct literal {
        int64_t value{};
        bool is_bool{};
    };

    //
    // statics
    //

    // 'true' or 'false'
    [[nodiscard]] static auto is_bool_literal(const std::string_view str)
        -> bool {

        return str == reserved_names::true_value or
               str == reserved_names::false_value;
    }

    // a number, a character or a 'bool'; a number may be malformed, e.g. '12x'
    [[nodiscard]] static auto is_literal(const token& tk) -> bool {
        return tk.is_number() or is_character_literal(tk.text()) or
               is_bool_literal(tk.text());
    }

    // e.g. '0x10', ''a'' or 'true', empty for a text that is no literal
    [[nodiscard]] static auto literal_value(const token& src_loc_tk,
                                            const std::string_view str)
        -> std::optional<literal> {

        if (is_bool_literal(str)) {
            return literal{
                .value{str == reserved_names::true_value ? 1 : 0},
                .is_bool{true},
            };
        }

        const std::optional<int64_t> value{parse_constant(src_loc_tk, str)};

        if (not value) {
            return std::nullopt;
        }

        return literal{
            .value{*value},
            .is_bool{},
        };
    }

    // the value is the byte, e.g. 'a' is 97 and '\xff' is 255
    [[nodiscard]] static auto parse_character(const token& src_loc_tk,
                                              const std::string_view str)
        -> int64_t {

        assert(is_character_literal(str));

        const std::string_view body{str.substr(1, str.size() - 2)};
        // note: 1 and -2 because the literal is between the quotes

        // note: a backslash would escape the closing quote
        if (body.size() == 1) {
            return static_cast<unsigned char>(body.at(0));
        }

        // e.g. 'ab'
        if (not body.starts_with('\\')) {
            throw_not_one_byte(src_loc_tk, str);
        }

        const token::escape esc{token::escape_at(body, 0)};

        const size_t escape_size{1 + esc.size};
        // note: 1 + for the backslash before the escape

        const std::optional<char> decoded{esc.decoded};

        // note: the tokenizer rejected an unsupported escape
        //       ('tokenizer::assert_escapes_supported')

        assert(decoded);

        // e.g. '\n\n'
        if (body.size() > escape_size) {
            throw_not_one_byte(src_loc_tk, str);
        }

        return static_cast<unsigned char>(*decoded);
    }

    // e.g. '12', '0x1f', '0b101' and ''a'' have a value, 'x' has none
    [[nodiscard]] static auto parse_constant(const token& src_loc_tk,
                                             const std::string_view str)
        -> std::optional<int64_t> {

        if (is_character_literal(str)) {
            return parse_character(src_loc_tk, str);
        }

        const int base{base_of(str)};
        std::string_view digits{str};

        if (base != decimal_base_) {
            digits.remove_prefix(2);
            // note: 2 for the '0x' or '0b' prefix
        }

        int64_t value{};

        const char* const begin{std::to_address(digits.begin())};
        const char* const end{std::to_address(digits.end())};

        const std::from_chars_result result{
            std::from_chars(begin, end, value, base),
        };

        if (result.ec == std::errc::result_out_of_range) {
            throw compile_error{
                src_loc_tk,
                std::format("constant {} is out of range", in_quotes(str))};
        }

        if (result.ec == std::errc{} and result.ptr == end) {
            return value;
        }

        return std::nullopt;
    }

  private:
    //
    // statics
    //

    // the base the prefix gives, e.g. 16 for '0x1f' and 2 for '0b101'
    [[nodiscard]] static auto base_of(const std::string_view str) -> int {
        constexpr int hex_base{16};
        constexpr int binary_base{2};

        if (str.starts_with("0x") or str.starts_with("0X")) {
            return hex_base;
        }

        if (str.starts_with("0b") or str.starts_with("0B")) {
            return binary_base;
        }

        return decimal_base_;
    }

    // the tokenizer keeps the quotes in the text of a character literal
    [[nodiscard]] static auto is_character_literal(const std::string_view str)
        -> bool {

        return str.size() >= 2 and str.starts_with('\'') and
               str.ends_with('\'');
    }

    [[noreturn]] static auto throw_not_one_byte(const token& src_loc_tk,
                                                const std::string_view str)
        -> void {

        throw compile_error{
            src_loc_tk,
            std::format("character literal {} must contain one byte, e.g. "
                        "'a' or '\\xe9'",
                        in_quotes(str))};
    }
};
