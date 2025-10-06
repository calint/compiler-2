// review: 2025-09-29

#include <cassert>
#include <cstddef>
#include <cstring>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <ranges>
#include <regex>
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "expr_any.hpp"
#include "expr_type_value.hpp"
#include "panic_exception.hpp"
#include "program.hpp"
#include "statement.hpp"
#include "stmt_call.hpp"
#include "stmt_call_asm_mov.hpp"
#include "stmt_call_asm_syscall.hpp"
#include "stmt_comment.hpp"
#include "stmt_identifier.hpp"
#include "stmt_if.hpp"
#include "stmt_loop.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"
#include "unary_ops.hpp"

namespace {
auto read_file_to_string(const char* file_name) -> std::string;
auto optimize_jumps_1(std::istream& is, std::ostream& os) -> void;
auto optimize_jumps_2(std::istream& is, std::ostream& os) -> void;
} // namespace

auto main(const int argc, const char* argv[]) -> int {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-container"
    const std::span<const char*> args{argv, static_cast<size_t>(argc)};
#pragma clang diagnostic pop

    const char* src_file_name{argc == 1 ? "prog.baz" : args[1]};

    std::string src;
    try {
        src = read_file_to_string(src_file_name);
        program prg{src};
        std::ofstream reproduced_source{"diff.baz"};
        prg.source_to(reproduced_source);
        reproduced_source.close();
        if (src != read_file_to_string("diff.baz")) {
            throw panic_exception(
                std::format("generated source differs. diff {} diff.baz",
                            std::string{src_file_name}));
        }

        // with jump optimizations
        std::stringstream ss1;
        std::stringstream ss2;
        prg.build(ss1);
        // prg.build(std::cout); // build without jump optimizations
        optimize_jumps_1(ss1, ss2);
        optimize_jumps_2(ss2, std::cout);

    } catch (const compiler_exception& e) {
        const auto [line, col]{
            toc::line_and_col_num_for_char_index(e.start_index, src)};
        std::cerr << "\n"
                  << src_file_name << ":" << line << ":" << col << ": " << e.msg
                  << '\n';
        return 1;
    } catch (const panic_exception& e) {
        std::cerr << "\npanic: " << e.what() << '\n';
        return 2;
    } catch (...) {
        std::cerr << "\nunknown exception" << '\n';
        return 3;
    }
}

//
// implementation of functions that could not be implemented in headers due to
// circular references
//

// declared in 'decouple.hpp'
// called from 'stmt_block' to solve circular dependencies with 'loop', 'if' and
// function calls
inline auto create_statement_from_tokenizer(toc& tc, tokenizer& tz, token tk)
    -> std::unique_ptr<statement> {

    if (tk.is_name("loop")) {
        return std::make_unique<stmt_loop>(tc, std::move(tk), tz);
    }
    if (tk.is_name("if")) {
        return std::make_unique<stmt_if>(tc, std::move(tk), tz);
    }
    if (tk.is_name("mov")) {
        return std::make_unique<stmt_call_asm_mov>(tc, std::move(tk), tz);
    }
    if (tk.is_name("syscall")) {
        return std::make_unique<stmt_call_asm_syscall>(tc, std::move(tk), tz);
    }

    throw panic_exception{"unexpected code path main:1"};
}

// declared in 'decouple.hpp'
// called from 'expr_ops_list' to solve circular dependencies with function
// calls
inline auto create_statement(toc& tc, tokenizer& tz)
    -> std::unique_ptr<statement> {

    const unary_ops uops{tz};
    token tk{tz.next_token()};
    if (tk.is_name("")) {
        throw compiler_exception(tk,
                                 "expected an identifier or a function call");
    }
    if (tk.name().starts_with("#")) {
        if (!uops.is_empty()) {
            throw compiler_exception(tk, "unexpected comment after unary ops");
        }
        // e.g.  print("hello") # comment
        return std::make_unique<stmt_comment>(tc, std::move(tk), tz);
    }
    if (tz.is_peek_char('(')) {
        // e.g.  foo(...)
        return std::make_unique<stmt_call>(tc, uops, std::move(tk), tz);
    }
    // e.g. 0x80, rax, identifiers
    return std::make_unique<stmt_identifier>(tc, tz, std::move(tk), uops);
}

