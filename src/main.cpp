// review: 2025-09-29

#include <cstdint>
#include <cstdio>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <print>
#include <ranges>
#include <regex>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "expr_type_value.hpp"
#include "panic_exception.hpp"
#include "program.hpp"
#include "statement.hpp"
#include "stmt_builtin_address_of.hpp"
#include "stmt_builtin_array_size_of.hpp"
#include "stmt_builtin_arrays_equal.hpp"
#include "stmt_builtin_equal.hpp"
#include "stmt_builtin_mov.hpp"
#include "stmt_builtin_syscall.hpp"
#include "stmt_call.hpp"
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

// NOLINTNEXTLINE(bugprone-exception-escape)
auto main(const int argc, const char* argv[]) -> int {

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-container"
    const std::span<const char*> args{argv, static_cast<size_t>(argc)};
#pragma clang diagnostic pop

    constexpr size_t default_stack_size{0x10000};
    // note: to avoid "magic number" lint

    // default values
    const char* src_file_name = "prog.baz";
    size_t stack_size = default_stack_size;
    bool checks_upper = false;
    bool checks_show_line = false;
    bool checks_lower = false;
    bool optimize_jumps = true;

    // parse arguments
    for (size_t i = 1; i < args.size(); i++) {
        const std::string_view arg{args[i]};

        if (arg == "--help" || arg == "-h") {
            const std::string_view prg{args[0]};
            std::println("Usage: {} [OPTIONS] [filename]", prg);
            std::println("");
            std::println("Options:");
            std::println("  --stack=SIZE        Set stack size (default: "
                         "0x10000/65536)");
            std::println(
                "                      Supports decimal and hex (0x prefix) ");
            std::println("  --checks=TYPE       Enable runtime checks:");
            std::println(
                "                        upper - check upper array bounds");
            std::println(
                "                        lower - check lower array bounds");
            std::println("                         line - report line number");
            std::println("             upper,lower,line - all");
            std::println("  --nopt              No jump optimizations");
            std::println("  --help, -h          Show this help message");
            std::println("");
            std::println("Arguments:");
            std::println(
                "  filename            Source file (default: prog.baz)");
            std::println("");
            std::println("Examples:");
            std::println("  {} myfile.baz", prg);
            std::println("  {} --stack=131072 --checks=upper prog.baz", prg);
            std::println("  {} --checks=upper,lower,line prog.baz", prg);
            std::println("  {} --checks=upper prog.baz", prg);
            return 0;
        }
        constexpr std::string_view stack_option{"--stack="};
        constexpr std::string_view checks_option{"--checks="};
        constexpr std::string_view nopt_option{"--nopt"};
        if (arg.starts_with(stack_option)) {
            try {
                stack_size = std::stoul(
                    std::string{arg.substr(stack_option.size())}, nullptr, 0);
            } catch (...) {
                std::println(stderr, "Could not parse stack size: \"{}\"",
                             arg.substr(stack_option.size()));
                std::println(stderr, "Use --help for usage information");
                return 1;
            }
        } else if (arg.starts_with(checks_option)) {
            const std::string_view checks{arg.substr(checks_option.size())};
            std::istringstream iss{std::string{checks}};

            checks_upper = false;
            checks_lower = false;
            checks_show_line = false;

            std::string option;
            while (std::getline(iss, option, ',')) {
                if (option == "upper") {
                    checks_upper = true;
                } else if (option == "lower") {
                    checks_lower = true;
                } else if (option == "line") {
                    checks_show_line = true;
                } else if (not option.empty()) {
                    std::println(std::cerr,
                                 "Invalid --checks option: '{}'. Supported "
                                 "options are: upper, lower, line.",
                                 option);
                    std::println(stderr, "Use --help for usage information");
                    return 1; // or some other error handling mechanism
                }
            }
        } else if (arg == nopt_option) {
            optimize_jumps = false;
        } else if (not arg.starts_with("--")) {
            // Assume it's the filename
            src_file_name = args[i];
        } else {
            std::println(stderr, "Error: Unknown option: {}", arg);
            std::println(stderr, "Use --help for usage information");
            return 1;
        }
    }

    std::string src;
    try {
        src = read_file_to_string(src_file_name);
        program prg{src, stack_size, checks_upper, checks_lower,
                    checks_show_line};
        // prg.source_to(std::cerr);
        std::ofstream reproduced_source{"diff.baz"};
        prg.source_to(reproduced_source);
        reproduced_source.close();
        if (src != read_file_to_string("diff.baz")) {
            throw panic_exception{std::format(
                "generated source differs. diff {} diff.baz", src_file_name)};
        }

        if (optimize_jumps) {
            // with jump optimizations
            std::stringstream ss1;
            std::stringstream ss2;
            prg.build(ss1);
            optimize_jumps_1(ss1, ss2);
            optimize_jumps_2(ss2, std::cout);
        } else {
            prg.build(std::cout);
        }

    } catch (const compiler_exception& e) {
        const auto [line, col]{
            toc::line_and_col_num_for_char_index(e.line, e.start_index, src)};
        std::println(stderr, "\n{}:{}:{}: {}", src_file_name, line, col, e.msg);
        return 1;
    } catch (const panic_exception& e) {
        std::println(stderr, "\npanic: {}", e.what());
        return 2;
    } catch (...) {
        std::println(stderr, "\nunknown exception");
        return 3;
    }
}

