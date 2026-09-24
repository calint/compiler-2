#pragma once

#include <cassert>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_arith.hpp"
#include "expression.hpp"
#include "toc.hpp"
#include "unary_ops.hpp"

// e.g. 'i8(x)' states that narrowing 'x' is intended, the store truncates
class stmt_builtin_narrow final : public expression {
    token open_paren_tk_;
    expr_arith arg_;
    token close_paren_tk_;
    std::string folded_; // a literal argument, narrowed and with unary ops

  public:
    stmt_builtin_narrow(toc& tc, unary_ops uops, const token tk, tokenizer& tz)
        : expression{tk, std::move(uops)},
          open_paren_tk_{tz.is_next_char_token('(')} {

        set_type(tc.get_type_or_throw(tk, tk.text()));

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
            toc::parse_constant(tk, arg_.identifier())};

        if (value) {
            folded_ = std::format("{}", narrow(*value));
        }
    }

    stmt_builtin_narrow() = default;

    [[nodiscard]] static auto is_builtin_name(const std::string_view name)
        -> bool {

        return name == "i8" or name == "i16" or name == "i32" or name == "i64";
    }

    auto source_to(std::ostream& os) const -> void override {
        expression::source_to(os);
        open_paren_tk_.source_to(os);
        arg_.source_to(os);
        close_paren_tk_.source_to(os);
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        return folded_.empty();
    }

    // a folded literal is a constant, and constants are identifiers
    [[nodiscard]] auto is_identifier() const -> bool override {
        return not folded_.empty();
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        assert(not folded_.empty());

        return folded_;
    }

    // the unary ops are part of 'folded_'
    [[nodiscard]] auto get_unary_ops() const -> const unary_ops& override {
        static const unary_ops none{};

        if (not folded_.empty()) {
            return none;
        }

        return expression::get_unary_ops();
    }

    // the builtin computes its own width before the destination narrows it
    [[nodiscard]] auto keeps_low_bits_when_narrowed() const -> bool override {
        return true;
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

            arg_.compile(tc, indent, dst_info);
            get_unary_ops().compile(tc, indent, dst_info.operand);

            return;
        }

        // '/', '%', '>>' and calls are computed at the argument's width, a
        // call's result must also have its declared type
        const operand wide{
            x.alloc_scratch_register(tok(), indent, arg_.get_type())};

        arg_.compile(tc, indent + 1, toc::make_ident_info_from_register(wide));
        x.copy_value(tok(), indent, dst_info.operand, wide);
        x.free_scratch_register(tok(), indent, wide);

        get_unary_ops().compile(tc, indent, dst_info.operand);
    }

    // only the builtin's own type counts, its argument is narrowed on purpose
    auto assert_not_narrowed([[maybe_unused]] const toc& tc,
                             const type& dst_type) const -> void override {

        assert_own_type_not_narrowed(dst_type);
    }

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

        arg_.visit_reads(var, reader);
    }

  private:
    [[nodiscard]] auto constant_value(const toc& tc) const
        -> std::optional<int64_t> {

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
        const int64_t result{expression::get_unary_ops().evaluate_constant(
            arg_.get_unary_ops().evaluate_constant(value))};

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
