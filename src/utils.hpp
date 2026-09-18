#pragma once

#include <cstddef>
#include <optional>
#include <regex>
#include <string_view>
#include <utility>

namespace utils {

constexpr size_t size_qword{8};
constexpr size_t size_dword{4};
constexpr size_t size_word{2};
constexpr size_t size_byte{1};

[[nodiscard]] auto get_before_dot(const std::string_view text)
    -> std::string_view;
[[nodiscard]] auto get_size_specifier(size_t size) -> std::string_view;
[[nodiscard]] auto get_text_between_brackets(const std::string_view text)
    -> std::optional<std::string_view>;
[[nodiscard]] auto register_size(const std::string_view operand) -> size_t;
[[nodiscard]] auto regex_ws() -> const std::regex&;
[[nodiscard]] auto regex_trim() -> const std::regex&;
[[nodiscard]] auto regex_nasm_comment() -> const std::regex&;
[[nodiscard]] auto line_and_col_num_for_char_index(size_t at_line,
                                                   size_t char_index_in_source,
                                                   const std::string_view src)
    -> std::pair<size_t, size_t>;

// NOLINTBEGIN(misc-definitions-in-headers)

[[nodiscard]] auto get_before_dot(const std::string_view text)
    -> std::string_view {
    if (const size_t position{text.find('.')};
        position != std::string_view::npos) {
        return text.substr(0, position);
    }
    return text;
}

[[nodiscard]] auto get_size_specifier(const size_t size) -> std::string_view {
    switch (size) {
    case size_qword:
        return "qword";
    case size_dword:
        return "dword";
    case size_word:
        return "word";
    case size_byte:
        return "byte";
    default:
        std::unreachable();
    }
}

[[nodiscard]] auto get_text_between_brackets(const std::string_view text)
    -> std::optional<std::string_view> {
    const size_t start{text.find('[')};
    if (start == std::string_view::npos) {
        return std::nullopt;
    }
    const size_t end{text.find(']', start)};
    if (end == std::string_view::npos) {
        return std::nullopt;
    }
    return text.substr(start + 1, end - start - 1);
}

// returns 0 if operand is not a register
[[nodiscard]] auto register_size(const std::string_view operand) -> size_t {
    if (operand == "rax" || operand == "rbx" || operand == "rcx" ||
        operand == "rdx" || operand == "rbp" || operand == "rsi" ||
        operand == "rdi" || operand == "rsp" || operand == "r8" ||
        operand == "r9" || operand == "r10" || operand == "r11" ||
        operand == "r12" || operand == "r13" || operand == "r14" ||
        operand == "r15") {
        return size_qword;
    }
    if (operand == "eax" || operand == "ebx" || operand == "ecx" ||
        operand == "edx" || operand == "ebp" || operand == "esi" ||
        operand == "edi" || operand == "esp" || operand == "r8d" ||
        operand == "r9d" || operand == "r10d" || operand == "r11d" ||
        operand == "r12d" || operand == "r13d" || operand == "r14d" ||
        operand == "r15d") {
        return size_dword;
    }
    if (operand == "ax" || operand == "bx" || operand == "cx" ||
        operand == "dx" || operand == "bp" || operand == "si" ||
        operand == "di" || operand == "sp" || operand == "r8w" ||
        operand == "r9w" || operand == "r10w" || operand == "r11w" ||
        operand == "r12w" || operand == "r13w" || operand == "r14w" ||
        operand == "r15w") {
        return size_word;
    }
    if (operand == "al" || operand == "ah" || operand == "bl" ||
        operand == "bh" || operand == "cl" || operand == "ch" ||
        operand == "dl" || operand == "dh" || operand == "spl" ||
        operand == "bpl" || operand == "sil" || operand == "dil" ||
        operand == "r8b" || operand == "r9b" || operand == "r10b" ||
        operand == "r11b" || operand == "r12b" || operand == "r13b" ||
        operand == "r14b" || operand == "r15b") {
        return size_byte;
    }
    return 0;
}

// pragma below: function-local statics below are only ever destroyed once,
// at program exit, which is intentional and harmless here
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
[[nodiscard]] auto regex_ws() -> const std::regex& {
    const static std::regex re{R"(\s+)"};
    return re;
}

[[nodiscard]] auto regex_trim() -> const std::regex& {
    const static std::regex re{R"(^\s+|\s+$)"};
    return re;
}

[[nodiscard]] auto regex_nasm_comment() -> const std::regex& {
    const static std::regex re{R"(^\s*;.*$)"};
    return re;
}
#pragma clang diagnostic pop

[[nodiscard]] auto line_and_col_num_for_char_index(const size_t at_line,
                                                   size_t char_index_in_source,
                                                   const std::string_view src)
    -> std::pair<size_t, size_t> {

    if (char_index_in_source >= src.size()) {
        return {at_line, 0};
    }

    size_t at_col{};
    while (src[char_index_in_source] != '\n') {
        ++at_col;
        if (char_index_in_source == 0) {
            break;
        }
        --char_index_in_source;
    }

    return {at_line, at_col};
}

// NOLINTEND(misc-definitions-in-headers)
} // namespace utils
