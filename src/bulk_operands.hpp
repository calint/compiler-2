#pragma once

#include <cstdint>
#include <format>
#include <functional>
#include <optional>
#include <ostream>
#include <string_view>

#include "compile_error.hpp"
#include "expr_any.hpp"
#include "ident_info.hpp"
#include "operand.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_reads.hpp"

// the arguments '(first, second, count)' of a built-in function that works on
// 'count' elements of two arrays, e.g. 'array_copy(a, b, 4)' and
// 'arrays_equal(a[1], b, 3)'; the messages name the first array 'source' and
// the second by the role the built-in gives it
// invariants: after parsing, each array is an array or an element of one where
// the operation starts; the element types match and a constant count is in the
// bounds of both once 'resolve_arrays' and 'assert_constant_count_in_bounds'
// have run, which is at compile time because the types are resolved then
class bulk_operands final {
    token open_paren_tk_;
    stmt_identifier first_;
    token first_delim_tk_;
    stmt_identifier second_;
    token second_delim_tk_;
    expr_any count_;
    token close_paren_tk_;
    // how the messages name the second array, e.g. 'destination'
    std::string_view second_role_;

  public:
    // what 'toc' resolved the two arrays to
    struct arrays_info {
        ident_info first;
        ident_info second;
    };

    bulk_operands(toc& tc, tokenizer& tz, const std::string_view second_role)
        : open_paren_tk_{tz.is_next_char_token('(')},
          second_role_{second_role} {

        if (open_paren_tk_.is_empty()) {
            throw compile_error{
                tz,
                std::format("expected '(', 'source', '{}', 'count', and ')'",
                            second_role_)};
        }

        first_ = {tc, {}, tz.next_token(), tz};

        first_.assert_array_or_element();

        first_delim_tk_ = tz.expect_char_token(
            ',', std::format("expected ',' followed by '{}' and 'count'",
                             second_role_));

        second_ = {tc, {}, tz.next_token(), tz};

        second_.assert_array_or_element();

        second_delim_tk_ =
            tz.expect_char_token(',', "expected ',' followed by 'count'");

        count_ = {tc, tz, tc.builtins().default_type(),
                  literal_typing::from_destination};

        close_paren_tk_ =
            tz.expect_char_token(')', "expected ')' after the arguments");
    }

    bulk_operands() = default;

    // a run-time count is left to the bounds check at run time
    auto assert_constant_count_in_bounds(const toc& tc) const -> void {
        const std::optional<int64_t> constant_count{count_.constant_value(tc)};

        if (not constant_count) {
            return;
        }

        first_.assert_count_in_bounds(tc, count_.tok(), *constant_count);
        second_.assert_count_in_bounds(tc, count_.tok(), *constant_count);
    }

    [[nodiscard]] auto count() const -> const expr_any& { return count_; }

    // the statement must outlive the emitter
    [[nodiscard]] auto count_emitter(toc& tc, const size_t indent) const
        -> std::function<void(const operand&)> {

        return make_count_emitter(tc, indent, count_);
    }

    [[nodiscard]] auto first() const -> const stmt_identifier& {
        return first_;
    }

    // the elements are compared or copied as a whole, so their types match
    [[nodiscard]] auto resolve_arrays(toc& tc) const -> arrays_info {
        arrays_info info{
            .first{tc.make_ident_info(first_)},
            .second{tc.make_ident_info(second_)},
        };

        if (not info.first.type_ref().is_same(info.second.type_ref())) {
            throw compile_error{
                second_.tok(),
                std::format("source type '{}' does not match {} type '{}'",
                            info.first.type_ref().name(), second_role_,
                            info.second.type_ref().name())};
        }

        return info;
    }

    [[nodiscard]] auto second() const -> const stmt_identifier& {
        return second_;
    }

    auto source_to(std::ostream& os) const -> void {
        open_paren_tk_.source_to(os);
        first_.source_to(os);
        first_delim_tk_.source_to(os);
        second_.source_to(os);
        second_delim_tk_.source_to(os);
        count_.source_to(os);
        close_paren_tk_.source_to(os);
    }

    // both arrays are read
    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void {

        first_.visit_reads(var, reader);
        second_.visit_reads(var, reader);
        count_.visit_reads(var, reader);
    }
};
