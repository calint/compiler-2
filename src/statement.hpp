#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <optional>
#include <ostream>
#include <print>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compile_error.hpp"
#include "ident_info.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"
#include "ub_assigned.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

// the source text of a statement on one line, for the comments of the output
class one_line_source final {
    // a part of the source text as it reads, a source comment, or a string or
    // character literal
    enum class piece_kind : uint8_t { code, comment, literal };

    struct source_piece {
        piece_kind kind{};
        std::string_view text;
    };

  public:
    //
    // statics
    //

    // one line for an assembler comment: whitespace and source comments
    // become single spaces, string and character literals are kept as written
    //   'a  =  b # note'  =>  'a = b'
    [[nodiscard]] static auto of(const std::string_view text) -> std::string {
        std::string collapsed;
        collapsed.reserve(text.size());

        bool pending_space{};

        // leading and trailing whitespace is dropped
        const auto add_pending_space = [&] -> void {
            if (pending_space and not collapsed.empty()) {
                collapsed.push_back(' ');
            }

            pending_space = false;
        };

        for (const source_piece& piece : split_into_pieces(text)) {
            if (piece.kind == piece_kind::comment) {
                pending_space = true;
                continue;
            }

            if (piece.kind == piece_kind::literal) {
                add_pending_space();
                collapsed += one_line_literal(piece.text);
                continue;
            }

            for (const char ch : piece.text) {
                if (is_ascii_space(ch)) {
                    pending_space = true;
                    continue;
                }

                add_pending_space();
                collapsed.push_back(ch);
            }
        }

        return collapsed;
    }

  private:
    //
    // statics
    //

    // after the line end of the comment starting at 'begin'
    [[nodiscard]] static auto comment_end(const std::string_view text,
                                          const size_t begin) -> size_t {

        const size_t newline{text.find('\n', begin)};

        // note: a comment on the last line has no line end
        if (newline == std::string_view::npos) {
            return text.size();
        }

        return newline + 1;
        // note: +1 because the line end belongs to the comment
    }

    [[nodiscard]] static auto is_ascii_space(const char ch) -> bool {
        return ch == ' ' or ch == '\t' or ch == '\n' or ch == '\r';
    }

    // after the closing quote of the literal starting at 'begin', an escaped
    // character cannot close it
    [[nodiscard]] static auto literal_end(const std::string_view text,
                                          const size_t begin) -> size_t {

        const char quote{text.at(begin)};
        bool escaped{};

        for (size_t index{begin + 1}; index < text.size(); ++index) {
            // note: +1 because the opening quote cannot close the literal
            const char ch{text.at(index)};

            // dropped, so it does not end an escape
            if (ch == '\r') {
                continue;
            }

            if (not escaped and ch == quote) {
                return index + 1;
                // note: +1 because the closing quote belongs to the literal
            }

            escaped = not escaped and ch == '\\';
        }

        // note: the tokenizer rejected an unterminated literal
        std::unreachable();
    }

    // a backslash before a line end continues the literal on the next line,
    // the comment stays on one; the carriage return of a crlf line end is
    // dropped so that a crlf continuation looks like a lf one
    [[nodiscard]] static auto one_line_literal(const std::string_view literal)
        -> std::string {

        std::string kept;
        bool escaped{};

        for (size_t i{}; i < literal.size(); ++i) {
            const char ch{literal.at(i)};

            if (ch == '\r' and literal.substr(i + 1).starts_with('\n')) {
                continue;
            }

            if (escaped and ch == '\n') {
                kept.pop_back();
                escaped = false;
                continue;
            }

            kept.push_back(ch);
            escaped = not escaped and ch == '\\';
        }

        return kept;
    }

    // a '#' inside a string or character literal does not start a comment and
    // a quote inside a comment does not start a literal
    //   'a = "#" # it's'  =>  code 'a = ', literal '"#"', code ' ',
    //                         comment '# it's'
    [[nodiscard]] static auto split_into_pieces(const std::string_view text)
        -> std::vector<source_piece> {

        std::vector<source_piece> pieces;
        size_t code_begin{};
        size_t index{};

        while (index < text.size()) {
            const char ch{text.at(index)};

            if (ch != '#' and ch != '"' and ch != '\'') {
                ++index;
                continue;
            }

            if (index > code_begin) {
                pieces.push_back({
                    .kind{piece_kind::code},
                    .text{text.substr(code_begin, index - code_begin)},
                });
            }

            const bool is_comment{ch == '#'};

            const size_t end{
                is_comment ? comment_end(text, index)
                           : literal_end(text, index),
            };

            pieces.push_back({
                .kind{is_comment ? piece_kind::comment : piece_kind::literal},
                .text{text.substr(index, end - index)},
            });

            index = end;
            code_begin = end;
        }

        if (code_begin < text.size()) {
            pieces.push_back({
                .kind{piece_kind::code},
                .text{text.substr(code_begin)},
            });
        }

        return pieces;
    }
};