//
// implementation of functions that could not be implemented in headers due
// to circular references
//

// declared in 'decouple.hpp'
// called from 'stmt_block' to solve circular dependencies with 'loop',
// 'if', 'mov', 'syscall'
[[nodiscard]] inline auto create_statement_in_stmt_block(toc& tc, tokenizer& tz,
                                                         const token tk)
    -> std::unique_ptr<statement> {

    // note: no 'std:move' on 'tk' because it is trivially copyable
    if (tk.is_text("loop")) {
        return std::make_unique<stmt_loop>(tc, tk, tz);
    }
    if (tk.is_text("if")) {
        return std::make_unique<stmt_if>(tc, tk, tz);
    }
    if (tk.is_text("mov")) {
        return std::make_unique<stmt_builtin_mov>(tc, tk, tz);
    }
    if (tk.is_text("syscall")) {
        return std::make_unique<stmt_builtin_syscall>(tc, tk, tz);
    }

    std::unreachable();
}

// declared in 'decouple.hpp'
// called from 'stmt_block'
[[nodiscard]] inline auto create_stmt_call(toc& tc, tokenizer& tz,
                                           const stmt_identifier& si)
    -> std::unique_ptr<statement> {

    if (si.elems().size() != 1) {
        throw compiler_exception{si.first_token(),
                                 "did not expect '.' in function name"};
    }

    return std::make_unique<stmt_call>(tc, si.get_unary_ops(), si.first_token(),
                                       tz);
}

// declared in 'decouple.hpp'
// called from 'expr_ops_list' to solve circular dependencies with function
// calls
[[nodiscard]] inline auto create_statement_in_expr_ops_list(toc& tc,
                                                            tokenizer& tz)
    -> std::unique_ptr<statement> {

    // note: no 'std:move' on 'tk' because it is trivially copyable
    unary_ops uops{tz};
    const token tk{tz.next_token()};
    if (tk.is_text("")) {
        throw compiler_exception{
            tk, "expected constant, identifier or function call"};
    }
    if (tk.text().starts_with("#")) {
        throw compiler_exception{tk, "unexpected comment in expression"};
    }
    if (tk.is_text("address_of")) {
        return std::make_unique<stmt_builtin_address_of>(tc, std::move(uops),
                                                         tk, tz);
    }
    if (tk.is_text("array_size_of")) {
        return std::make_unique<stmt_builtin_array_size_of>(tc, std::move(uops),
                                                            tk, tz);
    }
    if (tk.is_text("arrays_equal")) {
        return std::make_unique<stmt_builtin_arrays_equal>(tc, std::move(uops),
                                                           tk, tz);
    }
    if (tk.is_text("equal")) {
        return std::make_unique<stmt_builtin_equal>(tc, std::move(uops), tk,
                                                    tz);
    }
    if (tz.is_peek_char('(')) {
        // e.g.  foo(...)
        return std::make_unique<stmt_call>(tc, std::move(uops), tk, tz);
    }
    // e.g. 0x80, rax, identifiers
    return std::make_unique<stmt_identifier>(tc, std::move(uops), tk, tz);
}

