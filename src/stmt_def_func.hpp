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
    std::vector<func_return_info> returns_;
    stmt_block code_;

  public:
    stmt_def_func(toc& tc, token tk, tokenizer& tz)
        : statement{std::move(tk)}, name_tk_{tz.next_token()} {

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
                                    params_.back().tok().name())};
            }
        }
        ws_after_params_ = tz.next_whitespace_token();
        if (tz.is_next_char(':')) {
            // function returns
            while (true) {
                token type_tk{tz.next_token()};
                token ident_tk{tz.next_token()};
                if (tz.is_next_char('.')) {
                    throw compiler_exception{
                        ident_tk, "return variable name may not contain '.'"};
                }

                const type& tp{tc.get_type_or_throw(type_tk, type_tk.name())};

                if (not tp.is_built_in()) {
                    throw compiler_exception{
                        type_tk, "only built-in types allowed as return"};
                }

                returns_.emplace_back(std::move(type_tk), std::move(ident_tk),
                                      tp);

                if (not tz.is_next_char(',')) {
                    break;
                }
            }
            // set function type to first return type
            set_type(returns_.at(0).type_ref);
        } else {
            // no return, set type to 'void'
            set_type(tc.get_type_void());
        }

        tc.add_func(name_tk_, name_tk_.name(), get_type(), this);
        // dry-run compilation to catch errors before called
        tc.enter_func(name(), returns_);
        std::vector<std::string> allocated_named_registers;
        null_stream null_strm; // don't make output
        init_variables(tc, null_strm, 0, allocated_named_registers);
        code_ = {tc, tz};
        free_allocated_named_registers(tc, null_strm, 0,
                                       allocated_named_registers);
        tc.exit_func(name());
    }

    stmt_def_func() = default;
    stmt_def_func(const stmt_def_func&) = default;
    stmt_def_func(stmt_def_func&&) = default;
    auto operator=(const stmt_def_func&) -> stmt_def_func& = default;
    auto operator=(stmt_def_func&&) -> stmt_def_func& = default;

    ~stmt_def_func() override = default;

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
        os << "(";
        {
            size_t i{};
            for (const stmt_def_func_param& p : params_) {
                if (i++) {
                    os << ",";
                }
                p.source_to(os);
            }
        }
        os << ")";
        ws_after_params_.source_to(os);
        if (not returns_.empty()) {
            // return parameters
            os << ":";
            size_t i{};
            for (const func_return_info& ret_info : returns_) {
                if (i++) {
                    os << ":";
                }
                ret_info.type_tk.source_to(os);
                ret_info.ident_tk.source_to(os);
            }
        }
    }

    auto source_def_comment_to(std::ostream& os) const -> void {
        std::stringstream ss;
        source_def_to(ss, true);
        ss << '\n';

        const std::string src{ss.str()};
        // make comment friendly string replacing consecutive with one space
        const std::string res{
            std::regex_replace(src, std::regex(R"(\s+)"), " ")};
        os << res << '\n';
    }

    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] std::ostream& os,
                 [[maybe_unused]] size_t indent,
                 [[maybe_unused]] const std::string& dst = "") const
        -> void override {}

    [[nodiscard]] auto returns() const -> const std::vector<func_return_info>& {
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

    [[nodiscard]] auto name() const -> const std::string& {
        return name_tk_.name();
    }

  private:
    auto
    init_variables(toc& tc, std::ostream& os, size_t indent,
                   std::vector<std::string>& allocated_named_registers) const
        -> void {

        // does function have return?
        if (not returns().empty()) {
            // yes, declare variable for the return
            const token& id_tkn{returns().at(0).ident_tk};
            tc.add_var(id_tkn, os, indent + 1, id_tkn.name(), get_type(), false,
                       0);
        }

        // functions get arguments as aliases
        for (const stmt_def_func_param& prm : params_) {
            const type& prm_type{prm.get_type()};
            const std::string& prm_name{prm.name()};
            const std::string& prm_reg{prm.get_register_name_or_empty()};

            // is argument passed as named register?
            if (prm_reg.empty()) {
                // no, add it as variable
                tc.add_var(tok(), os, indent + 1, prm_name, prm_type, false, 0);
                continue;
            }

            // argument passed as named register
            toc::indent(os, indent + 1, true);
            os << prm_name << ": " << prm_reg << '\n';

            tc.alloc_named_register_or_throw(prm, os, indent + 1, prm_reg);
            tc.add_alias(prm_name, prm_reg);
            allocated_named_registers.emplace_back(prm_reg);
        }
    }

    static auto
    free_allocated_named_registers(toc& tc, std::ostream& os, size_t indent,
                                   const std::vector<std::string>& registers)
        -> void {

        // free allocated named register in reverse order
        for (const auto& reg : registers | std::views::reverse) {
            tc.free_named_register(os, indent + 1, reg);
        }
    }
};
