#pragma once
// reviewed: 2025-09-28

#include <format>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "token.hpp"
#include "unary_ops.hpp"

class toc;
class type;
class x86;

class statement {
    token token_;
    unary_ops uops_;
    const type* type_{};

    [[nodiscard]] static auto is_ascii_space(const char ch) -> bool {
        return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' ||
               ch == '\f' || ch == '\v';
    }

    [[nodiscard]] static auto collapse_whitespace(const std::string_view text)
        -> std::string {

        if (text.empty()) {
            return {};
        }

        size_t start{};
        while (start < text.size() and is_ascii_space(text[start])) {
            ++start;
        }
        if (start == text.size()) {
            return {};
        }

        std::string out;
        out.reserve(text.size() - start);

        bool in_whitespace{};

        for (size_t i{start}; i < text.size(); ++i) {
            const char ch{text[i]};
            if (is_ascii_space(ch)) {
                if (in_whitespace) {
                    continue;
                }
                out.push_back(' ');
                in_whitespace = true;
                continue;
            }

            out.push_back(ch);
            in_whitespace = false;
        }

        if (not out.empty() and out.back() == ' ') {
            out.pop_back();
        }

        return out;
    }

  public:
    explicit statement(token tk, unary_ops uops = {})
        : token_{tk}, uops_{std::move(uops)} {}

    virtual ~statement() = default;

    statement() = default;
    statement(const statement&) = default;
    statement(statement&&) = default;
    auto operator=(statement&&) -> statement& = default;
    auto operator=(statement const&) -> statement& = default;

    virtual auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] x86& x,
                         [[maybe_unused]] const size_t indent,
                         [[maybe_unused]] const ident_info& ident_info) const
        -> void {

        std::unreachable();
    }

    virtual auto source_to(std::ostream& os) const -> void {
        uops_.source_to(os);
        token_.source_to(os);
    }

    // one-line, whitespace-collapsed rendering of 'st's source, suitable for
    // an assembler comment
    [[nodiscard]] static auto trimmed_source(const std::string_view text)
        -> std::string {
        return collapse_whitespace(text);
    }

    // one-line, whitespace-collapsed rendering of 'st's source, suitable for
    // an assembler comment
    [[nodiscard]] static auto trimmed_source(const statement& st)
        -> std::string {

        std::stringstream ss;
        st.source_to(ss);
        return collapse_whitespace(ss.str());
    }

    // same as above, with a "'dst' 'op' " prefix before the rendered source
    [[nodiscard]] static auto trimmed_source(const statement& st,
                                             const std::string_view dst,
                                             const std::string_view op)
        -> std::string {
        std::stringstream ss;
        std::print(ss, "{} {} ", dst, op);
        st.source_to(ss);
        return collapse_whitespace(ss.str());
    }

    [[nodiscard]] virtual auto tok() const -> const token& { return token_; }

    auto set_type(const type& tp) -> void { type_ = &tp; }

    [[nodiscard]] auto get_type() const -> const type& {
        assert(type_ != nullptr);
        return *type_;
    }

    [[nodiscard]] virtual auto is_expression() const -> bool { return false; }

    [[nodiscard]] virtual auto identifier() const -> std::string_view {
        return token_.text();
    }

    [[nodiscard]] virtual auto get_unary_ops() const -> const unary_ops& {
        return uops_;
    }

    [[nodiscard]] virtual auto is_identifier() const -> bool { return false; }

    [[nodiscard]] virtual auto is_indexed() const -> bool { return false; }

    virtual auto compile_data([[maybe_unused]] const toc& tc,
                              [[maybe_unused]] x86& x) const -> void {}

    [[nodiscard]] virtual auto dat_size_bytes() const -> size_t { return 0; }

    [[nodiscard]] virtual auto compile_lea(
        [[maybe_unused]] toc& tc, [[maybe_unused]] x86& x,
        [[maybe_unused]] const size_t indent,
        [[maybe_unused]] const token& src_loc_tk,
        [[maybe_unused]] std::vector<std::string>& allocated_registers,
        [[maybe_unused]] const std::string& reg_size,
        [[maybe_unused]] const std::span<const std::string> lea_path) const
        -> operand {

        std::unreachable();
    }

    // used in UB check
    // returns true if 'var' is set in this statement
    [[nodiscard]] virtual auto
    is_var_set([[maybe_unused]] const std::string_view var) const -> bool {
        return false;
    }

    // used in UB check
    // throws if 'var' is used in this statement
    virtual auto assert_var_not_used(const std::string_view var) const -> void {
        if (identifier() == var) {
            throw compiler_exception{
                token_, std::format("use of uninitialized variable '{}'",
                                    identifier())};
        }
    }

    // used in UB check
    // returns true if code after this statement is considered "dead code"
    // applies to: 'return', 'break', 'continue'
    [[nodiscard]] virtual auto is_code_after_this_unreachable() const -> bool {
        return false;
    }
};
