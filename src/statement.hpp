#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <format>
#include <functional>
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

#include "compile_error.hpp"
#include "constant_parser.hpp"
#include "ident_info.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"
#include "ub_assigned.hpp"
#include "ub_reads.hpp"
#include "unary_ops.hpp"

// the node interface, the types below in reading order:
//
//   'one_line_source'  the source of a node on one line, for the comments of
//                      the listing
//   'statement'        the base of every node

// the source text of a statement on one line, for the comments of the output
class one_line_source final {
  public:
    //
    // statics
    //

    // one line for an assembler comment: whitespace and source comments
    // become single spaces, string and character literals are kept as
    // written; a '#' inside a literal does not start a comment and a quote
    // inside a comment does not start a literal
    //   'a  =  b # note'  =>  'a = b'
    [[nodiscard]] static auto of(const std::string_view text) -> std::string {
        std::string collapsed;
        collapsed.reserve(text.size());

        bool pending_space{};
        size_t index{};

        while (index < text.size()) {
            const char ch{text.at(index)};

            if (ch == '#') {
                pending_space = true;
                index = comment_end(text, index);
                continue;
            }

            if (is_ascii_space(ch)) {
                pending_space = true;
                ++index;
                continue;
            }

            // leading and trailing whitespace is dropped
            if (pending_space and not collapsed.empty()) {
                collapsed.push_back(' ');
            }

            pending_space = false;

            if (ch == '"' or ch == '\'') {
                index = append_literal(text, index, collapsed);
                continue;
            }

            collapsed.push_back(ch);
            ++index;
        }

        return collapsed;
    }

  private:
    //
    // statics
    //

    // appends the literal starting at 'begin' and returns where it ends,
    // after its closing quote, which an escaped character cannot be; a
    // backslash before a line end continues the literal on the next line, the
    // comment stays on one
    // note: the tokenizer rejects a carriage return that is not in a crlf
    //       line end, so dropping it makes a crlf continuation look like a lf
    //       one
    [[nodiscard]] static auto append_literal(const std::string_view text,
                                             const size_t begin,
                                             std::string& collapsed) -> size_t {

        const char quote{text.at(begin)};
        bool escaped{};

        collapsed.push_back(quote);

        for (size_t index{begin + 1}; index < text.size(); ++index) {
            // note: +1 because the opening quote cannot close the literal
            const char ch{text.at(index)};

            if (ch == '\r') {
                continue;
            }

            if (escaped and ch == '\n') {
                collapsed.pop_back();
                escaped = false;
                continue;
            }

            collapsed.push_back(ch);

            if (not escaped and ch == quote) {
                return index + 1;
                // note: +1 because the closing quote belongs to the literal
            }

            escaped = not escaped and ch == '\\';
        }

        // note: the tokenizer rejected an unterminated literal
        std::unreachable();
    }

    // after the line end of the comment starting at 'begin'
    [[nodiscard]] static auto comment_end(const std::string_view text,
                                          const size_t begin) -> size_t {

        const size_t newline{text.find('\n', begin)};

        // note: a comment on the last line of a file has no line end, e.g.
        //       the '# c' of a last line 'var g = 1 # c'

        if (newline == std::string_view::npos) {
            return text.size();
        }

        return newline + 1;
        // note: +1 because the line end belongs to the comment
    }

    [[nodiscard]] static auto is_ascii_space(const char ch) -> bool {
        return ch == ' ' or ch == '\t' or ch == '\n' or ch == '\r';
    }
};

class stmt_identifier;

// a node of the program: it parses itself in its constructor, writes its
// source back ('source_to') and compiles; a value is a node too, e.g.
// 'stmt_identifier', 'stmt_call' and the built-ins, the 'stmt_' prefix names a
// node, not a statement
//
// the whitespace before a value is held by a wrapper ('expr_any',
// 'expr_bool', 'expr_bool_op', a branch of 'stmt_if') in a whitespace-only
// token, while a list ('expr_arith') is a position and its first element
// holds it; a parse step named 'parse_' makes a node or a part that holds one,
// e.g. 'parse_condition', one named 'read_' takes tokens only, e.g.
// 'read_open_paren'
//
// the virtuals in groups, with the classes that override them, the default
// is 'std::unreachable()' where no other class reaches it:
//
//   every node     'compile', 'source_to', 'tok', 'trace_assignment'
//                  (control flow); 'visit_operands': overridden by every
//                  value and by every node whose 'trace_assignment' checks
//                  its reads with 'assert_result_not_read', a leaf lists none;
//                  'visit_reads' and 'visit_call_effects' walk the
//                  operands, 'stmt_identifier' adds its own read,
//                  'stmt_call' what its callee reads and writes,
//                  'stmt_builtin_io' the buffer it fills
//   a value        'fold': 'expr_zero', 'expr_bool', 'stmt_identifier',
//                  'stmt_builtin_array_length' and 'stmt_builtin_convert',
//                  forwarded by 'expr_arith' and 'expr_any';
//                  'assert_not_narrowed': the default checks the own type,
//                  'stmt_identifier' overrides it, forwarded by 'expr_arith'
//                  and 'expr_any'; 'keeps_low_bits': 'stmt_identifier' and
//                  'stmt_builtin_convert', forwarded by 'expr_arith'
//   a place        'operand_path': 'stmt_identifier' and 'expr_path',
//                  forwarded by 'expr_arith', 'expr_any' and 'expr_type'
//                  when they hold one; 'bare_path' and the free function
//                  'as_place' ('stmt_identifier.hpp') derive from it, the
//                  path is then asked through the members of
//                  'stmt_identifier'
class statement {
    token token_;
    unary_ops uops_;
    const type* type_{};

