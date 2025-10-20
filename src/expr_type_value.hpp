#pragma once
// reviewed: 2025-09-29

#include <memory>
#include <string_view>

#include "decouple.hpp"
#include "statement.hpp"

class stmt_identifier;

class expr_type_value final : public statement {
    std::vector<std::unique_ptr<expr_any>> exprs_;
    std::shared_ptr<stmt_identifier> stmt_ident_;
    token ws1_;
    // note: 'shared_ptr' because 'unique_ptr' poses compilation issues
    //       regarding circular references

  public:
    // note: constructor and destructor are implemented in 'main.cpp' where the
    // 'expr_any' definition is known. clang++ -std=c++23 has required it since
    // changes to handling of unique_ptr to incomplete types
    inline expr_type_value(toc& tc, tokenizer& tz, const type& tp);

    inline ~expr_type_value() override;

    expr_type_value() = default;
    expr_type_value(const expr_type_value&) = default;
    expr_type_value(expr_type_value&&) = default;
    auto operator=(const expr_type_value&) -> expr_type_value& = default;
    auto operator=(expr_type_value&&) -> expr_type_value& = default;
    // note: copy and assignment constructor will not compile if used

    auto compile(toc& tc, std::ostream& os, const size_t indent,
                 const std::string_view dst) const -> void override {

        if (is_make_copy()) {
            // note: sets the 'dst' register to the address of the start of this
            //       instance. called from 'stmt_assign_var'
            compile_copy(tc, os, indent, dst);
            return;
        }

        expr_type_value::compile_recursive(*this, tc, os, indent, tok().text(),
                                           dst, get_type());
    }

    auto compile_copy(toc& tc, std::ostream& os, size_t indent,
                      std::string_view dst) const -> void;

    // implemented in 'main.cpp' due to circular reference:
    // expr_type_value -> expr_any -> expr_type_value
    auto source_to(std::ostream& os) const -> void override;

    [[nodiscard]] auto is_make_copy() const -> bool {
        // todo explain this
        return not tok().is_text("");
    }

    auto assert_var_not_used(std::string_view var) const -> void override;

    [[nodiscard]] auto is_identifier() const -> bool override {
        return stmt_ident_ != nullptr;
    }

    [[nodiscard]] auto is_expression() const -> bool override;

    [[nodiscard]] auto identifier() const -> std::string_view override;

    auto compile_lea(const token& src_loc_tk, toc& tc, std::ostream& os,
                     size_t indent,
                     std::vector<std::string>& allocated_registers,
                     const std::string& reg_size,
                     std::span<const std::string> lea) const
        -> std::string override;

  private:
    // implemented in main.cpp due to circular reference:
    // expr_type_value -> expr_any -> expr_type_value
    inline static auto compile_recursive(const expr_type_value& etv, toc& tc,
                                         std::ostream& os, size_t indent,
                                         std::string_view src,
                                         std::string_view dst,
                                         const type& dst_type) -> void;
};
