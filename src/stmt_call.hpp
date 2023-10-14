#pragma once
#include "expr_any.hpp"
#include "stmt_def_func.hpp"

class stmt_call : public expression {
  vector<expr_any> args_{};
  bool no_args_{};
  token whitespace_after_{};

public:
  inline stmt_call(toc &tc, unary_ops uops, token tk, tokenizer &tz)
      : expression{move(tk), move(uops)} {

    set_type(tc.get_func_return_type_or_throw(tok(), identifier()));

    if (not tz.is_next_char('(')) {
      no_args_ = true;
      return;
    }

    if (not tc.is_func_builtin(tok(), identifier())) {
      // user defined function
      const stmt_def_func &func{tc.get_func_or_throw(tok(), identifier())};
      // try to create argument expressions of same type as parameter
      size_t i{0};
      const size_t n{func.params().size()};
      for (auto const &param : func.params()) {
        args_.emplace_back(tc, tz, param.get_type(), true);
        i++;
        if (i < n) {
          if (not tz.is_next_char(',')) {
            throw compiler_exception(tz, "expected argument " +
                                             to_string(i + 1) + " '" +
                                             param.name() + "'");
          }
        }
      }
      if (not tz.is_next_char(')')) {
        throw compiler_exception(tz, "expected ')' after arguments");
      }
    } else {
      // built-in function
      //? make this nicer. todo.
      bool expect_arg{false};
      while (true) {
        if (tz.is_next_char(')')) { // foo()
          if (expect_arg) {
            throw compiler_exception(tz, "expected argument after ','");
          }
          break;
        }
        //? default type because built-in function that takes parameter is 'mov'
        args_.emplace_back(tc, tz, tc.get_type_default(), true);
        expect_arg = tz.is_next_char(',');
      }
    }
    whitespace_after_ = tz.next_whitespace_token();
  }

  inline stmt_call() = default;
  inline stmt_call(const stmt_call &) = default;
  inline stmt_call(stmt_call &&) = default;
  inline auto operator=(const stmt_call &) -> stmt_call & = default;
  inline auto operator=(stmt_call &&) -> stmt_call & = default;

  inline ~stmt_call() override = default;

  inline void source_to(ostream &os) const override {
    expression::source_to(os);
    if (no_args_) {
      return;
    }
    os << "(";
    size_t i{args_.size() - 1};
    for (const auto &e : args_) {
      e.source_to(os);
      if (i--) {
        os << ",";
      }
    }
    os << ")";
    whitespace_after_.source_to(os);
  }

