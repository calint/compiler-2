#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
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
    stmt_builtin_array_copy(parse_toc& tc, const token& src_loc_tk,
                            tokenizer& tz)
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

    auto compile(compile_toc& tc, [[maybe_unused]] const destination& dst) const
        -> void override {

        comment_source(tc);

        const type& element_type{operands_.check(tc)};

        const std::optional<size_t> size_bytes{
            operands_.constant_size_bytes(tc, element_type),
        };

        // the overlap check compares the addresses of the loop at run time
        // where the destination may start inside the source; a call rejects
        // arguments that may share storage
        // ('stmt_call::assert_no_shared_storage'), e.g. two 'mut' parameters
        // bound to one array
        const bool checks_overlap{
            tc.checks().overlap and may_start_inside_source(tc),
        };

        bulk_operands::argument_emitters emit{operands_, tc, tok()};

        machine& x{tc.machine()};

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
    // the destination may start inside the source, e.g.
    // 'array_copy(a[0], a[1], n)' but not 'array_copy(s.a, s.b, n)', nor
    // from constant starts in one array a copy down, 'array_copy(a[1], a, n)',
    // or past the end of a constant count, 'array_copy(a, a[2], 2)'
    // note: the 'upper' check of the count keeps each copy inside the range
    //       of its path, from its start element to the end of the array
    [[nodiscard]] auto may_start_inside_source(const toc& tc) const -> bool {
        const std::optional<storage_path> src{operands_.first().storage(tc)};
        const std::optional<storage_path> dst{operands_.second().storage(tc)};

        // note: both are arrays of variables, a constant is not written
        assert(src and dst);

        if (not src->may_overlap(*dst)) {
            return false;
        }

        // note: two paths of one element type that overlap name the same
        //       array, an array cannot hold its own element type deeper; with
        //       a run-time index before the start, e.g. 's[j].v[1]', or the
        //       open range of an unsized parameter they name it or none, so
        //       the starts compare in one array; a call rejects arguments
        //       that may share storage, which would start at other elements

        const std::optional<int64_t> src_start{
            operands_.first().constant_start_index(tc),
        };

        const std::optional<int64_t> dst_start{
            operands_.second().constant_start_index(tc),
        };

        if (not src_start or not dst_start) {
            return true;
        }

        if (*dst_start <= *src_start) {
            return false;
        }

        const std::optional<int64_t> count{operands_.constant_count(tc)};

        return not count or *dst_start < *src_start + *count;
    }
};
