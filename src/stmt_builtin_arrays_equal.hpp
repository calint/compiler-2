#pragma once

#include <cstddef>
#include <format>
#include <functional>
#include <ostream>
#include <utility>

#include "bulk_operands.hpp"
#include "compile_error.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

// 'arrays_equal(a, b, count)' is a 'bool': whether the elements of the two
// ranges are equal; unary ops are rejected like on any 'bool' operand
class stmt_builtin_arrays_equal final : public statement {
    bulk_operands operands_;

  public:
    stmt_builtin_arrays_equal(toc& tc, unary_ops uops, const token src_loc_tk,
                              tokenizer& tz)
        : statement{src_loc_tk, std::move(uops)},
          operands_{tc, tz, "first", "second",
                    bulk_operands::second_use::read} {

        set_type(tc.builtins().boolean());
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        operands_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        // note: the parse rejected a destination that is not a 'bool'
        //       ('expr_any::assert_number_kind')

        assert(dst.type_ref().is_bool());

        compile_arrays_equal(tc, indent, dst.operand(), false);
    }

    auto compile_arrays_equal(toc& tc, const size_t indent, const operand& dst,
                              const bool inverted) const -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const bulk_operands::arrays_info arrays{operands_.check(tc)};

        // no elements are compared, the ranges are equal
        if (operands_.is_constant_empty(tc)) {
            x.copy_value(
                tok(), indent, dst,
                operand::imm(inverted ? 0 : 1, tc.builtins().boolean()));

            return;
        }

        const bulk_operands::run_time_emitters emit{
            operands_.emitters(tc, indent, tok(), arrays),
        };

        x.equal_elements(tok(), indent, arrays.first.type_ref().size_bytes(),
                         emit.count,
                         {
                             .alignment{arrays.first.type_ref().alignment()},
                             .lhs{emit.first},
                             .rhs{emit.second},
                             .dst{dst},
                             .inverted{inverted},
                         });
    }

    [[nodiscard]] auto is_arrays_equal_call() const -> bool override {
        return true;
    }

    auto visit_operands(const operand_visitor visitor) const -> void override {
        operands_.visit_operands(visitor);
    }
};
