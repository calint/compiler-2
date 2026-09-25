#pragma once

#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "almost_assembler.hpp"

// buffers x86_64 output so its jumps can be optimized, nasm resolves the
// distances

class almost_assembler_x86_64 final : public almost_assembler {
  public:
    // lines other than comments and blank lines, the measure for choosing
    // between versions of the same code
    [[nodiscard]] static auto count_instructions(const std::vector<line>& lines)
        -> size_t {

        size_t count{};
        for (const line& l : lines) {
            if (not is_comment_or_blank(l.text)) {
                ++count;
            }
        }

        return count;
    }

  private:
    [[nodiscard]] auto unconditional_jump_mnemonic() const
        -> std::string_view override {

        return "jmp";
    }

    [[nodiscard]] auto
    inverse_branch_mnemonic(const std::string_view mnemonic) const
        -> std::optional<std::string_view> override {

        constexpr std::array<std::pair<std::string_view, std::string_view>, 3>
            pairs{{
                {"je", "jne"},
                {"jg", "jle"},
                {"jge", "jl"},
            }};

        for (const auto& [first, second] : pairs) {
            if (mnemonic == first) {
                return second;
            }
            if (mnemonic == second) {
                return first;
            }
        }

        return std::nullopt;
    }

    [[nodiscard]] auto format_jump(const jump_info& jump) const
        -> std::string override {

        return std::format("{} {}", jump.mnemonic, jump.target);
    }

    [[nodiscard]] auto comment_prefix() const -> std::string_view override {
        return ";";
    }

    // every instruction or directive counts as one
    [[nodiscard]] auto text_code_size(const std::string_view text) const
        -> size_t override {

        const std::string_view code{code_part(text)};
        if (code.empty() or code.back() == ':' or
            code.starts_with("section ")) {
            return 0;
        }

        return 1;
    }

    [[nodiscard]] auto is_label_text(const std::string_view text) const
        -> bool override {

        const std::string_view code{code_part(text)};

        return not code.empty() and code.back() == ':';
    }

    [[nodiscard]] static auto is_comment_or_blank(const std::string_view text)
        -> bool {

        const size_t first{text.find_first_not_of(" \t\n\r\f\v")};

        return first == std::string_view::npos or text[first] == ';';
    }

    [[nodiscard]] static auto code_part(const std::string_view text)
        -> std::string_view {

        const std::string_view code{text.substr(0, text.find(';'))};
        const size_t first{code.find_first_not_of(" \t")};
        if (first == std::string_view::npos) {
            return {};
        }

        return code.substr(first, code.find_last_not_of(" \t") - first + 1);
    }
};
