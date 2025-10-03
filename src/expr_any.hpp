#pragma once
// reviewed: 2025-09-29

#include "bool_ops_list.hpp"
#include "expr_ops_list.hpp"
#include "expr_type_value.hpp"

class expr_any final : public statement {
    std::variant<expr_ops_list, bool_ops_list, expr_type_value> var_;

  public:
    expr_any(toc& tc, tokenizer& tz, const type& tp, bool in_args)
        : statement{tz.next_whitespace_token()} {

        set_type(tp);

        if (not tp.is_built_in()) {
            // destination is not built-in (register) value
            // assume assign type value
            var_ = expr_type_value{tc, tz, tp};
            return;
        }

        if (tp.name() == tc.get_type_bool().name()) {
            // destination is boolean
            var_ = bool_ops_list{tc, tz.next_whitespace_token(), tz};
            return;
        }

        // destination is built-in (register) value
        var_ = expr_ops_list{tc, tz, in_args};
    }

    expr_any() = default;
    expr_any(const expr_any&) = default;
    expr_any(expr_any&&) = default;
    auto operator=(const expr_any&) -> expr_any& = default;
    auto operator=(expr_any&&) -> expr_any& = default;

    ~expr_any() override = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);

        switch (var_.index()) {
        case 0:
            get<expr_ops_list>(var_).source_to(os);
            return;
        case 1:
            get<bool_ops_list>(var_).source_to(os);
            return;
        case 2:
            get<expr_type_value>(var_).source_to(os);
            return;
        default:
            throw panic_exception("unexpected code path expr_any:1");
        }
    }

    [[nodiscard]] auto is_expr_type_value() const -> size_t {
        return var_.index() == 2;
    }

    [[nodiscard]] auto is_expr_type_value_copy() const -> bool {
        if (not is_expr_type_value()) {
            return false;
        }
        return get<expr_type_value>(var_).is_make_copy();
    }

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 const std::string& dst = "") const -> void override {

        tc.comment_source(*this, os, indent);

        switch (var_.index()) {
        default:
            throw panic_exception("unexpected code path expr_any:2");
        case 0:
            // number expression
            get<expr_ops_list>(var_).compile(tc, os, indent, dst);
            return;
        case 2:
            // assign type value
            get<expr_type_value>(var_).compile(tc, os, indent, dst);
            return;
        case 1:
            // bool expression
            const bool_ops_list& bol{get<bool_ops_list>(var_)};
            // resolve the destination
            const ident_info& dst_info{tc.make_ident_info(tok(), dst, false)};
            // if not expression assign to destination
            if (not bol.is_expression()) {
                const ident_info& src_info{tc.make_ident_info(bol, false)};
                tc.asm_cmd(tok(), os, indent, "mov", dst_info.id_nasm,
                           src_info.id_nasm);
                return;
            }
            // expression
            // make unique labels considering in-lined functions
            const std::string& call_path{tc.get_call_path(tok())};
            const std::string& src_loc{
                tc.source_location_for_use_in_label(tok())};
            // unique partial label for this assembler location
            const std::string& postfix{
                src_loc + (call_path.empty() ? "" : ("_" + call_path))};
            // labels to jump to depending on the evaluation
            const std::string& jmp_to_if_true{"bool_true_" + postfix};
            const std::string& jmp_to_if_false{"bool_false_" + postfix};
            // the end of the assign
            const std::string& jmp_to_end{"bool_end_" + postfix};
            // compile and possibly evaluate constant expression
            std::optional<bool> const_eval{bol.compile(
                tc, os, indent, jmp_to_if_false, jmp_to_if_true, false)};
            // did the evaluation result in a constant?
            if (const_eval) {
                // yes, constant evaluation
                if (*const_eval) {
                    // constant evaluation is true
                    toc::asm_label(tok(), os, indent, jmp_to_if_true);
                    // note: label necessary for optimizing away the 'jmp' prior
                    // to the label in main as 'opt1'
                    tc.asm_cmd(tok(), os, indent, "mov", dst_info.id_nasm,
                               "true");
                    return;
                }
                // constant evaluation is false
                toc::asm_label(tok(), os, indent, jmp_to_if_false);
                // note: label necessary for optimizing away the 'jmp' prior
                // to the label in main as 'opt1'
                tc.asm_cmd(tok(), os, indent, "mov", dst_info.id_nasm, "false");
                return;
            }
            // not constant evaluation
            // label for where the condition to branch if true
            toc::asm_label(tok(), os, indent, jmp_to_if_true);
            tc.asm_cmd(tok(), os, indent, "mov", dst_info.id_nasm, "true");
            toc::asm_jmp(tok(), os, indent, jmp_to_end);
            // label for where the condition to branch if false
            toc::asm_label(tok(), os, indent, jmp_to_if_false);
            tc.asm_cmd(tok(), os, indent, "mov", dst_info.id_nasm, "false");
            toc::asm_label(tok(), os, indent, jmp_to_end);
            return;
        }
    }

    [[nodiscard]] auto is_expression() const -> bool override {
        switch (var_.index()) {
        case 0:
            // number expression
            return get<expr_ops_list>(var_).is_expression();
        case 1:
            // bool expression
            return get<bool_ops_list>(var_).is_expression();
        case 2:
            // assign type value
            return get<expr_type_value>(var_).is_expression();
        default:
            throw panic_exception("unexpected code path expr_any:3");
        }
        // note. 'expr_type_value' cannot be expression but may contain
        // expressions to be assigned to type members
    }

    [[nodiscard]] auto identifier() const -> const std::string& override {
        switch (var_.index()) {
        case 0:
            return get<expr_ops_list>(var_).identifier();
        case 1:
            return get<bool_ops_list>(var_).identifier();
        case 2:
            return get<expr_type_value>(var_).identifier();
        default:
            throw panic_exception("unexpected code path expr_any:4");
        }
    }

    [[nodiscard]] auto get_unary_ops() const -> const unary_ops& override {

        switch (var_.index()) {
        case 0:
            return get<expr_ops_list>(var_).get_unary_ops();
        case 1:
            return get<bool_ops_list>(var_).get_unary_ops();
        case 2:
            return get<expr_type_value>(var_).get_unary_ops();
        default:
            throw panic_exception("unexpected code path expr_any:5");
        }
        // note. 'expr_type_value' does not have 'unary_ops' and cannot be
        // argument in call
    }

    [[nodiscard]] auto as_assign_type_value() const -> const expr_type_value& {
        return get<expr_type_value>(var_);
    }
};
