#pragma once
// reviewed: 2025-09-29

#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "decouple.hpp"
#include "expr_bool_ops_list.hpp"
#include "expr_ops_list.hpp"
#include "expr_type_value.hpp"

class expr_any final : public statement {
    using expr_var =
        std::variant<expr_ops_list, expr_bool_ops_list, expr_type_value>;

    // helper template for nicer handling of variants using overloaded lambdas
    template <class... Ts> struct overloaded : Ts... {
        using Ts::operator()...;
    };

    std::vector<expr_var> vars_;
    size_t array_size_{};
    bool is_array_{};

  public:
    expr_any(toc& tc, tokenizer& tz, const type& tp, const bool in_args,
             const bool is_array, const size_t array_size)
        : statement{tz.next_whitespace_token()}, array_size_{array_size},
          is_array_{is_array} {

        set_type(tp);

        vars_.emplace_back(parse_variant(tc, tz, tp, in_args));
    }

    ~expr_any() override = default;

    expr_any() = default;
    expr_any(const expr_any&) = default;
    expr_any(expr_any&&) = default;
    auto operator=(const expr_any&) -> expr_any& = default;
    auto operator=(expr_any&&) -> expr_any& = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        std::visit([&os](const auto& e) -> void { e.source_to(os); }, vars_[0]);
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 const ident_info& dst_info) const -> void override {

        compile_variant(tc, os, indent, dst_info, tok(), vars_[0]);
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        return std::visit(
            [](const auto& e) -> bool { return e.is_expression(); }, vars_[0]);
    }

    [[nodiscard]] auto is_indexed() const -> bool override {
        return std::visit([](const auto& e) -> bool { return e.is_indexed(); },
                          vars_[0]);
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        return std::visit(
            [](const auto& e) -> std::string_view { return e.identifier(); },
            vars_[0]);
    }

    auto assert_var_not_used(const std::string_view var) const
        -> void override {
        std::visit(
            [&var](const auto& e) -> void { e.assert_var_not_used(var); },
            vars_[0]);
    }

    [[nodiscard]] auto get_unary_ops() const -> const unary_ops& override {
        return std::visit(
            [](const auto& e) -> const unary_ops& { return e.get_unary_ops(); },
            vars_[0]);
        // note: 'expr_type_value' does not have 'unary_ops' and cannot be
        //       an argument in call
    }

    [[nodiscard]] auto is_identifier() const -> bool override {
        return std::visit(
            [](const auto& e) -> bool { return e.is_identifier(); }, vars_[0]);
    }

    [[nodiscard]] auto
    compile_lea(const token& src_loc_tk, toc& tc, std::ostream& os,
                size_t indent, std::vector<std::string>& allocated_registers,
                const std::string& reg_size,
                const std::span<const std::string> lea_path) const
        -> operand override {

        return std::visit(
            [&](const auto& e) -> auto {
                return e.compile_lea(src_loc_tk, tc, os, indent,
                                     allocated_registers, reg_size, lea_path);
            },
            vars_[0]);
    }

    [[nodiscard]] auto as_expr_type_value() const -> const expr_type_value& {
        return get<expr_type_value>(vars_[0]);
    }

  private:
    [[nodiscard]] auto parse_variant(toc& tc, tokenizer& tz, const type& tp,
                                     const bool in_args) -> expr_var {

        if (not tp.is_built_in()) {
            // destination is not a built-in (register) value
            // assume assign type value
            return expr_type_value{tc, tz, tp};
        }

        if (tp.name() == tc.get_type_bool().name()) {
            // destination is boolean
            return expr_bool_ops_list{tc, tz.next_whitespace_token(), tz};
        }

        // destination is a built-in (register) value
        return expr_ops_list{tc, tz, in_args};
    }

    static auto compile_variant(toc& tc, std::ostream& os, const size_t indent,
                                const ident_info& dst_info, const token tk,
                                const expr_var& exp) -> void {
        std::visit(
            overloaded{
                [&](const expr_ops_list& e) -> auto {
                    e.compile(tc, os, indent, dst_info);
                },
                [&]([[maybe_unused]] const expr_type_value& e) -> auto {
                    e.compile(tc, os, indent, dst_info);
                },
                [&](const expr_bool_ops_list& e) -> auto {
                    // if not expression assign to destination
                    if (not e.is_expression()) {
                        const ident_info& src_info{tc.make_ident_info(e)};
                        if (src_info.is_const()) {
                            tc.asm_cmd(tk, os, indent, "mov",
                                       dst_info.operand.str(),
                                       std::to_string(src_info.const_value));
                            return;
                        }
                        tc.asm_cmd(tk, os, indent, "mov",
                                   dst_info.operand.str(),
                                   src_info.operand.str());
                        return;
                    }

                    // expression - make unique labels considering in-lined
                    // functions
                    const std::string_view call_path{tc.get_call_path(tk)};
                    const std::string src_loc{
                        tc.source_location_for_use_in_label(tk)};

                    // unique partial label for this assembler location
                    const std::string postfix{std::format(
                        "{}{}", src_loc,
                        (call_path.empty() ? std::string{}
                                           : std::format("_{}", call_path)))};

                    // labels to jump to depending on the evaluation
                    const std::string jmp_to_end{
                        std::format("bool_end_{}", postfix)};

                    // compile and possibly evaluate constant expression
                    const std::optional<bool> const_eval{
                        e.compile(tc, os, indent, jmp_to_end, jmp_to_end, false,
                                  dst_info.operand.str())};

                    // not constant evaluation
                    toc::asm_label(os, indent, jmp_to_end);

                    // did the evaluation result in a constant?
                    if (const_eval) {
                        // yes, constant evaluation
                        if (*const_eval) {
                            // constant evaluation is true
                            tc.asm_cmd(tk, os, indent, "mov",
                                       dst_info.operand.str(), "1");
                        } else {
                            // constant evaluation is false
                            tc.asm_cmd(tk, os, indent, "mov",
                                       dst_info.operand.str(), "0");
                        }
                    }
                }},
            exp);
    }
};
