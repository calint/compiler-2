#pragma once

#include <format>
#include <ostream>
#include <string>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
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

        if (x.compares_directly()) {
            compile_direct(tc, indent, lhs_info, rhs_info, size_bytes, dst,
                           inverted);

            return;
        }

        x.begin_memory_equal(tok(), indent);

        lhs_.compile_address(tc, indent, tok(), lhs_info.lea_path, {}, {},
                             [&](const operand& address) -> void {
                                 x.set_memory_equal_left(tok(), indent,
                                                         address);
                             });

        rhs_.compile_address(tc, indent, tok(), rhs_info.lea_path, {}, {},
                             [&](const operand& address) -> void {
                                 x.set_memory_equal_right(tok(), indent,
                                                          address);
                             });

        x.end_memory_equal(tok(), indent, size_bytes,
                           lhs_info.type_ref().alignment(), dst, inverted);
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
    // the machine reads both addresses where they are, without pointer
    // registers set up first, so both address paths stay allocated until it
    // has compared
    auto compile_direct(toc& tc, const size_t indent,
                        const ident_info& lhs_info, const ident_info& rhs_info,
                        const size_t size_bytes, const operand& dst,
                        const bool inverted) const -> void {

        machine& x{tc.machine()};

        std::vector<operand> allocated_registers;

        x.comment(lhs_.tok(), indent, statement::trimmed_source(lhs_));

        const operand lhs_address{
            lhs_.compile_lea(tc, indent, lhs_.first_token(),
                             allocated_registers, {}, lhs_info.lea_path, {}),
        };

        x.comment(rhs_.tok(), indent, statement::trimmed_source(rhs_));

        const operand rhs_address{
            rhs_.compile_lea(tc, indent, rhs_.first_token(),
                             allocated_registers, {}, rhs_info.lea_path, {}),
        };

        x.compare_memory(tok(), indent, lhs_address, rhs_address, size_bytes,
                         lhs_info.type_ref().alignment(), dst, inverted);

        x.free_scratch_registers(tok(), indent, allocated_registers);
    }

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
