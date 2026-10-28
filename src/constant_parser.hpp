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
  public:
    // the value of a literal, 'is_bool' for 'true' and 'false'
    struct literal {
        int64_t value{};
        bool is_bool{};
    };

    //
    // statics
    //

    // e.g. '12', ''a'' or 'true'; a number may still be malformed, e.g. '12x'
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

        if (body.size() == 1 and body.at(0) != '\\') {
            return static_cast<unsigned char>(body.at(0));
        }

        // e.g. 'ab' or '\n\n'
        const bool is_one_escape{
            body.starts_with('\\') and
                body.size() <= 1 + token::escape_length(body.substr(1)),
        };
        // note: 1 + for the backslash before the escape

        if (not is_one_escape) {
            throw compile_error{
                src_loc_tk,
                std::format("character literal {} must contain one character",
                            in_quotes(str))};
        }

        const std::optional<char> decoded{token::escape_at(body, 0).decoded};

        if (not decoded) {
            // note: the tokenizer checks a literal as it reads it, so
            //       'src_loc_tk' is the literal

            assert(src_loc_tk.is_text(str));

            const size_t backslash_index{src_loc_tk.start_index() + 1};
            // note: +1 because the backslash follows the opening quote

            const token escape_tk{
                token::position(src_loc_tk.file_index(), backslash_index,
                                src_loc_tk.at_line()),
            };

            throw compile_error{escape_tk, std::format("unsupported escape {}",
                                                       in_quotes(body))};
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

        constexpr int base_decimal{10};
        constexpr int base_hex{16};
        constexpr int base_binary{2};

        int base{base_decimal};
        std::string_view digits{str};

        if (has_base_prefix(str)) {
            const bool is_hex{str.at(1) == 'x' or str.at(1) == 'X'};
            // note: 1 is the letter after the '0' of the prefix

            base = is_hex ? base_hex : base_binary;
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

    // e.g. '0x1f' and '0b101'
    [[nodiscard]] static auto has_base_prefix(const std::string_view str)
        -> bool {

        return str.starts_with("0x") or str.starts_with("0X") or
               str.starts_with("0b") or str.starts_with("0B");
    }

    [[nodiscard]] static auto is_bool_literal(const std::string_view str)
        -> bool {

        return str == reserved_names::true_value or
               str == reserved_names::false_value;
    }

    // the tokenizer keeps the quotes in the text of a character literal
    [[nodiscard]] static auto is_character_literal(const std::string_view str)
        -> bool {

        return str.size() >= 2 and str.starts_with('\'') and
               str.ends_with('\'');
    }
};
