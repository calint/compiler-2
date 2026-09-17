#pragma once // reviewed: 2025-09-28
//           2026-09-09

#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "null_stream.hpp"
#include "stmt_assign_var.hpp"
#include "stmt_const.hpp"
#include "stmt_identifier.hpp"
#include "token.hpp"
#include "type.hpp"

class stmt_def_var final : public statement {
    static constexpr size_t threshold_for_rep_stos{32};

    token name_tk_;
    token type_delim_tk_;
    token type_tk_;
    token open_bracket_tk_;
    stmt_const array_size_const_;
    size_t array_size_{};
    token close_bracket_tk_;
    token equals_tk_;
    std::unique_ptr<stmt_assign_var> assign_var_;
    bool is_array_{};

  public:
    stmt_def_var(toc& tc, token tk, tokenizer& tz)
        : statement{tk}, name_tk_{tz.next_token()},
          type_delim_tk_{tz.is_next_char_token(':')} {

        // check if type declared
        if (not type_delim_tk_.is_empty()) {
            type_tk_ = tz.next_token();
            open_bracket_tk_ = tz.is_next_char_token('[');
            if (not open_bracket_tk_.is_empty()) {
                is_array_ = true;

                array_size_const_ = {tc, tz, 0};

                if (array_size_const_.has_value()) {
                    if (array_size_const_.value() <= 0) {
                        throw compiler_exception{
                            array_size_const_.tok(),
                            "expected a constant array size greater than 0"};
                    }
                    array_size_ =
                        static_cast<size_t>(array_size_const_.value());
                }

                close_bracket_tk_ = tz.is_next_char_token(']');
                if (close_bracket_tk_.is_empty()) {
                    throw compiler_exception{tz,
                                             "expected ']' after array size"};
                }
            }
        }

        // get type reference from the token
        const type& tp{type_tk_.text().empty()
                           ? tc.get_type_default()
                           : tc.get_type_or_throw(type_tk_, type_tk_.text())};
        set_type(tp);

        // expect initialization
        equals_tk_ = tz.is_next_char_token('=');
        const bool init_required{not equals_tk_.is_empty()};

        // add var to toc without causing output by passing a null stream
        null_stream null_strm;
        const var_info var{
            .name{name_tk_.text()},
            .type_ptr{&tp},
            .declared_at_tk{name_tk_},
            .is_array{is_array_},
            .array_size{array_size_},
            .reg{},
        };
        tc.add_var(name_tk_, null_strm, 0, var, false);

        if (init_required) {
            stmt_identifier si{tc, {}, name_tk_, tz};
            assign_var_ = std::make_unique<stmt_assign_var>(
                tc, tz, std::move(si), equals_tk_, is_array_, array_size_);
            if (is_array_ and array_size_ == 0) {
                array_size_ = assign_var_->array_size();
                if (array_size_ == 0) {
                    throw compiler_exception{
                        name_tk_, "expected array size greater than 0"};
                }
            }
        }

        assert_var_not_used(name_tk_.text());
        // note: asserts that the newly defined variable isn't used in the
        //       initialization
    }

    stmt_def_var() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        if (not type_tk_.is_empty()) {
            type_delim_tk_.source_to(os);
            type_tk_.source_to(os);
            if (is_array_) {
                open_bracket_tk_.source_to(os);
                array_size_const_.source_to(os);
                close_bracket_tk_.source_to(os);
            }
        }
        if (assign_var_) {
            equals_tk_.source_to(os);
            assign_var_->expression().source_to(os);
        }
    }

    auto compile(toc& tc, x86& x, const size_t indent,
                 [[maybe_unused]] const ident_info& dst) const
        -> void override {

        x.comment_source(tok(), indent, statement::trimmed_source(*this));

        const var_info var{
            .name{name_tk_.text()},
            .type_ptr{&get_type()},
            .declared_at_tk{name_tk_},
            .is_array{is_array_},
            .array_size{array_size_},
            .reg{},
        };
        tc.add_var(name_tk_, x.os, indent, var, false);

        const ident_info& dst_info{
            tc.make_ident_info(x, name_tk_, name_tk_.text())};

        if (assign_var_) {
            assign_var_->compile(tc, x, indent, dst_info);
            return;
        }

        // zero the variable data

        const size_t instance_count{array_size_ ? array_size_ : 1};
        const size_t bytes_count{instance_count * dst_info.type().size()};

        x.comment_start(name_tk_, indent);
        x.println( "zero {} * {} B = {} B", instance_count,
                     dst_info.type().size(), bytes_count);

        const std::string dst_addr{std::format("rsp - {}", -dst_info.stack_ix)};
        // note: -dst_info.stack_ix for nicer source formatting; is always
        //       negative
        x.zero(tok(), indent, dst_addr, bytes_count);
    }

    auto assert_var_not_used(const std::string_view var) const
        -> void override {

        if (assign_var_) {
            assign_var_->assert_var_not_used(var);
        }
    }
};
