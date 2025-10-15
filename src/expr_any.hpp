#pragma once
// reviewed: 2025-09-29

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
    expr_any(toc& tc, tokenizer& tz, const type& tp, const bool in_args)
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
        std::visit([&os](const auto& itm) { itm.source_to(os); }, var_);
    }

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 const std::string_view dst) const -> void override {

        tc.comment_source(*this, os, indent);
        std::visit(
            overloaded{
                [&](const expr_ops_list& expr) {
                    expr.compile(tc, os, indent, dst);
                },
                [&](const expr_type_value& expr) {
                    expr.compile(tc, os, indent, dst);
                },
                [&](const expr_bool_ops_list& bol) {
                    // resolve the destination
                    const ident_info& dst_info{tc.make_ident_info(tok(), dst)};

                    // if not expression assign to destination
                    if (not bol.is_expression()) {
                        const ident_info& src_info{tc.make_ident_info(bol)};
                        tc.asm_cmd(tok(), os, indent, "mov", dst_info.id_nasm,
                                   src_info.id_nasm);
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
                    const std::string jmp_to_if_true{
                        std::format("bool_true_{}", postfix)};
                    const std::string jmp_to_if_false{
                        std::format("bool_false_{}", postfix)};
                    const std::string jmp_to_end{
                        std::format("bool_end_{}", postfix)};

                    // compile and possibly evaluate constant expression
                    std::optional<bool> const_eval{
                        bol.compile(tc, os, indent, jmp_to_if_false,
                                    jmp_to_if_true, false)};

                    // did the evaluation result in a constant?
                    if (const_eval) {
                        // yes, constant evaluation
                        if (*const_eval) {
                            // constant evaluation is true
                            toc::asm_label(tok(), os, indent, jmp_to_if_true);
                            tc.asm_cmd(tok(), os, indent, "mov",
                                       dst_info.id_nasm, "true");
                            return;
                        }
                        // constant evaluation is false
                        toc::asm_label(tok(), os, indent, jmp_to_if_false);
                        tc.asm_cmd(tok(), os, indent, "mov", dst_info.id_nasm,
                                   "false");
                        return;
                    }

                    // not constant evaluation
                    toc::asm_label(tok(), os, indent, jmp_to_if_true);
                    tc.asm_cmd(tok(), os, indent, "mov", dst_info.id_nasm,
                               "true");
                    toc::asm_jmp(tok(), os, indent, jmp_to_end);

                    toc::asm_label(tok(), os, indent, jmp_to_if_false);
                    tc.asm_cmd(tok(), os, indent, "mov", dst_info.id_nasm,
                               "false");
                    toc::asm_label(tok(), os, indent, jmp_to_end);
                }},
            var_);
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        return std::visit([](const auto& itm) { return itm.is_expression(); },
                          var_);
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        return std::visit(
            [](const auto& itm) -> std::string_view {
                return itm.identifier();
            },
            var_);
    }

    auto assert_var_not_used(const std::string_view var) const
        -> void override {
        std::visit([&var](const auto& itm) { itm.assert_var_not_used(var); },
                   var_);
    }

    [[nodiscard]] auto get_unary_ops() const -> const unary_ops& override {
        return std::visit(
            [](const auto& itm) -> const unary_ops& {
                return itm.get_unary_ops();
            },
            var_);
        // note: 'expr_type_value' does not have 'unary_ops' and cannot be
        //       an argument in call
    }

    [[nodiscard]] auto is_expr_type_value() const -> bool {
        return var_.index() == 2;
    }

    [[nodiscard]] auto is_expr_ops_list() const -> bool {
        return var_.index() == 0;
    }

    [[nodiscard]] auto is_identifier() const -> bool override {
        return is_expr_type_value() and std::get<2>(var_).is_identifier();
    }

    auto compile_lea(const token& src_loc_tk, toc& tc, std::ostream& os,
                     size_t indent,
                     std::vector<std::string>& allocated_registers,
                     const std::string& reg_size,
                     const std::vector<std::string>& lea_path) const
        -> std::string override {

        return std::get<2>(var_).compile_lea(src_loc_tk, tc, os, indent,
                                             allocated_registers, reg_size,
                                             lea_path);
    }

    [[nodiscard]] auto as_expr_type_value() const -> const expr_type_value& {
        return get<expr_type_value>(var_);
    }

    [[nodiscard]] auto as_assign_type_value() const -> const expr_type_value& {
        return get<expr_type_value>(var_);
    }
};
