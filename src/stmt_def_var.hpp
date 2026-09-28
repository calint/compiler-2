#pragma once
// reviewed: 2025-09-28
//           2026-09-09

#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_arith.hpp"
#include "expr_bool.hpp"
#include "stmt_assign_var.hpp"
#include "stmt_const.hpp"
#include "stmt_identifier.hpp"
#include "token.hpp"
#include "type.hpp"

class stmt_def_var final : public statement {
    token name_tk_;
    token type_tk_;
    token open_bracket_tk_;
    stmt_const array_count_const_;
    size_t array_count_{};
    token close_bracket_tk_;
    token equals_tk_;
    std::unique_ptr<stmt_assign_var> assign_var_;
    bool is_array_{};

  public:
    stmt_def_var(toc& tc, const token tk, tokenizer& tz)
        : statement{tk}, name_tk_{tz.next_token()} {

        toc::assert_name_not_reserved(name_tk_);

        open_bracket_tk_ = tz.is_next_char_token('[');
        if (not open_bracket_tk_.is_empty()) {
            parse_array_size(tc, tz);
        }

        type_tk_ = tz.next_token();
        if (not tc.has_type(type_tk_.text())) {
            tz.put_back_token(type_tk_);
            type_tk_ = {};
        }

        // expect initialization
        equals_tk_ = tz.is_next_char_token('=');
        const bool init_required{not equals_tk_.is_empty()};

        // e.g. 'var a = i8[]{1, 2}' is an array by its initializer
        if (init_required and type_tk_.is_empty() and not is_array_) {
            is_array_ = starts_array_literal(tc, tz);
        }

        set_type(declared_type(tc, tz));

        // add var to toc without emitting output so the further parsing has the
        // variable declared
        tc.add_var(name_tk_, 0, make_var_info(), false);

        if (init_required) {
            stmt_identifier si{tc, {}, name_tk_, tz};
            assign_var_ = std::make_unique<stmt_assign_var>(
                tc, tz, std::move(si), equals_tk_, is_array_, array_count_);

            if (is_array_ and array_count_ == 0) {
                array_count_ = assign_var_->array_count();
                if (array_count_ == 0) {
                    throw compiler_exception{
                        name_tk_, "expected array size greater than 0"};
                }
            }
        }

        // the newly defined variable is not yet assigned in its initialization
        assert_var_not_used(
            name_tk_.text(),
            field_coverage{multiply_storage_size(
                get_type().size_bytes(), is_array_ ? array_count_ : 1)});
    }

    stmt_def_var() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        if (is_array_) {
            open_bracket_tk_.source_to(os);
            array_count_const_.source_to(os);
            close_bracket_tk_.source_to(os);
        }
        if (not type_tk_.is_empty()) {
            type_tk_.source_to(os);
        }
        if (assign_var_) {
            equals_tk_.source_to(os);
            assign_var_->expression().source_to(os);
        }
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        // an unsized array has its size from the initializer by now
        tc.add_var(name_tk_, indent, make_var_info(), false);

        const ident_info& var_dst_info{
            tc.make_ident_info(name_tk_, name_tk_.text())};

        if (assign_var_) {
            assign_var_->compile(tc, indent, var_dst_info);
            return;
        }

        // zero the variable data

        const size_t instance_count{array_count_ ? array_count_ : 1};
        const size_t size_bytes{multiply_storage_size(
            var_dst_info.type_ref().size_bytes(), instance_count)};

        x.comment(name_tk_, indent, "zero {} * {} B = {} B", instance_count,
                  var_dst_info.type_ref().size_bytes(), size_bytes);

        x.zero(tok(), indent, var_dst_info.operand, size_bytes,
               var_dst_info.type_ref().alignment());
    }

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

        if (assign_var_) {
            assign_var_->visit_reads(var, reader);
        }
    }

  private:
    // e.g. 'var b = x < 3' is a 'bool' and 'var p = point.at(1, 2)' a 'point'
    [[nodiscard]] auto declared_type(toc& tc, tokenizer& tz) const
        -> const type& {

        if (not type_tk_.is_empty()) {
            return tc.get_type_or_throw(type_tk_, type_tk_.text());
        }

        if (equals_tk_.is_empty()) {
            return tc.get_type_default();
        }

        if (is_array_) {
            return array_element_type(tc, tz);
        }

        // the parser of the initializer depends on its type so it is parsed
        // again once the type is known
        const token start_tk{tz.cur_position_token()};
        const type& tp{initializer_type(tc, tz)};
        tz.rewind_to_position(start_tk);

        return tp;
    }

    [[nodiscard]] auto make_var_info() const -> var_info {
        return {
            .name{name_tk_.text()},
            .type_ptr{&get_type()},
            .src_loc_tk{name_tk_},
            .is_array{is_array_},
            .array_len{array_count_},
            .reg{},
            .base_register{},
        };
    }

    // e.g. '[4]', or '[]' when the initializer gives the size
    auto parse_array_size(toc& tc, tokenizer& tz) -> void {
        is_array_ = true;

        array_count_const_ = {tc, tz, 0};

        if (array_count_const_.has_value()) {
            if (array_count_const_.value() <= 0) {
                throw compiler_exception{
                    array_count_const_.tok(),
                    "expected a constant array size greater than 0"};
            }

            array_count_ = static_cast<size_t>(array_count_const_.value());
        }

        close_bracket_tk_ = tz.is_next_char_token(']');
        if (close_bracket_tk_.is_empty()) {
            throw compiler_exception{tz, "expected ']' after array size"};
        }
    }

    //
    // statics
    //

    // e.g. 'i8' in 'i8[]{1, 2}', '{1, 2}' alone has the default type
    [[nodiscard]] static auto array_element_type(const toc& tc, tokenizer& tz)
        -> const type& {

        if (not starts_array_literal(tc, tz)) {
            return tc.get_type_default();
        }

        const token tk{tz.next_token()};
        tz.put_back_token(tk);

        return tc.get_type_or_throw(tk, tk.text());
    }

    // e.g. 'point{1, 2}', 'x < 3', 'flag', 'p', 'f(x)', 'i32(x)' or 'a + 1'
    [[nodiscard]] static auto initializer_type(toc& tc, tokenizer& tz)
        -> const type& {

        // 'expr_arith' cannot parse the '{' of a record literal
        const token tk{tz.next_token()};
        if (is_record_literal(tc, tk, tz)) {
            return tc.get_type_or_throw(tk, tk.text());
        }

        tz.put_back_token(tk);

        // 'expr_bool' parses any arithmetic too, as a comparison shorthand
        const expr_bool bol{tc, tz.next_whitespace_token(), tz};

        // a comparison, 'not', 'and' or 'or'
        const expr_arith* const arith{bol.arithmetic()};
        if (arith == nullptr) {
            return tc.get_type_bool();
        }

        // several operands have the default type like the constants
        if (not arith->is_single_operand()) {
            return tc.get_type_default();
        }

        // 'true' and 'false' are integer constants in arithmetic
        if (arith->is_identifier() and
            (arith->identifier() == "true" or arith->identifier() == "false")) {

            return tc.get_type_bool();
        }

        return arith->single_operand_type();
    }

    // e.g. 'i8[3]{1, 2}' or 'point[]{{1, 2}}'
    [[nodiscard]] static auto starts_array_literal(const toc& tc, tokenizer& tz)
        -> bool {

        const token tk{tz.next_token()};
        const bool is_literal{is_array_literal(tc, tk, tz)};
        tz.put_back_token(tk);

        return is_literal;
    }
};
