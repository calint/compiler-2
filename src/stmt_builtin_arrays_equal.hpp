#pragma once

#include <format>
#include <optional>
#include <ostream>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
#include "unary_ops.hpp"

class stmt_builtin_arrays_equal final : public expression {
    token open_paren_tk_;
    stmt_identifier from_;
    token from_delim_tk_;
    stmt_identifier to_;
    token to_delim_tk_;
    expr_any count_;
    token close_paren_tk_;

  public:
    stmt_builtin_arrays_equal(toc& tc, unary_ops uops, token tk, tokenizer& tz)
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
                tz, "expected '(', 'source', 'compare', 'count', and ')'"};
        }

        from_ = {tc, {}, tz.next_token(), tz};

        from_delim_tk_ = tz.is_next_char_token(',');
        if (from_delim_tk_.is_empty()) {
            throw compiler_exception{tz,
                                     "expected ',' then 'compare' and 'count'"};
        }

        to_ = {tc, {}, tz.next_token(), tz};

        to_delim_tk_ = tz.is_next_char_token(',');
        if (to_delim_tk_.is_empty()) {
            throw compiler_exception{tz, "expected ',' followed by 'count'"};
        }

        count_ = {tc, tz, tc.get_type_default(), true, false, 0};

        close_paren_tk_ = tz.is_next_char_token(')');
        if (close_paren_tk_.is_empty()) {
            throw compiler_exception{tok(), "expected ')' after the arguments"};
        }
    }

    stmt_builtin_arrays_equal() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        open_paren_tk_.source_to(os);
        from_.source_to(os);
        from_delim_tk_.source_to(os);
        to_.source_to(os);
        to_delim_tk_.source_to(os);
        count_.source_to(os);
        close_paren_tk_.source_to(os);
    }

    auto compile(toc& tc, x86& x, const size_t indent,
                 const ident_info& dst_info) const -> void override {

        x.comment_line(tok(), indent, statement::trimmed_source(*this));

        // allocate the register for rep movs
        x.alloc_named_register(tok(), indent, "rsi", tc.get_type_default());
        x.alloc_named_register(tok(), indent, "rdi", tc.get_type_default());
        x.alloc_named_register(tok(), indent, "rcx", tc.get_type_default());

        std::vector<std::string> allocated_scratch_registers;

        // size to 'rcx'
        x.comment_line(count_.tok(), indent, statement::trimmed_source(count_));

        count_.compile(tc, x, indent,
                       toc::make_ident_info_for_register(x, "rcx"));

        const ident_info from_info{tc.make_ident_info(x, from_)};
        const ident_info to_info{tc.make_ident_info(x, to_)};

        // from operand to rsi
        x.comment_line(from_.tok(), indent, statement::trimmed_source(from_));

        const operand from_operand{stmt_identifier::compile_effective_address(
            tc, x, indent, from_.first_token(), from_.elems(),
            allocated_scratch_registers, "rcx", from_info.lea_path)};

        x.lea(indent, "rsi", from_operand.address_str());

        for (const std::string& reg :
             allocated_scratch_registers | std::views::reverse) {
            x.free_scratch_register(tok(), indent, reg);
        }

        // to operand to 'rdi'
        x.comment_line(to_.tok(), indent, statement::trimmed_source(to_));

        allocated_scratch_registers.clear();

        const operand to_operand{stmt_identifier::compile_effective_address(
            tc, x, indent, to_.first_token(), to_.elems(),
            allocated_scratch_registers, "rcx", to_info.lea_path)};

        x.lea(indent, "rdi", to_operand.address_str());

        for (const std::string& reg :
             allocated_scratch_registers | std::views::reverse) {
            x.free_scratch_register(tok(), indent, reg);
        }

        if (from_info.type().name() != to_info.type().name()) {
            throw compiler_exception{
                tok(),
                std::format("source type '{}' does not match compare type '{}'",
                            from_info.type().name(), to_info.type().name())};
        }

        if (dst_info.type().name() != get_type().name()) {
            throw compiler_exception{
                tok(), std::format("destination type must be '{}', not '{}'",
                                   get_type().name(), dst_info.type().name())};
        }

        const size_t type_size{from_info.type().size()};

        if (type_size > 1) {
            // check whether it is possible to shift left instead of
            // multiplication
            if (std::optional<int> shl{
                    stmt_identifier::get_shift_amount(type_size)};
                shl) {

                x.op(tok(), indent, "shl", "rcx", std::format("{}", *shl));
            } else {
                x.op(tok(), indent, "imul", "rcx",
                     std::format("{}", type_size));
            }
        }

        // copy
        x.repe_cmps(indent, 'b');

        x.free_named_register(tok(), indent, "rcx");
        x.free_named_register(tok(), indent, "rdi");
        x.free_named_register(tok(), indent, "rsi");

        // set true if equal

        if (dst_info.is_register()) {
            x.setcc(indent, "e",
                    x.get_sized_register_operand(dst_info.operand.str(),
                                                 operand::size_byte));
            return;
        }

        // memory operand does not happen in current bool implementation

        std::unreachable();
    }
};
