#include <cassert>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_set>
#include <variant>

using namespace std;

#include "call_asm_mov.hpp"
#include "call_asm_syscall.hpp"
#include "compiler_exception.hpp"
#include "expr_any.hpp"
#include "program.hpp"
#include "stmt_if.hpp"
#include "stmt_loop.hpp"

static auto read_file_to_string(const char *file_name) -> string;
static void optimize_jumps_1(istream &is, ostream &os);
static void optimize_jumps_2(istream &is, ostream &os);

auto main(int argc, char *args[]) -> int {
  const char *src_file_name{argc == 1 ? "prog.baz" : args[1]};
  string src;
  try {
    src = read_file_to_string(src_file_name);
    program prg{src};
    ofstream reproduced_source{"diff.baz"};
    prg.source_to(reproduced_source);
    reproduced_source.close();
    if (src != read_file_to_string("diff.baz")) {
      throw panic_exception("generated source differs. diff " +
                            string{src_file_name} + " diff.baz");
    }

    // without jump optimizations
    // prg.build(cout);

    // with jump optimizations
    stringstream ss1;
    stringstream ss2;
    prg.build(ss1);
    optimize_jumps_1(ss1, ss2);
    optimize_jumps_2(ss2, cout);

  } catch (const compiler_exception &e) {
    const auto [line, col]{
        toc::line_and_col_num_for_char_index(e.start_char, src.c_str())};
    cerr << "\n"
         << src_file_name << ":" << line << ":" << col << ": " << e.msg << endl;
    return 1;
  } catch (const panic_exception &e) {
    cerr << "\nexception: " << e.what() << endl;
    return 2;
  } catch (...) {
    cerr << "\nexception" << endl;
    return 3;
  }
  return 0;
}

// called from 'stmt_block' to solve circular dependencies with 'loop', 'if' and
// function calls
inline auto create_statement_from_tokenizer(toc &tc, unary_ops uops, token tk,
                                            tokenizer &tz)
    -> unique_ptr<statement> {
  if (tk.is_name("loop")) {
    return make_unique<stmt_loop>(tc, move(tk), tz);
  }
  if (tk.is_name("if")) {
    return make_unique<stmt_if>(tc, move(tk), tz);
  }
  if (tk.is_name("mov")) {
    return make_unique<call_asm_mov>(tc, move(tk), tz);
  }
  if (tk.is_name("syscall")) {
    return make_unique<call_asm_syscall>(tc, move(tk), tz);
  }
  return make_unique<stmt_call>(tc, move(uops), move(tk), tz);
}

// called from 'expr_ops_list' to solve circular dependencies with function
// calls
inline auto create_statement_from_tokenizer(toc &tc, tokenizer &tz)
    -> unique_ptr<statement> {

  unary_ops uops{tz};
  token tk{tz.next_token()};
  if (tk.is_name("")) {
    throw compiler_exception(tk, "unexpected empty expression");
  }
  if (tk.name().starts_with("#")) {
    if (!uops.is_empty()) {
      throw compiler_exception(tk, "unexpected comment after unary ops");
    }
    // i.e.  print("hello") # comment
    return make_unique<stmt_comment>(tc, move(tk), tz);
  }
  if (tz.is_peek_char('(')) {
    // i.e.  f(...)
    return create_statement_from_tokenizer(tc, move(uops), move(tk), tz);
  }
  // i.e. 0x80, rax, identifiers
  unique_ptr<statement> st{make_unique<statement>(tk, move(uops))};
  const ident_resolved &st_resolved{tc.resolve_identifier(*st, false)};
  st->set_type(st_resolved.tp);
  return st;
}

inline auto create_expr_any_from_tokenizer(toc &tc, tokenizer &tz,
                                           const type &tp, bool in_args)
    -> unique_ptr<expr_any> {

  return make_unique<expr_any>(tc, tz, tp, in_args);
}