// declared in 'decouple.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
[[nodiscard]] inline auto create_expr_any(toc& tc, tokenizer& tz,
                                          const type& tp, const bool in_args)
    -> std::unique_ptr<expr_any> {

    return std::make_unique<expr_any>(tc, tz, tp, in_args);
}

// declared in 'expr_type_value.hpp'
// note: constructor and destructor is implemented in 'main.cpp' where the
//       'expr_any' definition is known. clang++ -std=c++23 has required it
//       since changes to handling of unique_ptr to incomplete types

inline expr_type_value::expr_type_value(toc& tc, tokenizer& tz, const type& tp)
    : statement{tz.next_token()} {

    set_type(tp);

    // is it an identifier?
    // note: token name would be empty at the "{x, y}" type of statement
    if (not tok().is_text("")) {
        // yes, e.g. obj.pos = p

        stmt_ident_ =
            std::make_shared<stmt_identifier>(tc, unary_ops{}, tok(), tz);

        if (tz.is_next_char('(')) {
            throw compiler_exception{tok(),
                                     "did not expect function call, but what "
                                     "to copy or initializer using '{...}'"};
        }

        // check that an identifier type matches the expected type
        const ident_info src_info{tc.make_ident_info(*stmt_ident_)};

        if (tp.name() != src_info.type_ptr->name()) {
            // note: checked a source location report ok
            throw compiler_exception{
                tok(),
                std::format("type '{}' does not match expected type '{}'",
                            src_info.type_ptr->name(), tp.name())};
        }

        return;
    }

    // e.g. obj.pos = {x, y}
    if (not tz.is_next_char('{')) {
        throw compiler_exception{
            tz,
            std::format("expected '{{' to open assign type '{}'", tp.name())};
    }

    const std::span<const type_field> flds{tp.fields()};
    const size_t nflds{flds.size()};
    for (size_t i{}; i < nflds; i++) {
        const type_field& fld{flds[i]};
        // create an expression that assigns to field
        // might recurse creating 'expr_type_value'
        exprs_.emplace_back(create_expr_any(tc, tz, *fld.type_ptr, true));

        if (i < nflds - 1) {
            if (not tz.is_next_char(',')) {
                throw compiler_exception{
                    tz, std::format(
                            "expected ',' and value of field '{}' in type '{}'",
                            flds[i + 1].name, tp.name())};
            }
        }
    }

    if (not tz.is_next_char('}')) {
        throw compiler_exception{
            tz,
            std::format("expected '}}' to close assign type '{}'", tp.name())};
    }

    ws1_ = tz.next_whitespace_token();
}

expr_type_value::~expr_type_value() = default;

// declared in 'expr_type_value.hpp'
// resolves circular reference: expr_type_value -> expr_any ->
// expr_type_value
inline auto expr_type_value::source_to(std::ostream& os) const -> void {
    // is it an identifier? because statement printed that
    if (is_make_copy()) {
        stmt_ident_->source_to(os);
        return;
    }

    statement::source_to(os);

    // not an identifier
    std::print(os, "{{");
    size_t i{};
    for (const std::unique_ptr<expr_any>& ea : exprs_) {
        if (i++) {
            std::print(os, ",");
        }
        ea->source_to(os);
    }
    std::print(os, "}}");
    ws1_.source_to(os);
}

