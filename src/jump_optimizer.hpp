#pragma once
// jump optimizations applied to the generated assembly as a post-processing
// pass

// NOLINTBEGIN(misc-definitions-in-headers)

#include <istream>
#include <optional>
#include <ostream>
#include <print>
#include <string>
#include <string_view>
#include <vector>

namespace jump_optimizer {

struct jump_info {
    std::string_view mnemonic;
    std::string_view label;
};

[[nodiscard]] static auto is_ascii_space(const char ch) -> bool {
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' ||
           ch == '\v';
}

[[nodiscard]] static auto is_ascii_lower(const char ch) -> bool {
    return ch >= 'a' && ch <= 'z';
}

[[nodiscard]] static auto is_ascii_alpha(const char ch) -> bool {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

[[nodiscard]] static auto is_ascii_alnum(const char ch) -> bool {
    return is_ascii_alpha(ch) || (ch >= '0' && ch <= '9');
}

[[nodiscard]] static auto leading_ws(const std::string_view line)
    -> std::string_view {

    size_t first{};
    while (first < line.size() && is_ascii_space(line[first])) {
        ++first;
    }
    if (first == line.size()) {
        return line;
    }
    return line.substr(0, first);
}

[[nodiscard]] static auto parse_jump(const std::string_view line)
    -> std::optional<jump_info> {

    size_t i{};
    while (i < line.size() && is_ascii_space(line[i])) {
        ++i;
    }
    if (i == line.size() || line[i] != 'j') {
        return std::nullopt;
    }

    const size_t mnemonic_start{i};
    ++i;
    size_t mnemonic_letters{};
    while (i < line.size() && mnemonic_letters < 2 && is_ascii_lower(line[i])) {
        ++i;
        ++mnemonic_letters;
    }

    if (mnemonic_letters == 0 || i >= line.size() ||
        not is_ascii_space(line[i])) {
        return std::nullopt;
    }

    const size_t mnemonic_end{i};

    while (i < line.size() && is_ascii_space(line[i])) {
        ++i;
    }
    if (i >= line.size()) {
        return std::nullopt;
    }

    const size_t label_start{i};
    size_t label_end{line.size()};
    while (label_end > label_start && is_ascii_space(line[label_end - 1])) {
        --label_end;
    }
    if (label_start == label_end) {
        return std::nullopt;
    }

    return jump_info{
        .mnemonic = line.substr(mnemonic_start, mnemonic_end - mnemonic_start),
        .label = line.substr(label_start, label_end - label_start),
    };
}

[[nodiscard]] static auto parse_label_strict(const std::string_view line)
    -> std::optional<std::string_view> {

    size_t i{};
    while (i < line.size() && is_ascii_space(line[i])) {
        ++i;
    }
    if (i == line.size()) {
        return std::nullopt;
    }

    const size_t label_start{i};
    if (not is_ascii_alpha(line[i]) && line[i] != '_') {
        return std::nullopt;
    }

    ++i;
    while (i < line.size() && (is_ascii_alnum(line[i]) || line[i] == '_')) {
        ++i;
    }

    if (i == line.size() || line[i] != ':') {
        return std::nullopt;
    }

    const size_t label_end{i};
    ++i;
    while (i < line.size()) {
        if (not is_ascii_space(line[i])) {
            return std::nullopt;
        }
        ++i;
    }

    return line.substr(label_start, label_end - label_start);
}

[[nodiscard]] static auto parse_label_any(const std::string_view line)
    -> std::optional<std::string_view> {

    size_t start{};
    while (start < line.size() && is_ascii_space(line[start])) {
        ++start;
    }
    if (start == line.size()) {
        return std::nullopt;
    }

    const size_t colon{line.find(':', start)};
    if (colon == std::string_view::npos) {
        return std::nullopt;
    }

    size_t end{colon};
    while (end > start && is_ascii_space(line[end - 1])) {
        --end;
    }
    if (end == start) {
        return std::nullopt;
    }

    return line.substr(start, end - start);
}

[[nodiscard]] static auto invert_jcc(const std::string_view jcc)
    -> std::optional<std::string_view> {

    if (jcc == "jne") {
        return "je";
    }
    if (jcc == "je") {
        return "jne";
    }
    if (jcc == "jg") {
        return "jle";
    }
    if (jcc == "jge") {
        return "jl";
    }
    if (jcc == "jl") {
        return "jge";
    }
    if (jcc == "jle") {
        return "jg";
    }
    return std::nullopt;
}

auto pass1(std::istream& is, std::ostream& os) -> void;
auto pass2(std::istream& is, std::ostream& os) -> void;

//
// pass 1
//
//  example:
//    jmp cmp_13_26
//    cmp_13_26:
//  to
//    cmp_13_26:
//
//  example:
//    jne bool_end_15_9
//    jmp bool_end_15_9
//    bool_end_15_9:
//  to
//    bool_end_15_9:
//
auto pass1(std::istream& is, std::ostream& os) -> void {

    size_t opts_count{};

    std::vector<std::string> pending_jumps;
    std::optional<std::string> pending_label;

    auto flush_pending = [&]() -> void {
        for (const std::string& buffered_line : pending_jumps) {
            std::println(os, "{}", buffered_line);
        }
        pending_jumps.clear();
        pending_label.reset();
    };

    std::string line;
    while (getline(is, line)) {
        if (const std::optional<jump_info> jump{parse_jump(line)}) {
            // keep buffering only while jumps target the same label
            if (not pending_label || *pending_label == jump->label) {
                pending_jumps.emplace_back(line);
                pending_label = std::string{jump->label};
                continue;
            }
            flush_pending();
            pending_jumps.emplace_back(line);
            pending_label = std::string{jump->label};
            continue;
        }

        if (const std::optional<std::string_view> lbl{
                parse_label_strict(line)}) {
            // target label reached: drop pending jumps, print label
            if (pending_label && *pending_label == *lbl) {
                opts_count += pending_jumps.size();
                pending_jumps.clear();
                pending_label.reset();
                std::println(os, "{}", line);
                continue;
            }
        }

        flush_pending();
        std::println(os, "{}", line);
    }

    flush_pending();
    std::println(os, ";          optimization pass 1: {}", opts_count);
}

//
// pass 2
//
// example:
//   jne cmp_14_26
//   jmp if_14_8_code
//   cmp_14_26:
// to
//   je if_14_8_code
//   cmp_14_26:
//
auto pass2(std::istream& is, std::ostream& os) -> void {

    size_t optimizations{};

    auto print2 = [&](const std::string_view a,
                      const std::string_view b) -> void {
        std::println(os, "{}", a);
        std::println(os, "{}", b);
    };

    auto print3 = [&](const std::string_view a, const std::string_view b,
                      const std::string_view c) -> void {
        std::println(os, "{}", a);
        std::println(os, "{}", b);
        std::println(os, "{}", c);
    };

    std::string first_line;
    while (getline(is, first_line)) {
        const std::optional<jump_info> jcc_match{parse_jump(first_line)};
        if (not jcc_match) {
            std::println(os, "{}", first_line);
            continue;
        }

        const std::string_view jcc{jcc_match->mnemonic};
        const std::string_view jcc_label{jcc_match->label};

        std::string second_line;
        if (not getline(is, second_line)) {
            std::println(os, "{}", first_line);
            return;
        }

        const std::optional<jump_info> jmp_match{parse_jump(second_line)};
        if (not jmp_match || jmp_match->mnemonic != "jmp") {
            print2(first_line, second_line);
            continue;
        }

        const std::string_view jmp_label{jmp_match->label};

        std::string third_line;
        if (not getline(is, third_line)) {
            print2(first_line, second_line);
            return;
        }

        const std::optional<std::string_view> lbl_match{
            parse_label_any(third_line)};
        if (not lbl_match) {
            print3(first_line, second_line, third_line);
            continue;
        }

        const std::string_view label{*lbl_match};

        if (jcc_label != label) {
            print3(first_line, second_line, third_line);
            continue;
        }

        //   jne cmp_14_26
        //   jmp if_14_8_code
        //   cmp_14_26:
        const std::optional<std::string_view> inverted_jcc{invert_jcc(jcc)};
        if (not inverted_jcc) {
            print3(first_line, second_line, third_line);
            continue;
        }
        //   je if_14_8_code
        //   cmp_14_26:

        // get the whitespaces
        const std::string_view ws_before{leading_ws(first_line)};

        std::println(os, "{}{} {}", ws_before, *inverted_jcc, jmp_label);
        std::println(os, "{}", third_line);
        ++optimizations;
    }

    std::println(os, ";          optimization pass 2: {}", optimizations);
}

} // namespace jump_optimizer

// NOLINTEND(misc-definitions-in-headers)
