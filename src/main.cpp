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
#include "program.hpp"
#include "stmt_if.hpp"
#include "stmt_loop.hpp"

static auto read_file_to_string(const char *file_name) -> string;
static void optimize_jumps_1(istream &is, ostream &os);
static void optimize_jumps_2(istream &is, ostream &os);

auto main(int argc, char *args[]) -> int {
  const char *src_file_name = argc == 1 ? "prog.baz" : args[1];
  string src;
  try {
    src = read_file_to_string(src_file_name);
    program prg{src};
    ofstream reproduced_source{"diff.baz"};
    prg.source_to(reproduced_source);
    reproduced_source.close();
    if (read_file_to_string(src_file_name) != read_file_to_string("diff.baz")) {
      throw panic_exception("generated source differs. diff " +
                            string{src_file_name} + " diff.baz");
    }

    // without jump optimizations
    //		 p.build(cout);

    // with jump optimizations
    stringstream ss1{};
    stringstream ss2{};
    prg.build(ss1);
    optimize_jumps_1(ss1, ss2);
    optimize_jumps_2(ss2, cout);

  } catch (const compiler_exception &e) {
    size_t start_char_in_line{0};
    const size_t lineno{toc::line_number_for_char_index(
        e.start_char, src.c_str(), start_char_in_line)};
    cerr << "\n"
         << src_file_name << ":" << lineno << ":" << start_char_in_line << ": "
         << e.msg << endl;
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

// called from stmt_block to solve circular dependencies with loop, if and calls
inline auto create_statement_from_tokenizer(toc &tc, token tk, unary_ops uops,
                                            tokenizer &t)
    -> unique_ptr<statement> {
  if (tk.is_name("loop")) {
    return make_unique<stmt_loop>(tc, move(tk), t);
  }
  if (tk.is_name("if")) {
    return make_unique<stmt_if>(tc, move(tk), t);
  }
  if (tk.is_name("mov")) {
    return make_unique<call_asm_mov>(tc, move(tk), t);
  }
  if (tk.is_name("syscall")) {
    return make_unique<call_asm_syscall>(tc, move(tk), t);
  }
  return make_unique<stmt_call>(tc, move(tk), move(uops), t);
}

// called from expr_ops_list to solve circular dependencies with calls
inline auto create_statement_from_tokenizer(toc &tc, tokenizer &t)
    -> unique_ptr<statement> {
  unary_ops uops{t};
  token tk = t.next_token();
  if (tk.is_name("")) {
    throw compiler_exception(tk, "unexpected empty expression");
  }

  if (tk.is_name("#")) {
    if (!uops.is_empty()) {
      throw compiler_exception(tk, "unexpected comment after unary ops");
    }
    // i.e.  print("hello") # comment
    return make_unique<stmt_comment>(tc, move(tk), t);
  }
  if (t.is_peek_char('(')) {
    // i.e.  f(...)
    return create_statement_from_tokenizer(tc, move(tk), move(uops), t);
  }
  // i.e. 0x80, rax, identifiers
  unique_ptr<statement> st{make_unique<statement>(move(tk), move(uops))};
  const ident_resolved &ir{tc.resolve_identifier(*st, false)};
  st->set_type(ir.tp);
  return st;
}

// solves circular reference unary_ops->toc->statement->unary_ops
inline void unary_ops::compile(toc & /*tc*/, ostream &os, size_t indent_level,
                               const string &dst_resolved) const {
  size_t i{ops_.size()};
  while (i--) {
    toc::indent(os, indent_level, false);
    const char op{ops_[i]};
    if (op == '~') {
      os << "not " << dst_resolved << endl;
    } else if (op == '-') {
      os << "neg " << dst_resolved << endl;
      //		}else if(op=='!'){
      //			os<<"xor "<<dst_resolved<<",1"<<endl;
    } else {
      throw panic_exception("unexpected code path " + string{__FILE__} + ":" +
                            to_string(__LINE__));
    }
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
        comments.push_back(line2);
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
        comments2.push_back(line2);
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
        comments3.push_back(line3);
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