// the width an expression is computed at compared with its own
enum class width_change : uint8_t { narrowed, widened };

class stmt_identifier;

// a node of the program: it parses itself in its constructor, writes its
// source back ('source_to') and compiles; the whitespace before a value is
// held by a wrapper ('expr_any', 'expr_bool', 'expr_bool_op',
// 'stmt_if_branch') in a whitespace-only token, while a list ('expr_arith') is
// a position and its first element holds it; the virtuals in groups, with the
// classes that override them, the default is 'std::unreachable()' where no
// other class reaches it; a parse step named 'parse_' makes a node or a part
// that holds one, e.g. 'parse_condition', one named 'read_' takes tokens only,
// e.g. 'read_open_paren'
//
//   every node     'compile', 'source_to', 'tok', 'trace_assignment'
//                  (control flow); 'visit_operands' (the default is
//                  'std::unreachable()'): a value, and a node whose
//                  'trace_assignment' checks its reads with
//                  'assert_result_not_read', a leaf says it has none;
//                  'visit_reads', 'visit_call_effects' and
//                  'has_inlined_call' walk the operands, 'stmt_identifier'
//                  adds its own read, 'stmt_call' and 'stmt_builtin_io' what
//                  the call writes, 'stmt_call' its callee
//   a value        'fold': 'expr_zero', 'expr_bool', 'stmt_identifier',
//                  'stmt_builtin_array_length' and 'stmt_builtin_convert',
//                  forwarded by 'expr_arith' and 'expr_any';
//                  'assert_not_narrowed': the default checks the own type,
//                  'stmt_identifier' overrides it, forwarded by 'expr_arith'
//                  and 'expr_any'; 'keeps_low_bits': 'stmt_identifier' and
//                  'stmt_builtin_convert', forwarded by 'expr_arith'
//   a place        'operand_path': 'stmt_identifier', forwarded by
//                  'expr_arith', 'expr_any' and 'expr_type' when they hold
//                  one; 'bare_path' and 'as_place' derive from it, the
//                  path is then asked through the members of
//                  'stmt_identifier'
class statement {
    token token_;
    unary_ops uops_;
    const type* type_{};

  public:
    explicit statement(const token tk, unary_ops uops = {})
        : token_{tk}, uops_{std::move(uops)} {}

    statement(const statement&) = default;
    statement(statement&&) = default;
    auto operator=(const statement&) -> statement& = default;
    auto operator=(statement&&) -> statement& = default;

    virtual ~statement() = default;

    //
    // virtual methods
    //

    // throws if the value would silently lose bits when stored as 'dst_type';
    // by default the value has the statement's own type, a value without a
    // built-in type, e.g. an instance, is not narrowed
    virtual auto assert_not_narrowed([[maybe_unused]] const toc& tc,
                                     const type& dst_type) const -> void {

        if (not dst_type.is_builtin() or not get_type().is_builtin() or
            get_type().size_bytes() <= dst_type.size_bytes()) {

            return;
        }

        throw_narrowed(tok(), trimmed_source(*this), get_type(), dst_type);
    }

    // may run more than once, what it adds to 'toc' is the same each time or
    // put back, see the class comment of 'toc'
    virtual auto compile([[maybe_unused]] toc& tc,
                         [[maybe_unused]] const size_t indent,
                         [[maybe_unused]] const destination& dst) const
        -> void {

        std::unreachable();
    }

    // the value known at compile time where it is used, wrapped to its
    // width, empty when it is computed at run time; it does not check that
    // the value fits, a conversion computes its argument at a narrower width
    // on purpose
    [[nodiscard]] virtual auto fold([[maybe_unused]] const toc& tc,
                                    [[maybe_unused]] const value_use& use) const
        -> std::optional<folded_value> {

        return std::nullopt;
    }

    // a call of an inlined function in this node or its operands, whose body
    // every compile of the node compiles again ('stmt_identifier.hpp')
    [[nodiscard]] virtual auto has_inlined_call() const -> bool;

