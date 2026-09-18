#pragma once
// jump optimizations applied to the generated assembly as a post-processing
// pass

// NOLINTBEGIN(misc-definitions-in-headers)

#include <cctype>
#include <istream>
#include <optional>
#include <ostream>
#include <print>
#include <string>
#include <string_view>
#include <vector>

namespace jump_optimizer {

struct jump_info {
    std::string mnemonic;
    std::string label;
};

[[nodiscard]] static auto leading_ws(const std::string_view line)
    -> std::string_view {

    const size_t first{line.find_first_not_of(" \t\n\r\f\v")};
    if (first == std::string_view::npos) {
        return line;
    }
    return line.substr(0, first);
}

[[nodiscard]] static auto trim_ws(const std::string_view text)
    -> std::string_view {

    const size_t first{text.find_first_not_of(" \t\n\r\f\v")};
    if (first == std::string_view::npos) {
        return {};
    }
    const size_t last{text.find_last_not_of(" \t\n\r\f\v")};
    return text.substr(first, last - first + 1);
}

[[nodiscard]] static auto parse_jump(const std::string_view line)
    -> std::optional<jump_info> {

    const std::string_view trimmed{trim_ws(line)};
    if (trimmed.size() < 3 || trimmed[0] != 'j') {
        return std::nullopt;
    }

    size_t i{1};
    size_t mnemonic_letters{};
    while (i < trimmed.size() &&
           std::islower(static_cast<unsigned char>(trimmed[i])) &&
           mnemonic_letters < 2) {
        ++i;
        ++mnemonic_letters;
    }

    if (mnemonic_letters == 0 || i >= trimmed.size() ||
        not std::isspace(static_cast<unsigned char>(trimmed[i]))) {
        return std::nullopt;
    }

    while (i < trimmed.size() &&
           std::isspace(static_cast<unsigned char>(trimmed[i]))) {
        ++i;
    }
    if (i >= trimmed.size()) {
        return std::nullopt;
    }

    const std::string_view label{trim_ws(trimmed.substr(i))};
    if (label.empty()) {
        return std::nullopt;
    }

    return jump_info{
        .mnemonic = std::string{trimmed.substr(0, 1 + mnemonic_letters)},
        .label = std::string{label},
    };
}

[[nodiscard]] static auto parse_label_strict(const std::string_view line)
    -> std::optional<std::string> {

    const std::string_view trimmed{trim_ws(line)};
    const size_t colon{trimmed.find(':')};
    if (colon == std::string_view::npos || colon + 1 != trimmed.size()) {
        return std::nullopt;
    }

    const std::string_view lbl{trimmed.substr(0, colon)};
    if (lbl.empty()) {
        return std::nullopt;
    }
    const unsigned char first{static_cast<unsigned char>(lbl.front())};
    if (not std::isalpha(first) && lbl.front() != '_') {
        return std::nullopt;
    }

    for (size_t i{1}; i < lbl.size(); ++i) {
        const unsigned char ch{static_cast<unsigned char>(lbl[i])};
        if (not std::isalnum(ch) && lbl[i] != '_') {
            return std::nullopt;
        }
    }

    return std::string{lbl};
}

[[nodiscard]] static auto parse_label_any(const std::string_view line)
    -> std::optional<std::string> {

    const std::string_view trimmed{trim_ws(line)};
    const size_t colon{trimmed.find(':')};
    if (colon == std::string_view::npos || colon == 0) {
        return std::nullopt;
    }
    return std::string{trim_ws(trimmed.substr(0, colon))};
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
        if (const auto jump{parse_jump(line)}) {
            // keep buffering only while jumps target the same label
            if (not pending_label || *pending_label == jump->label) {
                pending_jumps.emplace_back(line);
                pending_label = jump->label;
                continue;
            }
            flush_pending();
            pending_jumps.emplace_back(line);
            pending_label = jump->label;
            continue;
        }

        if (const auto lbl{parse_label_strict(line)}) {
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
        const auto jcc_match{parse_jump(first_line)};
        if (not jcc_match) {
            std::println(os, "{}", first_line);
            continue;
        }

        const std::string jcc{jcc_match->mnemonic};
        const std::string jcc_label{jcc_match->label};

        std::string second_line;
        if (not getline(is, second_line)) {
            std::println(os, "{}", first_line);
            return;
        }

        const auto jmp_match{parse_jump(second_line)};
        if (not jmp_match || jmp_match->mnemonic != "jmp") {
            print2(first_line, second_line);
            continue;
        }

        const std::string jmp_label{jmp_match->label};

        std::string third_line;
        if (not getline(is, third_line)) {
            print2(first_line, second_line);
            return;
        }

        const auto lbl_match{parse_label_any(third_line)};
        if (not lbl_match) {
            print3(first_line, second_line, third_line);
            continue;
        }

        const std::string& label{*lbl_match};

        if (jcc_label != label) {
            print3(first_line, second_line, third_line);
            continue;
        }

        //   jne cmp_14_26
        //   jmp if_14_8_code
        //   cmp_14_26:
        const auto inverted_jcc{invert_jcc(jcc)};
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
