#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_arith.hpp"
#include "expression.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_unset_var.hpp"
#include "unary_ops.hpp"

// e.g. 'i8(x)' converts 'x' to the type, a narrowing store truncates on
// purpose; 'int(x)' converts to the default type of the target
class stmt_builtin_convert final : public expression {
    token open_paren_tk_;
    expr_arith arg_;
    token close_paren_tk_;
    std::string folded_; // a literal argument, narrowed and with unary ops

  public:
    stmt_builtin_convert(toc& tc, unary_ops uops, const token tk, tokenizer& tz)
        : expression{tk, std::move(uops)},
          open_paren_tk_{tz.is_next_char_token('(')} {

        set_type(conversion_type(tc, tk));

        arg_ = {tc, tz, true};

        close_paren_tk_ = tz.is_next_char_token(')');

        if (close_paren_tk_.is_empty()) {
            throw compiler_exception{tz, "expected ')' after the argument"};
        }

        // a folded literal takes every constant path, e.g. inline arguments
        if (arg_.is_expression()) {
            return;
        }

        const std::optional<int64_t> value{
            constant_parser::parse_constant(tk, arg_.identifier()),
        };

        if (value) {
            folded_ = std::format("{}", narrow(*value));
        }
    }

    // e.g. 'i8' in 'var x = i8' is 'i8(0)' and 'bool' in 'var b = bool' is
    // 'false'
    stmt_builtin_convert(const toc& tc, const token tk)
        : expression{tk, {}}, folded_(tk.is_text("bool") ? "false" : "0") {

        set_type(conversion_type(tc, tk));
    }

    stmt_builtin_convert() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        expression::source_to(os);

        // the bare form has no argument
        if (open_paren_tk_.is_empty()) {
            return;
        }

        open_paren_tk_.source_to(os);
        arg_.source_to(os);
        close_paren_tk_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        // an out-of-range immediate makes nasm warn, so fold it here
        if (const std::optional<int64_t> value{constant_value(tc)}; value) {
            x.copy_value(tok(), indent, dst_info.operand,
                         operand::imm(std::format("{}", *value), get_type()));

            return;
        }

        // the store at the narrow width does the truncation, and an argument
        // of the same width computes the same either way
        if (arg_.keeps_low_bits_when_narrowed() or
            arg_.get_type().size_bytes() == get_type().size_bytes()) {

            if (dst_info.is_register()) {
                compile_in_destination(tc, indent, dst_info);
                return;
            }

            // a load/store machine operates on the memory destination with a
            // load and a store each time, a wide register needs one store
            x.emit_most_efficient(
                tok(), indent,
                [&] -> void { compile_in_destination(tc, indent, dst_info); },
                [&] -> void { compile_wide(tc, indent, dst_info); });

            return;
        }

        // '/', '%', '>>' and calls are computed at the argument's width, a
        // call's result must also have its declared type
        compile_wide(tc, indent, dst_info);
    }

    // only the builtin's own type counts, its argument is narrowed on purpose
    auto assert_not_narrowed([[maybe_unused]] const toc& tc,
                             const type& dst_type) const -> void override {

        assert_own_type_not_narrowed(dst_type);
    }

    // the unary ops are part of 'folded_'
    [[nodiscard]] auto get_unary_ops() const -> const unary_ops& override {
        static const unary_ops none{};

        if (not folded_.empty()) {
            return none;
        }

        return expression::get_unary_ops();
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        assert(not folded_.empty());

        return folded_;
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        return folded_.empty();
    }

    // a folded literal is a constant, and constants are identifiers
    [[nodiscard]] auto is_identifier() const -> bool override {
        return not folded_.empty();
    }

    // the builtin computes its own width before the destination narrows it
    [[nodiscard]] auto keeps_low_bits_when_narrowed() const -> bool override {
        return true;
    }

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

        arg_.visit_reads(var, reader);
    }

    //
    // statics
    //

    // the default type has no name of its own, e.g. 'i64' on x86_64 and 'i32'
    // on rv32i
    [[nodiscard]] static auto conversion_type(const toc& tc, const token& tk)
        -> const type& {

        if (tk.is_text("int")) {
            return tc.get_type_default();
        }

        return tc.get_type_or_throw(tk, tk.text());
    }

  private:
    auto compile_in_destination(toc& tc, const size_t indent,
                                const ident_info& dst_info) const -> void {

        arg_.compile(tc, indent, dst_info);
        get_unary_ops().compile(tc, indent, tok(), dst_info.operand);
    }

    // the argument is computed at its own width and stored narrowed
    auto compile_wide(toc& tc, const size_t indent,
                      const ident_info& dst_info) const -> void {

        machine& x{tc.machine()};

        const operand wide{
            x.alloc_scratch_register(tok(), indent, arg_.get_type()),
        };

        arg_.compile(tc, indent + 1, toc::make_ident_info_from_register(wide));

        // '-' and '~' give the same low bits at any width, so the one store
        // also narrows their result
        get_unary_ops().compile(tc, indent, tok(), wide);

        x.copy_value(tok(), indent, dst_info.operand, wide);
        x.free_scratch_register(tok(), indent, wide);
    }

    [[nodiscard]] auto constant_value(const toc& tc) const
        -> std::optional<int64_t> {

        // the bare form has no argument
        if (open_paren_tk_.is_empty()) {
            return 0;
        }

        if (arg_.is_expression()) {
            return std::nullopt;
        }

        const ident_info info{tc.make_ident_info(arg_)};

        if (not info.is_const()) {
            return std::nullopt;
        }

        return narrow(info.const_value);
    }

    // applies the unary ops at the argument's width before narrowing
    [[nodiscard]] auto narrow(const int64_t value) const -> int64_t {
        const int64_t result{
            expression::get_unary_ops().evaluate_constant(
                arg_.get_unary_ops().evaluate_constant(value)),
        };

        switch (get_type().size_bytes()) {
        case sizeof(int8_t):
            return static_cast<int8_t>(result);

        case sizeof(int16_t):
            return static_cast<int16_t>(result);

        case sizeof(int32_t):
            return static_cast<int32_t>(result);

        default:
            return result;
        }
    }
};
