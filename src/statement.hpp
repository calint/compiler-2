#pragma once
// reviewed: 2025-09-28

#include <format>
#include <ranges>
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
class machine;

class statement {
    token token_;
    unary_ops uops_;
    const type* type_{};

    [[nodiscard]] static auto is_ascii_space(const char ch) -> bool {
        return ch == ' ' or ch == '\t' or ch == '\n' or ch == '\r' or
               ch == '\f' or ch == '\v';
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

        for (const char ch : text | std::views::drop(start)) {
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
    explicit statement(const token tk, unary_ops uops = {})
        : token_{tk}, uops_{std::move(uops)} {}

    virtual ~statement() = default;

    statement() = default;
    statement(const statement&) = default;
    statement(statement&&) = default;
    auto operator=(statement&&) -> statement& = default;
    auto operator=(const statement&) -> statement& = default;

    virtual auto compile([[maybe_unused]] toc& tc,
                         [[maybe_unused]] const size_t indent,
                         [[maybe_unused]] const ident_info& dst_info) const
        -> void {

        std::unreachable();
    }

    [[nodiscard]] virtual auto is_array_element() const -> bool {
        return false;
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

        return collapse_whitespace(ss.view());
    }

    // same as above, with a "'dst' 'op' " prefix before the rendered source
    [[nodiscard]] static auto trimmed_source(const statement& st,
                                             const std::string_view dst,
                                             const std::string_view op)
        -> std::string {

        std::stringstream ss;
        std::print(ss, "{} {} ", dst, op);
        st.source_to(ss);

        return collapse_whitespace(ss.view());
    }

    [[nodiscard]] virtual auto tok() const -> const token& { return token_; }

    auto set_type(const type& tp) -> void { type_ = &tp; }

    [[nodiscard]] auto get_type() const -> const type& {
        assert(type_ != nullptr);

        return *type_;
    }

    [[nodiscard]] virtual auto is_expression() const -> bool { return false; }

    [[nodiscard]] virtual auto produces_boolean() const -> bool {
        return false;
    }

    virtual auto compile_boolean([[maybe_unused]] toc& tc,
                                 [[maybe_unused]] const size_t indent,
                                 [[maybe_unused]] const operand& dst,
                                 [[maybe_unused]] const bool inverted) const
        -> void {

        std::unreachable();
    }

    [[nodiscard]] virtual auto identifier() const -> std::string_view {
        return token_.text();
    }

    [[nodiscard]] virtual auto get_unary_ops() const -> const unary_ops& {
        return uops_;
    }

    [[nodiscard]] auto make_constant_operand(const ident_info& info) const
        -> operand {

        assert(info.is_const());

        return operand::imm(
            std::format("{}{}", get_unary_ops().to_string(), info.const_value),
            info.type_ref());
    }

    [[nodiscard]] virtual auto is_identifier() const -> bool { return false; }

    [[nodiscard]] virtual auto is_indexed() const -> bool { return false; }

    virtual auto compile_data([[maybe_unused]] toc& tc) const -> void {}

    [[nodiscard]] virtual auto dat_size_bytes() const -> size_t { return 0; }

    [[nodiscard]] virtual auto
    compile_lea([[maybe_unused]] toc& tc, [[maybe_unused]] const size_t indent,
                [[maybe_unused]] const token& src_loc_tk,
                [[maybe_unused]] std::vector<operand>& allocated_registers,
                [[maybe_unused]] const operand& reg_count,
                [[maybe_unused]] const std::span<const operand> lea_path,
                [[maybe_unused]] const operand& address_register) const
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
    // returns true if a 'return' may be reached before 'var' is set
    [[nodiscard]] virtual auto
    may_return_unset([[maybe_unused]] const std::string_view var) const
        -> bool {

        return false;
    }

    // used in UB check
    // returns true if a 'break' of the enclosing loop may be reached before
    // 'var' is set
    [[nodiscard]] virtual auto
    may_break_unset([[maybe_unused]] const std::string_view var) const -> bool {

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
