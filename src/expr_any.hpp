#pragma once
// reviewed: 2025-09-29

#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_bool_ops_list.hpp"
#include "expr_ops_list.hpp"
#include "expr_type_value.hpp"

class expr_any final : public statement {
    using expr_variant =
        std::variant<expr_ops_list, expr_bool_ops_list, expr_type_value>;

    // helper template for nicer handling of variants using overloaded lambdas
    template <class... Ts> struct overloaded : Ts... {
        using Ts::operator()...;
    };

    std::vector<expr_variant> vars_;
    std::vector<token> vars_delims_tk_;
    token open_brace_tk_;
    token close_brace_tk_;
    size_t array_size_{};
    bool is_array_{};
    bool is_identifier_{};

  public:
    expr_any(toc& tc, tokenizer& tz, const type& tp, const bool in_args,
             const bool is_array, const size_t array_size)
        : statement{tz.next_whitespace_token()}, array_size_{array_size},
          is_array_{is_array} {

        set_type(tp);

        // the basic case
        if (not is_array) {
            vars_.emplace_back(parse_variant(tc, tz, tp, in_args));
            return;
        }

        // array

        // check if it is '{ ... }' or identifier e.g. 'str.data'

        open_brace_tk_ = tz.is_next_char_token('{');
        if (open_brace_tk_.is_empty()) {
            // TODO: explain why expr_type_value has multiple responsibilities
            vars_.emplace_back(expr_type_value{tc, tz, tp});
            is_identifier_ = true;
            return;
        }

        size_t counter{};
        while (true) {
            close_brace_tk_ = tz.is_next_char_token('}');
            if (not close_brace_tk_.is_empty()) {
                break;
            }

            if (counter++) {
                const token t{tz.is_next_char_token(',')};
                if (t.is_empty()) {
                    throw compiler_exception(
                        tz, std::format("expected ',' followed by initializer "
                                        "for type '{}'",
                                        tp.name()));
                }
                vars_delims_tk_.emplace_back(t);
            }
            vars_.emplace_back(parse_variant(tc, tz, tp, in_args));
        }

        if (array_size_ == 0) {
            array_size_ = counter;
        }
    }

