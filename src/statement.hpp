#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
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
#include "machine.hpp"
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
        piece_kind kind;
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
    // the comment stays on one, a carriage return is dropped so that a crlf
    // continuation looks like a lf one
    [[nodiscard]] static auto one_line_literal(const std::string_view literal)
        -> std::string {

        std::string kept;
        bool escaped{};

        for (const char ch : literal) {
            if (ch == '\r') {
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

// where the range of a bulk operation is checked: a constant count with a
// constant start was checked at compile time, e.g. 'array_copy(a[1], b, 2)'
enum class range_check : uint8_t { run_time, compile_time };

class stmt_identifier;

// a node of the program: it parses itself in its constructor, writes its
// source back ('source_to') and compiles; the virtuals in groups, with the
// classes that override them, the default is 'std::unreachable()' where no
// other class reaches it
//
//   every node     'compile', 'source_to', 'tok', 'trace_assignment'
//                  (control flow), 'visit_reads' and 'visit_call_effects'
//                  (the nodes that hold expressions)
//   a value        'fold': 'expr_zero', 'expr_bool', 'stmt_identifier' and
//                  the built-ins with a value, forwarded by 'expr_arith' and
//                  'expr_any'; 'assert_not_narrowed': 'stmt_identifier',
//                  'stmt_call' and the built-ins, forwarded by 'expr_arith'
//                  and 'expr_any'; 'keeps_low_bits': 'stmt_identifier' and
//                  'stmt_builtin_convert', forwarded by 'expr_arith';
//                  'is_expression': 'expression', answered by what it holds
//                  in 'expr_arith'
//   a condition    'is_arrays_equal_call' and 'compile_arrays_equal':
//                  'stmt_builtin_arrays_equal', forwarded by 'expr_arith',
//                  so 'expr_bool' needs not include the built-in
//   a place        'operand_path': 'stmt_identifier', forwarded by
//                  'expr_arith', 'expr_any' and 'expr_type' when they hold
//                  one; 'place_path' and 'as_place' derive from it, the
//                  path is then asked through the members of
//                  'stmt_identifier'
class statement {
    token token_;
    unary_ops uops_;
    const type* type_{};

  public:
    explicit statement(const token tk, unary_ops uops = {})
        : token_{tk}, uops_{std::move(uops)} {}

    statement() = default;
    statement(const statement&) = default;
    statement(statement&&) = default;
    auto operator=(const statement&) -> statement& = default;
    auto operator=(statement&&) -> statement& = default;

    virtual ~statement() = default;

    //
    // virtual methods
    //

    // throws if the value would silently lose bits when stored as 'dst_type'
    virtual auto
    assert_not_narrowed([[maybe_unused]] const toc& tc,
                        [[maybe_unused]] const type& dst_type) const -> void {}

    virtual auto compile([[maybe_unused]] toc& tc,
                         [[maybe_unused]] const size_t indent,
                         [[maybe_unused]] const destination& dst) const
        -> void {

        std::unreachable();
    }

    // the result of a call to 'arrays_equal' stored in 'dst', negated when
    // 'inverted', e.g. the shorthand 'ok = not arrays_equal(a, b, 4)'
    virtual auto
    compile_arrays_equal([[maybe_unused]] toc& tc,
                         [[maybe_unused]] const size_t indent,
                         [[maybe_unused]] const operand& dst,
                         [[maybe_unused]] const bool inverted) const -> void {

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

    [[nodiscard]] virtual auto is_arrays_equal_call() const -> bool {
        return false;
    }

    // the value is computed, it is not a name or a constant read in place
    [[nodiscard]] virtual auto is_expression() const -> bool { return false; }

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

    // definite assignment: checks the reads of the traced variable and records
    // its assignments
    virtual auto trace_assignment(assignment_flow& flow) const -> void {
        assert_var_not_used(flow);
    }

    // reports what the bodies of the calls in this node read and write, in
    // the order they are called, see 'call_effect'; a node that holds
    // expressions forwards it
    virtual auto
    visit_call_effects([[maybe_unused]] const toc& tc,
                       [[maybe_unused]] const call_effect_visitor visitor) const
        -> void {}

    // reports every read of 'var' in this statement, of every variable when
    // none, a leaf is never named like a variable
    virtual auto visit_reads(const read_filter var,
                             [[maybe_unused]] const read_visitor reader) const
        -> void {

        assert(tok().text() != var);
    }

    //
    // class methods
    //

    // definite assignment: throws if the traced variable is read outside of
    // the assigned bytes
    auto assert_var_not_used(const assignment_flow& flow) const -> void {
        const std::optional<std::string_view> var{flow.traced_var()};

        if (not var) {
            return;
        }

        assert_var_not_used(*var, flow.assigned());
    }

    // definite assignment: throws if 'var' is read outside of the 'assigned'
    // bytes
    auto assert_var_not_used(const std::string_view var,
                             const assigned_bytes& assigned) const -> void {

        visit_reads(
            var,
            [&var, &assigned](
                const token& src_loc_tk,
                [[maybe_unused]] const std::string_view read_text,
                const byte_range& accessed_bytes,
                [[maybe_unused]] const access_path& accessed_path) -> void {
                if (assigned.covers(accessed_bytes)) {
                    return;
                }

                throw_uninitialized(src_loc_tk, var);
            });
    }

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

    // the path of names, fields and indexes this node is, the operand path
    // without unary ops, e.g. 'p.x' or 'a[i]', not '-x'; a constant or a
    // literal is a path too, 'as_place' tells storage
    // note: defined in 'stmt_identifier.hpp', where 'stmt_identifier' is
    //       complete
    [[nodiscard]] auto place_path() const -> const stmt_identifier*;

    [[nodiscard]] auto reads_var(const std::string_view var) const -> bool {
        bool is_read{};

        visit_reads(
            var,
            [&is_read]([[maybe_unused]] const token& src_loc_tk,
                       [[maybe_unused]] const std::string_view read_text,
                       [[maybe_unused]] const byte_range& accessed_bytes,
                       [[maybe_unused]] const access_path& accessed_path)
                -> void { is_read = true; });

        return is_read;
    }

    auto set_type(const type& tp) -> void { type_ = &tp; }

    // an identifier parsed as a possible method receiver gets its unary ops
    // after it turned out to be a value
    auto set_unary_ops(unary_ops uops) -> void { uops_ = std::move(uops); }

    //
    // statics
    //

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

    // one-line, whitespace-collapsed rendering of source text, suitable for
    // an assembler comment
    [[nodiscard]] static auto trimmed_source(const std::string_view text)
        -> std::string {

        return one_line_source::of(text);
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
    // for statements whose value has the statement's own type
    auto assert_own_type_not_narrowed(const type& dst_type) const -> void {
        if (not dst_type.is_builtin() or not get_type().is_builtin() or
            get_type().size_bytes() <= dst_type.size_bytes()) {

            return;
        }

        throw_narrowed(tok(), trimmed_source(*this), get_type(), dst_type);
    }

    //
    // statics
    //

    // e.g. 'box<i8>' as the type of a field or a parameter, an alias names
    // an instance, a type argument list never follows a type name there
    static auto assert_no_type_arguments(tokenizer& tz, const token& type_tk)
        -> void {

        if (type_tk.is_empty() or tz.peek_char_after_whitespace() != '<') {
            return;
        }

        throw compile_error{
            type_tk, std::format("'{}<...>' is not a type, name an instance "
                                 "first, e.g. 'type str = {}<...>'",
                                 type_tk.text(), type_tk.text())};
    }

    // read-only-ness is fixed where the name is declared, so one check at
    // parse suffices; 'action' completes "cannot ...", e.g. 'assign to'
    static auto assert_not_read_only(const token& src_loc_tk,
                                     const std::string_view action,
                                     const std::string_view name,
                                     const ident_info& info) -> void {

        if (not info.is_read_only()) {
            return;
        }

        throw compile_error{src_loc_tk,
                            std::format("cannot {} read-only {}{}", action,
                                        in_quotes(name), read_only_hint(info))};
    }

    // why the name cannot be written, e.g. ", 'x' is declared with 'let'"
    [[nodiscard]] static auto read_only_hint(const ident_info& info)
        -> std::string {

        const std::string_view root{info.root_id()};

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

    [[noreturn]] static auto throw_uninitialized(const token& src_loc_tk,
                                                 const std::string_view var)
        -> void {

        throw compile_error{
            src_loc_tk,
            std::format("use of uninitialized variable {}", in_quotes(var))};
    }
};
