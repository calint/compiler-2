#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "access_path.hpp"
#include "compile_error.hpp"
#include "decouple.hpp"
#include "expr_arith.hpp"
#include "ident_info.hpp"
#include "operand.hpp"
#include "parse_lookahead.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"
#include "ub_reads.hpp"

//
// a value of a user type or an array and how it is written in parts, the
// types below in reading order:
//
//   'literal_typing'         whether a literal names its type
//   'value_kind'             an instance or an array, named in the messages
//   'throw_known_type', 'assert_names_type', 'assert_value_ends',
//   'throw_known_literal_type', 'assert_not_array_literal'
//                            the messages of a value its destination does
//                            not take
//   'full_list', 'parse_braced_items'
//                            the items of a list in braces
//   'part_step', 'part_location'
//                            a part of a destination, a field or an element
//   'value_write', 'bytes_write', 'zero_fill', 'write_comment', 'write_step',
//   'write_plan', 'emit_write_plan'
//                            the steps that write a value in parts, read by
//                            the compile and by the aliasing check
//   'expr_path'              a path taken whole, copied as bytes
//   'expr_type'              a literal, a type name alone, a copy or a call
//

// whether a literal names its type, e.g. 'point{1, 2}': only where the name
// decides the type, elsewhere the destination gives it, e.g. '{1, 2}'
enum class literal_typing : uint8_t { written, from_destination };

// a value that is no number, named in the messages of 'assert_value_ends'
enum class value_kind : uint8_t { instance, array };

// a type written where the destination gives it, 'value' is how to write the
// value without it, e.g. '{...}' or the zero value '{}'
[[noreturn]] inline auto throw_known_type(const token& type_tk,
                                          const std::string_view type_name,
                                          const std::string_view value)
    -> void {

    throw compile_error{
        type_tk, std::format("the destination gives the type {}, write {}",
                             in_quotes(type_name), in_quotes(value))};
}

// the type named at 'type_tk' is 'tp', the type of the destination
// note: compared by type, a type parameter names the type of its argument
inline auto assert_names_type(const toc& tc, const token& type_tk,
                              const type& tp) -> void {

    const type& named_type{tc.get_type_or_throw(type_tk)};

    if (named_type.is_same(tp)) {
        return;
    }

    throw_type_mismatch(type_tk, tp.name(), named_type.name());
}

// 'kind' of 'tp' written at 'value_tk' is not continued nor reached into,
// e.g. the '+' of 'p + 1' or of 'i8[2]{1, 2} + 1' and the '.x' of
// 'point{1, 2}.x'; an equality gets the message of a number destination, it
// makes a condition
inline auto assert_value_ends(const tokenizer& tz, const token& value_tk,
                              const value_kind kind, const type& tp) -> void {

    const bool is_array{kind == value_kind::array};

    if (is_reached_into(tz)) {
        throw_literal_not_operand(
            value_tk, is_array ? std::string{"an array literal"}
                               : std::format("an instance literal of {}",
                                             in_quotes(tp.name())));
    }

    if (not continues_expression(tz)) {
        return;
    }

    const std::optional<comparison_operator> op{
        comparison_of(tz.peek_operator()),
    };

    // note: instances and arrays compare only by '==' and '!='
    if (op and is_equality(*op)) {
        expr_arith::assert_no_condition_next(tz);
    }

    throw compile_error{value_tk,
                        std::format("{} of {} is not a number",
                                    is_array ? "an array" : "an instance",
                                    in_quotes(tp.name()))};
}

// a literal that names the type its destination gives, at 'type_tk' with
// 'tz' after the type; an operator after the literal is reported instead, as
// after '{...}', e.g. the '+' of 'p = point{1, 2} + 1'
[[noreturn]] inline auto
throw_known_literal_type(const tokenizer& tz, const token& type_tk,
                         const value_kind kind, const type& tp,
                         const std::string_view type_name) -> void {

    tokenizer after_literal{tz};

    const bool has_values{after_literal.peek_char_after_whitespace() == '{'};

    // note: a literal that does not end is reported by its type
    if (not has_values or after_literal.skip_braced_block()) {
        assert_value_ends(after_literal, type_tk, kind, tp);
    }

    const bool is_zero{not has_values or is_zero_value(tz)};

    throw_known_type(type_tk, type_name, is_zero ? "{}" : "{...}");
}

