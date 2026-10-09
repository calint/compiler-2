#pragma once
// reviewed: 2025-09-28
//           2026-09-09

#include <cstddef>
#include <format>
#include <functional>
#include <ostream>
#include <string_view>
#include <utility>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_arith.hpp"
#include "expr_bool.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "stmt_assign_var.hpp"
#include "stmt_builtin_convert.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "type.hpp"
#include "ub_check.hpp"

// e.g. 'var x = i32(0)', the initializer gives the type; 'let' with a
// non-constant initializer is a 'var' that is read-only once initialized
class stmt_def_var final : public statement {
    token name_tk_;
    size_t array_count_{};
    token equals_tk_;
    stmt_assign_var assign_var_;
    bool is_array_{};
    bool is_let_{};

  public:
    stmt_def_var(toc& tc, const token tk, tokenizer& tz)
        : statement{tk}, name_tk_{tz.next_token()}, is_let_{tk.is_text("let")} {

        toc::assert_valid_name(name_tk_);

        const token after_name_tk{tz.cur_position_token()};

        equals_tk_ = parse_initializer_equals(tz, tk.text());

        deduce_declaration(tc, tz);

        // add var to toc without emitting output so the further parsing has the
        // variable declared
        tc.add_var(name_tk_, 0, make_var_info(), var_kind::var);

        // the identifier is parsed where it ends at '=', past it the '[2]' in
        // 'var a = [2]{}' would read as an index
        tz.rewind_to_position(after_name_tk);
        stmt_identifier si{tc, {}, name_tk_, tz};
        equals_tk_ = parse_initializer_equals(tz, tk.text());

        assign_var_ = {tc, tz, std::move(si), equals_tk_};

        // the newly defined variable is not yet assigned in its initialization
        assert_var_not_used(name_tk_.text(),
                            field_coverage{multiply_storage_size(
                                name_tk_, get_type().size_bytes(),
                                is_array_ ? array_count_ : 1)});

        // marked after the initializer, which writes the variable; statements
        // parsed from here on cannot assign it
        if (is_let_) {
            tc.make_var_read_only(name_tk_.text(), read_only_cause::let);
        }
    }

    stmt_def_var() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        equals_tk_.source_to(os);
        assign_var_.expression().source_to(os);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        // an unsized array has its size from the initializer by now
        tc.add_var(name_tk_, indent, make_var_info(), var_kind::var);

        const ident_info& var_dst_info{
            tc.make_ident_info(name_tk_, name_tk_.text()),
        };

