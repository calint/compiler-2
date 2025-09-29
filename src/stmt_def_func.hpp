#pragma once
// reviewed: 2025-09-28

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
            throw compiler_exception(name_tk_,
                                     "expected '(' after function name");
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
                throw compiler_exception(params_.back().tok(),
                                         "expected ',' after parameter '" +
                                             params_.back().tok().name() + "'");
            }
        }
        ws_after_params_ = tz.next_whitespace_token();
        if (tz.is_next_char(':')) {
            // function returns
            while (true) {
                const token type_tk{tz.next_token()};
                const token ident_tk{tz.next_token()};
                if (ident_tk.name().contains('.')) {
                    throw compiler_exception(
                        ident_tk, "return variable name may not contain '.'");
                }
                const struct func_return_info ret_info{
                    .type_tk = type_tk,
                    .ident_tk = ident_tk,
                    .type_ref = tc.get_type_or_throw(type_tk, type_tk.name())};

                if (not ret_info.type_ref.is_built_in()) {
                    throw compiler_exception{
                        ret_info.type_tk,
                        "only built-in types allowed as return"};
                }

                returns_.emplace_back(ret_info);
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
        free_allocated_registers(tc, null_strm, 0, allocated_named_registers);
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
        const std::string res{regex_replace(src, std::regex(R"(\s+)"), " ")};
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
            tc.add_var(id_tkn, os, indent + 1, id_tkn.name(), get_type(),
                       false);
        }

        // functions get arguments as aliases
        const size_t n{params_.size()};
        for (size_t i{}; i < n; i++) {
            const stmt_def_func_param& param{params_.at(i)};
            const type& param_type{param.get_type()};
            const std::string& param_name{param.name()};
            const std::string& param_reg{param.get_register_name_or_empty()};

            // is argument passed as named register?
            if (param_reg.empty()) {
                // no, add it as variable
                tc.add_var(tok(), os, indent + 1, param_name, param_type, true);
                continue;
            }

            // argument passed as named register
            toc::indent(os, indent + 1, true);
            os << param_name << ": " << param_reg << '\n';

            tc.alloc_named_register_or_throw(param, os, indent + 1, param_reg);
            tc.add_alias(param_name, param_reg);
            allocated_named_registers.emplace_back(param_reg);
        }
    }

    static void free_allocated_registers(
        toc& tc, std::ostream& os, size_t indent,
        const std::vector<std::string>& allocated_named_registers) {

        // free allocated named register in reverse order
        size_t i{allocated_named_registers.size()};
        while (i--) {
            tc.free_named_register(os, indent + 1,
                                   allocated_named_registers.at(i));
        }
    }
};
