#pragma once

#include "decouple.hpp"
#include "statement.hpp"
#include "type.hpp"

class expr_type_value final : public statement {
  std::vector<std::unique_ptr<expr_any>> exprs_{};

public:
  // note: constructor and destructor is implemented in 'main.cpp' where the
  // 'expr_any' definition is known. clang++ -std=c++23 requires it since
  // changes to handling of unique_ptr to incomplete types
  inline expr_type_value(toc &tc, tokenizer &tz, const type &tp);
  inline expr_type_value() = default;
  inline expr_type_value(const expr_type_value &) = default;
  inline expr_type_value(expr_type_value &&) = default;
  inline auto operator=(const expr_type_value &) -> expr_type_value & = default;
  inline auto operator=(expr_type_value &&) -> expr_type_value & = default;
  // note: copy and assignment constructor will not compile if used

  inline ~expr_type_value() override;

  inline void compile(toc &tc, std::ostream &os, size_t indent,
                      const std::string &dst) const override {

    expr_type_value::compile_recursive(*this, tc, os, indent + 1, tok().name(),
                                       dst, get_type());
  }

  // implemented in 'main.cpp' due to circular reference
  //   expr_type_value -> expr_any -> expr_type_value
  inline void source_to(std::ostream &os) const override;

private:
  // implemented in main.cpp due to circular reference
  //   expr_type_value -> expr_any -> expr_type_value
  inline static void compile_recursive(const expr_type_value &atv, toc &tc,
                                       std::ostream &os, size_t indent,
                                       const std::string &src,
                                       const std::string &dst,
                                       const type &dst_type);
};
