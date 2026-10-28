#pragma once

#include <cstddef>
#include <cstdint>
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

        bulk_operands::run_time_emitters emit{operands_, tc, indent, tok()};

        x.copy_elements(tok(), indent, arrays.first.type_ref().size_bytes(),
                        emit.count(),
                        {
                            .alignment{arrays.first.type_ref().alignment()},
                            .src{emit.first()},
                            .dst{emit.second()},
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

        machine& x{tc.machine()};

        machine::held_registers held{x, tok(), indent};

        // the walk checks a run-time start against the end less the count
        const lea_request range{
            .range_count{count},
            .start_register{},
            .address_register{},
        };

        x.comment(src.tok(), indent, statement::trimmed_source(src));

        const operand src_operand{
            src.compile_lea(tc, indent, src.first_token(), held, range),
        };

        x.comment(dst.tok(), indent, statement::trimmed_source(dst));

        const operand dst_operand{
            dst.compile_lea(tc, indent, dst.first_token(), held, range),
        };

        x.copy_block(tok(), indent, dst_operand, src_operand,
                     multiply_storage_size(
                         tok(), arrays.first.type_ref().size_bytes(), count),
                     arrays.first.type_ref().alignment());
    }
};
