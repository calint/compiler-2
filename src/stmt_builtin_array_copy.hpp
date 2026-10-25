#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <ostream>
#include <vector>

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
        : statement{src_loc_tk}, operands_{tc, tz, "source", "destination"} {

        const stmt_identifier& dst{operands_.second()};

        dst.record_write(tc, dst.tok(), "copy into", make_ident_info(tc, dst));
    }

    stmt_builtin_array_copy() = default;

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

        const bulk_operands::arrays_info arrays{operands_.check(tc, true)};

        // nothing is copied
        if (operands_.is_constant_empty(tc)) {
            return;
        }

        const std::optional<int64_t> count{
            operands_.count().constant_value(tc),
        };

        // the overlap check compares the addresses of the loop at run time
        if (count and not tc.checks().overlap) {
            compile_constant_count(tc, indent, arrays,
                                   static_cast<size_t>(*count));

            return;
        }

        const std::function<void(const operand&)> emit_count{
            operands_.count_emitter(tc, indent),
        };

        const machine::address_emitter emit_src{
            operands_.first().address_emitter_of(
                tc, indent, tok(), arrays.first,
                operands_.range_check_of(tc, operands_.first())),
        };

        const machine::address_emitter emit_dst{
            operands_.second().address_emitter_of(
                tc, indent, tok(), arrays.second,
                operands_.range_check_of(tc, operands_.second())),
        };

        x.copy_elements(tok(), indent, arrays.first.type_ref().size_bytes(),
                        emit_count,
                        {
                            .alignment{arrays.first.type_ref().alignment()},
                            .src{emit_src},
                            .dst{emit_dst},
                            .overlap{tc.checks().overlap_options()},
                        });
    }

    // the destination is written, only its index is read
    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        operands_.first().visit_reads(var, reader);
        operands_.second().visit_index_reads(var, reader);
        operands_.count().visit_reads(var, reader);
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

        const operand src_count{
            src_check == range_check::run_time ? count_register : operand{},
        };

        x.comment(src.tok(), indent, statement::trimmed_source(src));

        const operand src_operand{
            src.compile_lea(
                tc, indent, src.first_token(), held,
                {
                    .reg_count{src_count},
                    .address_by_call{arrays.first.address_by_call()},
                    .address_register{},
                }),
        };

        const operand dst_count{
            dst_check == range_check::run_time ? count_register : operand{},
        };

        x.comment(dst.tok(), indent, statement::trimmed_source(dst));

        const operand dst_operand{
            dst.compile_lea(
                tc, indent, dst.first_token(), held,
                {
                    .reg_count{dst_count},
                    .address_by_call{arrays.second.address_by_call()},
                    .address_register{},
                }),
        };

        x.copy(tok(), indent, dst_operand, src_operand,
               multiply_storage_size(
                   tok(), arrays.first.type_ref().size_bytes(), count),
               arrays.first.type_ref().alignment());
    }
};