// an array literal at 'tk' of kind 'tm' where a single value of 'tp' is
// expected, also one reached into, e.g. 'p = [2]{1, 2}' or
// 'p = point[2]{}[0]'; a condition may compare one, which the comparison
// rejects as an operand
inline auto assert_not_array_literal(const toc& tc, const tokenizer& tz,
                                     const token& tk, const term_kind tm,
                                     const type& tp) -> void {

    const bool is_array_literal{
        tm == term_kind::array_literal or
            (tk.is_none() and is_default_array_literal(tz)),
    };

    if (not is_array_literal) {
        return;
    }

    if (is_literal_reached_into(tz)) {
        throw_literal_not_operand(tk, "an array literal");
    }

    if (tp.is_bool() and not is_literal_value(tz)) {
        return;
    }

    // e.g. the 'i8' of 'p = i8[2]{1, 2}', none for the default type of
    // 'p = [2]{1, 2}'
    const type& element_type{
        tk.is_none() ? tc.builtins().default_type() : tc.get_type_or_throw(tk),
    };

    throw_type_mismatch(tk, tp.name(),
                        std::format("{}[]", element_type.name()));
}

// a list in braces that holds at most 'capacity' items and the messages when
// it is full: a value after a trailing ',' is 'too_many', anything else
// 'unclosed', e.g. the '3' of '{1, 2, 3}' or the ')' of '{1, 2)'
struct full_list {
    size_t capacity{};
    std::string too_many;
    std::string unclosed;
};

// the items of a list in braces after its '{' and the delimiters between
// them, appended to 'delims_tk', up to the '}' it returns; 'full' is none
// when the length is not known; 'parse_item' parses the item at an index
// and 'delimiter_error' is the message of a missing ',' before it; a
// trailing ',' lets each item end its line the same way
[[nodiscard]] inline auto
parse_braced_items(tokenizer& tz, std::vector<token>& delims_tk,
                   const std::optional<full_list>& full,
                   const std::function_ref<std::string(size_t)> delimiter_error,
                   const std::function_ref<void(size_t)> parse_item) -> token {

    for (size_t index{}; not full or index < full->capacity; ++index) {
        // an item after the first follows a ',', a '}' ends the list without
        // one
        const bool has_delimiter{
            index > 0 and tz.peek_char_after_whitespace() != '}',
        };

        if (has_delimiter) {
            delims_tk.emplace_back(
                tz.expect_char_token(',', delimiter_error(index)));
        }

        if (const std::optional<token> close_brace_tk{
                tz.is_next_char_token('}'),
            }) {

            return *close_brace_tk;
        }

        parse_item(index);
    }

    // note: only a full list leaves the loop
    assert(full);

    // a full list ends with its '}', after a trailing ',' too
    const std::optional<token> delimiter_tk{tz.is_next_char_token(',')};

    if (delimiter_tk) {
        delims_tk.emplace_back(*delimiter_tk);
    }

    if (const std::optional<token> close_brace_tk{
            tz.is_next_char_token('}'),
        }) {

        return *close_brace_tk;
    }

    // the error is at the value that is too much
    if (delimiter_tk and starts_value(tz)) {
        throw compile_error{tz, full->too_many};
    }

    throw compile_error{tz, full->unclosed};
}

// a step from a destination to a part of it: the field 'field', or the
// element 'element_index' when 'field' is null
struct part_step {
    const type_field* field{};
    size_t element_index{};
};

// where a part of a value is in its destination, e.g. the field 'a' and the
// element 3 for 'q.a[3]' of 'q'
class part_location final {
    std::vector<part_step> steps_;

  public:
    [[nodiscard]] auto element(const size_t index) const -> part_location {
        part_location result{*this};

        result.steps_.push_back({
            .field{},
            .element_index{index},
        });

        return result;
    }

    // the path of the elements from 'first' to before 'end' of the array
    // here, e.g. the elements a zero fill writes
    [[nodiscard]] auto elements_path(const size_t first, const size_t end) const
        -> access_path {

        access_path result{path()};
        result.add_index_range(first, end);
        return result;
    }

    [[nodiscard]] auto field(const type_field& fld) const -> part_location {
        part_location result{*this};

        result.steps_.push_back({
            .field{&fld},
            .element_index{},
        });

        return result;
    }

    // the path from the destination, as the aliasing check names storage
    [[nodiscard]] auto path() const -> access_path {
        access_path result{};

        for (const part_step& step : steps_) {
            if (step.field != nullptr) {
                result.add_field(step.field->offset);
            } else {
                result.add_fixed_index(step.element_index);
            }
        }

        return result;
    }

    // the destination of the part inside 'dst'
    [[nodiscard]] auto within(const destination& dst) const -> destination {
        destination result{dst};

        for (const part_step& step : steps_) {
            if (step.field != nullptr) {
                result = result.field(*step.field);
            } else {
                result = result.element(step.element_index);
            }
        }

        return result;
    }
};

