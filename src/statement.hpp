#pragma once
#include <utility>

#include "compiler_exception.hpp"
#include "token.hpp"
#include "unary_ops.hpp"

class toc;
class type;

class statement {
  token token_{};
  unary_ops uops_{};
  const type *type_{};

public:
  inline explicit statement(unary_ops uops, token tk)
      : token_{move(tk)}, uops_{move(uops)} {

    validate_identifier_name(token_);
  }

  inline statement() = default;
  inline statement(statement &&) = default;
  inline statement(const statement &) = default;
  inline auto operator=(const statement &) -> statement & = default;
  inline auto operator=(statement &&) -> statement & = default;

  inline virtual ~statement() = default;

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

  //? where to place this validation. empty token in statement is ok
  inline static void validate_identifier_name(const token &tk) {
    // if (tk.is_empty()) {
    //   throw compiler_exception(tk, "unexpected empty identifier");
    // }
    if (tk.name().ends_with(".")) {
      throw compiler_exception(tk, "unexpected '.' at the end of '" +
                                       tk.name() + "'");
    }
  }
};
