#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <utility>

#include "compile_error.hpp"
#include "ident_info.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "stmt_call.hpp"
#include "stmt_identifier.hpp"
#include "token.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

// 'array_length(a)' is the length of the array 'a' in the default type, a
// constant whenever the length is known, see 'fold'
class stmt_builtin_array_length final : public statement {
    // the one argument is an array
    static constexpr std::array<size_t, 1> array_indexes_{0};

    builtin_arguments args_;

  public:
    stmt_builtin_array_length(toc& tc, unary_ops uops, const token src_loc_tk,
                              tokenizer& tz)
        : statement{src_loc_tk, std::move(uops)},
          args_{tc, tz, src_loc_tk, array_indexes_, 1, 1} {

        array().assert_array();

        // the length is folded, so an index would be neither computed nor
        // checked at run time
        array().assert_constant_indexes(tc, tok().text());

        set_type(tc.builtins().default_type());
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        args_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        // note: a narrower destination is rejected by 'assert_not_narrowed'
        //       or the argument check of a conversion, a 'bool' by its
        //       boolean check, and no type is wider

        assert(dst.type_ref().is_same(tc.builtins().default_type()));

        const ident_info src_info{make_ident_info(tc, array())};

        // note: the parse rejected an argument that is not an array
        assert(src_info.is_var() and src_info.is_array());

        // unknown in a function checked without a call, whose code is
        // discarded, e.g. the left side of 'array_length(a) * i', which an
        // operation with a run-time right side compiles without folding
        if (src_info.is_length_unknown()) {
            return;
        }

        // note: the destination has the default type, so the length folds
        //       and the list copies it itself

        std::unreachable();
    }

    // a narrower width stays unfolded, 'assert_not_narrowed' rejects it
    [[nodiscard]] auto fold(const toc& tc, const value_use& use) const
        -> std::optional<folded_value> override {

        if (use.width_ref().size_bytes() < get_type().size_bytes()) {
            return std::nullopt;
        }

        const ident_info src_info{make_ident_info(tc, array())};

        // note: the parse rejected an argument that is not an array
        assert(src_info.is_var() and src_info.is_array());

        const std::optional<size_t> array_len{src_info.array_len()};

        // an unsized array parameter has no length before its call
        if (not array_len) {
            return std::nullopt;
        }

        const int64_t length{static_cast<int64_t>(*array_len)};

        // typed, so a comparison gets the verdict of the length computed at
        // run time, e.g. in a function checked without a call
        return folded_value{
            .value{get_unary_ops().evaluate_constant(length)},
            .own_type{&get_type()},
        };
    }

    // the length is known at compile time so the value of an array is not
    // read, only its indexes are
    auto visit_operands(const operand_visitor visitor) const -> void override {
        visitor({
            .node{&args_.at(0)},
            .written_place{&array()},
        });
    }

  private:
    [[nodiscard]] auto array() const -> const stmt_identifier& {
        return *args_.at(0).bare_path();
    }
};