// called from 'stmt_block'
inline auto create_stmt_call(toc& tc, tokenizer& tz, const stmt_identifier& si)
    -> std::unique_ptr<statement> {
    return std::make_unique<stmt_call>(tc, si.get_unary_ops(), si.first_token(),
                                       tz);
}
// declared in 'decouple.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
inline auto create_expr_any(toc& tc, tokenizer& tz, const type& tp,
                            bool in_args) -> std::unique_ptr<expr_any> {

    return std::make_unique<expr_any>(tc, tz, tp, in_args);
}

// declared in 'expr_type_value.hpp'
// note: constructor and destructor is implemented in 'main.cpp' where the
// 'expr_any' definition is known. clang++ -std=c++23 requires it since
// changes to handling of unique_ptr to incomplete types

inline expr_type_value::expr_type_value(toc& tc, tokenizer& tz, const type& tp)
    : statement{tz.next_token()} {

    set_type(tp);

    // is it an identifier?
    // note: token name would be empty at "{ x, y }" type of statement
    if (not tok().is_name("")) {
        // yes, e.g. obj.pos = p

        stmt_ident_ = std::make_shared<stmt_identifier>(tc, tz, tok());

        // check that identifier type matches expected type
        const ident_info ii{
            tc.make_ident_info(tok(), stmt_ident_->identifier(), false)};

        if (tp.name() != ii.type_ref.name()) {
            // note: checked source location report ok
            throw compiler_exception{
                tok(),
                std::format("type '{}' does not match expected type '{}'",
                            ii.type_ref.name(), tp.name())};
        }

        return;
    }

    // e.g. obj.pos = {x, y}
    if (not tz.is_next_char('{')) {
        throw compiler_exception(
            tz,
            std::format("expected '{{' to open assign type '{}'", tp.name()));
    }

    const std::vector<type_field>& flds{tp.fields()};
    const size_t nflds{flds.size()};
    for (size_t i{}; i < nflds; i++) {
        const type_field& fld{flds.at(i)};
        // create expression that assigns to field
        // might recurse creating 'expr_type_value'
        exprs_.emplace_back(create_expr_any(tc, tz, fld.tp, true));

        if (i < nflds - 1) {
            if (not tz.is_next_char(',')) {
                throw compiler_exception(
                    tz, std::format(
                            "expected ',' and value of field '{}' in type '{}'",
                            flds[i + 1].name, tp.name()));
            }
        }
    }

    if (not tz.is_next_char('}')) {
        throw compiler_exception(
            tz,
            std::format("expected '}}' to close assign type '{}'", tp.name()));
    }
}

expr_type_value::~expr_type_value() = default;

// declared in 'expr_type_value.hpp'
// resolves circular reference: expr_type_value -> expr_any -> expr_type_value
inline void expr_type_value::source_to(std::ostream& os) const {
    // is it an identifier? because that was printed by statement
    if (is_make_copy()) {
        stmt_ident_->source_to(os);
        return;
    }

    statement::source_to(os);

    // not an identifier
    os << '{';
    size_t i{};
    for (const std::unique_ptr<expr_any>& ea : exprs_) {
        if (i++) {
            os << ',';
        }
        ea->source_to(os);
    }
    os << '}';
}

// declared in 'expr_type_value.hpp'
// resolves circular reference: expr_type_value -> expr_any -> expr_type_values
inline auto expr_type_value::compile_copy(toc& tc, std::ostream& os,
                                          size_t indent,
                                          const std::string& dst) const
    -> void {

    stmt_identifier::compile_address_calculation(tok(), tc, os, indent,
                                                 stmt_ident_->elems(), dst);
}

