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

#include "compile_error.hpp"
#include "constant_arith.hpp"
#include "constant_parser.hpp"
#include "expr_arith.hpp"
#include "expression.hpp"
#include "ident_info.hpp"
#include "operand.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_reads.hpp"
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

        arg_ = {tc, tz};

        if (not arg_.get_type().is_builtin()) {
            throw compile_error{
                arg_.tok(),
                std::format("cannot convert a value of type '{}'",
                            arg_.get_type().name()),
            };
        }

        close_paren_tk_ =
            tz.expect_char_token(')', "expected ')' after the argument");

        // a folded literal takes every constant path, e.g. inline arguments
        if (arg_.is_expression()) {
            return;
        }

        const std::optional<int64_t> value{
            constant_parser::parse_constant(tk, arg_.name_or_literal()),
        };

        if (value) {
            folded_ = std::format("{}", narrow(tc, *value));
        }
    }

    // e.g. 'i8' in 'var x = i8' is 'i8(0)'
    stmt_builtin_convert(const toc& tc, const token tk)
        : expression{tk, {}}, folded_{tk.is_text("bool") ? "false" : "0"} {

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

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        // the overflow check applies to the constant, not to the argument
        const std::optional<int64_t> value{constant_value(tc)};

        const toc::overflow_check_pause pause{tc};

        // an out-of-range immediate makes nasm warn, so fold it here
        if (value) {
            x.copy_value(tok(), indent, dst.operand(),
                         operand::imm(*value, get_type()));

            return;
        }

        // a wider destination would compute the argument at its own width and
        // keep the bits that the conversion drops
        if (dst.type_ref().size_bytes() > get_type().size_bytes()) {
            compile_narrowed(tc, indent, dst);
            return;
        }

        // the destination is at most as wide as the type, the store at its
        // width does the truncation; that is right when the low bits of the
        // argument do not depend on its high bits or nothing is narrowed
        const bool is_same_width{
            dst.type_ref().size_bytes() == get_type().size_bytes() and
                arg_.get_type().size_bytes() == get_type().size_bytes(),
        };

        // computed in the destination, the argument is at its width
        const bool is_narrowed{
            dst.type_ref().size_bytes() < arg_.get_type().size_bytes(),
        };

        const bool keeps_low_bits{
            is_narrowed ? arg_.keeps_low_bits_when_narrowed()
                        : arg_.keeps_low_bits_when_widened(),
        };

        // a shift count is checked against the width the shift is computed
        // at, which a narrower destination would lower
        const bool narrows_checked_shift{tc.checks().shift and is_narrowed};

        if ((keeps_low_bits or is_same_width) and not narrows_checked_shift) {

            // a narrower argument wraps at the width of the destination, a
            // register of the argument's width would wrap earlier
            if (dst.is_register() or
                arg_.get_type().size_bytes() < dst.type_ref().size_bytes()) {

                compile_in_destination(tc, indent, dst);
                return;
            }

            // a load/store machine operates on the memory destination with a
            // load and a store each time, a wide register needs one store
            tc.emit_most_efficient(
                tok(), indent,
                [&] -> void { compile_in_destination(tc, indent, dst); },
                [&] -> void { compile_wide(tc, indent, dst); });

            return;
        }

        // '/', '%', '>>' and calls are computed at the argument's width, a
        // call's result must also have its declared type
        compile_wide(tc, indent, dst);
    }

    // only the builtin's own type counts, its argument is narrowed on purpose
    auto assert_not_narrowed([[maybe_unused]] const toc& tc,
                             const type& dst_type) const -> void override {

        assert_own_type_not_narrowed(dst_type);
    }

    // a narrower width stays unfolded, 'assert_not_narrowed' rejects it
    [[nodiscard]] auto folded_constant(const toc& tc,
                                       const type& width_type) const
        -> std::optional<int64_t> override {

        if (width_type.size_bytes() < get_type().size_bytes()) {
            return std::nullopt;
        }

        return constant_value(tc);
    }

    // the unary ops are part of 'folded_'
    [[nodiscard]] auto get_unary_ops() const -> const unary_ops& override {
        static const unary_ops none{};

        if (not folded_.empty()) {
            return none;
        }

        return expression::get_unary_ops();
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        return folded_.empty();
    }

    // a folded literal is a constant, and constants are identifiers
    [[nodiscard]] auto is_name_or_literal() const -> bool override {
        return not folded_.empty();
    }

    // the builtin computes its own width before the destination narrows it
    [[nodiscard]] auto keeps_low_bits_when_narrowed() const -> bool override {
        return true;
    }

    [[nodiscard]] auto keeps_low_bits_when_widened() const -> bool override {
        return true;
    }

    [[nodiscard]] auto name_or_literal() const -> std::string_view override {
        assert(not folded_.empty());

        return folded_;
    }

    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

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
            return tc.builtins().default_type();
        }

        return tc.get_type_or_throw(tk, tk.text());
    }

  private:
    auto compile_in_destination(toc& tc, const size_t indent,
                                const destination& dst) const -> void {

        arg_.compile(tc, indent, dst);

        machine& x{tc.machine()};

        get_unary_ops().compile(x, indent, tok(), dst.operand(),
                                tc.checks().overflow_options());
    }

    // the value is made at the width of the type, then widens into the
    // destination
    auto compile_narrowed(toc& tc, const size_t indent,
                          const destination& dst) const -> void {

        machine& x{tc.machine()};

        const operand narrowed{
            x.alloc_scratch_register(tok(), indent, get_type()),
        };

        compile(tc, indent, destination::of_register(narrowed));

        x.copy_value(tok(), indent, dst.operand(), narrowed);
        x.free_scratch_register(tok(), indent, narrowed);
    }

    // the argument is computed at its own width and stored narrowed
    auto compile_wide(toc& tc, const size_t indent,
                      const destination& dst) const -> void {

        machine& x{tc.machine()};

        const operand wide{
            x.alloc_scratch_register(tok(), indent, arg_.get_type()),
        };

        arg_.compile(tc, indent + 1, destination::of_register(wide));

        // '-' and '~' give the same low bits at any width, so the one store
        // also narrows their result
        get_unary_ops().compile(x, indent, tok(), wide,
                                tc.checks().overflow_options());

        x.copy_value(tok(), indent, dst.operand(), wide);
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

        return narrow(tc, info.const_value());
    }

    // the unary ops of the argument apply at its width, the value is narrowed
    // and the unary ops of the conversion apply to the narrowed value, under
    // the overflow check a result that does not fit the type is an error
    [[nodiscard]] auto narrow(const toc& tc, const int64_t value) const
        -> int64_t {

        const int64_t converted{
            constant_arith::wrap_to_width(
                arg_.get_unary_ops().evaluate_constant(value), get_type()),
        };

        const int64_t result{
            expression::get_unary_ops().evaluate_constant(converted),
        };

        const int64_t narrowed{
            constant_arith::wrap_to_width(result, get_type()),
        };

        const int64_t minimum{
            constant_arith::signed_minimum(get_type().size_bits()),
        };

        if (tc.checks().overflow and
            expression::get_unary_ops().negation_overflows(converted,
                                                           minimum)) {

            throw compile_error{tok(),
                                "constant expression overflows the type"};
        }

        return narrowed;
    }
};
