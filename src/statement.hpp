#pragma once
// reviewed: 2025-09-28

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <optional>
#include <ostream>
#include <print>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "machine.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"
#include "ub_unset_var.hpp"
#include "unary_ops.hpp"

class toc;

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
        return ch == ' ' or ch == '\t' or ch == '\n' or ch == '\r' or
               ch == '\f' or ch == '\v';
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

    // the bytes of its root variable that an identifier path reaches, a path
    // with a run-time index reaches its whole array
    [[nodiscard]] virtual auto accessed_range() const
        -> std::optional<field_coverage::range> {

        std::unreachable();
    }

    // throws if the value would silently lose bits when stored as 'dst_type'
    virtual auto
    assert_not_narrowed([[maybe_unused]] const toc& tc,
                        [[maybe_unused]] const type& dst_type) const -> void {}

    virtual auto compile([[maybe_unused]] toc& tc,
                         [[maybe_unused]] const size_t indent,
                         [[maybe_unused]] const ident_info& dst_info) const
        -> void {

        std::unreachable();
    }

    // 'use' runs while the scratch registers that build the address are still
    // allocated, then they are freed
    virtual auto compile_address(
        [[maybe_unused]] toc& tc, [[maybe_unused]] const size_t indent,
        [[maybe_unused]] const token& src_loc_tk,
        [[maybe_unused]] const lea_request& request,
        [[maybe_unused]] const std::function_ref<void(const operand&)> use)
        const -> void {

        std::unreachable();
    }

    virtual auto compile_boolean([[maybe_unused]] toc& tc,
                                 [[maybe_unused]] const size_t indent,
                                 [[maybe_unused]] const operand& dst,
                                 [[maybe_unused]] const bool inverted) const
        -> void {

        std::unreachable();
    }

    virtual auto compile_data([[maybe_unused]] toc& tc) const -> void {
        std::unreachable();
    }

    [[nodiscard]] virtual auto
    compile_lea([[maybe_unused]] toc& tc, [[maybe_unused]] const size_t indent,
                [[maybe_unused]] const token& src_loc_tk,
                [[maybe_unused]] std::vector<operand>& allocated_registers,
                [[maybe_unused]] const lea_request& request) const -> operand {

        std::unreachable();
    }

    [[nodiscard]] virtual auto dat_size_bytes() const -> size_t {
        std::unreachable();
    }

    // the value computed in a register of 'width_type' when known at compile,
    // empty when computed at run time
    [[nodiscard]] virtual auto
    folded_constant([[maybe_unused]] const toc& tc,
                    [[maybe_unused]] const type& width_type) const
        -> std::optional<int64_t> {

        return std::nullopt;
    }

    [[nodiscard]] virtual auto get_unary_ops() const -> const unary_ops& {
        return uops_;
    }

    [[nodiscard]] virtual auto identifier() const -> std::string_view {
        return token_.text();
    }

    [[nodiscard]] virtual auto is_array_element() const -> bool {
        return false;
    }

    // lets callers identify 'expr_arith' without rtti
    [[nodiscard]] virtual auto is_expr_arith() const -> bool { return false; }

    [[nodiscard]] virtual auto is_expression() const -> bool { return false; }

    [[nodiscard]] virtual auto is_identifier() const -> bool { return false; }

    [[nodiscard]] virtual auto is_indexed() const -> bool { return false; }

    // true when computing at a narrower width gives the same low bits
    [[nodiscard]] virtual auto keeps_low_bits_when_narrowed() const -> bool {
        return false;
    }

    // where the name an error is about is, the last name of a path for an
    // identifier, e.g. 'b' of 'a.b'
    [[nodiscard]] virtual auto name_token() const -> const token& {
        return token_;
    }

    [[nodiscard]] virtual auto produces_boolean() const -> bool {
        return false;
    }

    virtual auto source_to(std::ostream& os) const -> void {
        uops_.source_to(os);
        token_.source_to(os);
    }

    [[nodiscard]] virtual auto tok() const -> const token& { return token_; }

    // used in UB check
    // checks the reads of 'flow.var' and records its assignments
    virtual auto trace_assignment(assignment_flow& flow) const -> void {
        assert_var_not_used(flow.var, flow.assigned);
    }

    // reports every read of 'var' in this statement, a leaf is never named
    // like a variable
    virtual auto visit_reads(const std::string_view var,
                             [[maybe_unused]] const read_visitor reader) const
        -> void {

        assert(identifier() != var);
    }

    //
    // class methods
    //

    // emits the address of this statement for a bulk operation, 'info' is what
    // 'toc' resolved it to
    // nothing is emitted here, the backend calls the returned emitter when it
    // needs the address: the emitter builds it with 'compile_address', which
    // passes it to the backend's 'use' while the scratch registers that built
    // it are still allocated and frees them afterwards. it refers to its
    // arguments, so it is only valid within the call that made it
    [[nodiscard]] auto address_emitter_of(toc& tc, const size_t indent,
                                          const token& src_loc_tk,
                                          const ident_info& info) const
        -> machine::address_emitter {

        return [this, &tc, indent, &src_loc_tk, &info](
                   const operand& reg_count, const operand& address_register,
                   const machine::address_use use) -> void {
            compile_address(tc, indent, src_loc_tk,
                            {
                                .reg_count{reg_count},
                                .lea_path{info.lea_path},
                                .address_register{address_register},
                            },
                            use);
        };
    }

    // used in UB check
    // throws if 'var' is read outside of the 'assigned' bytes
    auto assert_var_not_used(const std::string_view var,
                             const field_coverage& assigned) const -> void {

        visit_reads(
            var,
            [&var, &assigned](
                const token& src_loc_tk,
                [[maybe_unused]] const std::string_view read_text,
                const std::optional<field_coverage::range>& accessed) -> void {
                const bool is_assigned{
                    accessed ? assigned.covers(*accessed) : assigned.is_full(),
                };

                if (is_assigned) {
                    return;
                }

                throw_uninitialized(src_loc_tk, var);
            });
    }

    [[nodiscard]] auto get_type() const -> const type& {
        assert(type_ != nullptr);

        return *type_;
    }

    [[nodiscard]] auto make_constant_operand(const ident_info& info) const
        -> operand {

        assert(info.is_const());

        return operand::imm(
            std::format("{}{}", get_unary_ops().to_string(), info.const_value),
            info.type_ref());
    }

    [[nodiscard]] auto reads_var(const std::string_view var) const -> bool {
        bool is_read{};

        visit_reads(
            var,
            [&is_read](
                [[maybe_unused]] const token& src_loc_tk,
                [[maybe_unused]] const std::string_view read_text,
                [[maybe_unused]] const std::optional<field_coverage::range>&
                    accessed) -> void { is_read = true; });

        return is_read;
    }

    auto set_type(const type& tp) -> void { type_ = &tp; }

    // an identifier parsed as a possible method receiver gets its unary ops
    // after it turned out to be a value
    auto set_unary_ops(unary_ops uops) -> void { uops_ = std::move(uops); }

    //
    // statics
    //

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

    // writes are rejected at parse, so a function that is never called is
    // checked too; 'action' completes "cannot ...", e.g. 'assign to'
    static auto assert_not_read_only(const token& src_loc_tk,
                                     const std::string_view action,
                                     const std::string_view name,
                                     const ident_info& info) -> void {

        if (not info.is_read_only()) {
            return;
        }

        throw compiler_exception{
            src_loc_tk, std::format("cannot {} read-only '{}'{}", action, name,
                                    read_only_hint(info))};
    }

    [[nodiscard]] static auto fits_size_bytes(const int64_t value,
                                              const size_t size_bytes) -> bool {

        switch (size_bytes) {
        case sizeof(int8_t):
            return std::in_range<int8_t>(value);

        case sizeof(int16_t):
            return std::in_range<int16_t>(value);

        case sizeof(int32_t):
            return std::in_range<int32_t>(value);

        default:
            return true;
        }
    }

    // shared by 'dat' and 'var', 'keyword' names the declaration in the
    // example
    [[nodiscard]] static auto
    parse_initializer_equals(tokenizer& tz, const std::string_view keyword)
        -> token {

        const token equals_tk{
            tz.expect_char_token(
                '=', std::format("expected '=' followed by an initializer, "
                                 "e.g. '{} x = i32(0)'",
                                 keyword)),
        };

        return equals_tk;
    }

    // why the name cannot be written, e.g. ", 'x' is declared with 'let'"
    [[nodiscard]] static auto read_only_hint(const ident_info& info)
        -> std::string {

        const std::string_view root{info.root_id()};

        if (info.read_only_why == read_only_cause::let) {
            return std::format(", '{}' is declared with 'let'", root);
        }

        if (info.read_only_why == read_only_cause::param) {
            if (root == reserved_names::self) {
                return ", the method is not declared 'mut'";
            }

            return std::format(", parameter '{}' is not declared 'mut'", root);
        }

        if (info.read_only_why == read_only_cause::foo_element) {
            return std::format(", '{}' is an element of a read-only array",
                               root);
        }

        assert(info.read_only_why == read_only_cause::foo_counter);

        return std::format(", '{}' is the counter of 'foo'", root);
    }

    // shared by 'dat' and 'var' initializers, 'array_count' 0 takes the size
    // of the string
    [[nodiscard]] static auto
    string_array_count(const type& string_element_type, const token& string_tk,
                       const type& element_type, const size_t array_count)
        -> size_t {

        const size_t size_bytes{string_tk.string_size_bytes()};

        // arrays of zero length are not allowed
        if (size_bytes == 0 and array_count == 0) {
            throw compiler_exception{
                string_tk, "an empty string is not valid for an array with "
                           "unspecified size"};
        }

        if (array_count != 0 and size_bytes > array_count) {
            throw compiler_exception{
                string_tk, std::format("string size {} overflows array size {}",
                                       size_bytes, array_count)};
        }

        if (not element_type.is_same(string_element_type)) {
            throw compiler_exception{string_tk,
                                     "only arrays of type 'i8' can be "
                                     "initialized with strings"};
        }

        if (array_count == 0) {
            return size_bytes;
        }

        return array_count;
    }

    [[noreturn]] static auto throw_narrowed(const token& src_loc_tk,
                                            const std::string_view source,
                                            const type& src_type,
                                            const type& dst_type) -> void {

        throw compiler_exception{
            src_loc_tk,
            std::format("'{}' of type '{}' is narrowed to '{}', use '{}(...)'",
                        source, src_type.name(), dst_type.name(),
                        dst_type.name())};
    }

    [[noreturn]] static auto throw_uninitialized(const token& src_loc_tk,
                                                 const std::string_view var)
        -> void {

        throw compiler_exception{
            src_loc_tk, std::format("use of uninitialized variable '{}'", var)};
    }
};
