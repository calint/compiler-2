#pragma once
// reviewed: 2025-09-29

#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "decouple.hpp"
#include "expr_bool_ops_list.hpp"
#include "expr_ops_list.hpp"
#include "expr_type_value.hpp"

class expr_any final : public statement {
    std::variant<expr_ops_list, expr_bool_ops_list, expr_type_value> var_;

    // helper template for nicer handling of variants using overloaded lambdas
    template <class... Ts> struct overloaded : Ts... {
        using Ts::operator()...;
    };

  public:
    expr_any(toc& tc, tokenizer& tz, const type& tp, const bool in_args,
             [[maybe_unused]] const bool is_array = false)
        : statement{tz.next_whitespace_token()} {

        set_type(tp);

        if (not tp.is_built_in()) {
            // destination is not a built-in (register) value
            // assume assign type value
            var_ = expr_type_value{tc, tz, tp};
            return;
        }

        if (tp.name() == tc.get_type_bool().name()) {
            // destination is boolean
            var_ = expr_bool_ops_list{tc, tz.next_whitespace_token(), tz};
            return;
        }

        // destination is a built-in (register) value
        var_ = expr_ops_list{tc, tz, in_args};
    }

    ~expr_any() override = default;

    expr_any() = default;
    expr_any(const expr_any&) = default;
    expr_any(expr_any&&) = default;
    auto operator=(const expr_any&) -> expr_any& = default;
    auto operator=(expr_any&&) -> expr_any& = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        std::visit([&os](const auto& e) -> void { e.source_to(os); }, var_);
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 const ident_info& dst_info) const -> void override {
        std::visit(
            overloaded{
                [&](const expr_ops_list& e) -> void {
                    e.compile(tc, os, indent, dst_info);
                },
                [&]([[maybe_unused]] const expr_type_value& e) -> void {
                    std::unreachable();
                },
                [&](const expr_bool_ops_list& e) -> void {
                    // if not expression assign to destination
                    if (not e.is_expression()) {
                        const ident_info& src_info{tc.make_ident_info(e)};
                        if (src_info.is_const()) {
                            tc.asm_cmd(tok(), os, indent, "mov",
                                       dst_info.operand.str(),
                                       std::to_string(src_info.const_value));
                            return;
                        }
                        tc.asm_cmd(tok(), os, indent, "mov",
                                   dst_info.operand.str(),
                                   src_info.operand.str());
                        return;
                    }

                    // expression - make unique labels considering in-lined
                    // functions
                    const std::string_view call_path{tc.get_call_path(tok())};
                    const std::string src_loc{
                        tc.source_location_for_use_in_label(tok())};

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
                            tc.asm_cmd(tok(), os, indent, "mov",
                                       dst_info.operand.str(), "1");
                        } else {
                            // constant evaluation is false
                            tc.asm_cmd(tok(), os, indent, "mov",
                                       dst_info.operand.str(), "0");
                        }
                    }
                }},
            var_);
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        return std::visit(
            [](const auto& e) -> bool { return e.is_expression(); }, var_);
    }

    [[nodiscard]] auto is_indexed() const -> bool override {
        return std::visit([](const auto& e) -> bool { return e.is_indexed(); },
                          var_);
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        return std::visit(
            [](const auto& e) -> std::string_view { return e.identifier(); },
            var_);
    }

    auto assert_var_not_used(const std::string_view var) const
        -> void override {
        std::visit(
            [&var](const auto& e) -> void { e.assert_var_not_used(var); },
            var_);
    }

    [[nodiscard]] auto get_unary_ops() const -> const unary_ops& override {
        return std::visit(
            [](const auto& e) -> const unary_ops& { return e.get_unary_ops(); },
            var_);
        // note: 'expr_type_value' does not have 'unary_ops' and cannot be
        //       an argument in call
    }

    [[nodiscard]] auto is_expr_type_value() const -> bool {
        return std::holds_alternative<expr_type_value>(var_);
    }

    [[nodiscard]] auto is_identifier() const -> bool override {
        return std::visit(
            [](const auto& e) -> bool { return e.is_identifier(); }, var_);
    }

    auto compile_lea(const token& src_loc_tk, toc& tc, std::ostream& os,
                     size_t indent,
                     std::vector<std::string>& allocated_registers,
                     const std::string& reg_size,
                     const std::span<const std::string> lea_path) const
        -> operand override {

        return std::visit(
            [&](const auto& e) -> operand {
                return e.compile_lea(src_loc_tk, tc, os, indent,
                                     allocated_registers, reg_size, lea_path);
            },
            var_);
    }

    [[nodiscard]] auto as_expr_type_value() const -> const expr_type_value& {
        return get<expr_type_value>(var_);
    }
};