    // true when computing at the width that 'change' gives keeps the low bits
    [[nodiscard]] virtual auto
    keeps_low_bits([[maybe_unused]] const width_change change) const -> bool {

        return false;
    }

    // the identifier a value is when it is one name or literal, its unary ops
    // included, e.g. 'x', '-x', 'p.a[i]' or '3', not 'x + 1', 'f(x)' or '{}';
    // the nodes that hold one forward it
    [[nodiscard]] virtual auto operand_path() const -> const stmt_identifier* {
        return nullptr;
    }

    virtual auto source_to(std::ostream& os) const -> void {
        uops_.source_to(os);
        token_.source_to(os);
    }

    [[nodiscard]] virtual auto tok() const -> const token& { return token_; }

    // definite assignment: checks the reads of the result and records its
    // assignments
    virtual auto trace_assignment(assignment_flow& flow) const -> void {
        assert_result_not_read(flow);
    }

    // reports what the bodies of the calls in this node read and write, in
    // the order they are called, see 'call_effect'; the calls of the operands
    // ('stmt_identifier.hpp')
    virtual auto visit_call_effects(const toc& tc,
                                    const call_effect_visitor visitor) const
        -> void;

    // the operands of this node in evaluation order, see 'list_operand'; the
    // walks 'visit_reads' and 'visit_call_effects' read them; a value and a
    // statement whose 'trace_assignment' checks its reads say what they are,
    // a leaf that it has none
    virtual auto
    visit_operands([[maybe_unused]] const operand_visitor visitor) const
        -> void {

        std::unreachable();
    }

    // reports every read in this statement: the reads of the operands, of a
    // written place only its indexes ('stmt_identifier.hpp')
    virtual auto visit_reads(const read_visitor reader) const -> void;

    //
    // class methods
    //

    // definite assignment: throws if the result is read outside of the
    // bytes assigned so far ('stmt_identifier.hpp')
    auto assert_result_not_read(const assignment_flow& flow) const -> void;

    // the path of names, fields and indexes this node is, the operand path
    // without unary ops, e.g. 'p.x' or 'a[i]', not '-x'; a constant or a
    // literal is a path too, 'as_place' tells storage
    // note: defined in 'stmt_identifier.hpp', where 'stmt_identifier' is
    //       complete
    [[nodiscard]] auto bare_path() const -> const stmt_identifier*;

    // the compile of a value that must fit 'dst', checked like 'fold_checked'
    auto compile_checked(toc& tc, const size_t indent,
                         const destination& dst) const -> void {

        assert_not_narrowed(tc, dst.type_ref());
        compile(tc, indent, dst);
    }

    // the fold of a value that must fit where it is used, checked as its
    // compile is, so a folded value is rejected where a computed one is
    // note: the check follows the fold, which keeps the low bits of an
    //       operand that does not fit
    [[nodiscard]] auto fold_checked(const toc& tc, const value_use& use) const
        -> std::optional<folded_value> {

        std::optional<folded_value> folded{fold(tc, use)};

        if (folded) {
            assert_not_narrowed(tc, use.width_ref());
        }

        return folded;
    }

    [[nodiscard]] auto get_type() const -> const type& {
        assert(type_ != nullptr);

        return *type_;
    }

    [[nodiscard]] auto get_unary_ops() const -> const unary_ops& {
        return uops_;
    }

    // a value of a list that is computed, not one name or literal read in
    // place ('operand_path')
    [[nodiscard]] auto is_computed() const -> bool {
        return operand_path() == nullptr;
    }

    // a path rooted at the name 'var' is read ('stmt_identifier.hpp')
    [[nodiscard]] auto reads_name(std::string_view var) const -> bool;

    auto set_type(const type& tp) -> void { type_ = &tp; }

    // an identifier parsed as a possible method receiver gets its unary ops
    // after it turned out to be a value
    auto set_unary_ops(unary_ops uops) -> void { uops_ = std::move(uops); }

    //
    // statics
    //

    // e.g. 'box<i8>' as the type of a field, a parameter or a result: only a
    // generic type takes arguments, and there an alias names its instance
    static auto assert_no_type_arguments(const toc& tc, tokenizer& tz,
                                         const token& type_tk) -> void {

        if (tz.peek_char_after_whitespace() != '<') {
            return;
        }

        if (tc.types().has(type_tk.text())) {
            throw compile_error{type_tk,
                                std::format("{} is a type, not a generic type",
                                            in_quotes(type_tk.text()))};
        }

        // e.g. an unknown name or the type parameter 'T' of 'x T<3>'
        if (not tc.generics().has_type(type_tk.text())) {
            throw compile_error{
                type_tk,
                std::format("{} is not a generic type, it takes no type "
                            "arguments",
                            in_quotes(type_tk.text()))};
        }

        throw compile_error{
            type_tk,
            std::format("{} is not a type, {}",
                        in_quotes(std::format("{}<...>", type_tk.text())),
                        instance_hint(type_tk.text()))};
    }