// 'item' compiled at the part writes all of it, e.g. a scalar, a call or a
// copy, never a literal with parts of its own
struct value_write {
    const statement* item{};
};

// constant bytes stored at the part, e.g. the elements of '{1, 2}' or the
// characters of a string
struct bytes_write {
    token src_loc_tk;
    std::string bytes;
    // the label of the read-only constant, "str" or "init"
    std::string_view constant_kind;
};

// bytes that no listed value writes are zeroed, e.g. padding, unlisted fields
// and elements, the rest of an array after a string
struct zero_fill {
    token src_loc_tk;
    std::string comment;
    // from the start of the part, at which the operand has 'value_type'
    size_t offset{};
    const type* value_type{};
    size_t size_bytes{};
    size_t alignment{};
};

// a comment line written before the steps it describes, e.g. "copy field
// 'x'"
struct write_comment {
    token src_loc_tk;
    std::string text;
};

// one step of writing a value in parts, see 'write_plan'
struct write_step {
    part_location at;
    std::variant<value_write, bytes_write, zero_fill, write_comment> action;
    // the paths from the destination that are written once the step is done
    std::vector<access_path> covered;
};

// the steps that write a value, in the order they are emitted, e.g. 'q =
// {{1}, x}' with 'q' of 'type st {a i8[4], b i8}': '1' at 'a[0]', zero
// 'a[1]' to 'a[3]', then 'x' at 'b'; the compile emits it and the aliasing
// check of an assignment reads it, so what is checked is what is emitted
using write_plan = std::vector<write_step>;

// writes the steps of 'plan' at 'dst'
inline auto emit_write_plan(toc& tc, const size_t indent,
                            const write_plan& plan, const destination& dst)
    -> void {

    machine& x{tc.machine()};

    for (const write_step& step : plan) {
        const destination part_dst{step.at.within(dst)};

        if (const value_write* const value{
                std::get_if<value_write>(&step.action),
            }) {

            value->item->compile_checked(tc, indent, part_dst);
            continue;
        }

        if (const bytes_write* const bytes{
                std::get_if<bytes_write>(&step.action),
            }) {

            x.store_bytes(bytes->src_loc_tk, indent, bytes->bytes,
                          part_dst.operand(), part_dst.type_ref().alignment(),
                          [&](const std::string_view laid_out) -> std::string {
                              return tc.add_read_only_constant(
                                  bytes->constant_kind, bytes->src_loc_tk,
                                  std::string{laid_out});
                          });

            continue;
        }

        if (const zero_fill* const zero{std::get_if<zero_fill>(&step.action)}) {
            x.comment(zero->src_loc_tk, indent, zero->comment);

            x.zero(zero->src_loc_tk, indent,
                   part_dst.operand_at(zero->offset, *zero->value_type),
                   zero->size_bytes, zero->alignment);

            continue;
        }

        const write_comment& comment{std::get<write_comment>(step.action)};

        x.comment(comment.src_loc_tk, indent, comment.text);
    }
}

// a path taken whole: the copy of an instance, e.g. the 'p' of 'q = p', the
// source of a whole array copy, e.g. the 'a' of 'var b = a', or a path a call
// reads as a place, a method receiver or the array of a built-in
// ('path_argument'); compiled, it copies the bytes of its value
class expr_path final : public statement {
    stmt_identifier path_;

  public:
    explicit expr_path(stmt_identifier path)
        : statement{path.tok()}, path_{std::move(path)} {

        set_type(path_.get_type());
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        path_.source_to(os);
    }

    // e.g. 'pt1 = pt2' or 'var b = a'
    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override {

        const type& dst_type{dst.type_ref()};

        const ident_info src_info{make_ident_info(tc, path_)};

        // note: the parse checked the type of the source
        assert(dst_type.is_same(src_info.type_ref()));

        machine& x{tc.machine()};

        machine::held_registers held{x, tok(), indent};

        const operand src_op{path_.get_lea_operand(tc, indent, src_info, held)};

        const size_t size_bytes{src_info.value_size_bytes(tok())};

        x.copy_block(tok(), indent, dst.operand(), src_op, size_bytes,
                     dst_type.alignment());
    }

    [[nodiscard]] auto operand_path() const -> const stmt_identifier* override {
        return &path_;
    }

    auto visit_operands(const operand_visitor visitor) const -> void override {
        visitor({
            .node{&path_},
            .index_only_place{},
        });
    }

    //
    // statics
    //

    // the source of a whole array copy, an array of 'element_type', e.g. the
    // 'a' of 'var b = a'
    [[nodiscard]] static auto parse_whole_array(toc& tc, tokenizer& tz,
                                                const type& element_type)
        -> expr_path {

        expr_path source{stmt_identifier::parse_whole_array(tc, tz)};

        const type& src_type{source.get_type()};

        if (not element_type.is_same(src_type)) {
            throw_type_mismatch(source.path_.last_token(), element_type.name(),
                                src_type.name());
        }

        return source;
    }
};

