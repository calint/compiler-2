#pragma once

#include <cstddef>
#include <optional>
#include <string_view>
#include <utility>

namespace utils {

[[nodiscard]] auto get_before_dot(const std::string_view text)
    -> std::string_view;

[[nodiscard]] auto get_text_between_brackets(const std::string_view text)
    -> std::optional<std::string_view>;

[[nodiscard]] auto line_and_col_num_for_char_index(size_t at_line,
                                                   size_t char_index_in_source,
                                                   const std::string_view src)
    -> std::pair<size_t, size_t>;

// NOLINTBEGIN(misc-definitions-in-headers)

[[nodiscard]] auto get_before_dot(const std::string_view text)
    -> std::string_view {

    return text.substr(0, text.find('.'));
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