// declared in 'expr_type_value.hpp'
// resolves circular reference: expr_type_value -> expr_any -> expr_type_values
inline auto expr_type_value::compile_recursive(const expr_type_value& etv,
                                               toc& tc, std::ostream& os,
                                               size_t indent,
                                               const std::string& src,
                                               const std::string& dst,
                                               const type& dst_type) -> void {

    tc.comment_source(etv, os, indent);

    // is it an identifier?
    if (not src.empty()) {
        // yes, e.g. obj.pos = p
        const type& src_type{tc.make_ident_info(etv.tok(), src, true).type_ref};
        if (src_type.name() != dst_type.name()) {
            throw compiler_exception(
                etv.tok(), std::format("cannot assign '{}' to '{}' because "
                                       "'{}' is '{}' and '{}' is '{}'",
                                       src, dst, src, src_type.name(), dst,
                                       dst_type.name()));
        }

        for (const type_field& fld : dst_type.fields()) {
            if (fld.tp.is_built_in()) {
                const std::string& src_info{
                    tc.make_ident_info(
                          etv.tok(), std::format("{}.{}", src, fld.name), false)
                        .id_nasm};
                const std::string& dst_info{
                    tc.make_ident_info(
                          etv.tok(), std::format("{}.{}", dst, fld.name), false)
                        .id_nasm};
                tc.asm_cmd(etv.tok(), os, indent, "mov", dst_info, src_info);
                continue;
            }

            // not a built-in, recurse
            compile_recursive(etv, tc, os, indent + 1,
                              std::format("{}.{}", src, fld.name),
                              std::format("{}.{}", dst, fld.name), fld.tp);
        }
        return;
    }

    // e.g. obj.pos = {x, foo(y), z}
    // e.g. o2 = {p, p2, z}
    // e.g. o2 = {{1, 2, 3}, p2, z}
    const std::vector<type_field>& flds{dst_type.fields()};
    const size_t n{flds.size()};
    for (size_t i{}; i < n; i++) {
        const type_field& fld{flds.at(i)};
        if (fld.tp.is_built_in()) {
            etv.exprs_.at(i)->compile(tc, os, indent,
                                      std::format("{}.{}", dst, fld.name));
            continue;
        }
        // not a built-in type, recurse
        compile_recursive(etv.exprs_.at(i)->as_assign_type_value(), tc, os,
                          indent + 1, etv.exprs_.at(i)->identifier(),
                          std::format("{}.{}", dst, fld.name), fld.tp);
    }
}

// declared in 'unary_ops.hpp'
// solves circular reference: unary_ops -> toc -> statement -> unary_ops
inline void unary_ops::compile([[maybe_unused]] toc& tc, std::ostream& os,
                               size_t indnt,
                               const std::string& dst_info) const {

    for (auto op : std::views::reverse(ops_)) {
        switch (op) {
        case '~':
            toc::asm_not(ws_before_, os, indnt, dst_info);
            break;
        case '-':
            toc::asm_neg(ws_before_, os, indnt, dst_info);
            break;
        default:
            throw panic_exception("unexpected code path main:1");
        }
    }
}