// note: members that use 'expr_any' or 'stmt_call' are
//       implemented in 'expr_type_impl.hpp', which includes their headers;
//       'expr_any.hpp' and 'stmt_call.hpp' include this one

// a value of a user type: a list of fields '{...}', the type name alone as its
// zero value, a copy of a name or a call
class expr_type final : public statement {
    // the fields of a literal, e.g. '{1, 2}' of 'point{1, 2}', none for the
    // type name alone
    struct field_list {
        token open_brace_tk;
        std::vector<std::unique_ptr<expr_any>> exprs;
        std::vector<token> delims_tk;
        token close_brace_tk;
    };

    // a literal, or the copy or the call that is the value, an 'expr_path'
    // or a 'stmt_call'
    std::variant<field_list, std::unique_ptr<statement>> value_;

  public:
    // parses 'expr_any' items and creates the 'stmt_call' or 'expr_path'; the
    // type name alone is its zero value, e.g. 'var p = point' is 'point{}'
    expr_type(toc& tc, tokenizer& tz, const type& tp,
              const literal_typing typing);

    // a 'unique_ptr' of a class that is incomplete here needs
    // the special members where 'expr_any' is complete

    expr_type(expr_type&&) noexcept;

    expr_type(const expr_type&) = delete;
    auto operator=(const expr_type&) -> expr_type& = delete;
    auto operator=(expr_type&&) -> expr_type& = delete;

    ~expr_type() override;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override;

    // a copy is of its source, e.g. the 'p' of 'q = p'
    [[nodiscard]] auto operand_path() const -> const stmt_identifier* override {
        const statement* const value{copy_or_call()};
        return value != nullptr ? value->operand_path() : nullptr;
    }

    // a copy or a call reads its source instead of the '{...}' items
    auto visit_operands(const operand_visitor visitor) const -> void override;

    //
    // class methods
    //

    // the values of the listed fields of a literal, e.g. '1' and '2' of
    // 'point{1, 2}'
    [[nodiscard]] auto field_values() const
        -> std::span<const std::unique_ptr<expr_any>> {

        return std::get<field_list>(value_).exprs;
    }

    // e.g. 'point{1, 2}' or 'point', not a copy of 'p' or of a call result
    [[nodiscard]] auto is_literal() const -> bool {
        return std::holds_alternative<field_list>(value_);
    }

    // appends the steps that write the value at 'at'
    auto plan_writes(const toc& tc, const part_location& at,
                     write_plan& plan) const -> void;

  private:
    // the copy or the call that is the value, null for a literal
    [[nodiscard]] auto copy_or_call() const -> const statement* {
        const std::unique_ptr<statement>* const value{
            std::get_if<std::unique_ptr<statement>>(&value_),
        };

        return value != nullptr ? value->get() : nullptr;
    }

    // creates the 'stmt_call' or 'expr_path'
    auto parse_copy_source(toc& tc, tokenizer& tz, const term_kind tm) -> void;

    // creates an 'expr_any' for each field
    auto parse_fields(toc& tc, tokenizer& tz) -> void;

    // padding is zeroed too so that instances compare equal byte by byte,
    // 'offset' bytes into the instance at 'at'; 'covered' are the unlisted
    // fields among the bytes, none for padding
    auto plan_unwritten(const part_location& at, const std::string_view what,
                        const size_t offset, const size_t size_bytes,
                        std::vector<access_path> covered,
                        write_plan& plan) const -> void {

        if (size_bytes == 0) {
            return;
        }

        plan.push_back({
            .at{at},
            .action{
                zero_fill{
                    .src_loc_tk{tok()},
                    .comment{std::format("zero {}: {} B", what, size_bytes)},
                    .offset{offset},
                    .value_type{&get_type()},
                    .size_bytes{size_bytes},
                    .alignment{
                        offset_alignment(offset, get_type().alignment()),
                    },
                },
            },
            .covered{std::move(covered)},
        });
    }

    //
    // statics
    //

    // a field value of 'parse_fields' names no field, e.g. the 'x:' of
    // 'point{x: 1, y: 2}'
    static auto assert_field_unnamed(const tokenizer& tz) -> void {
        tokenizer probe{tz};

        const token name_tk{probe.next_token()};

        if (not name_tk.is_word() or not probe.is_next_char_token(':')) {
            return;
        }

        throw compile_error{
            name_tk,
            "fields are given in order, without names, e.g. 'point{1, 2}'"};
    }
};
