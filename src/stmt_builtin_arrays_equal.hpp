#pragma once

#include <cstddef>
#include <format>
#include <functional>
#include <ostream>
#include <utility>

#include "bulk_operands.hpp"
#include "compile_error.hpp"
#include "expression.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

// 'arrays_equal(a, b, count)' is a 'bool': whether the elements of the two
// ranges are equal; it takes no unary ops
class stmt_builtin_arrays_equal final : public expression {
    bulk_operands operands_;

  public:
    stmt_builtin_arrays_equal(toc& tc, unary_ops uops, const token src_loc_tk,
                              tokenizer& tz)
        : expression{src_loc_tk, std::move(uops)} {

        assert_no_unary_ops();

        set_type(tc.builtins().boolean());

        // note: parsed after the check so that a unary operation is reported
        //       before an error in the arguments

        operands_ = {tc, tz, "first", "second"};
    }

    stmt_builtin_arrays_equal() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        operands_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        // the destination is named in the source, the error is at its name
        assert_destination_type(dst.type_ref(), dst.error_token(tok()));

        compile_boolean(tc, indent, dst.operand(), false);
    }

    auto compile_boolean(toc& tc, const size_t indent, const operand& dst,
                         const bool inverted) const -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const bulk_operands::arrays_info arrays{operands_.resolve_arrays(tc)};

        operands_.assert_constant_count_in_bounds(tc);

        const std::function<void(const operand&)> emit_count{
            operands_.count_emitter(tc, indent),
        };

        const machine::address_emitter emit_lhs{
            operands_.first().address_emitter_of(
                tc, indent, tok(), arrays.first,
                operands_.range_check_of(tc, operands_.first())),
        };

        const machine::address_emitter emit_rhs{
            operands_.second().address_emitter_of(
                tc, indent, tok(), arrays.second,
                operands_.range_check_of(tc, operands_.second())),
        };

        x.arrays_equal(tok(), indent, arrays.first.type_ref().size_bytes(),
                       emit_count,
                       {
                           .alignment{arrays.first.type_ref().alignment()},
                           .lhs{emit_lhs},
                           .rhs{emit_rhs},
                           .dst{dst},
                           .inverted{inverted},
                       });
    }

    [[nodiscard]] auto produces_boolean() const -> bool override {
        return true;
    }

    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        operands_.visit_reads(var, reader);
    }

  private:
    auto assert_destination_type(const type& dst_type,
                                 const token& src_loc_tk) const -> void {

        if (dst_type.is_same(get_type())) {
            return;
        }

        throw compile_error{
            src_loc_tk,
            std::format("expected type '{}', got '{}'", dst_type.name(),
                        get_type().name()),
        };
    }

    // a 'bool' result has no meaningful negation
    auto assert_no_unary_ops() const -> void {
        if (get_unary_ops().is_empty()) {
            return;
        }

        throw compile_error{
            tok(), "this built-in function does not accept unary operations"};
    }
};
