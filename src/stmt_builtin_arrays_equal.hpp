#pragma once

#include <cassert>
#include <cstddef>
#include <optional>
#include <ostream>
#include <utility>

#include "bulk_operands.hpp"
#include "parse_lookahead.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "type.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

// 'arrays_equal(a, b, count)' is a 'bool': whether the elements of the two
// ranges are equal; unary ops are rejected like on any 'bool' operand
class stmt_builtin_arrays_equal final : public statement {
    bulk_operands operands_;

  public:
    stmt_builtin_arrays_equal(parse_toc& tc, unary_ops uops,
                              const token& src_loc_tk, tokenizer& tz)
        : statement{src_loc_tk, std::move(uops)},
          operands_{tc,      tz,       src_loc_tk,
                    "first", "second", bulk_operands::second_use::read} {

        set_type(builtin_function_type(tc, src_loc_tk));
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        operands_.source_to(os);
    }

    auto compile(compile_toc& tc, const destination& dst) const
        -> void override {

        // note: the parse rejected a destination that is not a 'bool'
        //       ('expr_any::assert_number_kind')

        assert(dst.type_ref().is_bool());

        comment_source(tc);

        const type& element_type{operands_.check(tc)};

        const std::optional<size_t> size_bytes{
            operands_.constant_size_bytes(tc, element_type),
        };

        bulk_operands::argument_emitters emit{operands_, tc, tok()};

        machine& x{tc.machine()};

        // a known size is compared like two instances; a count of 0 stays a
        // run-time walk, which compares nothing
        if (size_bytes and *size_bytes > 0) {
            emit.both_addresses(
                [&](const operand& lhs, const operand& rhs) -> void {
                    x.memory_equal({tok(), tc.indent()}, *size_bytes, lhs, rhs,
                                   {
                                       .alignment{element_type.alignment()},
                                       .dst{dst.operand()},
                                       .inverted{},
                                   });
                });

            return;
        }

        x.equal_elements({tok(), tc.indent()}, element_type.size_bytes(),
                         emit.count(), emit.first(), emit.second(),
                         element_type.alignment(), dst.operand());
    }

    auto visit_operands(const operand_visitor visitor) const -> void override {
        operands_.visit_operands(visitor);
    }
};
