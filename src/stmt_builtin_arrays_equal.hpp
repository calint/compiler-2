#pragma once

#include <format>
#include <ostream>
#include <string>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
#include "unary_ops.hpp"

class stmt_builtin_arrays_equal final : public expression {
    token open_paren_tk_;
    stmt_identifier lhs_;
    token lhs_delim_tk_;
    stmt_identifier rhs_;
    token rhs_delim_tk_;
    expr_any count_;
    token close_paren_tk_;

  public:
    stmt_builtin_arrays_equal(toc& tc, unary_ops uops, const token src_loc_tk,
                              tokenizer& tz)
        : expression{src_loc_tk, std::move(uops)},
          open_paren_tk_{tz.is_next_char_token('(')} {

        assert_no_unary_ops();

        set_type(tc.get_type_bool());

        if (open_paren_tk_.is_empty()) {
            throw compiler_exception{
                tz, "expected '(', 'source', 'compare', 'count', and ')'"};
        }

        lhs_ = {tc, {}, tz.next_token(), tz};

        lhs_delim_tk_ = tz.is_next_char_token(',');
        if (lhs_delim_tk_.is_empty()) {
            throw compiler_exception{tz,
                                     "expected ',' then 'compare' and 'count'"};
        }

        rhs_ = {tc, {}, tz.next_token(), tz};

        rhs_delim_tk_ = tz.is_next_char_token(',');
        if (rhs_delim_tk_.is_empty()) {
            throw compiler_exception{tz, "expected ',' followed by 'count'"};
        }

        count_ = {tc, tz, tc.get_type_default(), true, false, 0};

        close_paren_tk_ = tz.is_next_char_token(')');
        if (close_paren_tk_.is_empty()) {
            throw compiler_exception{tz, "expected ')' after the arguments"};
        }
    }

    stmt_builtin_arrays_equal() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        open_paren_tk_.source_to(os);
        lhs_.source_to(os);
        lhs_delim_tk_.source_to(os);
        rhs_.source_to(os);
        rhs_delim_tk_.source_to(os);
        count_.source_to(os);
        close_paren_tk_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        // the destination is named in the source, the error is at its name
        assert_destination_type(dst_info.type_ref(),
                                dst_info.error_token(tok()));

        compile_boolean(tc, indent, dst_info.operand, false);
    }

    auto compile_boolean(toc& tc, const size_t indent, const operand& dst,
                         const bool inverted) const -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const ident_info lhs_info{tc.make_ident_info(lhs_)};
        const ident_info rhs_info{tc.make_ident_info(rhs_)};

        if (lhs_info.type_ref().name() != rhs_info.type_ref().name()) {
            throw compiler_exception{
                rhs_.tok(),
                std::format("source type '{}' does not match compare type '{}'",
                            lhs_info.type_ref().name(),
                            rhs_info.type_ref().name())};
        }

        assert_destination_type(dst.type_ref(), tok());

        const operand count_register{x.begin_memory_equal(tok(), indent)};

        x.comment(count_.tok(), indent, statement::trimmed_source(count_));

        count_.compile(tc, indent,
                       toc::make_ident_info_from_register(count_register));

        lhs_.compile_address(tc, indent, tok(), lhs_info.lea_path,
                             count_register, x.memory_equal_left_register(),
                             [&](const operand& address) -> void {
                                 x.set_memory_equal_left(tok(), indent,
                                                         address);
                             });

        rhs_.compile_address(tc, indent, tok(), rhs_info.lea_path,
                             count_register, x.memory_equal_right_register(),
                             [&](const operand& address) -> void {
                                 x.set_memory_equal_right(tok(), indent,
                                                          address);
                             });

        x.end_arrays_equal(tok(), indent, lhs_info.type_ref().size_bytes(),
                           lhs_info.type_ref().alignment(), dst, inverted);
    }

    [[nodiscard]] auto produces_boolean() const -> bool override {
        return true;
    }

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

        lhs_.visit_reads(var, reader);
        rhs_.visit_reads(var, reader);
        count_.visit_reads(var, reader);
    }

  private:
    auto assert_destination_type(const type& dst_type,
                                 const token& src_loc_tk) const -> void {

        if (dst_type.name() == get_type().name()) {
            return;
        }

        throw compiler_exception{
            src_loc_tk,
            std::format("destination type must be '{}', not '{}'",
                        get_type().name(), dst_type.name()),
        };
    }
};
