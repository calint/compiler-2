#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <ostream>

#include "bulk_operands.hpp"
#include "expr_any.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "type.hpp"
#include "ub_reads.hpp"

// e.g. 'array_copy(a[1], b[0], n)' copies 'n' elements of 'a' into 'b'; the
// copied elements are not tracked, so the destination stays unassigned
class stmt_builtin_array_copy final : public statement {
    bulk_operands operands_;

  public:
    stmt_builtin_array_copy(toc& tc, const token src_loc_tk, tokenizer& tz)
        : statement{src_loc_tk}, operands_{tc,
                                           tz,
                                           src_loc_tk,
                                           "source",
                                           "destination",
                                           bulk_operands::second_use::written} {

        const stmt_identifier& dst{operands_.second()};

        dst.record_write(tc, dst.tok(), "copy into", make_ident_info(tc, dst));
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        operands_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const bulk_operands::arrays_info arrays{operands_.check(tc)};

        const std::optional<int64_t> count{
            operands_.count().constant_value(tc),
        };

        // the overlap check compares the addresses of the loop at run time
        if (count and not tc.checks().overlap) {
            compile_constant_count(tc, indent, arrays,
                                   static_cast<size_t>(*count));

            return;
        }

        const bulk_operands::run_time_emitters emit{
            operands_.emitters(tc, indent, tok()),
        };

        x.copy_elements(tok(), indent, arrays.first.type_ref().size_bytes(),
                        emit.count,
                        {
                            .alignment{arrays.first.type_ref().alignment()},
                            .src{emit.first},
                            .dst{emit.second},
                            .overlap{tc.checks().overlap_options()},
                        });
    }

    auto visit_operands(const operand_visitor visitor) const -> void override {
        operands_.visit_operands(visitor);
    }

  private:
    // the size is known so the copy is unrolled when small and the width
    // follows the alignment of both addresses
    auto compile_constant_count(toc& tc, const size_t indent,
                                const bulk_operands::arrays_info& arrays,
                                const size_t count) const -> void {

        const stmt_identifier& src{operands_.first()};
        const stmt_identifier& dst{operands_.second()};
        const expr_any& count_expr{operands_.count()};

        machine& x{tc.machine()};

        machine::held_registers held{x, tok(), indent};

        const range_check src_check{operands_.range_check_of(tc, src)};
        const range_check dst_check{operands_.range_check_of(tc, dst)};

        // the range checks of a run-time start compare 'start + count' in
        // registers
        operand count_register;

        if (tc.checks().is_bounds() and (src_check == range_check::run_time or
                                         dst_check == range_check::run_time)) {

            count_register = x.alloc_scratch_register(
                tok(), indent, tc.builtins().default_type());

            held.add(count_register);

            x.comment(count_expr.tok(), indent,
                      statement::trimmed_source(count_expr));

            count_expr.compile(tc, indent,
                               destination::of_register(count_register));
        }

        const std::optional<operand> src_count{
            src_check == range_check::run_time ? std::optional{count_register}
                                               : std::nullopt,
        };

        x.comment(src.tok(), indent, statement::trimmed_source(src));

        const operand src_operand{
            src.compile_lea(tc, indent, src.first_token(), held,
                            {
                                .reg_count{src_count},
                                .is_range_start{true},
                                .address_register{},
                            }),
        };

        const std::optional<operand> dst_count{
            dst_check == range_check::run_time ? std::optional{count_register}
                                               : std::nullopt,
        };

        x.comment(dst.tok(), indent, statement::trimmed_source(dst));

        const operand dst_operand{
            dst.compile_lea(tc, indent, dst.first_token(), held,
                            {
                                .reg_count{dst_count},
                                .is_range_start{true},
                                .address_register{},
                            }),
        };

        x.copy_block(tok(), indent, dst_operand, src_operand,
                     multiply_storage_size(
                         tok(), arrays.first.type_ref().size_bytes(), count),
                     arrays.first.type_ref().alignment());
    }
};