// declared in 'expr_type_value.hpp'
// resolves circular reference: expr_type_value -> expr_any ->
// expr_type_value
auto expr_type_value::compile_assign(toc& tc, std::ostream& os, size_t indent,
                                     const type& dst_type,
                                     operand& dst_op) const -> void {
    if (is_identifier()) {
        const ident_info src_info{tc.make_ident_info(*this)};
        if (dst_type.name() != src_info.type_ptr->name()) {
            throw compiler_exception{
                tok(),
                std::format(
                    "destination type '{}' does not match source type '{}'",
                    dst_type.name(), src_info.type_ptr->name())};
        }
        std::vector<std::string> allocated_registers;
        operand src_op;
        if (is_indexed() or src_info.has_lea()) {
            src_op = compile_lea(tok(), tc, os, indent, allocated_registers, "",
                                 src_info.lea_path);
        } else {
            src_op = src_info.operand;
        }
        const size_t nbytes{src_info.is_array
                                ? src_info.array_size * dst_type.size()
                                : dst_type.size()};
        tc.rep_movs(tok(), os, indent, src_op.address_str(),
                    dst_op.address_str(), nbytes);

        dst_op.displacement += static_cast<int32_t>(nbytes);

        for (const std::string& reg :
             allocated_registers | std::views::reverse) {
            tc.free_scratch_register(tok(), os, indent, reg);
        }
        return;
    }

    size_t i{};
    for (const type_field& fld : dst_type.fields()) {
        tc.comment_start(tok(), os, indent);
        std::println(os, "copy field '{}'", fld.name);

        if (not fld.type_ptr->is_built_in()) {
            // a not-builtin statement is `expr_type_value`
            const expr_type_value& expr{exprs_[i]->as_expr_type_value()};
            expr.compile_assign(tc, os, indent, *fld.type_ptr, dst_op);
            i++;
            continue;
        }

        // built-in

        const expr_any& src{*exprs_[i]};

        const std::string dst_accessor{dst_op.str(fld.type_ptr->size())};

        if (src.is_expression() or (src.is_identifier() and tc.has_lea(src))) {
            // built-in, expression
            if (fld.is_array) {
                // built-in, expression, array
                const ident_info src_info{tc.make_ident_info(src)};
                validate_array_assignment(src.tok(), fld, src_info);
                tc.rep_movs(src.tok(), os, indent, src, src_info,
                            dst_op.address_str(), fld.size);
            } else {
                // built-in, expression, not array
                const ident_info dst_info{
                    tc.make_ident_info(src.tok(), dst_accessor)};
                src.compile(tc, os, indent, dst_info);
            }
        } else {
            // built-in, not expression
            const ident_info src_info{tc.make_ident_info(src)};
            if (src_info.is_const()) {
                // built-in, not expression, constant
                tc.asm_cmd(src.tok(), os, indent, "mov", dst_accessor,
                           std::format("{}{}", src.get_unary_ops().to_string(),
                                       src_info.const_value));
            } else {
                // built-in, not expression, not constant
                if (fld.is_array) {
                    // built-in, not expression, not constant, array
                    validate_array_assignment(src.tok(), fld, src_info);
                    tc.rep_movs(src.tok(), os, indent,
                                src_info.operand.address_str(),
                                dst_op.address_str(), fld.size);
                } else {
                    // built-in, not expression, not constant, not array
                    tc.asm_cmd(src.tok(), os, indent, "mov", dst_accessor,
                               src_info.operand.str());
                    src.get_unary_ops().compile(tc, os, indent, dst_accessor);
                }
            }
        }
        dst_op.displacement += static_cast<int32_t>(fld.size);
        i++;
    }
}

