#pragma once
#include "token.hpp"
#include "unary_ops.hpp"

class toc;
class type;

class statement {
public:
  inline explicit statement(token tk, unary_ops uops = {})
      : token_{move(tk)}, uops_{move(uops)} {}
  inline virtual ~statement() = default;

  inline statement() = default;
  inline statement(statement &&) = default;
  inline statement(const statement &) = default;
  inline auto operator=(const statement &) -> statement & = default;
  inline auto operator=(statement &&) -> statement & = default;

  inline virtual void compile([[maybe_unused]] toc &tc, ostream &os,
                              [[maybe_unused]] size_t indent,
                              [[maybe_unused]] const string &dst = "") const {
    uops_.source_to(os);
    token_.compile_to(os);
  }

  inline virtual void source_to(ostream &os) const {
    uops_.source_to(os);
    token_.source_to(os);
  }

  [[nodiscard]] inline auto tok() const -> const token & { return token_; }

  [[nodiscard]] inline virtual auto is_in_data_section() const -> bool {
    return false;
  }

  [[nodiscard]] inline virtual auto is_expression() const -> bool {
    return false;
  }

  [[nodiscard]] inline virtual auto identifier() const -> const string & {
    return token_.name();
  }

  [[nodiscard]] inline virtual auto get_unary_ops() const -> const unary_ops & {
    return uops_;
  }

  inline void set_type(const type &tp) { type_ = &tp; }

  [[nodiscard]] inline virtual auto get_type() const -> const type & {
    return *type_;
  }

private:
  token token_{};
  unary_ops uops_{};
  const type *type_{};
};
