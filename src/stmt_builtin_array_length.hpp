#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <string_view>
#include <utility>

#include "compile_error.hpp"
#include "expression.hpp"
#include "ident_info.hpp"
#include "operand.hpp"
#include "stmt_identifier.hpp"
#include "token.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

class stmt_builtin_array_length final : public expression {
    token open_paren_tk_;
    stmt_identifier stmt_ident_;
    token close_paren_tk_;

  public:
    stmt_builtin_array_length(toc& tc, unary_ops uops, const token src_loc_tk,
                              tokenizer& tz)
        : expression{src_loc_tk, std::move(uops)},
          open_paren_tk_{tz.is_next_char_token('(')} {

        set_type(tc.builtins().default_type());

        if (open_paren_tk_.is_empty()) {
            throw compile_error{tz, "expected '(' followed by an identifier"};
        }

        stmt_ident_ = {tc, {}, tz.next_token(), tz};

        close_paren_tk_ =
            tz.expect_char_token(')', "expected ')' after the argument");
    }

    stmt_builtin_array_length() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        expression::source_to(os);
        open_paren_tk_.source_to(os);
        stmt_ident_.source_to(os);
        close_paren_tk_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        // note: a narrower destination is rejected by 'assert_not_narrowed'
        //       or the argument check of a conversion, a 'bool' by its
        //       boolean check, and no type is wider
        assert(dst.type_ref().is_same(tc.builtins().default_type()));

        const ident_info src_info{tc.make_ident_info(stmt_ident_)};

        if (not src_info.is_var() or not src_info.is_array()) {
            throw compile_error{
                stmt_ident_.first_token(),
                std::format("'{}' is not an array",
                            statement::trimmed_source(stmt_ident_))};
        }

        // unknown in a function checked without a call, whose code is
        // discarded, e.g. the left side of 'array_length(a) * i', which an
        // operation with a run-time right side compiles without folding
        if (src_info.is_length_unknown()) {
            return;
        }

        const std::optional<int64_t> length{
            folded_constant(tc, dst.type_ref()),
        };

        // note: the destination has the default type, so the length folds
        assert(length);

        x.copy_value(tok(), indent, dst.operand(),
                     operand::imm(*length, dst.type_ref()));
    }

    auto assert_not_narrowed([[maybe_unused]] const toc& tc,
                             const type& dst_type) const -> void override {

        assert_own_type_not_narrowed(dst_type);
    }

    // a narrower width stays unfolded, 'assert_not_narrowed' rejects it
    [[nodiscard]] auto folded_constant(const toc& tc,
                                       const type& width_type) const
        -> std::optional<int64_t> override {

        if (not width_type.is_same(tc.builtins().default_type())) {
            return std::nullopt;
        }

        const ident_info src_info{tc.make_ident_info(stmt_ident_)};

        // 'compile' reports an argument that is not an array
        if (not src_info.is_var() or not src_info.is_array()) {
            return std::nullopt;
        }

        const std::optional<size_t> array_len{src_info.array_len()};

        // an unsized array parameter has no length before its call
        if (not array_len) {
            return std::nullopt;
        }

        return get_unary_ops().evaluate_constant(
            static_cast<int64_t>(*array_len));
    }

    // the length is known at compile time so the value of an array is not
    // read; a name that is not an array, which 'compile' rejects, counts as
    // read, so a variable named in its own initializer is uninitialized, e.g.
    // the 'a' of 'let a = array_length(a)'
    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        if (not stmt_ident_.is_array()) {
            stmt_ident_.visit_reads(var, reader);
            return;
        }

        stmt_ident_.visit_index_reads(var, reader);
    }
};
