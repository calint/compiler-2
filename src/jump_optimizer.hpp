#pragma once
// jump optimizations applied to the generated assembly as a post-processing
// pass

// NOLINTBEGIN(misc-definitions-in-headers)

#include <cstdint>
#include <istream>
#include <ostream>
#include <print>
#include <regex>
#include <string>
#include <vector>

namespace jump_optimizer {

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
    const std::regex rx_jcc{R"(^\s*j[a-z]{1,2}\s+(.+)\s*$)"};
    const std::regex rx_lbl{R"(^\s*([a-zA-Z_][a-zA-Z0-9_]*):\s*$)"};
    std::smatch match;
    size_t opts_count{};

    struct elem {
        enum class type : uint8_t { JCC, LABEL };
        std::string line;
        std::string label;
        type type{};
    };

    std::vector<elem> buffer;
    auto flush_buffer = [&]() -> void {
        for (const elem& e : buffer) {
            std::println(os, "{}", e.line);
        }
        buffer.clear();
    };

    std::string line;
    while (getline(is, line)) {
        if (std::regex_search(line, match, rx_jcc)) {
            // matched a jcc (conditional or unconditional jump)
            if (buffer.empty() or (buffer.back().type == elem::type::JCC and
                                   buffer.back().label == match[1])) {
                // buffer jccs to same label
                buffer.emplace_back(line, match[1], elem::type::JCC);
            } else {
                // jcc to a different label than the buffer, flush buffer
                flush_buffer();
                // add it to the buffer to start a new sequence
                buffer.emplace_back(line, match[1], elem::type::JCC);
            }
        } else if (std::regex_search(line, match, rx_lbl)) {
            // matched a label
            if (not buffer.empty() and buffer.back().type == elem::type::JCC and
                buffer.back().label == match[1]) {
                // label after jccs to this label, remove the jccs, print
                // the label
                opts_count += buffer.size();
                buffer.clear();
                std::println(os, "{}", line);
            } else {
                // label that differs from the buffered jccs, flush buffer
                // and print the label
                flush_buffer();
                std::println(os, "{}", line);
            }
        } else {
            // matched something else, flush buffer, print it
            flush_buffer();
            std::println(os, "{}", line);
        }
    }
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
    const std::regex rx_jmp{R"(^\s*jmp\s+(.+)\s*$)"};
    const std::regex rx_jcc{R"(^\s*(j[a-z][a-z]?)\s+(.+)\s*$)"};
    const std::regex rx_lbl{R"(^\s*(.+):.*$)"};
    std::smatch match;

    size_t optimizations{};

    std::string line1;
    while (getline(is, line1)) {
        if (not std::regex_search(line1, match, rx_jcc)) {
            std::println(os, "{}", line1);
            continue;
        }

        const std::string jcc{match[1]};
        const std::string jcc_lbl{match[2]};

        std::string line2;
        if (not getline(is, line2)) {
            std::println(os, "{}", line1);
            return;
        }

        if (not std::regex_search(line2, match, rx_jmp)) {
            std::println(os, "{}", line1);
            std::println(os, "{}", line2);
            continue;
        }

        const std::string jmp_lbl{match[1]};

        std::string line3;
        if (not getline(is, line3)) {
            std::println(os, "{}", line1);
            std::println(os, "{}", line2);
            return;
        }

        if (not std::regex_search(line3, match, rx_lbl)) {
            std::println(os, "{}", line1);
            std::println(os, "{}", line2);
            std::println(os, "{}", line3);
            continue;
        }

        const std::string lbl{match[1]};

        if (jcc_lbl != lbl) {
            std::println(os, "{}", line1);
            std::println(os, "{}", line2);
            std::println(os, "{}", line3);
            continue;
        }

        //   jne cmp_14_26
        //   jmp if_14_8_code
        //   cmp_14_26:
        std::string jxx_inv;
        if (jcc == "jne") {
            jxx_inv = "je";
        } else if (jcc == "je") {
            jxx_inv = "jne";
        } else if (jcc == "jg") {
            jxx_inv = "jle";
        } else if (jcc == "jge") {
            jxx_inv = "jl";
        } else if (jcc == "jl") {
            jxx_inv = "jge";
        } else if (jcc == "jle") {
            jxx_inv = "jg";
        } else {
            std::println(os, "{}", line1);
            std::println(os, "{}", line2);
            std::println(os, "{}", line3);
            continue;
        }
        //   je if_14_8_code
        //   cmp_14_26:

        // get the whitespaces
        const std::string ws_before{
            line1.substr(0, line1.find_first_not_of(" \t\n\r\f\v"))};

        std::println(os, "{}{} {}", ws_before, jxx_inv, jmp_lbl);
        std::println(os, "{}", line3);
        ++optimizations;
    }

    std::println(os, ";          optimization pass 2: {}", optimizations);
}

} // namespace jump_optimizer

// NOLINTEND(misc-definitions-in-headers)
