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
#include "type.hpp"
#include "ub_unset_var.hpp"
#include "unary_ops.hpp"

class toc;
class machine;

class statement {
    token token_;
    unary_ops uops_;
    const type* type_{};

    [[nodiscard]] static auto is_ascii_space(const char ch) -> bool {
        return ch == ' ' or ch == '\t' or ch == '\n' or ch == '\r' or
               ch == '\f' or ch == '\v';
    }

    // one line for an assembler comment: whitespace and source comments
    // become single spaces, string and character literals are kept as written
    [[nodiscard]] static auto collapse_whitespace(const std::string_view text)
        -> std::string {

        std::string out;
        out.reserve(text.size());

        bool pending_space{};
        bool in_comment{};
        bool escaped{};

        // the quote of the string or character literal being copied
        char quote{};

        for (const char ch : text) {
            if (in_comment) {
                in_comment = ch != '\n';
                continue;
            }

            // a '#' inside a string or character literal does not start a
            // comment
            if (quote != '\0') {
                out.push_back(ch);
                if (not escaped and ch == quote) {
                    quote = '\0';
                }
                escaped = not escaped and ch == '\\';
                continue;
            }

            if (ch == '#') {
                in_comment = true;
                pending_space = true;
                continue;
            }

            if (is_ascii_space(ch)) {
                pending_space = true;
                continue;
            }

            // leading and trailing whitespace is dropped
            if (pending_space and not out.empty()) {
                out.push_back(' ');
            }
            pending_space = false;

            out.push_back(ch);
            if (ch == '"' or ch == '\'') {
                quote = ch;
            }
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

    // reports every read of 'var' in this statement
    virtual auto visit_reads(const std::string_view var,
                             const read_visitor reader) const -> void {

        if (identifier() == var) {
            reader(token_, identifier(), std::nullopt);
        }
    }

    // used in UB check
    // throws if 'var' is read outside of the 'assigned' bytes
    auto assert_var_not_used(const std::string_view var,
                             const field_coverage& assigned) const -> void {

        visit_reads(
            var,
            [&var, &assigned](
                const token& use_tk,
                [[maybe_unused]] const std::string_view read_text,
                const std::optional<field_coverage::range>& accessed) -> void {
                const bool is_assigned{accessed ? assigned.covers(*accessed)
                                                : assigned.is_full()};

                if (is_assigned) {
                    return;
                }

                throw_uninitialized(use_tk, var);
            });
    }

    [[nodiscard]] auto reads_var(const std::string_view var) const -> bool {
        bool is_read{};

        visit_reads(
            var,
            [&is_read](
                [[maybe_unused]] const token& use_tk,
                [[maybe_unused]] const std::string_view read_text,
                [[maybe_unused]] const std::optional<field_coverage::range>&
                    accessed) -> void { is_read = true; });

        return is_read;
    }

    // used in UB check
    // checks the reads of 'flow.var' and records its assignments
    virtual auto trace_assignment(assignment_flow& flow) const -> void {
        assert_var_not_used(flow.var, flow.assigned);
    }

    // throws if the value would silently lose bits when stored as 'dst_type'
    virtual auto
    assert_not_narrowed([[maybe_unused]] const toc& tc,
                        [[maybe_unused]] const type& dst_type) const -> void {}

    // true when computing at a narrower width gives the same low bits
    [[nodiscard]] virtual auto keeps_low_bits_when_narrowed() const -> bool {
        return false;
    }

  protected:
    [[nodiscard]] static auto fits_size_bytes(const int64_t value,
                                              const size_t size_bytes) -> bool {

        switch (size_bytes) {
        case sizeof(int8_t):
            return std::in_range<int8_t>(value);

        case sizeof(int16_t):
            return std::in_range<int16_t>(value);

        case sizeof(int32_t):
            return std::in_range<int32_t>(value);

        default:
            return true;
        }
    }

    [[noreturn]] static auto throw_narrowed(const token& src_loc_tk,
                                            const std::string_view source,
                                            const type& src_type,
                                            const type& dst_type) -> void {

        throw compiler_exception{
            src_loc_tk,
            std::format("'{}' of type '{}' is narrowed to '{}', use '{}(...)'",
                        source, src_type.name(), dst_type.name(),
                        dst_type.name())};
    }

    // for statements whose value has the statement's own type
    auto assert_own_type_not_narrowed(const type& dst_type) const -> void {
        if (not dst_type.is_builtin() or not get_type().is_builtin() or
            get_type().size_bytes() <= dst_type.size_bytes()) {

            return;
        }

        throw_narrowed(tok(), trimmed_source(*this), get_type(), dst_type);
    }

    [[noreturn]] static auto throw_uninitialized(const token& use_tk,
                                                 const std::string_view var)
        -> void {

        throw compiler_exception{
            use_tk, std::format("use of uninitialized variable '{}'", var)};
    }
};