        assign_var_.compile(tc, indent, var_dst_info);
    }

    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        assign_var_.visit_reads(var, reader);
    }

  private:
    // e.g. 'var c = a' copies the whole array 'a'
    auto deduce_array_copy(toc& tc, const token& tk, const expr_arith& arith)
        -> void {

        const ident_info info{tc.make_ident_info(arith)};

        if (not info.is_array) {
            return;
        }

        // e.g. an unsized array parameter
        if (info.array_len == 0) {
            throw compiler_exception{
                tk, std::format("size of array '{}' is not known",
                                arith.identifier())};
        }

        is_array_ = true;
        array_count_ = info.array_len;
    }

    // e.g. 'var b = x < 3' is a 'bool', 'var a = i8[]{1, 2}' an 'i8' array and
    // 'var s = "hi"' an 'i8' array of 2
    auto deduce_declaration(toc& tc, tokenizer& tz) -> void {
        const bool is_string{tz.peek_char_after_whitespace() == '"'};

        if (not is_string and not starts_array_literal(tc, tz)) {
            set_type(tc.get_type_default());

            trial_parse(tc, tz, [this, &tc, &tz] -> void {
                deduce_from_initializer(tc, tz);
            });

            return;
        }

        is_array_ = true;
        set_type(is_string ? tc.get_type_i8() : array_element_type(tc, tz));

        // later statements read the size while parsing, e.g. 'var b = a'
        token bracket_tk;

        trial_parse(tc, tz, [this, &tc, &tz, &bracket_tk] -> void {
            const expr_any initializer{tc, tz, get_type(), false, true, 0};

            array_count_ = initializer.array_count();
            bracket_tk = initializer.open_bracket_token();
        });

        if (array_count_ == 0) {
            // e.g. the '[' of 'var a = i8[]{}', a string has no bracket
            throw compiler_exception{
                bracket_tk.is_empty() ? name_tk_ : bracket_tk,
                "expected array size greater than 0",
            };
        }
    }

    // e.g. 'point{1, 2}', 'x < 3', 'flag', 'p', 'f(x)', 'i32(x)', 'a + 1' or
    // the array 'a'
    auto deduce_from_initializer(toc& tc, tokenizer& tz) -> void {
        // 'expr_arith' cannot parse the '{' of an instance literal
        const token tk{tz.next_token()};

        if (is_record_literal(tc, tk, tz) or is_bare_record_type(tc, tk, tz)) {
            set_type(tc.get_type_or_throw(tk, tk.text()));
            return;
        }

        // e.g. 'var x = i8' is 'i8(0)'
        if (is_bare_builtin_type(tc, tk, tz)) {
            set_type(stmt_builtin_convert::conversion_type(tc, tk));
            return;
        }

        tz.put_back_token(tk);

        // 'expr_bool' parses any arithmetic too, as a comparison shorthand
        const expr_bool condition{tc, tz.next_whitespace_token(), tz};

        // a comparison, 'not', 'and' or 'or'
        const expr_arith* const arith{condition.arithmetic()};

        if (arith == nullptr) {
            set_type(tc.get_type_bool());
            return;
        }

        // several operands have the default type like the constants
        if (not arith->is_single_operand()) {
            set_type(tc.get_type_default());
            return;
        }

        set_type(arith->single_operand_type());

        if (not arith->is_identifier() or
            not arith->get_unary_ops().is_empty()) {

            return;
        }

        deduce_array_copy(tc, tk, *arith);
    }

    [[nodiscard]] auto make_var_info() const -> var_info {
        return {
            .name{name_tk_.text()},
            .type_ptr{&get_type()},
            .src_loc_tk{name_tk_},
            .is_array{is_array_},
            .array_len{array_count_},
            .pointer_register{},
            .base_register{},
            .value_register{},
            .foo_array{},
        };
    }

    // the initializer is parsed to learn the type or size and parsed again
    // once they are known; a placeholder variable resolves a read in its own
    // initializer, e.g. 'var x = x + 1', which is rejected later as
    // uninitialized
    auto trial_parse(toc& tc, tokenizer& tz,
                     const std::function_ref<void()> parse) const -> void {

        const token start_tk{tz.cur_position_token()};

        tc.enter_block();
        tc.add_var(name_tk_, 0, make_var_info(), var_kind::var);

        parse();

        tc.exit_block();
        tz.rewind_to_position(start_tk);
    }

    //
    // statics
    //

    // e.g. 'i8' in 'i8[]{1, 2}' or the default type in '[]{1, 2}'
    [[nodiscard]] static auto array_element_type(const toc& tc, tokenizer& tz)
        -> const type& {

        if (is_default_array_literal(tz)) {
            return tc.get_type_default();
        }

        const token tk{tz.next_token()};
        tz.put_back_token(tk);

        return tc.get_type_or_throw(tk, tk.text());
    }

    // e.g. 'i8[3]{1, 2}', '[]{1, 2}' or 'point[]{{1, 2}}'
    [[nodiscard]] static auto starts_array_literal(const toc& tc, tokenizer& tz)
        -> bool {

        if (is_default_array_literal(tz)) {
            return true;
        }

        const token tk{tz.next_token()};
        const bool is_literal{is_array_literal(tc, tk, tz)};
        tz.put_back_token(tk);

        return is_literal;
    }
};
