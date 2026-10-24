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
//
// 'T(arg)' behaves like a temporary of type T that holds 'arg' computed at W
// and wrapped to T. W is the wider of T and the type of the argument, which is
// that of its first element. The argument follows the rule of a list at W: a
// wider element is rejected as narrowed, '+', '-' and '*' wrap (it is
// compiled into a wrapping destination, a call in it is checked as anywhere
// else) and its own unary ops apply at W. The conversion's own '-' and '~'
// apply where the value is used, at the width D of the destination.
//
// where the conversion's own unary ops apply, 'ops_place_for', the first row
// that matches:
//   widened          D > T, no ops: made in a register of T, then widened
//                    into the destination
//   after_widening   D > T, ops: made at T in the destination register (a
//                    scratch one for memory), widened in place, the ops
//                    applied at D
//   checked_at_type  D == T, ops, the overflow check: in a register of T,
//                    checked there, then copied
//   fused            otherwise (D <= T): with the argument, at the width of
//                    its computation below
//
// where the argument is computed for a target 't' at most T wide,
// 'plan_computation', the first row that matches:
//   1   a list narrower than T: at T, in 't' when it is as wide as T, else
//       in a scratch register of T
//   2   't' has the argument's type or the argument keeps its low bits, and
//       no checked shift is narrowed: in 't' when it is a register or wider
//       than the argument, else measured against a scratch register of the
//       argument's type
//   3a  an argument at least as wide as T, or a single operand without unary
//       ops: in a scratch register of the argument's type
//   3b  a narrower single operand with unary ops, e.g. 'i16(-g())': as row 1
class stmt_builtin_convert final : public expression {
    token open_paren_tk_;
    expr_arith arg_;
    token close_paren_tk_;

    // where the conversion's own unary ops apply, see the class comment
    enum class ops_place : uint8_t {
        fused,
        checked_at_type,
        widened,
        after_widening,
    };

    // where the argument is computed for a target at most as wide as the type
    struct computation_plan {
        enum class place : uint8_t {
            // the target, at its width
            destination,
            // a register of 'scratch_type', then copied into the target
            scratch,
            // the shorter of 'destination' and 'scratch'
            measured,
        };

        place where{place::destination};
        // null for 'destination'
        const type* scratch_type{};
    };

  public:
    stmt_builtin_convert(toc& tc, unary_ops uops, const token tk, tokenizer& tz)
        : expression{tk, std::move(uops)},
          open_paren_tk_{tz.is_next_char_token('(')} {

        set_type(tc.get_type_or_throw(tk));

        arg_ = {tc, tz};

        if (not arg_.get_type().is_builtin()) {
            throw compile_error{
                arg_.tok(),
                std::format("cannot convert a value of type {}",
                            in_quotes(arg_.get_type().name())),
            };
        }

        // e.g. the '>' of 'int(a > 2)', a 'bool' converts but a condition
        // is stored first
        expr_arith::assert_no_condition_next(tz);

        close_paren_tk_ =
            tz.expect_char_token(')', "expected ')' after the argument");
    }

    // e.g. 'i8' in 'var x = i8' is 'i8(0)'
    stmt_builtin_convert(const toc& tc, const token tk) : expression{tk, {}} {
        set_type(tc.get_type_or_throw(tk));
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

        // note: a list copies a constant itself
        assert(not fold(tc, tc.use_of(dst)));

        // the argument follows the rule of a list at the width it is computed
        // at, e.g. the 'i16' 'b' of 'i8(a + b)' is narrowed to 'i8'
        arg_.assert_not_narrowed(tc, width_type());

        const ops_place place{ops_place_for(tc, dst)};

        if (place == ops_place::widened or
            place == ops_place::checked_at_type) {

            compile_in_type_register(tc, indent, dst);
            return;
        }

        if (place == ops_place::after_widening) {
            compile_after_widening(tc, indent, dst);
            return;
        }

        compile_argument(tc, indent, dst, get_unary_ops());
    }

    // only the builtin's own type counts, its argument is narrowed on purpose
    auto assert_not_narrowed([[maybe_unused]] const toc& tc,
                             const type& dst_type) const -> void override {

        assert_own_type_not_narrowed(dst_type);
    }

