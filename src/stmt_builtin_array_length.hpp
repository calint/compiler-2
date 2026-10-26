#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string_view>
#include <utility>

#include "compile_error.hpp"
#include "ident_info.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
#include "token.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

// 'array_length(a)' is the length of the array 'a' in the default type, a
// constant whenever the length is known, see 'fold'
class stmt_builtin_array_length final : public statement {
    token open_paren_tk_;
    stmt_identifier stmt_ident_;
    token close_paren_tk_;

  public:
    stmt_builtin_array_length(toc& tc, unary_ops uops, const token src_loc_tk,
                              tokenizer& tz)
        : statement{src_loc_tk, std::move(uops)},
          open_paren_tk_{
              tz.expect_char_token('(',
                                   "expected '(' followed by an identifier"),
          },
          stmt_ident_{parse_argument(tc, tz, src_loc_tk)},
          close_paren_tk_{
              tz.expect_char_token(')', "expected ')' after the argument"),
          } {

        set_type(tc.builtins().default_type());
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
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

        const ident_info src_info{make_ident_info(tc, stmt_ident_)};

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

    auto assert_not_narrowed([[maybe_unused]] const toc& tc,
                             const type& dst_type) const -> void override {

        assert_own_type_not_narrowed(dst_type);
    }

    // a narrower width stays unfolded, 'assert_not_narrowed' rejects it
    [[nodiscard]] auto fold(const toc& tc, const value_use& use) const
        -> std::optional<folded_value> override {

        if (use.width_ref().size_bytes() < get_type().size_bytes()) {
            return std::nullopt;
        }

        const ident_info src_info{make_ident_info(tc, stmt_ident_)};

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
            .node{&stmt_ident_},
            .written_place{&stmt_ident_},
        });
    }

  private:
    //
    // statics
    //

    [[nodiscard]] static auto parse_argument(toc& tc, tokenizer& tz,
                                             const token& name_tk)
        -> stmt_identifier {

        tz.assert_no_missing_argument(name_tk.text(), 1);

        stmt_identifier array{stmt_identifier::parse_whole_array(tc, tz)};

        tz.assert_no_surplus_argument(name_tk.text(), 1);

        return array;
    }
};