auto expr_type_value::validate_array_assignment(const token& tok,
                                                const type_field& fld,
                                                const ident_info& src_info)
    -> void {

    if (not src_info.is_array) {
        throw compiler_exception{tok, "source is not an array"};
    }
    if (fld.type_ptr->name() != src_info.type_ptr->name()) {
        throw compiler_exception{
            tok, std::format("destination type '{}' does not match "
                             "source type '{}'",
                             fld.type_ptr->name(), src_info.type_ptr->name())};
    }
    if (fld.array_size != src_info.array_size) {
        throw compiler_exception{
            tok, std::format("destination array size ({}) does not "
                             "match source array size ({})",
                             fld.array_size, src_info.array_size)};
    }
}

// declared in 'expr_type_value.hpp'
// resolves circular reference: expr_type_value -> expr_any ->
// expr_type_values
auto expr_type_value::assert_var_not_used(const std::string_view var) const
    -> void {

    for (const std::unique_ptr<expr_any>& e : exprs_) {
        e->assert_var_not_used(var);
    }
}

[[nodiscard]] auto expr_type_value::compile_lea(
    const token& src_loc_tk, toc& tc, std::ostream& os, size_t indent,
    std::vector<std::string>& allocated_registers, const std::string& reg_size,
    const std::span<const std::string> lea_path) const -> operand {

    return stmt_ident_->compile_lea(src_loc_tk, tc, os, indent,
                                    allocated_registers, reg_size, lea_path);
}

[[nodiscard]] auto expr_type_value::identifier() const -> std::string_view {
    if (stmt_ident_) {
        return stmt_ident_->identifier();
    }
    return statement::identifier();
}

[[nodiscard]] auto expr_type_value::is_indexed() const -> bool {
    return stmt_ident_ and stmt_ident_->is_indexed();
}

// declared in 'unary_ops.hpp'
// solves circular reference: unary_ops -> toc -> statement -> unary_ops
inline auto unary_ops::compile([[maybe_unused]] toc& tc, std::ostream& os,
                               const size_t indnt,
                               const std::string_view dst_info) const -> void {

    for (const char op : ops_ | std::views::reverse) {
        switch (op) {
        case '~':
            toc::asm_not(os, indnt, dst_info);
            break;
        case '-':
            toc::asm_neg(os, indnt, dst_info);
            break;
        default:
            std::unreachable();
        }
    }
}