    // a constant of the type, which wraps at the width of its use: the
    // argument folded like a list at W, where it wraps, narrowed to the type,
    // then the conversion's own unary ops apply at the width it is used at,
    // at least that of the type, e.g. 'i16' for '-i8(-128)' in an 'i16' list,
    // where under the overflow check a result that does not fit is an error
    [[nodiscard]] auto fold(const toc& tc, const value_use& use) const
        -> std::optional<folded_value> override {

        // the bare form has no argument, e.g. the 'i8' of 'var x = i8'
        if (open_paren_tk_.is_empty()) {
            return folded_value{
                .value{},
                .own_type{&get_type()},
            };
        }

        const value_use arg_use{
            .width{&width_type()},
            .checks_overflow{false},
        };

        const std::optional<folded_value> arg{arg_.fold(tc, arg_use)};

        if (not arg) {
            return std::nullopt;
        }

        // note: an untyped constant converts whole, e.g. 'i32(0xfedc1234)' on
        //       a target whose default type is 'i32', a list is a list at W

        if (arg->is_typed() or not arg_.is_single_operand()) {
            arg_.assert_not_narrowed(tc, width_type());
        }

        const int64_t converted{
            constant_arith::wrap_to_width(arg->value, get_type()),
        };

        const type& use_width{use.width_ref()};
        const type& result_type{*wider(get_type(), use_width)};

        const int64_t value{
            get_unary_ops().evaluate_at(converted, result_type,
                                        use.checks_overflow),
        };

        // a narrower width keeps the low bits, e.g. 44 of the 'i16' 300 of
        // 'i8(i16(k))'
        return folded_value{
            .value{constant_arith::wrap_to_width(value, use_width)},
            .own_type{&get_type()},
        };
    }

    // the builtin computes its own width before the destination narrows it
    [[nodiscard]] auto
    keeps_low_bits([[maybe_unused]] const width_change change) const
        -> bool override {

        return true;
    }