  inline void compile(toc &tc, ostream &os, size_t indent,
                      const string &dst = "") const override {
    tc.comment_source(*this, os, indent);
    const stmt_def_func &func{tc.get_func_or_throw(tok(), identifier())};
    const string &func_nm{func.name()};

    // check that the same number of arguments are provided as expected
    if (func.params().size() != args_.size()) {
      throw compiler_exception(
          this->tok(), "function '" + func_nm + "' expects " +
                           to_string(func.params().size()) + " argument" +
                           (func.params().size() == 1 ? "" : "s") + " but " +
                           to_string(args_.size()) + " " +
                           (args_.size() == 1 ? "is" : "are") + " provided");
    }

    // check that argument types match the parameters
    for (size_t i{0}; i < args_.size(); i++) {
      const expr_any &ea{args_[i]};
      const stmt_def_func_param &param{func.param(i)};
      const type &arg_type{ea.get_type()};
      const type &param_type{param.get_type()};
      if (arg_type.is_built_in() and param_type.is_built_in()) {
        //? check if it is integral (not bool)
        if (param_type.size() < arg_type.size()) {
          throw compiler_exception(
              ea.tok(),
              "argument " + to_string(i + 1) + " of type '" + arg_type.name() +
                  "' would be truncated when passed to parameter of type '" +
                  param_type.name() + "'");
        }
        continue;
      }
      if (arg_type.name() != param_type.name()) {
        throw compiler_exception(ea.tok(),
                                 "argument " + to_string(i + 1) + " of type '" +
                                     arg_type.name() +
                                     "' does not match parameter of type '" +
                                     param_type.name() + "'");
      }
    }

    // check that return value matches the type
    if (not dst.empty()) {
      if (func.returns().empty()) {
        throw compiler_exception(
            this->tok(),
            "cannot assign from function that does not return value");
      }

      const type &return_type{func.get_type()};
      const ident_resolved &dst_resolved{
          tc.resolve_identifier(tok(), dst, false)};
      //?
      if (dst_resolved.type_ref.size() < return_type.size()) {
        throw compiler_exception(
            this->tok(), "return type '" + return_type.name() +
                             "' would be truncated when copied to '" + dst +
                             "' of type '" + dst_resolved.type_ref.name() + "'");
      }
    }

    if (not func.is_inline()) {
      // stack is: <base>,
      tc.enter_call(tok(), os, indent);
      // stack is: <base>,vars,regs,

      // push arguments starting with the last
      // some arguments might be passed through registers
      vector<string> allocated_args_registers;
      size_t nbytes_of_args_on_stack{0};
      size_t i{args_.size()};
      while (i--) {
        const expr_any &ea{args_[i]};
        const stmt_def_func_param &param{func.param(i)};
        // is the argument passed through a register?
        const string &arg_reg{param.get_register_name_or_empty()};
        const bool argument_passed_in_register{not arg_reg.empty()};
        if (argument_passed_in_register) {
          tc.alloc_named_register_or_throw(ea, os, indent, arg_reg);
          allocated_args_registers.emplace_back(arg_reg);
        }
        if (ea.is_expression()) {
          if (argument_passed_in_register) {
            // compile expression with the result stored in arg_reg
            ea.compile(tc, os, indent + 1, arg_reg);
            continue;
          }
          // argument passed through stack
          const string &reg{tc.alloc_scratch_register(ea.tok(), os, indent)};
          // compile expression with the result stored in sr
          ea.compile(tc, os, indent + 1, reg);
          // argument is passed to function through the stack
          toc::asm_push(ea.tok(), os, indent, reg);
          // free scratch register
          tc.free_scratch_register(os, indent, reg);
          // keep track of how many arguments are on the stack
          nbytes_of_args_on_stack += 8;
          continue;
        }
        // not an expression, resolve identifier to nasm
        const ident_resolved &arg_resolved{tc.resolve_identifier(ea, true)};
        if (argument_passed_in_register) {
          // move the identifier to the requested register
          if (arg_resolved.is_const()) {
            tc.asm_cmd(ea.tok(), os, indent, "mov", arg_reg,
                       ea.get_unary_ops().to_string() + arg_resolved.id_nasm);
            continue;
          }
          // not a constant
          tc.asm_cmd(ea.tok(), os, indent, "mov", arg_reg,
                     arg_resolved.id_nasm);
          ea.get_unary_ops().compile(tc, os, indent, arg_reg);
          continue;
        }
        // argument not passed through register
        // push identifier on the stack
        if (arg_resolved.is_const()) {
          toc::asm_push(ea.tok(), os, indent,
                        ea.get_unary_ops().to_string() + arg_resolved.id_nasm);
          nbytes_of_args_on_stack += 8;
          continue;
        }
        if (ea.get_unary_ops().is_empty()) {
          // check if argument size is register size
          if (arg_resolved.type_ref.size() == tc.get_type_default().size()) {
            toc::asm_push(ea.tok(), os, indent, arg_resolved.id_nasm);
            nbytes_of_args_on_stack += 8;
            continue;
          }
          // value to be pushed is not a 64 bit value
          // push can only be done with 64 or 16 bits values on x86_64
          const string &reg{tc.alloc_scratch_register(ea.tok(), os, indent)};
          tc.asm_cmd(ea.tok(), os, indent, "mov", reg, arg_resolved.id_nasm);
          toc::asm_push(ea.tok(), os, indent, reg);
          tc.free_scratch_register(os, indent, reg);
          nbytes_of_args_on_stack += 8;
          //? what about 16 bit values
          continue;
        }
        // unary ops must be applied
        const string &reg{tc.alloc_scratch_register(ea.tok(), os, indent)};
        tc.asm_cmd(ea.tok(), os, indent, "mov", reg, arg_resolved.id_nasm);
        ea.get_unary_ops().compile(tc, os, indent, reg);
        toc::asm_push(ea.tok(), os, indent, reg);
        tc.free_scratch_register(os, indent, reg);
        nbytes_of_args_on_stack += 8;
      }

      // stack is: <base>,vars,regs,args,
      toc::asm_call(tok(), os, indent, func_nm);

      tc.exit_call(tok(), os, indent, nbytes_of_args_on_stack,
                   allocated_args_registers);
      // stack is: <base>, (if this was not a call nested in another call's
      // arguments)
      //       or: <base>,vars,regs,

      // handle return value
      if (not dst.empty()) {
        // function returns value in rax, copy return value to dst
        get_unary_ops().compile(tc, os, indent, "rax");
        const ident_resolved &dst_resolved{
            tc.resolve_identifier(tok(), dst, false)};
        tc.asm_cmd(tok(), os, indent, "mov", dst_resolved.id_nasm, "rax");
      }
      return;
    }

    //
    // inlined function
    //

    toc::indent(os, indent, true);
    func.source_def_comment_to(os);

    // create unique labels for in-lined functions based on location of source
    // where the call occurred
    const string &call_path{tc.get_inline_call_path(tok())};
    const string &src_loc{tc.source_location_for_use_in_label(tok())};
    const string &new_call_path{
        call_path.empty() ? src_loc : (src_loc + "_" + call_path)};
    const string &ret_jmp_label{func_nm + "_" + new_call_path + "_end"};

    toc::indent(os, indent + 1, true);
    os << "inline: " << new_call_path << endl;

    vector<string> allocated_named_registers;
    vector<string> allocated_scratch_registers;
    vector<string> allocated_registers_in_order;

    // buffer the aliases of arguments
    vector<tuple<string, string>> aliases_to_add;

    // if function returns value
    if (not dst.empty()) {
      // alias 'from' identifier to 'dst' identifier
      const string &from{func.returns()[0].ident_tk.name()};
      const string &to{dst};
      aliases_to_add.emplace_back(from, to);
      toc::indent(os, indent + 1, true);
      os << "alias " << from << " -> " << to << endl;
    }

    size_t i{0};
    for (const expr_any &ea : args_) {
      const stmt_def_func_param &param{func.param(i)};
      i++;
      // does the parameter want the value passed through a register?
      string arg_reg{param.get_register_name_or_empty()};
      if (not arg_reg.empty()) {
        // argument is passed through register
        tc.alloc_named_register_or_throw(ea, os, indent + 1, arg_reg);
        allocated_named_registers.emplace_back(arg_reg);
        allocated_registers_in_order.emplace_back(arg_reg);
      }
      if (ea.is_expression()) {
        // argument is an expression, evaluate and store in arg_reg
        if (arg_reg.empty()) {
          // no particular register requested for the argument
          arg_reg = tc.alloc_scratch_register(ea.tok(), os, indent + 1);
          allocated_scratch_registers.emplace_back(arg_reg);
          allocated_registers_in_order.emplace_back(arg_reg);
        }
        toc::indent(os, indent + 1, true);
        os << "alias " << param.identifier() << " -> " << arg_reg << endl;
        // compile expression and store result in 'arg_reg'
        // note. 'unary_ops' are part of 'expr_ops_list'
        ea.compile(tc, os, indent + 1, arg_reg);
        // alias parameter name to the register containing its value
        aliases_to_add.emplace_back(param.identifier(), arg_reg);
        continue;
      }

      // argument is not an expression
      // check if argument is passed through register
      if (arg_reg.empty()) {
        // argument not passed through register
        // no register allocated for the argument
        // alias parameter name to the argument identifier
        if (ea.get_unary_ops().is_empty()) {
          const string &arg_id{ea.identifier()};
          aliases_to_add.emplace_back(param.identifier(), arg_id);
          toc::indent(os, indent + 1, true);
          os << "alias " << param.identifier() << " -> " << arg_id << endl;
          continue;
        }
        // unary ops must be applied
        const ident_resolved &arg_resolved{tc.resolve_identifier(ea, true)};
        const string &scratch_reg{
            tc.alloc_scratch_register(ea.tok(), os, indent + 1)};
        allocated_registers_in_order.emplace_back(scratch_reg);
        allocated_scratch_registers.emplace_back(scratch_reg);
        tc.asm_cmd(param.tok(), os, indent + 1, "mov", scratch_reg,
                   arg_resolved.id_nasm);
        ea.get_unary_ops().compile(tc, os, indent + 1, scratch_reg);
        aliases_to_add.emplace_back(param.identifier(), scratch_reg);
        toc::indent(os, indent + 1, true);
        os << "alias " << param.identifier() << " -> " << scratch_reg << endl;
        continue;
      }
      // argument passed through register
      // alias parameter name to the register
      aliases_to_add.emplace_back(param.identifier(), arg_reg);
      toc::indent(os, indent + 1, true);
      os << "alias " << param.identifier() << " -> " << arg_reg << endl;
      // move argument to register specified in param
      const ident_resolved &arg_resolved{tc.resolve_identifier(ea, true)};
      if (arg_resolved.is_const()) {
        tc.asm_cmd(param.tok(), os, indent + 1, "mov", arg_reg,
                   ea.get_unary_ops().to_string() + arg_resolved.id_nasm);
        continue;
      }
      // not a const, move argument to register
      tc.asm_cmd(param.tok(), os, indent + 1, "mov", arg_reg,
                 arg_resolved.id_nasm);
      ea.get_unary_ops().compile(tc, os, indent + 1, arg_reg);
    }

    // enter function creating a new scope from which
    //   prior variables are not visible
    tc.enter_func(func_nm, func.returns(), true, new_call_path, ret_jmp_label);

    // add the aliases to the context of this scope
    for (const auto &e : aliases_to_add) {
      const string &from{get<0>(e)};
      const string &to{get<1>(e)};
      tc.add_alias(from, to);
    }

    // compile in-lined code
    func.code().compile(tc, os, indent);

    // free allocated registers in reverse order of allocation
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (auto it{allocated_registers_in_order.rbegin()};
         it != allocated_registers_in_order.rend(); ++it) {
      const string &reg{*it};
      if (find(allocated_scratch_registers.begin(),
               allocated_scratch_registers.end(),
               reg) != allocated_scratch_registers.end()) {
        tc.free_scratch_register(os, indent + 1, reg);
        continue;
      }
      if (find(allocated_named_registers.begin(),
               allocated_named_registers.end(),
               reg) != allocated_named_registers.end()) {
        tc.free_named_register(os, indent + 1, reg);
        continue;
      }
      throw panic_exception("unexpected code path " + string{__FILE__} + ":" +
                            to_string(__LINE__));
    }

    // provide an exit label for 'return' to jump to instead of assembler 'ret'
    toc::asm_label(tok(), os, indent, ret_jmp_label);

    // if the result of the call has unary ops
    if (not get_unary_ops().is_empty()) {
      if (func.returns().empty()) {
        throw compiler_exception(this->tok(),
                                 "function call has unary operations but it "
                                 "does not return a value");
      }

      const ident_resolved &ret_resolved{tc.resolve_identifier(
          tok(), func.returns()[0].ident_tk.name(), true)};
      get_unary_ops().compile(tc, os, indent, ret_resolved.id_nasm);
    }
    // pop scope
    tc.exit_func(func_nm);
  }

  [[nodiscard]] inline auto arg(size_t ix) const -> const statement & {
    return args_[ix];
  }

  [[nodiscard]] inline auto arg_count() const -> size_t { return args_.size(); }
};
