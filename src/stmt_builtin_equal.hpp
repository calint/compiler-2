#pragma once

#include <format>
#include <ostream>
#include <ranges>
#include <string>
#include <string_view>
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
    stmt_builtin_equal(toc& tc, unary_ops uops, token tk, tokenizer& tz)
        : expression{tk, std::move(uops)},
          open_paren_tk_{tz.is_next_char_token('(')} {

        if (not statement::get_unary_ops().is_empty()) {
            throw compiler_exception{
                tok(),
                "this built-in function does not accept unary operations"};
        }

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
            throw compiler_exception{tok(), "expected ')' after the arguments"};
        }
    }

    stmt_builtin_equal() = default;

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

        x86& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        // allocate the register for rep movs
        x.alloc_named_register(tok(), indent, "rsi", tc.get_type_default());
        x.alloc_named_register(tok(), indent, "rdi", tc.get_type_default());
        x.alloc_named_register(tok(), indent, "rcx", tc.get_type_default());

        std::vector<std::string> allocated_scratch_registers;

        const ident_info lhs_info{tc.make_ident_info(lhs_)};
        if (lhs_info.is_const()) {
            throw compiler_exception{lhs_.tok(), "constant not supported"};
        }
        const ident_info rhs_info{tc.make_ident_info(rhs_)};
        if (rhs_info.is_const()) {
            throw compiler_exception{rhs_.tok(), "constant not supported"};
        }

        // from operand to rsi
        x.comment(lhs_.tok(), indent, statement::trimmed_source(lhs_));

        const operand lhs_operand{stmt_identifier::compile_effective_address(
            tc, indent, lhs_.first_token(), lhs_.elems(),
            allocated_scratch_registers, "", lhs_info.lea_path)};

        x.lea(indent, "rsi", lhs_operand.address_str());

        for (const std::string& reg :
             allocated_scratch_registers | std::views::reverse) {

            x.free_scratch_register(tok(), indent, reg);
        }

        // to operand to 'rdi'
        x.comment(rhs_.tok(), indent, statement::trimmed_source(rhs_));

        allocated_scratch_registers.clear();

        const operand rhs_operand{stmt_identifier::compile_effective_address(
            tc, indent, rhs_.first_token(), rhs_.elems(),
            allocated_scratch_registers, "", rhs_info.lea_path)};

        x.lea(indent, "rdi", rhs_operand.address_str());

        for (const std::string& reg :
             allocated_scratch_registers | std::views::reverse) {

            x.free_scratch_register(tok(), indent, reg);
        }

        if (lhs_info.type_ref().name() != rhs_info.type_ref().name()) {
            throw compiler_exception{
                tok(), std::format("source and compare types are not the "
                                   "same. source is '{}' and compare is '{}'",
                                   lhs_info.type_ref().name(),
                                   rhs_info.type_ref().name())};
        }

        const size_t type_size{lhs_info.type_ref().size()};

        char rep_size{'b'};
        size_t rcx{type_size};

        // check comparing 2 arrays of the same size without indexing
        if (lhs_info.is_array and not lhs_.is_indexed() and
            rhs_info.is_array and not rhs_.is_indexed()) {

            if (lhs_info.array_size != rhs_info.array_size) {
                throw compiler_exception(lhs_.tok(),
                                         "cannot compare arrays of different "
                                         "sizes");
            }

            rcx *= lhs_info.array_size;
        }

        if ((rcx % operand::size_qword) == 0) {
            rep_size = 'q';
            rcx /= operand::size_qword;
        } else if ((rcx % operand::size_dword) == 0) {
            rep_size = 'd';
            rcx /= operand::size_dword;
        } else if ((rcx % operand::size_word) == 0) {
            rep_size = 'w';
            rcx /= operand::size_word;
        }
        x.mov(tok(), indent, "rcx", std::to_string(rcx));

        // copy
        x.repe_cmps(indent, rep_size);

        x.free_named_register(tok(), indent, "rcx");
        x.free_named_register(tok(), indent, "rdi");
        x.free_named_register(tok(), indent, "rsi");

        // set true if equal

        if (dst_info.is_register()) {
            x.setcc(indent, "e",
                    x86::get_sized_register_operand(dst_info.operand.str(),
                                                    operand::size_byte));

            return;
        }

        x.setcc(indent, "e", dst_info.operand.str(operand::size_byte));
    }
};