    expr_any() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        open_brace_tk_.source_to(os);
        if (not vars_.empty()) {
            std::visit([&os](const auto& e) -> void { e.source_to(os); },
                       vars_.front());
            for (const auto [d, e] : std::views::zip(
                     vars_delims_tk_, vars_ | std::views::drop(1))) {
                d.source_to(os);
                std::visit([&os](const auto& el) -> void { el.source_to(os); },
                           e);
            }
        }
        close_brace_tk_.source_to(os);
    }

    auto compile(toc& tc, x86& x, const size_t indent,
                 const ident_info& dst_info) const -> void override {

        if (is_array_identifier()) {
            const ident_info src_info{tc.make_ident_info(x, *this)};
            if (not src_info.is_array) {
                throw compiler_exception{tok(), "source must be an array"};
            }
        }

        // the base case
        if (is_identifier_ or not is_array_) {
            compile_variant(tc, x, indent, dst_info, tok(), vars_[0]);
            return;
        }

        // assign array elements

        ident_info ii{dst_info};

        for (const auto [i, el] : std::views::enumerate(vars_)) {
            x.comment_line(tok(), indent, "[{}]", i);
            compile_variant(tc, x, indent, ii, tok(), el);
            ii.operand.displacement += static_cast<int32_t>(ii.type().size());
        }

        const size_t diff{(array_size_ - vars_.size())};
        if (diff == 0) {
            return;
        }

        const size_t nbytes{diff * ii.type().size()};

        x.comment_line(tok(), indent,
                       "zero remaining elements: {} * {} B = {} B", diff,
                       ii.type().size(), nbytes);
        x.zero(tok(), indent, ii.operand.address_str(), nbytes);
    }

    [[nodiscard]] auto is_array() const -> bool { return is_array_; }

    [[nodiscard]] auto is_array_identifier() const -> bool {
        return is_array_ and is_identifier_;
    }

    [[nodiscard]] auto is_empty() const -> bool { return vars_.empty(); }

    [[nodiscard]] auto is_expression() const -> bool override {
        if (is_array_) {
            return true;
        }

        return std::visit(
            [](const auto& e) -> bool { return e.is_expression(); }, vars_[0]);
    }

    [[nodiscard]] auto is_indexed() const -> bool override {
        if (is_array_) {
            return false;
        }

        return std::visit([](const auto& e) -> bool { return e.is_indexed(); },
                          vars_[0]);
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        // TODO: explain why this cannot be empty at this point
        return std::visit(
            [](const auto& e) -> std::string_view { return e.identifier(); },
            vars_[0]);
    }

    auto assert_var_not_used(const std::string_view var) const
        -> void override {

        if (is_array_) {
            for (const expr_variant& el : vars_) {
                std::visit(
                    [&var](const auto& e) -> void {
                        e.assert_var_not_used(var);
                    },
                    el);
            }
            return;
        }

        std::visit(
            [&var](const auto& e) -> void { e.assert_var_not_used(var); },
            vars_[0]);
    }

    [[nodiscard]] auto get_unary_ops() const -> const unary_ops& override {
        if (is_array_) {
            return statement::get_unary_ops();
        }

        return std::visit(
            [](const auto& e) -> const unary_ops& { return e.get_unary_ops(); },
            vars_[0]);

        // note: 'expr_type_value' does not have 'unary_ops' and cannot be
        //       an argument in call
    }

    [[nodiscard]] auto is_identifier() const -> bool override {
        if (is_identifier_) {
            return true;
        }

        return std::visit(
            [](const auto& e) -> bool { return e.is_identifier(); }, vars_[0]);
    }

    [[nodiscard]] auto
    compile_lea(const token& src_loc_tk, toc& tc, x86& x, size_t indent,
                std::vector<std::string>& allocated_registers,
                const std::string& reg_size,
                const std::span<const std::string> lea_path) const
        -> operand override {

        return std::visit(
            [&](const auto& e) -> operand {
                return e.compile_lea(src_loc_tk, tc, x, indent,
                                     allocated_registers, reg_size, lea_path);
            },
            vars_[0]);
    }

    [[nodiscard]] auto as_expr_type_value() const -> const expr_type_value& {
        return get<expr_type_value>(vars_[0]);
    }

    [[nodiscard]] auto array_size() const -> size_t { return array_size_; }

    [[nodiscard]] auto tok() const -> const token& override {
        if (vars_.empty()) {
            return statement::tok();
        }

        return std::visit(
            [&](const auto& e) -> const token& { return e.tok(); }, vars_[0]);
    }

  private:
    [[nodiscard]] static auto parse_variant(toc& tc, tokenizer& tz,
                                            const type& tp, const bool in_args)
        -> expr_variant {

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

    static auto compile_variant(toc& tc, x86& x, const size_t indent,
                                const ident_info& dst_info, const token tk,
                                const expr_variant& exp) -> void {
        std::visit(
            overloaded{
                [&](const expr_ops_list& e) -> void {
                    e.compile(tc, x, indent, dst_info);
                },
                [&]([[maybe_unused]] const expr_type_value& e) -> void {
                    e.compile(tc, x, indent, dst_info);
                },
                [&](const expr_bool_ops_list& e) -> void {
                    // if not expression assign to destination
                    if (not e.is_expression()) {
                        const ident_info& src_info{tc.make_ident_info(x, e)};
                        if (not src_info.is_const()) {
                            std::unreachable();
                        }
                        x.mov(tk, indent, dst_info.operand.str(),
                              std::format("{}", src_info.const_value));
                        return;
                    }

                    // expression - make unique labels considering in-lined
                    // functions
                    const std::string_view call_path{tc.get_call_path()};
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
                        e.compile(tc, x, indent, jmp_to_end, jmp_to_end, false,
                                  dst_info.operand.str())};

                    // not constant evaluation
                    x.label(indent, jmp_to_end);

                    // did the evaluation result in a constant?
                    if (const_eval) {
                        // yes, constant evaluation
                        if (*const_eval) {
                            // constant evaluation is true
                            x.mov(tk, indent, dst_info.operand.str(), "1");
                        } else {
                            // constant evaluation is false
                            x.mov(tk, indent, dst_info.operand.str(), "0");
                        }
                    }
                }},
            exp);
    }
};
