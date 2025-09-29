#pragma once
// reviewed: 2025-09-30

#include "decouple.hpp"
#include "statement.hpp"
#include "type.hpp"

class expr_type_value final : public statement {
    std::vector<std::unique_ptr<expr_any>> exprs_;

  public:
    // note: constructor and destructor is implemented in 'main.cpp' where the
    // 'expr_any' definition is known. clang++ -std=c++23 requires it since
    // changes to handling of unique_ptr to incomplete types
    inline expr_type_value(toc& tc, tokenizer& tz, const type& tp);

    expr_type_value() = default;
    expr_type_value(const expr_type_value&) = default;
    expr_type_value(expr_type_value&&) = default;
    auto operator=(const expr_type_value&) -> expr_type_value& = default;
    auto operator=(expr_type_value&&) -> expr_type_value& = default;
    // note: copy and assignment constructor will not compile if used

    inline ~expr_type_value() override;

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 const std::string& dst) const -> void override {

        expr_type_value::compile_recursive(*this, tc, os, indent + 1,
                                           tok().name(), dst, get_type());
    }

    // implemented in 'main.cpp' due to circular reference:
    // expr_type_value -> expr_any -> expr_type_value
    inline void source_to(std::ostream& os) const override;

  private:
    // implemented in main.cpp due to circular reference:
    // expr_type_value -> expr_any -> expr_type_value
    inline static auto compile_recursive(const expr_type_value& etv, toc& tc,
                                         std::ostream& os, size_t indent,
                                         const std::string& src,
                                         const std::string& dst,
                                         const type& dst_type) -> void;
};