namespace {
//  opt1
//  example:
//    jmp cmp_13_26
//    cmp_13_26:
//  to
//    cmp_13_26:
auto optimize_jumps_1(std::istream& is, std::ostream& os) -> void {
    const std::regex rxjmp{R"(^\s*jmp\s+(.+)\s*$)"};
    const std::regex rxlbl{R"(^\s*(.+):.*$)"};
    const std::regex rxcomment{R"(^\s*;.*$)"};
    std::smatch match;
    while (true) {
        std::string line1;
        getline(is, line1);
        if (is.eof()) { //? what if there is no new line at end of file?
            break;
        }

        if (not std::regex_search(line1, match, rxjmp)) {
            os << line1 << '\n';
            continue;
        }
        const std::string jmplbl{match[1]};

        std::string line2;
        std::vector<std::string> comments;
        while (true) { // read comments
            getline(is, line2);
            if (std::regex_match(line2, rxcomment)) {
                comments.emplace_back(line2);
                continue;
            }
            break;
        }

        if (not std::regex_search(line2, match, rxlbl)) {
            os << line1 << '\n';
            for (const std::string& s : comments) {
                os << s << '\n';
            }
            os << line2 << '\n';
            continue;
        }

        const std::string lbl{match[1]};

        // if not same label then output the buffered data and continues
        if (jmplbl != lbl) {
            os << line1 << '\n';
            for (const std::string& s : comments) {
                os << s << '\n';
            }
            os << line2 << '\n';
            continue;
        }

        // write the label without the preceding jmp
        for (const std::string& s : comments) {
            os << s << '\n';
        }
        // os << line2 << "  ; opt1" << '\n';
        os << line2 << '\n';
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
auto optimize_jumps_2(std::istream& is, std::ostream& os) -> void {
    const std::regex rxjmp{R"(^\s*jmp\s+(.+)\s*$)"};
    const std::regex rxjxx{R"(^\s*(j[a-z][a-z]?)\s+(.+)\s*$)"};
    const std::regex rxlbl{R"(^\s*(.+):.*$)"};
    const std::regex rxcomment{R"(^\s*;.*$)"};
    std::smatch match;

    while (true) {
        std::string line1;
        getline(is, line1);
        if (is.eof()) { //? what if there is no new line at end of file?
            break;
        }

        if (not std::regex_search(line1, match, rxjxx)) {
            os << line1 << '\n';
            continue;
        }
        const std::string jxx{match[1]};
        const std::string jxxlbl{match[2]};

        std::string line2;
        std::vector<std::string> comments2;
        while (true) { // read comments
            getline(is, line2);
            if (std::regex_match(line2, rxcomment)) {
                comments2.emplace_back(line2);
                continue;
            }
            break;
        }
        if (not std::regex_search(line2, match, rxjmp)) {
            os << line1 << '\n';
            for (const std::string& s : comments2) {
                os << s << '\n';
            }
            os << line2 << '\n';
            continue;
        }
        const std::string jmplbl{match[1]};

        std::string line3;
        std::vector<std::string> comments3;
        while (true) { // read comments
            getline(is, line3);
            if (std::regex_match(line3, rxcomment)) {
                comments3.emplace_back(line3);
                continue;
            }
            break;
        }

        if (not std::regex_search(line3, match, rxlbl)) {
            os << line1 << '\n';
            for (const std::string& s : comments2) {
                os << s << '\n';
            }
            os << line2 << '\n';
            for (const std::string& s : comments3) {
                os << s << '\n';
            }
            os << line3 << '\n';
            continue;
        }

        const std::string lbl{match[1]};

        if (jxxlbl != lbl) {
            os << line1 << '\n';
            for (const std::string& s : comments2) {
                os << s << '\n';
            }
            os << line2 << '\n';
            for (const std::string& s : comments3) {
                os << s << '\n';
            }
            os << line3 << '\n';
            continue;
        }

        //   jne cmp_14_26
        //   jmp if_14_8_code
        //   cmp_14_26:
        std::string jxx_inv;
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
            os << line1 << '\n';
            for (const std::string& s : comments2) {
                os << s << '\n';
            }
            os << line2 << '\n';
            for (const std::string& s : comments3) {
                os << s << '\n';
            }
            os << line3 << '\n';
            continue;
        }
        //   je if_14_8_code
        //   cmp_14_26:
        // get the whitespaces
        const std::string& ws_before{
            line1.substr(0, line1.find_first_not_of(" \t\n\r\f\v"))};
        for (const std::string& s : comments2) {
            os << s << '\n';
        }
        // os << ws_before << jxx_inv << " " << jmplbl << "  ; opt2" << '\n';
        os << ws_before << jxx_inv << " " << jmplbl << '\n';
        for (const std::string& s : comments3) {
            os << s << '\n';
        }
        os << line3 << '\n';
    }
}

auto read_file_to_string(const char* file_name) -> std::string {
    std::ifstream fs{file_name};
    if (not fs.is_open()) {
        throw panic_exception(
            std::format("cannot open file '{}'", std::string{file_name}));
    }
    return std::string{std::istreambuf_iterator<char>{fs},
                       std::istreambuf_iterator<char>{}};
}

} // end of anonymous namespace
