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
#include "expr_arith.hpp"
#include "expr_bool.hpp"
#include "expr_type.hpp"

class expr_any final : public statement {
    using expr_variant = std::variant<expr_arith, expr_bool, expr_type>;

    // helper template for nicer handling of variants using overloaded lambdas
    template <class... Ts> struct overloaded : Ts... {
        using Ts::operator()...;
    };

    std::vector<expr_variant> vars_;
    std::vector<token> var_delims_tk_;
    token open_brace_tk_;
    token close_brace_tk_;
    size_t array_count_{};
    bool is_array_{};
    bool is_identifier_{};

  public:
    expr_any(toc& tc, tokenizer& tz, const type& tp, const bool in_args,
             const bool is_array, const size_t array_count)
        : statement{tz.next_whitespace_token()}, array_count_{array_count},
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
            vars_.emplace_back(expr_type{tc, tz, tp});
            is_identifier_ = true;

            return;
        }

        while (true) {
            close_brace_tk_ = tz.is_next_char_token('}');
            if (not close_brace_tk_.is_empty()) {
                break;
            }

            if (not vars_.empty()) {
                const token t{tz.is_next_char_token(',')};
                if (t.is_empty()) {
                    throw compiler_exception{
                        tz, std::format("expected ',' followed by initializer "
                                        "for type '{}'",
                                        tp.name())};
                }
                var_delims_tk_.emplace_back(t);
            }
            vars_.emplace_back(parse_variant(tc, tz, tp, in_args));
        }

        if (array_count_ == 0) {
            array_count_ = vars_.size();
        }
    }

    expr_any() = default;

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        open_brace_tk_.source_to(os);
        if (not vars_.empty()) {
            vars_.front().visit([&os](const auto& expression) -> void {
                expression.source_to(os);
            });

            for (const auto [d, e] :
                 std::views::zip(var_delims_tk_, vars_ | std::views::drop(1))) {

                d.source_to(os);
                e.visit([&os](const auto& expression) -> void {
                    expression.source_to(os);
                });
            }
        }
        close_brace_tk_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {

        if (is_array_identifier()) {
            const ident_info src_info{tc.make_ident_info(*this)};
            if (not src_info.is_array) {
                throw compiler_exception{tok(), "source must be an array"};
            }
        }

        // the base case
        if (is_identifier_ or not is_array_) {
            compile_variant(tc, indent, dst_info, tok(), vars_[0]);
            return;
        }

        // assign array elements

        ident_info cur_dst_info{dst_info};

        machine& x{tc.machine()};

        for (const auto [i, e] : std::views::enumerate(vars_)) {
            x.comment(tok(), indent, "[{}]", i);
            compile_variant(tc, indent, cur_dst_info, tok(), e);
            cur_dst_info.operand.increment_offset(
                address_offset(cur_dst_info.type_ref().size_bytes()));
        }

        const size_t remaining_count{(array_count_ - vars_.size())};
        if (remaining_count == 0) {
            return;
        }

        const size_t size_bytes{multiply_storage_size(
            cur_dst_info.type_ref().size_bytes(), remaining_count)};

        x.comment(tok(), indent, "zero remaining elements: {} * {} B = {} B",
                  remaining_count, cur_dst_info.type_ref().size_bytes(),
                  size_bytes);

        x.zero(tok(), indent, cur_dst_info.operand, size_bytes);
    }

    [[nodiscard]] auto is_array_element() const -> bool override {
        if (is_array_ or vars_.size() != 1) {
            return false;
        }

        return vars_[0].visit([](const auto& expression) -> bool {
            return expression.is_array_element();
        });
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

        return vars_[0].visit([](const auto& expression) -> bool {
            return expression.is_expression();
        });
    }

    [[nodiscard]] auto is_indexed() const -> bool override {
        if (is_array_) {
            return false;
        }

        return vars_[0].visit([](const auto& expression) -> bool {
            return expression.is_indexed();
        });
    }

    [[nodiscard]] auto identifier() const -> std::string_view override {
        return vars_[0].visit([](const auto& expression) -> std::string_view {
            return expression.identifier();
        });
    }

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

        if (is_array_) {
            for (const expr_variant& e : vars_) {
                e.visit([&var, &reader](const auto& expression) -> void {
                    expression.visit_reads(var, reader);
                });
            }

            return;
        }

        vars_[0].visit([&var, &reader](const auto& expression) -> void {
            expression.visit_reads(var, reader);
        });
    }

    [[nodiscard]] auto get_unary_ops() const -> const unary_ops& override {
        if (is_array_) {
            return statement::get_unary_ops();
        }

        return vars_[0].visit([](const auto& expression) -> const unary_ops& {
            return expression.get_unary_ops();
        });

        // note: 'expr_type' does not have 'unary_ops' and cannot be
        //       an argument in call
    }

    [[nodiscard]] auto is_identifier() const -> bool override {
        if (is_identifier_) {
            return true;
        }

        return vars_[0].visit([](const auto& expression) -> bool {
            return expression.is_identifier();
        });
    }

    [[nodiscard]] auto compile_lea(toc& tc, const size_t indent,
                                   const token& src_loc_tk,
                                   std::vector<operand>& allocated_registers,
                                   const operand& reg_count,
                                   const std::span<const operand> lea_path,
                                   const operand& address_register) const
        -> operand override {

        return vars_[0].visit([&](const auto& expression) -> operand {
            return expression.compile_lea(tc, indent, src_loc_tk,
                                          allocated_registers, reg_count,
                                          lea_path, address_register);
        });
    }

    [[nodiscard]] auto as_expr_type() const -> const expr_type& {
        return get<expr_type>(vars_[0]);
    }

    auto assert_record_value_not_reading(
        const expr_type::record_destination& dst) const -> void {

        if (is_array_ or not std::holds_alternative<expr_type>(vars_[0])) {
            return;
        }

        as_expr_type().assert_not_reading(dst);
    }

    [[nodiscard]] auto array_count() const -> size_t { return array_count_; }

    [[nodiscard]] auto tok() const -> const token& override {
        if (vars_.empty()) {
            return statement::tok();
        }

        return vars_[0].visit([](const auto& expression) -> const token& {
            return expression.tok();
        });
    }

  private:
    [[nodiscard]] static auto parse_variant(toc& tc, tokenizer& tz,
                                            const type& tp, const bool in_args)
        -> expr_variant {

        if (not tp.is_builtin()) {
            // destination is not a built-in (register) value
            // assume assign type value
            return expr_type{tc, tz, tp};
        }

        if (tp.name() == tc.get_type_bool().name()) {
            // destination is boolean
            return expr_bool{tc, tz.next_whitespace_token(), tz};
        }

        // destination is a built-in (register) value
        return expr_arith{tc, tz, in_args};
    }

    static auto compile_variant(toc& tc, const size_t indent,
                                const ident_info& dst_info,
                                const token src_loc_tk, const expr_variant& exp)
        -> void {

        exp.visit(overloaded{
            [&](const expr_arith& e) -> void {
                // the value boundary is where a wider source loses bits
                e.assert_not_narrowed(tc, dst_info.type_ref());
                e.compile(tc, indent, dst_info);
            },
            [&](const expr_type& e) -> void {
                e.compile(tc, indent, dst_info);
            },
            [&](const expr_bool& e) -> void {
                machine& x{tc.machine()};

                // if not expression assign to destination
                if (not e.is_expression()) {
                    const ident_info& src_info{tc.make_ident_info(e)};
                    if (not src_info.is_const()) {
                        std::unreachable();
                    }
                    x.copy_value(
                        src_loc_tk, indent, dst_info.operand,
                        operand::imm(std::format("{}", src_info.const_value),
                                     src_info.type_ref()));
                    return;
                }

                // stored comparisons would change what later elements read
                if (dst_info.is_register() or
                    not e.reads_var(dst_info.root_id())) {

                    compile_bool_list(tc, indent, src_loc_tk, e,
                                      dst_info.operand);

                    return;
                }

                const operand reg{x.alloc_scratch_register(
                    src_loc_tk, indent, dst_info.type_ref())};

                compile_bool_list(tc, indent, src_loc_tk, e, reg);
                x.copy_value(src_loc_tk, indent, dst_info.operand, reg);
                x.free_scratch_register(src_loc_tk, indent, reg);
            }});
    }

    static auto compile_bool_list(toc& tc, const size_t indent,
                                  const token& src_loc_tk, const expr_bool& e,
                                  const operand& dst) -> void {

        // make unique labels considering inlined functions
        const std::string_view call_path{tc.get_call_path()};

        const std::string src_loc{
            tc.source_location_for_use_in_label(src_loc_tk)};

        // unique partial label for this assembler location
        const std::string postfix{
            std::format("{}{}", src_loc,
                        (call_path.empty() ? std::string{}
                                           : std::format("_{}", call_path)))};

        // labels to jump to depending on the evaluation
        const std::string jmp_to_end{std::format("bool_end_{}", postfix)};

        // compile and possibly evaluate constant expression
        const std::optional<bool> const_eval{
            e.compile(tc, indent, jmp_to_end, jmp_to_end, dst)};

        machine& x{tc.machine()};

        // not constant evaluation
        x.label(indent, jmp_to_end);

        // did the evaluation result in a constant?
        if (const_eval) {
            x.store_boolean(src_loc_tk, indent, dst, *const_eval);
        }
    }
};