  public:
    explicit statement(const token& tk, unary_ops uops = {})
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
            get_type().check_width_bytes() <= dst_type.check_width_bytes()) {

            return;
        }

        throw_narrowed(tok(), trimmed_source(*this), get_type(), dst_type);
    }

    // emits the code of the node through 'tc', a value written to 'dst'; may
    // run more than once, what it adds to 'toc' is the same each time or put
    // back, see the class comment of 'toc'
    virtual auto compile([[maybe_unused]] toc& tc,
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

    // true when computing the value at another width keeps its low bits
    [[nodiscard]] virtual auto keeps_low_bits() const -> bool { return false; }

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

    // the operands of this node in evaluation order, see 'node_operand'
    virtual auto
    visit_operands([[maybe_unused]] const operand_visitor visitor) const
        -> void {

        std::unreachable();
    }

    // reports every read in this statement: the reads of the operands, of an
    // 'index_only_place' only its indexes ('stmt_identifier.hpp')
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

    // the source as a listing comment, not rendered in discarded code
    auto comment_source(toc& tc) const -> void {
        if (tc.is_dry_run()) {
            return;
        }

        machine& x{tc.machine()};

        x.comment({tok(), tc.indent()}, trimmed_source(*this));
    }

    // the compile of a value that must fit 'dst', checked like 'fold_checked'
    auto compile_checked(toc& tc, const destination& dst) const -> void {
        assert_not_narrowed(tc, dst.type_ref());
        compile(tc, dst);
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
            throw_not_generic(type_tk, "type", type_tk.text());
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
    // parses, e.g. the '"i8"' of 'a "i8"' or of 'res "i8" {', or the '7' of
    // 'a 7', which is not a type that is missing
    static auto assert_not_literal_type(const token& type_tk) -> void {
        if (type_tk.is_string()) {
            throw compile_error{type_tk, "expected a type name, got a string"};
        }

        if (constant_parser::is_literal(type_tk)) {
            throw compile_error{
                type_tk,
                std::format("expected a type name, got the constant {}",
                            in_quotes(type_tk.text()))};
        }
    }

    // 'compile_value' computes a value into a scratch register of
    // 'scratch_type', which is then copied into 'dst', so 'dst' keeps its
    // value until the whole value is computed, or the copy widens it
    static auto compile_through_scratch(
        toc& tc, const token& src_loc_tk, const operand& dst,
        const type& scratch_type,
        const std::function_ref<void(const operand&)> compile_value) -> void {

        machine& x{tc.machine()};

        const operand reg{
            x.alloc_scratch_register({src_loc_tk, tc.indent()}, scratch_type),
        };

        compile_value(reg);

        x.copy_value({src_loc_tk, tc.indent()}, dst, reg);
        x.free_scratch_register({src_loc_tk, tc.indent()}, reg);
    }

    // the type after the name of a field or a parameter, none for the default
    // type; a keyword names no type, e.g. the 'func' of a definition after a
    // '(' or '{' that is not closed
    [[nodiscard]] static auto read_declared_type(const toc& tc, tokenizer& tz)
        -> std::optional<token> {

        const token type_tk{tz.next_token()};

        if (type_tk.is_none() or
            std::ranges::contains(reserved_names::keywords, type_tk.text())) {

            tz.put_back_token(type_tk);
            return std::nullopt;
        }

        assert_not_literal_type(type_tk);
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

    // the source of 'st' on one line ('one_line_source::of'), for a listing
    // comment or a message
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
            return std::format(", {} is the index of 'foo'", in_quotes(root));
        }

        assert(info.read_only_why() == read_only_cause::foo_length);

        return std::format(", {} is the length of the array of 'foo'",
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
