#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <optional>
#include <ostream>
#include <string_view>
#include <vector>

#include "compiler_exception.hpp"
#include "expr_any.hpp"
#include "ident_info.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

class stmt_builtin_array_copy final : public statement {
    token open_paren_tk_;
    stmt_identifier src_;
    token src_delim_tk_;
    stmt_identifier dst_;
    token dst_delim_tk_;
    expr_any count_;
    token close_paren_tk_;

  public:
    stmt_builtin_array_copy(toc& tc, const token src_loc_tk, tokenizer& tz)
        : statement{src_loc_tk}, open_paren_tk_{tz.is_next_char_token('(')} {

        if (open_paren_tk_.is_empty()) {
            throw compiler_exception{
                tz, "expected '(', 'from', 'to', 'count', and ')'"};
        }

        set_type(tc.builtins().void_type());

        src_ = {tc, {}, tz.next_token(), tz};

        src_.assert_array_or_element("source");

        src_delim_tk_ = tz.expect_char_token(
            ',', "expected ',' followed by 'to' and 'count'");

        dst_ = {tc, {}, tz.next_token(), tz};

        dst_.assert_array_or_element("destination");

        assert_not_read_only(dst_.tok(), "copy into", dst_.name_or_literal(),
                             tc.make_ident_info(dst_));

        dst_delim_tk_ =
            tz.expect_char_token(',', "expected ',' followed by 'count'");

        count_ = {tc, tz, tc.builtins().default_type(),
                  literal_typing::from_destination};

        close_paren_tk_ =
            tz.expect_char_token(')', "expected ')' after the arguments");
    }

    stmt_builtin_array_copy() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        open_paren_tk_.source_to(os);
        src_.source_to(os);
        src_delim_tk_.source_to(os);
        dst_.source_to(os);
        dst_delim_tk_.source_to(os);
        count_.source_to(os);
        close_paren_tk_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const ident_info array_src_info{tc.make_ident_info(src_)};
        const ident_info array_dst_info{tc.make_ident_info(dst_)};

        if (not array_src_info.type_ref().is_same(array_dst_info.type_ref())) {
            throw compiler_exception{
                dst_.tok(),
                std::format("source type '{}' does not match destination "
                            "type '{}'",
                            array_src_info.type_ref().name(),
                            array_dst_info.type_ref().name())};
        }

        const std::optional<int64_t> count{count_.constant_value(tc)};

        if (count) {
            src_.assert_count_in_bounds(tc, count_.tok(), *count);
            dst_.assert_count_in_bounds(tc, count_.tok(), *count);
        }

        // nothing is copied, so a start at the end of the array, which only a
        // range may have, is not addressed
        if (count == 0) {
            return;
        }

        // the overlap check compares the addresses of the loop at run time
        if (count and not tc.checks().overlap) {
            compile_constant_count(tc, indent, array_src_info, array_dst_info,
                                   static_cast<size_t>(*count));

            return;
        }

        const std::function<void(const operand&)> emit_count{
            make_count_emitter(tc, indent, count_),
        };

        const machine::address_emitter emit_src{
            src_.address_emitter_of(tc, indent, tok(), array_src_info),
        };

        const machine::address_emitter emit_dst{
            dst_.address_emitter_of(tc, indent, tok(), array_dst_info),
        };

        x.copy_elements(tok(), indent, array_src_info.type_ref().size_bytes(),
                        emit_count,
                        {
                            .alignment{array_src_info.type_ref().alignment()},
                            .src{emit_src},
                            .dst{emit_dst},
                            .overlap{tc.checks().overlap_options()},
                        });
    }

    // the copied elements are not tracked, so the destination stays unassigned
    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        src_.visit_reads(var, reader);
        dst_.visit_index_reads(var, reader);
        count_.visit_reads(var, reader);
    }

  private:
    // the size is known so the copy is unrolled when small and the width
    // follows the alignment of both addresses
    auto compile_constant_count(toc& tc, const size_t indent,
                                const ident_info& array_src_info,
                                const ident_info& array_dst_info,
                                const size_t count) const -> void {

        machine& x{tc.machine()};

        std::vector<operand> allocated_scratch_registers;

        // the range checks compare 'start + count' in registers
        operand count_register;

        if (tc.checks().is_bounds()) {
            count_register = x.alloc_scratch_register(
                tok(), indent, tc.builtins().default_type());

            allocated_scratch_registers.push_back(count_register);

            x.comment(count_.tok(), indent, statement::trimmed_source(count_));

            count_.compile(tc, indent,
                           destination::of_register(count_register));
        }

        x.comment(src_.tok(), indent, statement::trimmed_source(src_));

        const operand src_operand{
            src_.compile_lea(tc, indent, src_.first_token(),
                             allocated_scratch_registers,
                             {
                                 .reg_count{count_register},
                                 .known{array_src_info.known()},
                                 .address_register{},
                             }),
        };

        x.comment(dst_.tok(), indent, statement::trimmed_source(dst_));

        const operand dst_operand{
            dst_.compile_lea(tc, indent, dst_.first_token(),
                             allocated_scratch_registers,
                             {
                                 .reg_count{count_register},
                                 .known{array_dst_info.known()},
                                 .address_register{},
                             }),
        };

        x.copy(tok(), indent, src_operand, dst_operand,
               multiply_storage_size(
                   tok(), array_src_info.type_ref().size_bytes(), count),
               array_src_info.type_ref().alignment());

        x.free_scratch_registers(tok(), indent, allocated_scratch_registers);
    }
};
