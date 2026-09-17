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

class stmt_builtin_array_copy final : public statement {
    token open_paren_tk_;
    stmt_identifier from_;
    token from_delim_tk_;
    stmt_identifier to_;
    token to_delim_tk_;
    expr_any count_;
    token close_paren_tk_;

  public:
    stmt_builtin_array_copy(toc& tc, token tk, tokenizer& tz)
        : statement{tk}, open_paren_tk_{tz.is_next_char_token('(')} {

        if (open_paren_tk_.is_empty()) {
            throw compiler_exception{
                tz, "expected '(', 'from', 'to', 'count', and ')'"};
        }

        set_type(tc.get_type_void());

        from_ = {tc, {}, tz.next_token(), tz};

        from_delim_tk_ = tz.is_next_char_token(',');
        if (from_delim_tk_.is_empty()) {
            throw compiler_exception{
                tz, "expected ',' followed by 'to' and 'count'"};
        }

        to_ = {tc, {}, tz.next_token(), tz};

        to_delim_tk_ = tz.is_next_char_token(',');
        if (to_delim_tk_.is_empty()) {
            throw compiler_exception{tz, "expected ',' followed by 'count'"};
        }

        count_ = {tc, tz, tc.get_type_default(), true, false, 0};

        close_paren_tk_ = tz.is_next_char_token(')');
        if (close_paren_tk_.is_empty()) {
            throw compiler_exception{tz, "expected ')' after the arguments"};
        }
    }

    stmt_builtin_array_copy() = default;

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
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        x.comment_source(*this, indent);

        const ident_info from_info{tc.make_ident_info(x, from_)};
        const ident_info to_info{tc.make_ident_info(x, to_)};

        // allocate the register for rep movs
        x.alloc_named_register_or_throw(tok(), indent, "rsi",
                                         tc.get_type_default());
        x.alloc_named_register_or_throw(tok(), indent, "rdi",
                                         tc.get_type_default());
        x.alloc_named_register_or_throw(tok(), indent, "rcx",
                                         tc.get_type_default());

        std::vector<std::string> allocated_scratch_registers;

        // size to 'rcx'
        x.comment_source(count_, indent);
        count_.compile(tc, x, indent, tc.make_ident_info_for_register(x, "rcx"));

        // from operand to rsi
        x.comment_source(from_, indent);
        const operand from_operand{stmt_identifier::compile_effective_address(
            from_.first_token(), tc, x, indent, from_.elems(),
            allocated_scratch_registers, "rcx", from_info.lea_path)};

        x.lea( indent, "rsi", from_operand.address_str());

        for (const std::string& reg :
             allocated_scratch_registers | std::views::reverse) {
            x.free_scratch_register(tok(), indent, reg);
        }

        // to operand to 'rdi'
        allocated_scratch_registers.clear();
        x.comment_source(to_, indent);
        const operand to_operand{stmt_identifier::compile_effective_address(
            to_.first_token(), tc, x, indent, to_.elems(),
            allocated_scratch_registers, "rcx", to_info.lea_path)};

        x.lea( indent, "rdi", to_operand.address_str());

        for (const std::string& reg :
             allocated_scratch_registers | std::views::reverse) {
            x.free_scratch_register(tok(), indent, reg);
        }

        if (from_info.type().name() != to_info.type().name()) {
            throw compiler_exception{
                tok(),
                std::format("source type '{}' does not match destination "
                            "type '{}'",
                            from_info.type().name(), to_info.type().name())};
        }

        const size_t type_size{from_info.type().size()};

        if (type_size > 1) {
            // check whether it is possible to shift left instead of
            // multiplication
            if (const std::optional<int> shl{
                    stmt_identifier::get_shift_amount(type_size)};
                shl) {

                x.op(tok(), indent, "shl", "rcx",
                        std::format("{}", *shl));
            } else {
                x.op(tok(), indent, "imul", "rcx",
                        std::format("{}", type_size));
            }
        }

        // copy
        x.rep_movs( indent, 'b');
        // note: toc::rep_movs does not work because rsi, rdi and rcx are
        //       expresstion

        x.free_named_register(tok(), indent, "rcx");
        x.free_named_register(tok(), indent, "rdi");
        x.free_named_register(tok(), indent, "rsi");
    }
};