// solves circular reference unary_ops->toc->statement->unary_ops
inline void unary_ops::compile(toc & /*tc*/, ostream &os, size_t indent_level,
                               const string &dst_resolved) const {
  size_t i{ops_.size()};
  while (i--) {
    toc::indent(os, indent_level, false);
    const char op{ops_[i]};
    switch (op) {
    case '~':
      os << "not " << dst_resolved << endl;
      break;
    case '-':
      os << "neg " << dst_resolved << endl;
      //		}else if(op=='!'){
      //			os<<"xor "<<dst_resolved<<",1"<<endl;
      break;
    default:
      throw panic_exception("unary_ops::compile 1");
    }
  }
}

inline void expr_type_value::source_to(ostream &os) const {
  statement::source_to(os);
  if (not tok().is_name("")) {
    return;
  }
  os << '{';
  size_t i{0};
  for (const auto &ea : exprs_) {
    if (i++) {
      os << ',';
    }
    ea->source_to(os);
  }
  os << '}';
}

inline void expr_type_value::compile_recursive(const expr_type_value &atv,
                                               toc &tc, ostream &os,
                                               size_t indent, const string &src,
                                               const string &dst,
                                               const type &dst_type) {

  tc.comment_source(atv, os, indent);
  if (not src.empty()) {
    // e.g. obj.pos = p
    const ident_resolved &id_res{tc.resolve_identifier(atv.tok(), src, true)};
    if (id_res.tp.name() != dst_type.name()) {
      throw compiler_exception(
          atv.tok(), "cannot assign '" + src + "' to '" + dst + "' because '" +
                         src + "' is '" + id_res.tp.name() + "' and '" + dst +
                         "' is '" + dst_type.name() + "'");
    }
    const vector<type_field> &flds{dst_type.fields()};
    const size_t n{flds.size()};
    for (size_t i{0}; i < n; i++) {
      const type_field &fld{flds[i]};
      if (fld.tp.is_built_in()) {
        const string &src_resolved{
            tc.resolve_identifier(atv.tok(), src + "." + fld.name, false)
                .id_nasm};
        const string &dst_resolved{
            tc.resolve_identifier(atv.tok(), dst + "." + fld.name, false)
                .id_nasm};
        tc.asm_cmd(atv.tok(), os, indent, "mov", dst_resolved, src_resolved);
        continue;
      }
      // not a register built-in type, recurse
      compile_recursive(atv, tc, os, indent + 1, src + "." + fld.name,
                        dst + "." + fld.name, fld.tp);
    }
    return;
  }

  // e.g. obj.pos = {x, y, z}
  // e.g. o2 = {p, p2, z}
  // e.g. o2 = {{1, 2, 3}, p2, z}
  const vector<type_field> &flds{dst_type.fields()};
  const size_t n{flds.size()};
  for (size_t i{0}; i < n; i++) {
    const type_field &fld{flds[i]};
    if (fld.tp.is_built_in()) {
      atv.exprs_[i]->compile(tc, os, indent, dst + "." + fld.name);
      continue;
    }
    compile_recursive(atv.exprs_[i]->as_assign_type_value(), tc, os, indent + 1,
                      atv.exprs_[i]->identifier(), dst + "." + fld.name,
                      fld.tp);
  }
}

// inline const type&statement::get_type(toc&tc)const{
//	const ident_resolved&ir{tc.resolve_ident_to_nasm(*this,false)};
//	return ir.tp;
// }
//  opt1
//  example:
//    jmp cmp_13_26
//    cmp_13_26:
//  to
//    cmp_13_26:
static void optimize_jumps_1(istream &is, ostream &os) {
  const regex rxjmp{R"(^\s*jmp\s+(.+)\s*$)"};
  const regex rxlbl{R"(^\s*(.+):.*$)"};
  smatch match;
  while (true) {
    string line1;
    getline(is, line1);
    if (is.eof()) {
      break;
    }

    if (!regex_search(line1, match, rxjmp)) {
      os << line1 << endl;
      continue;
    }
    const string &jmplbl{match[1]};

    string line2;
    vector<string> comments;
    while (true) { // read comments
      getline(is, line2);
      if (line2.starts_with(';')) {
        comments.emplace_back(line2);
        continue;
      }
      break;
    }

    if (!regex_search(line2, match, rxlbl)) {
      os << line1 << endl;
      for (const string &s : comments) {
        os << s << endl;
      }
      os << line2 << endl;
      continue;
    }

    const string &lbl{match[1]};

    if (jmplbl != lbl) {
      os << line1 << endl;
      for (const auto &s : comments) {
        os << s << endl;
      }
      os << line2 << endl;
      continue;
    }

    // write the label without the preceding jmp
    for (const string &s : comments) {
      os << s << endl;
    }
    os << line2 << "  ; opt1" << endl;
  }
}

