#pragma once
#include "statement.hpp"
#include "stmt_def_field.hpp"
#include "stmt_def_func.hpp"
#include "stmt_def_type.hpp"
#include "toc.hpp"
#include "tokenizer.hpp"

class program final {
  // built-in types
  type type_void{"void", 0, true};
  type type_i64{"i64", 8, true};
  type type_i32{"i32", 4, true};
  type type_i16{"i16", 2, true};
  type type_i8{"i8", 1, true};
  type type_bool{"bool", 1, true};

  statement prg{}; // place-holder
  vector<unique_ptr<statement>> statements_{};
  toc tc_;

public:
  inline explicit program(const string &source) : tc_{source} {
    const token prgtk{prg.tok()};
    // add built-in assembler calls
    const vector<string> assembler_funcs{"mov", "syscall"};
    for (const string &str : assembler_funcs) {
      tc_.add_func(prgtk, str, type_void, nullptr);
    }
    tc_.add_type(prgtk, type_i64);
    tc_.add_type(prgtk, type_i32);
    tc_.add_type(prgtk, type_i16);
    tc_.add_type(prgtk, type_i8);
    tc_.add_type(prgtk, type_bool);
    tc_.add_type(prgtk, type_void);

    tc_.set_type_void(type_void);
    tc_.set_type_bool(type_bool);
    tc_.set_type_default(type_i64);

    tokenizer tkz{source};
    while (true) {
      const token tk{tkz.next_token()};
      if (tk.is_empty()) {
        if (tkz.is_eos()) {
          break;
        }
        throw compiler_exception(tk, "unexpected '" + string{tkz.next_char()} +
                                         "'");
      }
      if (tk.is_name("field")) {
        statements_.emplace_back(make_unique<stmt_def_field>(tc_, tk, tkz));
      } else if (tk.is_name("func")) {
        statements_.emplace_back(make_unique<stmt_def_func>(tc_, tk, tkz));
      } else if (tk.is_name("type")) {
        statements_.emplace_back(make_unique<stmt_def_type>(tc_, tk, tkz));
      } else if (tk.is_name("#")) {
        statements_.emplace_back(make_unique<stmt_comment>(tc_, tk, tkz));
      } else if (tk.is_name("")) {
        //? should this be able to happen?
        // throw panic_exception("program:1");
        statements_.emplace_back(make_unique<statement>(tk, unary_ops{}));
      } else {
        throw compiler_exception(tk, "unexpected keyword '" + tk.name() + "'");
      }
    }
  }

  inline program() = delete;
  inline program(const program &) = delete;
  inline program(program &&) = delete;
  inline auto operator=(const program &) -> program & = delete;
  inline auto operator=(program &&) -> program & = delete;

  inline ~program() = default;

  inline void source_to(ostream &os) const {
    for (const auto &st : statements_) {
      st->source_to(os);
    }
  }

  inline void compile(toc &tc, ostream &os, size_t indent,
                      [[maybe_unused]] const string &dst = "") const {
    os << "; generated by baz\n\n";
    os << "true equ 1\nfalse equ 0\n";

    os << "\nsection .data\nalign 4\n";
    for (const auto &st : statements_) {
      if (st->is_in_data_section()) {
        st->compile(tc, os, indent);
      }
    }

    os << "\nsection .bss\nalign 4\nstk resd 1024\nstk.end:\n";
    os << "\nsection .text\nalign 4\nbits 64\nglobal _start\n_start:\nmov "
          "rsp,stk.end\nmov rbp,rsp\njmp main\n\n";
    for (const auto &st : statements_) {
      if (not st->is_in_data_section()) {
        st->compile(tc, os, indent);
      }
    }

    // get the main function and compile
    const stmt_def_func &func_main{tc.get_func_or_throw(prg, "main")};
    if (not func_main.is_inline()) {
      throw compiler_exception(func_main.tok(),
                               "main function must be declared inline");
    }

    os << "main:" << endl;
    tc.enter_func("main", "", "", true, "");
    func_main.code().compile(tc, os, indent);
    tc.exit_func("main");
    os << "; main end" << endl;
    os << endl;
    os << "; system call: exit 0" << endl;
    os << "mov rax, 60" << endl;
    os << "mov rdi, 0" << endl;
    os << "syscall" << endl;
  }

  inline void build(ostream &os) {
    compile(tc_, os, 0);
    tc_.finish(os);
  }
};
