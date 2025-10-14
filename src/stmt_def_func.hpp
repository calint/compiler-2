#pragma once
// reviewed: 2025-09-28

#include <ranges>

#include "compiler_exception.hpp"
#include "stmt_block.hpp"
#include "stmt_def_func_param.hpp"

class stmt_def_func final : public statement {
    token name_tk_;
    std::vector<stmt_def_func_param> params_;
    token ws_after_params_; // whitespace after ')'
    std::optional<func_return_info> returns_;
    stmt_block code_;

  public:
    stmt_def_func(toc& tc, token tk, tokenizer& tz)
        : statement{tk}, name_tk_{tz.next_token()} {

        if (not tz.is_next_char('(')) {
            throw compiler_exception{name_tk_,
                                     "expected '(' after function name"};
        }
        // read parameters definition
        while (true) {
            if (tz.is_next_char(')')) {
                break;
            }
            params_.emplace_back(tc, tz);
            if (tz.is_next_char(')')) {
                break;
            }
            if (not tz.is_next_char(',')) {
                throw compiler_exception{
                    tz, std::format("expected ',' or ')' after parameter '{}'",
                                    params_.back().tok().text())};
            }
        }
        ws_after_params_ = tz.next_whitespace_token();
        if (tz.is_next_char(':')) {
            // function returns
            const token type_tk{tz.next_token()};
            const token ident_tk{tz.next_token()};
            if (tz.is_next_char('.')) {
                throw compiler_exception{
                    ident_tk, "return variable name may not contain '.'"};
            }

            const type& tp{tc.get_type_or_throw(type_tk, type_tk.text())};

            if (not tp.is_built_in()) {
                throw compiler_exception{
                    type_tk, "only built-in types allowed as return"};
            }

            returns_.emplace(type_tk, ident_tk, tp);

            // set function type to first return type
            set_type(tp);
        } else {
            // no return, set type to 'void'
            set_type(tc.get_type_void());
        }

        tc.add_func(name_tk_, std::string{name_tk_.text()},
                    statement::get_type(), this);
        // dry-run compilation to catch errors before called
        tc.enter_func(name(), returns_);
        std::vector<std::string> allocated_named_registers;
        null_stream null_strm; // don't make output
        init_variables(tc, null_strm, 0, allocated_named_registers);
        code_ = {tc, tz};
        free_allocated_named_registers(tc, null_strm, 0, tok(),
                                       allocated_named_registers);
        tc.exit_func(name());
    }

    ~stmt_def_func() override = default;

    stmt_def_func() = default;
    stmt_def_func(const stmt_def_func&) = default;
    stmt_def_func(stmt_def_func&&) = default;
    auto operator=(const stmt_def_func&) -> stmt_def_func& = delete;
    auto operator=(stmt_def_func&&) -> stmt_def_func& = delete;

    auto source_to(std::ostream& os) const -> void override {
        source_def_to(os, false);
        code_.source_to(os);
    }

    auto source_def_to(std::ostream& os, const bool summary) const -> void {
        if (not summary) {
            statement::source_to(os);
        }
        name_tk_.source_to(os);
        // function has parameters
        std::print(os, "(");
        {
            size_t i{};
            for (const stmt_def_func_param& p : params_) {
                if (i++) {
                    std::print(os, ",");
                }
                p.source_to(os);
            }
        }
        std::print(os, ")");
        ws_after_params_.source_to(os);
        if (returns_) {
            // return parameters
            std::print(os, ":");
            returns_->type_tk.source_to(os);
            returns_->ident_tk.source_to(os);
        }
    }

    auto source_def_comment_to(toc& tc, std::ostream& os,
                               const size_t indent) const -> void {

        std::stringstream ss;
        source_def_to(ss, true);
        std::println(ss, "");

        const std::string src{ss.str()};
        // make comment friendly string replacing consecutive with one space
        const std::string res{std::regex_replace(src, tc.regex_ws, " ")};

        tc.comment_start(name_tk_, os, indent);
        std::println(os, "{}", res);
    }

    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] std::ostream& os,
                 [[maybe_unused]] const size_t indent,
                 [[maybe_unused]] const std::string_view dst) const
        -> void override {}

    [[nodiscard]] auto returns() const
        -> const std::optional<func_return_info>& {

        return returns_;
    }

    [[nodiscard]] auto param(const size_t ix) const
        -> const stmt_def_func_param& {
        return params_.at(ix);
    }

    [[nodiscard]] auto params() const
        -> const std::vector<stmt_def_func_param>& {
        return params_;
    }

    [[nodiscard]] auto code() const -> const stmt_block& { return code_; }

    [[nodiscard]] auto name() const -> std::string_view {
        return name_tk_.text();
    }

  private:
    auto
    init_variables(toc& tc, std::ostream& os, const size_t indent,
                   std::vector<std::string>& allocated_named_registers) const
        -> void {

        // does the function have return?
        if (returns_) {
            // yes, declare variable for the return
            const token& ret_tk{returns_->ident_tk};
            const var_info var{
                .name{ret_tk.text()},
                .type_ref = get_type(),
                .declared_at_tk{ret_tk},
            };
            tc.add_var(ret_tk, os, indent + 1, var);
        }

        // functions get arguments as aliases
        for (const stmt_def_func_param& prm : params_) {
            const type& prm_type{prm.get_type()};
            const std::string_view prm_name{prm.name()};
            const std::string prm_reg{prm.get_register_name_or_empty()};

            // is argument passed as named register?
            if (prm_reg.empty()) {
                // no, add it as a variable
                const var_info var{
                    .name{prm_name},
                    .type_ref = prm_type,
                    .declared_at_tk{prm.tok()},
                    .is_array = prm.is_array(),
                };
                tc.add_var(tok(), os, indent + 1, var);
                continue;
            }

            // argument passed as a  named register
            toc::indent(os, indent + 1, true);
            std::println(os, "{}: {}", prm_name, prm_reg);

            tc.alloc_named_register_or_throw(prm.tok(), os, indent + 1,
                                             prm_reg);
            tc.add_alias(
                {.from = std::string{prm_name}, .to = prm_reg, .lea = ""});
            allocated_named_registers.emplace_back(prm_reg);
        }
    }

    static auto free_allocated_named_registers(
        toc& tc, std::ostream& os, const size_t indent, const token& src_loc_tk,
        const std::vector<std::string>& registers) -> void {

        // free allocated named register in reverse order
        for (const auto& reg : registers | std::views::reverse) {
            tc.free_named_register(src_loc_tk, os, indent + 1, reg);
        }
    }
};