    // where it is read, also in a generic definition that no instance
    // parses, e.g. the '"i8"' of 'a "i8"' or of 'res "i8" {'
    static auto assert_not_string_type(const token& type_tk) -> void {
        if (type_tk.is_string()) {
            throw compile_error{type_tk, "expected a type name, got a string"};
        }
    }

    // the type after the name of a field or a parameter, none for the default
    // type, e.g. the 'x' of '{x, y}' or 'a[4]'; a keyword names no type, e.g.
    // the 'func' of a definition after a '(' or '{' that is not closed
    [[nodiscard]] static auto read_declared_type(const toc& tc, tokenizer& tz)
        -> std::optional<token> {

        const token type_tk{tz.next_token()};

        if (type_tk.is_none() or
            std::ranges::contains(reserved_names::keywords, type_tk.text())) {

            tz.put_back_token(type_tk);
            return std::nullopt;
        }

        assert_not_string_type(type_tk);
        assert_no_type_arguments(tc, tz, type_tk);

        return type_tk;
    }

    // 'items' with the delimiter tokens between them, e.g. 'x, y' of
    // 'f(x, y)'; a delimiter after the last item is the caller's
    template <std::ranges::forward_range Items>
    static auto source_delimited_to(std::ostream& os, const Items& items,
                                    const std::span<const token> delims)
        -> void {

        if (std::ranges::empty(items)) {
            return;
        }

        items.front().source_to(os);

        for (const auto [d, e] :
             std::views::zip(delims, items | std::views::drop(1))) {

            d.source_to(os);
            e.source_to(os);
        }

        // a trailing delimiter, e.g. the last ',' of '{1, 2,}'
        if (std::ssize(delims) == std::ranges::distance(items)) {
            delims.back().source_to(os);
        }
    }

    // one-line, whitespace-collapsed rendering of 'st's source, suitable for
    // an assembler comment
    [[nodiscard]] static auto trimmed_source(const statement& st)
        -> std::string {

        std::stringstream ss;
        st.source_to(ss);
        return one_line_source::of(ss.view());
    }

    // same as above, with a "'dst' 'op' " prefix before the rendered source
    [[nodiscard]] static auto trimmed_source(const statement& st,
                                             const std::string_view dst,
                                             const std::string_view op)
        -> std::string {

        std::stringstream ss;
        std::print(ss, "{} {} ", dst, op);
        st.source_to(ss);
        return one_line_source::of(ss.view());
    }

  protected:
    // note: only the empty block of 'stmt_block' has no token
    statement() = default;

    //
    // statics
    //

    // why the name 'root' cannot be written, e.g. ", 'x' is declared with
    // 'let'"
    [[nodiscard]] static auto read_only_hint(const std::string_view root,
                                             const ident_info& info)
        -> std::string {

        if (info.read_only_why() == read_only_cause::let) {
            return std::format(", {} is declared with 'let'", in_quotes(root));
        }

        if (info.read_only_why() == read_only_cause::param) {
            if (root == reserved_names::self) {
                return ", the method is not declared 'mut'";
            }

            return std::format(", parameter {} is not declared 'mut'",
                               in_quotes(root));
        }

        if (info.read_only_why() == read_only_cause::foo_element) {
            return std::format(", {} is an element of a read-only array",
                               in_quotes(root));
        }

        if (info.read_only_why() == read_only_cause::foo_counter) {
            return std::format(", {} is the counter of 'foo'", in_quotes(root));
        }

        assert(info.read_only_why() == read_only_cause::foo_length);

        return std::format(", {} is the size of the array of 'foo'",
                           in_quotes(root));
    }

    [[noreturn]] static auto throw_narrowed(const token& src_loc_tk,
                                            const std::string_view source,
                                            const type& src_type,
                                            const type& dst_type) -> void {

        throw compile_error{
            src_loc_tk,
            std::format("{} of type {} is narrowed to {}, use '{}(...)'",
                        in_quotes(source), in_quotes(src_type.name()),
                        in_quotes(dst_type.name()), dst_type.name())};
    }
};
