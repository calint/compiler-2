#pragma once

#include <cstddef>
#include <format>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expression.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
#include "token.hpp"
#include "ub_unset_var.hpp"
#include "unary_ops.hpp"

class stmt_builtin_equal final : public expression {
    token open_paren_tk_;
    stmt_identifier lhs_;
    token lhs_delim_tk_;
    stmt_identifier rhs_;
    token close_paren_tk_;

  public:
    stmt_builtin_equal(toc& tc, unary_ops uops, const token src_loc_tk,
                       tokenizer& tz)
        : expression{src_loc_tk, std::move(uops)},
          open_paren_tk_{tz.is_next_char_token('(')} {

        assert_no_unary_ops();

        set_type(tc.get_type_bool());

        if (open_paren_tk_.is_empty()) {
            throw compiler_exception{
                tz, "expected '(', 'source', 'compare', and ')'"};
        }

        lhs_ = {tc, {}, tz.next_token(), tz};

        lhs_delim_tk_ = tz.is_next_char_token(',');

        if (lhs_delim_tk_.is_empty()) {
            throw compiler_exception{tz, "expected ',' followed by 'compare'"};
        }

        rhs_ = {tc, {}, tz.next_token(), tz};

        close_paren_tk_ = tz.is_next_char_token(')');

        if (close_paren_tk_.is_empty()) {
            throw compiler_exception{tz, "expected ')' after the arguments"};
        }
    }

    stmt_builtin_equal() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        open_paren_tk_.source_to(os);
        lhs_.source_to(os);
        lhs_delim_tk_.source_to(os);
        rhs_.source_to(os);
        close_paren_tk_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        compile_boolean(tc, indent, dst_info.operand, false);
    }

    auto compile_boolean(toc& tc, const size_t indent, const operand& dst,
                         const bool inverted) const -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const ident_info lhs_info{make_operand_info(tc, lhs_)};
        const ident_info rhs_info{make_operand_info(tc, rhs_)};

        if (not lhs_info.type_ref().is_same(rhs_info.type_ref())) {
            throw compiler_exception{
                rhs_.tok(),
                std::format("source and compare types are not the "
                            "same. source is '{}' and compare is '{}'",
                            lhs_info.type_ref().name(),
                            rhs_info.type_ref().name())};
        }

        size_t size_bytes{lhs_info.type_ref().size_bytes()};

        // a whole array is not compared as its first element
        if (lhs_info.is_array != rhs_info.is_array) {
            toc::assert_not_whole_array(lhs_, lhs_info);
            toc::assert_not_whole_array(rhs_, rhs_info);
        }

        // check comparing 2 arrays of the same size without indexing
        // note: a whole array on one side only was rejected above
        if (lhs_info.is_array) {

            if (lhs_info.array_len != rhs_info.array_len) {
                throw compiler_exception{lhs_.tok(),
                                         "cannot compare arrays of different "
                                         "sizes"};
            }

            size_bytes = multiply_storage_size(lhs_.tok(), size_bytes,
                                               lhs_info.array_len);
        }

        const auto emit_lhs{
            [&](const operand& reg_count, const operand& address_register,
                const machine::address_use use) -> void {
                lhs_.compile_address(tc, indent, tok(),
                                     {
                                         .reg_count{reg_count},
                                         .lea_path{lhs_info.lea_path},
                                         .address_register{address_register},
                                     },
                                     use);
            },
        };

        const auto emit_rhs{
            [&](const operand& reg_count, const operand& address_register,
                const machine::address_use use) -> void {
                rhs_.compile_address(tc, indent, tok(),
                                     {
                                         .reg_count{reg_count},
                                         .lea_path{rhs_info.lea_path},
                                         .address_register{address_register},
                                     },
                                     use);
            },
        };

        x.memory_equal(tok(), indent, size_bytes,
                       {
                           .alignment{lhs_info.type_ref().alignment()},
                           .lhs{emit_lhs},
                           .rhs{emit_rhs},
                           .dst{dst},
                           .inverted{inverted},
                       });
    }

    [[nodiscard]] auto produces_boolean() const -> bool override {
        return true;
    }

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

        lhs_.visit_reads(var, reader);
        rhs_.visit_reads(var, reader);
    }

  private:
    //
    // statics
    //

    // memory is compared, so a constant has nothing to compare
    [[nodiscard]] static auto
    make_operand_info(const toc& tc, const stmt_identifier& identifier)
        -> ident_info {

        ident_info info{tc.make_ident_info(identifier)};

        if (info.is_const()) {
            throw compiler_exception{identifier.tok(),
                                     "constant not supported"};
        }

        return info;
    }
};
