#pragma once

#include <cassert>
#include <cstddef>
#include <optional>
#include <ostream>

#include "bulk_operands.hpp"
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
    stmt_builtin_array_copy(toc& tc, const token& src_loc_tk, tokenizer& tz)
        : statement{src_loc_tk}, operands_{tc,
                                           tz,
                                           src_loc_tk,
                                           "source",
                                           "destination",
                                           bulk_operands::second_use::written} {

        const stmt_identifier& dst{operands_.second()};

        dst.record_write(tc, "copy into");
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        operands_.source_to(os);
    }

    auto compile(toc& tc, [[maybe_unused]] const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        comment_source(tc);

        const type& element_type{operands_.check(tc)};

        const std::optional<size_t> size_bytes{
            operands_.constant_size_bytes(tc, element_type),
        };

        // the overlap check compares the addresses of the loop at run time;
        // only one variable can overlap itself, a call rejects arguments that
        // may share storage ('stmt_call::assert_no_shared_storage'), e.g. two
        // 'mut' parameters bound to one array
        const bool checks_overlap{
            tc.checks().overlap and may_overlap(tc),
        };

        bulk_operands::argument_emitters emit{operands_, tc, tok()};

        // the size is known so the copy is unrolled when small and the width
        // follows the alignment of both addresses
        if (size_bytes and not checks_overlap) {
            emit.both_addresses(
                [&](const operand& src, const operand& dst_address) -> void {
                    x.copy_block({tok(), tc.indent()}, dst_address, src,
                                 *size_bytes, element_type.alignment());
                });

            return;
        }

        x.copy_elements({tok(), tc.indent()}, element_type.size_bytes(),
                        emit.count(),
                        {
                            .alignment{element_type.alignment()},
                            .src{emit.first()},
                            .dst{emit.second()},
                            .overlap{
                                .enabled{checks_overlap},
                            },
                        });
    }

    auto visit_operands(const operand_visitor visitor) const -> void override {
        operands_.visit_operands(visitor);
    }

  private:
    // the two arrays are in the same variable, e.g. 'array_copy(a[0], a[1], n)'
    [[nodiscard]] auto may_overlap(const toc& tc) const -> bool {
        const std::optional<storage_path> src{operands_.first().storage(tc)};
        const std::optional<storage_path> dst{operands_.second().storage(tc)};

        // note: both are arrays of variables, a constant is not written
        assert(src and dst);

        return src->root.is_same(dst->root);
    }
};
