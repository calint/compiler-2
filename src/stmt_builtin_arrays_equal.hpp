#pragma once

#include <format>
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

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        x86& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const std::string_view count_register{
            x.begin_memory_equal(tok(), indent)};

        std::vector<std::string> allocated_scratch_registers;

        x.comment(count_.tok(), indent, statement::trimmed_source(count_));

        count_.compile(tc, indent,
                       tc.make_ident_info_from_register(count_register));

        const ident_info from_info{tc.make_ident_info(from_)};
        const ident_info to_info{tc.make_ident_info(to_)};

        if (from_info.type_ref().name() != to_info.type_ref().name()) {
            throw compiler_exception{
                tok(),
                std::format("source type '{}' does not match compare type '{}'",
                            from_info.type_ref().name(),
                            to_info.type_ref().name())};
        }

        if (dst_info.type_ref().name() != get_type().name()) {
            throw compiler_exception{
                tok(),
                std::format("destination type must be '{}', not '{}'",
                            get_type().name(), dst_info.type_ref().name())};
        }

        x.comment(from_.tok(), indent, statement::trimmed_source(from_));

        const operand from_operand{stmt_identifier::compile_effective_address(
            tc, indent, from_.first_token(), from_.elems(),
            allocated_scratch_registers, count_register, from_info.lea_path)};

        x.set_memory_equal_left(indent, from_operand.address_str());

        for (const std::string& reg :
             allocated_scratch_registers | std::views::reverse) {

            x.free_scratch_register(tok(), indent, reg);
        }

        x.comment(to_.tok(), indent, statement::trimmed_source(to_));

        allocated_scratch_registers.clear();

        const operand to_operand{stmt_identifier::compile_effective_address(
            tc, indent, to_.first_token(), to_.elems(),
            allocated_scratch_registers, count_register, to_info.lea_path)};

        x.set_memory_equal_right(indent, to_operand.address_str());

        for (const std::string& reg :
             allocated_scratch_registers | std::views::reverse) {

            x.free_scratch_register(tok(), indent, reg);
        }

        if (dst_info.is_register()) {
            x.end_arrays_equal(tok(), indent, from_info.type_ref().size(),
                               dst_info.operand);

            return;
        }

        // memory operand does not happen in current bool implementation

        std::unreachable();
    }
};
