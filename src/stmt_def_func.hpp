#pragma once
// reviewed: 2025-09-28

#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "stmt_block.hpp"
#include "stmt_def_func_param.hpp"

class stmt_def_func final : public statement {
    token name_tk_;
    token open_paren_tk_;
    std::vector<stmt_def_func_param> params_;
    std::vector<token> param_delims_tk_;
    token close_paren_tk_;
    token return_delim_tk_;
    std::optional<func_return_info> returns_;
    stmt_block code_;

  public:
    stmt_def_func(toc& tc, const token tk, tokenizer& tz)
        : statement{tk}, name_tk_{tz.next_token()},
          open_paren_tk_{tz.is_next_char_token('(')} {

        if (open_paren_tk_.is_empty()) {
            throw compiler_exception{name_tk_,
                                     "expected '(' after function name"};
        }

        // read parameter definitions
        size_t counter{};
        while (true) {
            close_paren_tk_ = tz.is_next_char_token(')');
            if (not close_paren_tk_.is_empty()) {
                break;
            }

            if (counter++) {
                const token t{tz.is_next_char_token(',')};
                if (t.is_empty()) {
                    throw compiler_exception{
                        tz,
                        std::format("expected ',' or ')' after parameter '{}'",
                                    params_.back().tok().text())};
                }
                param_delims_tk_.emplace_back(t);
            }

            params_.emplace_back(tc, tz);
        }

        return_delim_tk_ = tz.is_next_char_token(':');
        if (not return_delim_tk_.is_empty()) {
            // function returns
            const token type_tk{tz.next_token()};
            const token ident_tk{tz.next_token()};
            const type& tp{tc.get_type_or_throw(type_tk, type_tk.text())};
            returns_.emplace(type_tk, ident_tk, &tp);
            set_type(tp);
        } else {
            // no return, set type to 'void'
            set_type(tc.get_type_void());
        }

        tc.add_func(name_tk_, std::string{name_tk_.text()},
                    statement::get_type(), this);

        // establish the function scope before parsing its body
        tc.enter_func(name(), returns_);

        // register variables without emitting output so that the function body
        // can be parsed

        if (returns_) {
            // declare the return variable
            const token& ret_tk{returns_->ident_tk};

            if (ret_tk.text().empty()) {
                throw compiler_exception(ret_tk,
                                         "expected return reference name");
            }

            const var_info var{
                .name{ret_tk.text()},
                .type_ptr{&get_type()},
                .src_loc_tk{ret_tk},
                .reg{},
            };

            tc.add_var(ret_tk, 0, var, false);
        }

        for (const stmt_def_func_param& param : params_) {
            const type& param_type{param.get_type()};
            const std::string_view param_name{param.name()};

            const var_info var{
                .name{param_name},
                .type_ptr{&param_type},
                .src_loc_tk{param.tok()},
                .is_array{param.is_array()},
                .reg{},
            };

            tc.add_var(param.tok(), 0, var, false);
        }

        code_ = {tc, tz};

        tc.exit_func(name());
    }

    stmt_def_func() = default;

    auto source_to(std::ostream& os) const -> void override {
        source_def_to(os, false);
        code_.source_to(os);
    }

    auto source_def_to(std::ostream& os, const bool summary) const -> void {
        if (not summary) {
            statement::source_to(os);
        }

        name_tk_.source_to(os);
        open_paren_tk_.source_to(os);
        if (not params_.empty()) {
            params_.front().source_to(os);
            for (const auto [d, e] : std::views::zip(
                     param_delims_tk_, params_ | std::views::drop(1))) {

                d.source_to(os);
                e.source_to(os);
            }
        }
        close_paren_tk_.source_to(os);

        if (returns_) {
            return_delim_tk_.source_to(os);
            returns_->type_tk.source_to(os);
            returns_->ident_tk.source_to(os);
        }
    }

    auto source_def_comment_to(machine& x, const size_t indent) const -> void {
        std::stringstream ss;
        source_def_to(ss, true);
        x.comment(name_tk_, indent, "{}", statement::trimmed_source(ss.str()));
    }

    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {}

    [[nodiscard]] auto returns() const
        -> const std::optional<func_return_info>& {

        return returns_;
    }

    [[nodiscard]] auto param(const size_t ix) const
        -> const stmt_def_func_param& {

        return params_[ix];
    }

    [[nodiscard]] auto params() const -> std::span<const stmt_def_func_param> {
        return params_;
    }

    [[nodiscard]] auto code() const -> const stmt_block& { return code_; }

    [[nodiscard]] auto name() const -> std::string_view {
        return name_tk_.text();
    }
};