// opt2
// example:
//   jne cmp_14_26
//   jmp if_14_8_code
//   cmp_14_26:
// to
//   je if_14_8_code
//   cmp_14_26:
static void optimize_jumps_2(istream &is, ostream &os) {
  const regex rxjmp{R"(^\s*jmp\s+(.+)\s*$)"};
  const regex rxjxx{R"(^\s*(j[a-z][a-z]?)\s+(.+)\s*$)"};
  const regex rxlbl{R"(^\s*(.+):.*$)"};
  const regex rxcomment{R"(^\s*;.*$)"};
  smatch match;

  while (true) {
    string line1;
    getline(is, line1);
    if (is.eof()) {
      break;
    }

    if (!regex_search(line1, match, rxjxx)) {
      os << line1 << endl;
      continue;
    }
    const string &jxx{match[1]};
    const string &jxxlbl{match[2]};

    string line2;
    vector<string> comments2;
    while (true) { // read comments
      getline(is, line2);
      if (regex_match(line2, rxcomment)) {
        comments2.emplace_back(line2);
        continue;
      }
      break;
    }
    if (!regex_search(line2, match, rxjmp)) {
      os << line1 << endl;
      for (const auto &s : comments2) {
        os << s << endl;
      }
      os << line2 << endl;
      continue;
    }
    const string &jmplbl{match[1]};

    string line3;
    vector<string> comments3;
    while (true) { // read comments
      getline(is, line3);
      if (regex_match(line3, rxcomment)) {
        comments3.emplace_back(line3);
        continue;
      }
      break;
    }

    if (!regex_search(line3, match, rxlbl)) {
      os << line1 << endl;
      for (const auto &s : comments2) {
        os << s << endl;
      }
      os << line2 << endl;
      for (const auto &s : comments3) {
        os << s << endl;
      }
      os << line3 << endl;
      continue;
    }
    const string lbl{match[1]};

    if (jxxlbl != lbl) {
      os << line1 << endl;
      for (const auto &s : comments2) {
        os << s << endl;
      }
      os << line2 << endl;
      for (const auto &s : comments3) {
        os << s << endl;
      }
      os << line3 << endl;
      continue;
    }

    //   jne cmp_14_26
    //   jmp if_14_8_code
    //   cmp_14_26:
    string jxx_inv;
    if (jxx == "jne") {
      jxx_inv = "je";
    } else if (jxx == "je") {
      jxx_inv = "jne";
    } else if (jxx == "jg") {
      jxx_inv = "jle";
    } else if (jxx == "jge") {
      jxx_inv = "jl";
    } else if (jxx == "jl") {
      jxx_inv = "jge";
    } else if (jxx == "jle") {
      jxx_inv = "jg";
    } else {
      os << line1 << endl;
      for (const auto &s : comments2) {
        os << s << endl;
      }
      os << line2 << endl;
      for (const auto &s : comments3) {
        os << s << endl;
      }
      os << line3 << endl;
      continue;
    }
    //   je if_14_8_code
    //   cmp_14_26:
    const string &ws{line1.substr(0, line1.find_first_not_of(" \t\n\r\f\v"))};
    for (const auto &s : comments2) {
      os << s << endl;
    }
    os << ws << jxx_inv << " " << jmplbl << "  ; opt2" << endl;
    for (const auto &s : comments3) {
      os << s << endl;
    }
    os << line3 << endl;
  }
}

static auto read_file_to_string(const char *file_name) -> string {
  ifstream fs{file_name};
  if (not fs.is_open()) {
    throw panic_exception("cannot open file '" + string{file_name} + "'");
  }
  stringstream buf{};
  buf << fs.rdbuf();
  return buf.str();
}
