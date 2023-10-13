#pragma once
#include "stmt_block.hpp"
#include "stmt_def_func_param.hpp"

class stmt_def_func final : public statement {
  token name_{};
  vector<stmt_def_func_param> params_{};
  token whitespace_after_params_{};
  vector<func_return_info> returns_{};
  stmt_block code_{};
  token inline_tk_{};
  bool no_args_{};

public:
  inline stmt_def_func(toc &tc, token tk, tokenizer &tz)
      : statement{move(tk)}, name_{tz.next_token()} {

    if (name_.is_name("inline")) {
      inline_tk_ = name_;
      name_ = tz.next_token();
    }
    if (not tz.is_next_char('(')) {
      no_args_ = true;
    }
    if (not no_args_) {
      while (true) {
        if (tz.is_next_char(')')) {
          break;
        }
        params_.emplace_back(tc, tz);
        if (tz.is_next_char(')')) {
          break;
        }
        if (not tz.is_next_char(',')) {
          throw compiler_exception(params_.back().tok(),
                                   "expected ',' after parameter '" +
                                       params_.back().tok().name() + "'");
        }
      }
    }
    whitespace_after_params_ = tz.next_whitespace_token();
    if (tz.is_next_char(':')) { // returns
      while (true) {
        const token type_tk{tz.next_token()};
        const struct func_return_info ret_info {
          type_tk,
              tz.next_token(), tc.get_type_or_throw(type_tk, type_tk.name())
        };
        returns_.emplace_back(ret_info);
        if (tz.is_next_char(',')) {
          continue;
        }
        break;
      }
      if (not returns_.empty()) {
        set_type(returns_[0].tp);
      }
    } else {
      // no return
      set_type(tc.get_type_void());
    }
    tc.add_func(name_, name_.name(), get_type(), this);
    tc.enter_func(name(), "", "", false, returns_);
    vector<string> allocated_named_registers{};
    null_stream ns{}; // don't make output while parsing
    init_variables(tc, ns, 0, allocated_named_registers);
    code_ = {tc, tz};
    free_allocated_registers(tc, ns, 0, allocated_named_registers);
    tc.exit_func(name());
  }

  inline stmt_def_func() = default;
  inline stmt_def_func(const stmt_def_func &) = default;
  inline stmt_def_func(stmt_def_func &&) = default;
  inline auto operator=(const stmt_def_func &) -> stmt_def_func & = default;
  inline auto operator=(stmt_def_func &&) -> stmt_def_func & = default;

  inline ~stmt_def_func() override = default;

  inline void source_to(ostream &os) const override {
    source_def_to(os, false);
    code_.source_to(os);
  }

  inline void source_def_to(ostream &os, const bool summary) const {
    if (not summary) {
      statement::source_to(os);
      inline_tk_.source_to(os);
    }
    name_.source_to(os);
    if (not no_args_) {
      os << "(";
      const size_t n{params_.size() - 1};
      size_t i{0};
      for (const stmt_def_func_param &p : params_) {
        p.source_to(os);
        if (i++ != n) {
          os << ",";
        }
      }
      os << ")";
    }
    whitespace_after_params_.source_to(os);
    if (not returns_.empty()) {
      os << ":";
      size_t i{0};
      for (const func_return_info &ri : returns_) {
        if (i++) {
          os << ":";
        }
        ri.type_tk.source_to(os);
        ri.ident_tk.source_to(os);
      }
    }
  }

  inline void source_def_comment_to(ostream &os) const {
    stringstream ss;
    source_def_to(ss, true);
    ss << endl;

    const string s{ss.str()};
    const string res{regex_replace(s, regex("\\s+"), " ")};
    os << res << endl;
  }

  inline void compile(toc &tc, ostream &os, size_t indent,
                      [[maybe_unused]] const string &dst = "") const override {
    if (is_inline()) {
      return;
    }

    toc::asm_label(tok(), os, indent, name());
    toc::indent(os, indent + 1, true);
    source_def_comment_to(os);

    tc.enter_func(name(), "", "", false, returns_);

    toc::asm_push(tok(), os, indent + 1, "rbp");
    tc.asm_cmd(tok(), os, indent + 1, "mov", "rbp", "rsp");

    vector<string> allocated_named_registers;
    init_variables(tc, os, indent, allocated_named_registers);

    code_.compile(tc, os, indent, "");

    if (!returns().empty()) {
      const string &ret_name{returns()[0].ident_tk.name()};
      const ident_resolved &ret_resolved{
          tc.resolve_identifier(tok(), ret_name, true)};
      tc.asm_cmd(tok(), os, indent + 1, "mov", "rax", ret_resolved.id_nasm);
    }

    toc::asm_pop(tok(), os, indent + 1, "rbp");
    toc::asm_ret(tok(), os, indent + 1);

    free_allocated_registers(tc, os, indent, allocated_named_registers);

    os << endl;
    tc.exit_func(name());
  }

  [[nodiscard]] inline auto returns() const -> const vector<func_return_info> & {
    return returns_;
  }

  [[nodiscard]] inline auto param(const size_t ix) const
      -> const stmt_def_func_param & {
    return params_[ix];
  }

  [[nodiscard]] inline auto params() const
      -> const vector<stmt_def_func_param> & {
    return params_;
  }

  [[nodiscard]] inline auto code() const -> const stmt_block & { return code_; }

  [[nodiscard]] inline auto name() const -> const string & {
    return name_.name();
  }

  [[nodiscard]] inline auto is_inline() const -> bool {
    return not inline_tk_.is_empty();
  }

private:
  inline void init_variables(toc &tc, ostream &os, size_t indent,
                             vector<string> &allocated_named_registers) const {
    // return binding
    if (not returns().empty()) {
      const string &nm{returns()[0].ident_tk.name()};
      tc.add_var(tok(), os, indent + 1, nm, get_type(), false);
    }

    // stack is now: ...,[prev rsp],...,[arg n],[arg n-1],...,[arg 1],[return
    // address],[rbp]
    //   (some arguments might be passed through registers)

    // define variables in the called context by mapping arguments to stack
    const size_t n{params_.size()};
    size_t stk_ix{2U << 3U}; // skip [rbp] and [return address] on stack
    for (size_t i{0}; i < n; i++) {
      const stmt_def_func_param &pm{params_[i]};
      const type &arg_type{pm.get_type()};
      const string &pm_nm{pm.name()};
      const string &reg{pm.get_register_or_empty()};
      if (reg.empty()) {
        // argument not passed as register
        tc.add_func_arg(tok(), os, indent + 1, pm_nm, arg_type, int(stk_ix));
        // only 64 bits values on the stack
        stk_ix += tc.get_type_default().size();
        continue;
      }
      // argument passed as named register
      toc::indent(os, indent + 1, true);
      os << pm_nm << ": " << reg << endl;
      tc.alloc_named_register_or_throw(pm, os, indent + 1, reg);
      // ! check if arg_type fits in register
      allocated_named_registers.emplace_back(reg);
      tc.add_alias(pm_nm, reg);
    }
  }

  inline static void
  free_allocated_registers(toc &tc, ostream &os, size_t indent,
                           const vector<string> &allocated_named_registers) {
    size_t i{allocated_named_registers.size()};
    while (i--) {
      tc.free_named_register(os, indent + 1, allocated_named_registers[i]);
    }
  }
};