    auto visit_call_effects(const toc& tc,
                            const call_effect_visitor visitor) const
        -> void override {

        arg_.visit_call_effects(tc, visitor);
    }

    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        arg_.visit_reads(var, reader);
    }

  private:
    // the value is made in the register at the width of the type, the unary
    // ops apply after it widens in place; a memory destination gets it from a
    // scratch register
    auto compile_after_widening(toc& tc, const size_t indent,
                                const destination& dst) const -> void {

        machine& x{tc.machine()};

        const operand converted{
            dst.is_register()
                ? x.register_view(dst.operand(), get_type())
                : x.alloc_scratch_register(tok(), indent, get_type()),
        };

        compile_at_type(tc, indent, converted);

        const operand widened{x.register_view(converted, dst.type_ref())};

        x.copy_value(tok(), indent, widened, converted);

        get_unary_ops().compile(x, indent, tok(), widened,
                                tc.overflow_options(dst));

        if (dst.is_register()) {
            return;
        }

        x.copy_value(tok(), indent, dst.operand(), widened);
        x.free_scratch_register(tok(), indent, converted);
    }

    // the arithmetic of the argument wraps, 'uops' apply to the converted
    // value as the target checks them; the argument is an operand, so a
    // narrower call result widens into a wider register, e.g. 'i16((g()))'
    auto compile_argument(toc& tc, const size_t indent,
                          const destination& target,
                          const unary_ops& uops) const -> void {

        const machine::runtime_check uops_check{tc.overflow_options(target)};

        const auto in_destination{
            [&] -> void {
                arg_.compile(tc, indent, target.as_wrapping().as_operand());

                machine& x{tc.machine()};

                uops.compile(x, indent, tok(), target.operand(), uops_check);
            },
        };

        const computation_plan plan{plan_computation(tc, target)};

        const auto in_scratch{
            [&] -> void {
                machine& x{tc.machine()};

                const operand wide{
                    x.alloc_scratch_register(tok(), indent, *plan.scratch_type),
                };

                arg_.compile(
                    tc, indent,
                    destination::of_register(wide).as_wrapping().as_operand());

                // a 'bool' is not negated, e.g. the '-' of '-i8(b)' applies
                // to the copy
                if (plan.scratch_type->is_bool()) {
                    x.copy_value(tok(), indent, target.operand(), wide);
                    x.free_scratch_register(tok(), indent, wide);

                    uops.compile(x, indent, tok(), target.operand(),
                                 uops_check);

                    return;
                }

                // a narrower register widens in place first, so the ops apply
                // at the width of the target, e.g. the '-' of '-i16(g(v))' of
                // an 'i8' -128 gives 128
                if (not uops.is_empty() and
                    plan.scratch_type->size_bytes() <
                        target.type_ref().size_bytes()) {

                    const operand widened{
                        x.register_view(wide, target.type_ref()),
                    };

                    x.copy_value(tok(), indent, widened, wide);

                    uops.compile(x, indent, tok(), widened, uops_check);

                    x.copy_value(tok(), indent, target.operand(), widened);
                    x.free_scratch_register(tok(), indent, wide);

                    return;
                }

                // '-' and '~' give the same low bits at any width, so the one
                // store also narrows their result
                uops.compile(x, indent, tok(), wide, uops_check);

                x.copy_value(tok(), indent, target.operand(), wide);
                x.free_scratch_register(tok(), indent, wide);
            },
        };

        if (plan.where == computation_plan::place::destination) {
            in_destination();
            return;
        }

        if (plan.where == computation_plan::place::scratch) {
            in_scratch();
            return;
        }

        machine& x{tc.machine()};

        // a load/store machine operates on the memory destination with a
        // load and a store each time, a wide register needs one store
        x.emit_most_efficient(tok(), indent, in_destination, in_scratch);
    }

    // the argument is made in 'reg' at the width of the type, where its
    // arithmetic wraps
    auto compile_at_type(toc& tc, const size_t indent, const operand& reg) const
        -> void {

        compile_argument(tc, indent, destination::of_register(reg), {});
    }

    // the value is made in a register of the type, where the unary ops apply,
    // then copied, e.g. '-i8(x)' of an 'x' of 128 overflows 'i8' there; a
    // wider destination widens the copy
    auto compile_in_type_register(toc& tc, const size_t indent,
                                  const destination& dst) const -> void {

        machine& x{tc.machine()};

        const operand converted{
            x.alloc_scratch_register(tok(), indent, get_type()),
        };

        compile_at_type(tc, indent, converted);

        get_unary_ops().compile(x, indent, tok(), converted,
                                tc.overflow_options(dst));

        x.copy_value(tok(), indent, dst.operand(), converted);
        x.free_scratch_register(tok(), indent, converted);
    }

    [[nodiscard]] auto ops_place_for(const toc& tc,
                                     const destination& dst) const
        -> ops_place {

        // a wider destination would compute the argument at its own width and
        // keep the bits that the conversion drops; the unary ops apply where
        // the value is used, e.g. 'w + -i8(v)' of a 'v' of -128 adds 128
        if (dst.type_ref().size_bytes() > get_type().size_bytes()) {
            if (get_unary_ops().is_empty()) {
                return ops_place::widened;
            }

            return ops_place::after_widening;
        }

        // the arithmetic of the argument wraps, the unary ops of the
        // conversion apply to the converted value as the destination checks
        // them
        if (tc.checks_overflow(dst) and not get_unary_ops().is_empty()) {
            // note: a narrower destination is only the argument of an outer
            //       conversion, whose arithmetic wraps

            assert(dst.type_ref().size_bytes() == get_type().size_bytes());

            return ops_place::checked_at_type;
        }

        return ops_place::fused;
    }

    // the argument is computed at the width of the type
    [[nodiscard]] auto plan_at_type(const destination& target) const
        -> computation_plan {

        // e.g. the 'i16' of 'i8(i16(a * (a >> 1)))' is narrowed after it
        if (target.type_ref().size_bytes() < get_type().size_bytes()) {
            return {
                .where{computation_plan::place::scratch},
                .scratch_type{&get_type()},
            };
        }

        return {
            .where{computation_plan::place::destination},
            .scratch_type{},
        };
    }

    [[nodiscard]] auto plan_computation(const toc& tc,
                                        const destination& target) const
        -> computation_plan {

        const size_t type_size_bytes{get_type().size_bytes()};
        const size_t target_size_bytes{target.type_ref().size_bytes()};
        const type& arg_type{arg_.get_type()};

        // a narrower argument is computed at the width of the type, where its
        // arithmetic wraps, e.g. 'i16(a * (a >> 1))' of an 'i8' 'a' of 20 is
        // 200; a single operand keeps its own width, e.g. a call's result
        if (not arg_.is_single_operand() and
            arg_type.size_bytes() < type_size_bytes) {

            return plan_at_type(target);
        }

        // computed in the target, the argument is at its width
        const bool is_narrowed{target_size_bytes < arg_type.size_bytes()};

        const bool keeps_low_bits{
            arg_.keeps_low_bits(is_narrowed ? width_change::narrowed
                                            : width_change::widened),
        };

        // a shift count is checked against the width the shift is computed
        // at, which a narrower target would lower
        const bool narrows_checked_shift{tc.checks().shift and is_narrowed};

        // the target is at most as wide as the type, the store at its width
        // does the truncation; that is right when the low bits of the
        // argument do not depend on its high bits or nothing is converted
        if ((keeps_low_bits or target.type_ref().is_same(arg_type)) and
            not narrows_checked_shift) {

            // a narrower argument wraps at the width of the target, a
            // register of the argument's width would wrap earlier
            if (target.is_register() or
                arg_type.size_bytes() < target_size_bytes) {

                return {
                    .where{computation_plan::place::destination},
                    .scratch_type{},
                };
            }

            return {
                .where{computation_plan::place::measured},
                .scratch_type{&arg_type},
            };
        }

        // '/', '%', '>>' and calls are computed at the argument's width, a
        // call's result must also have its declared type
        if (arg_type.size_bytes() >= type_size_bytes or
            arg_.single_operand().get_unary_ops().is_empty()) {

            return {
                .where{computation_plan::place::scratch},
                .scratch_type{&arg_type},
            };
        }

        // the unary ops of a narrower single operand apply at the width of
        // the type, e.g. the '-' of 'i16(-g(v))'
        return plan_at_type(target);
    }

    // the wider of the type and the argument's type, which is that of its
    // first element; on a tie the argument's type, so the 'b' of 'i8(b)' is
    // checked as a 'bool'
    [[nodiscard]] auto width_type() const -> const type& {
        return *wider(arg_.get_type(), get_type());
    }

    //
    // statics
    //

    // 'a' on a size tie; a type lives in the type table, so the pointer
    // stays valid
    [[nodiscard]] static auto wider(const type& a, const type& b)
        -> const type* {

        return b.size_bytes() > a.size_bytes() ? &b : &a;
    }
};
