#pragma once
#include "stmt_block.hpp"
#include "stmt_def_func_param.hpp"

class stmt_def_func final : public statement {
  token name_tk_{};
  vector<stmt_def_func_param> params_{};
  token ws_after_params_{}; // whitespace after ')'
  vector<func_return_info> returns_{};
  stmt_block code_{};
  token inline_tk_{};
  bool no_args_{};

public:
  inline stmt_def_func(toc &tc, token tk, tokenizer &tz)
      : statement{move(tk)}, name_tk_{tz.next_token()} {

    if (name_tk_.is_name("inline")) {
      // handle 'func inline foo(...)'
      inline_tk_ = name_tk_;
      name_tk_ = tz.next_token();
    }
    if (not tz.is_next_char('(')) {
      // no parameters expected
      no_args_ = true;
    }
    if (not no_args_) {
      // read parameters definition
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
    ws_after_params_ = tz.next_whitespace_token();
    if (tz.is_next_char(':')) {
      // function returns
      while (true) {
        const token type_tk{tz.next_token()};
        const struct func_return_info ret_info {
          type_tk,
              tz.next_token(), tc.get_type_or_throw(type_tk, type_tk.name())
        };
        returns_.emplace_back(ret_info);
        if (not tz.is_next_char(',')) {
          break;
        }
      }
      // set function type to first return type
      set_type(returns_[0].tp);
    } else {
      // no return, set type to 'void's
      set_type(tc.get_type_void());
    }
    tc.add_func(name_tk_, name_tk_.name(), get_type(), this);
    tc.enter_func(name(), returns_, is_inline());
    // dry-run compile step to setup context for code block parsing
    vector<string> allocated_named_registers{};
    null_stream null_strm{}; // don't make output
    init_variables(tc, null_strm, 0, allocated_named_registers);
    code_ = {tc, tz};
    free_allocated_registers(tc, null_strm, 0, allocated_named_registers);
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
    name_tk_.source_to(os);
    if (not no_args_) {
      // function has parameters
      os << "(";
      size_t i{0};
      for (const stmt_def_func_param &p : params_) {
        if (i++) {
          os << ",";
        }
        p.source_to(os);
      }
      os << ")";
    }
    ws_after_params_.source_to(os);
    if (not returns_.empty()) {
      // return parameters
      os << ":";
      size_t i{0};
      for (const func_return_info &ret_info : returns_) {
        if (i++) {
          os << ":";
        }
        ret_info.type_tk.source_to(os);
        ret_info.ident_tk.source_to(os);
      }
    }
  }

  inline void source_def_comment_to(ostream &os) const {
    stringstream ss;
    source_def_to(ss, true);
    ss << endl;

    const string src{ss.str()};
    // make comment friendly string replacing consecutive with one space
    const string res{regex_replace(src, regex(R"(\s+)"), " ")};
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

    tc.enter_func(name(), returns_);
    // setup stack
    toc::asm_push(tok(), os, indent + 1, "rbp");
    tc.asm_cmd(tok(), os, indent + 1, "mov", "rbp", "rsp");
    // initiate variables and parameters passed through registers
    vector<string> allocated_named_registers;
    init_variables(tc, os, indent, allocated_named_registers);
    // compile function body
    code_.compile(tc, os, indent, "");
    // if function returns copy return value to rax
    if (not returns().empty()) {
      const string &ret_name{returns_[0].ident_tk.name()};
      const ident_resolved &ret_resolved{
          tc.resolve_identifier(tok(), ret_name, true)};
      tc.asm_cmd(tok(), os, indent + 1, "mov", "rax", ret_resolved.id_nasm);
    }
    // restore 'rbp' and return
    toc::asm_pop(tok(), os, indent + 1, "rbp");
    toc::asm_ret(tok(), os, indent + 1);
    // free allocated named registers
    free_allocated_registers(tc, os, indent, allocated_named_registers);
    // empty line
    os << endl;
    // exit function context
    tc.exit_func(name());
  }

  [[nodiscard]] inline auto returns() const
      -> const vector<func_return_info> & {
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
    return name_tk_.name();
  }

  [[nodiscard]] inline auto is_inline() const -> bool {
    return not inline_tk_.is_empty();
  }

private:
  inline void init_variables(toc &tc, ostream &os, size_t indent,
                             vector<string> &allocated_named_registers) const {

    if (not returns().empty()) {
      // declare variable for the return
      const token &id_tkn{returns()[0].ident_tk};
      tc.add_var(id_tkn, os, indent + 1, id_tkn.name(), get_type(), false);
    }

    if (is_inline()) {
      const size_t n{params_.size()};
      for (size_t i{0}; i < n; i++) {
        const stmt_def_func_param &param{params_[i]};
        const type &param_type{param.get_type()};
        const string &param_name{param.name()};
        const string &param_reg{param.get_register_name_or_empty()};
        if (param_reg.empty()) {
          // argument not passed as register, add it as variable
          tc.add_var(tok(), os, indent + 1, param_name, param_type, true);
          continue;
        }

        // argument passed as named register
        toc::indent(os, indent + 1, true);
        os << param_name << ": " << param_reg << endl;
        tc.alloc_named_register_or_throw(param, os, indent + 1, param_reg);
        // make an alias from argument name to register
        tc.add_alias(param_name, param_reg);
        // mark register as initiated
        tc.set_var_is_initiated(param_reg);
        // add it to list of allocated registers
        allocated_named_registers.emplace_back(param_reg);
      }
      return;
    }

    //
    // not inline function
    //

    // stack is now: ...,[prev rsp],...,[arg n],[arg n-1],...,[arg 1],[return
    // address],[rbp]
    //   (some arguments might be passed through registers)

    // set up stack index relative to what rbp will be
    //   skip [rbp] and [return address] on stack
    size_t stk_ix{2U << 3U};
    
    // define variables in the called context by mapping arguments to stack
    const size_t n{params_.size()};
    for (size_t i{0}; i < n; i++) {
      const stmt_def_func_param &param{params_[i]};
      const type &param_type{param.get_type()};
      const string &param_name{param.name()};
      const string &param_reg{param.get_register_name_or_empty()};
      if (param_reg.empty()) {
        // argument not passed as register
        tc.add_func_arg(tok(), os, indent + 1, param_name, param_type,
                        int(stk_ix));
        if (param_type.size() > tc.get_type_default().size()) {
          //? todo. support for passing arbitrary size argument
          throw compiler_exception(
              param.tok(),
              "parameter '" + param_name + "' of type '" + param_type.name() +
                  "' can not be passed through the stack "
                  "because its size, " +
                  to_string(param_type.size()) +
                  " bytes, is greater than register size of " +
                  to_string(tc.get_type_default().size()) + " bytes");
        }
        stk_ix += tc.get_type_default().size();
        continue;
      }

      // argument passed as named register
      toc::indent(os, indent + 1, true);
      os << param_name << ": " << param_reg << endl;
      tc.alloc_named_register_or_throw(param, os, indent + 1, param_reg);
      // mark register as initiated
      tc.set_var_is_initiated(param_reg);
      //? check if arg_type fits in register
      allocated_named_registers.emplace_back(param_reg);
      tc.add_alias(param_name, param_reg);
    }
  }

  inline static void
  free_allocated_registers(toc &tc, ostream &os, size_t indent,
                           const vector<string> &allocated_named_registers) {

    // free allocated named register in reverse order
    //? necessary to be in reversed order?
    size_t i{allocated_named_registers.size()};
    while (i--) {
      tc.free_named_register(os, indent + 1, allocated_named_registers[i]);
    }
  }
};