namespace {

//
// jump optimizer, pass 1
//
//  example:
//    jmp cmp_13_26
//    cmp_13_26:
//  to
//    cmp_13_26:
//
//  example:
//    jne bool_end_15_9
//    jmp bool_end_15_9
//    bool_end_15_9:
//  to
//    bool_end_15_9:
//
auto optimize_jumps_1(std::istream& is, std::ostream& os) -> void {
    const std::regex rx_jcc{R"(^\s*j[a-z]{1,2}\s+(.+)\s*$)"};
    const std::regex rx_lbl{R"(^\s*([a-zA-Z_][a-zA-Z0-9_]*):\s*$)"};
    std::smatch match;
    size_t opts_count{};

    struct elem {
        enum class type : uint8_t { JCC, LABEL };
        std::string line;
        std::string label;
        type type{};
    };

    std::vector<elem> buffer;
    auto flush_buffer = [&]() -> void {
        for (const elem& e : buffer) {
            std::println(os, "{}", e.line);
        }
        buffer.clear();
    };

    std::string line;
    while (getline(is, line)) {
        if (std::regex_search(line, match, rx_jcc)) {
            // matched a jcc (conditional or unconditional jump)
            if (buffer.empty() or (buffer.back().type == elem::type::JCC and
                                   buffer.back().label == match[1])) {
                // buffer jccs to same label
                buffer.emplace_back(line, match[1], elem::type::JCC);
            } else {
                // jcc to a different label than the buffer, flush buffer
                flush_buffer();
                // add it to the buffer to start a new sequence
                buffer.emplace_back(line, match[1], elem::type::JCC);
            }
        } else if (std::regex_search(line, match, rx_lbl)) {
            // matched a label
            if (not buffer.empty() and buffer.back().type == elem::type::JCC and
                buffer.back().label == match[1]) {
                // label after jccs to this label, remove the jccs, print
                // the label
                opts_count += buffer.size();
                buffer.clear();
                std::println(os, "{}", line);
            } else {
                // label that differs from the buffered jccs, flush buffer and
                // print the label
                flush_buffer();
                std::println(os, "{}", line);
            }
        } else {
            // matched something else, flush buffer, print it
            flush_buffer();
            std::println(os, "{}", line);
        }
    }
    std::println(os, ";          optimization pass 1: {}", opts_count);
}

//
// jump optimizer, pass 2
//
// example:
//   jne cmp_14_26
//   jmp if_14_8_code
//   cmp_14_26:
// to
//   je if_14_8_code
//   cmp_14_26:
//
auto optimize_jumps_2(std::istream& is, std::ostream& os) -> void {
    const std::regex rx_jmp{R"(^\s*jmp\s+(.+)\s*$)"};
    const std::regex rx_jcc{R"(^\s*(j[a-z][a-z]?)\s+(.+)\s*$)"};
    const std::regex rx_lbl{R"(^\s*(.+):.*$)"};
    std::smatch match;

    size_t optimizations{};

    std::string line1;
    while (getline(is, line1)) {
        if (not std::regex_search(line1, match, rx_jcc)) {
            std::println(os, "{}", line1);
            continue;
        }

        const std::string jcc{match[1]};
        const std::string jcc_lbl{match[2]};

        std::string line2;
        if (not getline(is, line2)) {
            std::println(os, "{}", line1);
            return;
        }

        if (not std::regex_search(line2, match, rx_jmp)) {
            std::println(os, "{}", line1);
            std::println(os, "{}", line2);
            continue;
        }

        const std::string jmp_lbl{match[1]};

        std::string line3;
        if (not getline(is, line3)) {
            std::println(os, "{}", line1);
            std::println(os, "{}", line2);
            return;
        }

        if (not std::regex_search(line3, match, rx_lbl)) {
            std::println(os, "{}", line1);
            std::println(os, "{}", line2);
            std::println(os, "{}", line3);
            continue;
        }

        const std::string lbl{match[1]};

        if (jcc_lbl != lbl) {
            std::println(os, "{}", line1);
            std::println(os, "{}", line2);
            std::println(os, "{}", line3);
            continue;
        }

        //   jne cmp_14_26
        //   jmp if_14_8_code
        //   cmp_14_26:
        std::string jxx_inv;
        if (jcc == "jne") {
            jxx_inv = "je";
        } else if (jcc == "je") {
            jxx_inv = "jne";
        } else if (jcc == "jg") {
            jxx_inv = "jle";
        } else if (jcc == "jge") {
            jxx_inv = "jl";
        } else if (jcc == "jl") {
            jxx_inv = "jge";
        } else if (jcc == "jle") {
            jxx_inv = "jg";
        } else {
            std::println(os, "{}", line1);
            std::println(os, "{}", line2);
            std::println(os, "{}", line3);
            continue;
        }
        //   je if_14_8_code
        //   cmp_14_26:

        // get the whitespaces
        const std::string ws_before{
            line1.substr(0, line1.find_first_not_of(" \t\n\r\f\v"))};

        std::println(os, "{}{} {}", ws_before, jxx_inv, jmp_lbl);
        std::println(os, "{}", line3);
        optimizations++;
    }

    std::println(os, ";          optimization pass 2: {}", optimizations);
}

[[nodiscard]] auto read_file_to_string(const char* file_name) -> std::string {
    std::ifstream fs{file_name};
    if (not fs.is_open()) {
        throw panic_exception(std::format("cannot open file '{}'", file_name));
    }
    return std::string{std::istreambuf_iterator<char>{fs},
                       std::istreambuf_iterator<char>{}};
}

} // end of anonymous namespace
