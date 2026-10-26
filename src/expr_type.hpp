#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
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
#include "ident_info.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "type.hpp"
#include "ub_reads.hpp"

//
// a value of a user type or an array and how it is written in parts, the
// types below in reading order:
//
//   'literal_typing'         whether a literal names its type
//   'part_step', 'part_location'
//                            a part of a destination, a field or an element
//   'value_write', 'bytes_write', 'zero_fill', 'write_comment', 'write_step',
//   'write_plan', 'emit_write_plan'
//                            the steps that write a value in parts, read by
//                            the compile and by the aliasing check
//   'expr_type'              a literal, a type name alone, a copy or a call
//

// whether a literal names its type, e.g. 'point{1, 2}': only where the name
// decides the type, elsewhere the destination gives it, e.g. '{1, 2}'
enum class literal_typing : uint8_t { written, from_destination };

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
        access_path result;

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
    // stored as a string constant, otherwise as a bytes constant
    bool is_string{};
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

            x.store_bytes(
                bytes->src_loc_tk, indent, bytes->bytes, part_dst.operand(),
                part_dst.type_ref().alignment(), [&] -> std::string {
                    if (bytes->is_string) {
                        return tc.add_string_constant(bytes->src_loc_tk);
                    }

                    return tc.add_bytes_constant(bytes->src_loc_tk,
                                                 bytes->bytes);
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

// note: members that use 'expr_any', 'stmt_identifier' or 'stmt_call' are
//       implemented in 'expr_type_impl.hpp', those headers include this one
//       through 'expr_any.hpp', so their definitions are incomplete here

// a value of a user type or an array: a list of fields or elements '{...}',
// the type name alone as its zero value, a copy of a name or a call; also
// the source of a whole array copy, which only a name can be,
// 'is_array_destination_' selects that role
class expr_type final : public statement {
    // the fields of a literal, e.g. '{1, 2}' of 'point{1, 2}', none for the
    // type name alone, e.g. 'point'
    struct field_list {
        token open_brace_tk;
        std::vector<std::unique_ptr<expr_any>> exprs;
        std::vector<token> delims_tk;
        token close_brace_tk;
    };

    std::variant<field_list, std::unique_ptr<stmt_identifier>,
                 std::unique_ptr<stmt_call>>
        value_;

    // only a whole array destination may copy a whole array source
    bool is_array_destination_{};

  public:
    // what the destination of the value holds, a whole array is filled only
    // by a copy of an array, e.g. 'var b = a'
    enum class destination_kind : uint8_t { instance, whole_array };

    // parses 'expr_any' items and creates the 'stmt_call' or
    // 'stmt_identifier'; the type name alone is its zero value, e.g.
    // 'var p = point' is 'point{}'
    expr_type(toc& tc, tokenizer& tz, const type& tp,
              const destination_kind kind, const literal_typing typing);

    // a method receiver that has already been parsed
    explicit expr_type(std::unique_ptr<stmt_identifier> receiver);

    // a 'unique_ptr' of a class that is incomplete here needs
    // the special members where 'stmt_identifier' and 'stmt_call' are complete

    expr_type(expr_type&&) noexcept;

    expr_type(const expr_type&) = delete;
    auto operator=(const expr_type&) -> expr_type& = delete;

    ~expr_type() override;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override;

    auto compile(toc& tc, const size_t indent, const destination& dst) const
        -> void override;

    [[nodiscard]] auto operand_path() const -> const stmt_identifier* override;

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

    auto operator=(expr_type&&) noexcept -> expr_type&;

    // appends the steps that write the value at 'at'
    auto plan_writes(const toc& tc, const part_location& at,
                     write_plan& plan) const -> void;

    //
    // statics
    //

    // unlisted elements are zero like unlisted fields, the first
    // 'listed_count' of the array at 'at' are written
    static auto
    plan_remaining_elements(const token& src_loc_tk, const part_location& at,
                            const type& element_type, const size_t listed_count,
                            const size_t array_count, write_plan& plan)
        -> void {

        const size_t remaining_count{array_count - listed_count};

        if (remaining_count == 0) {
            return;
        }

        const size_t size_bytes{
            multiply_storage_size(src_loc_tk, element_type.size_bytes(),
                                  remaining_count),
        };

        plan.push_back({
            .at{at},
            .action{
                zero_fill{
                    .src_loc_tk{src_loc_tk},
                    .comment{
                        std::format("zero remaining elements: {} * {} B = "
                                    "{} B",
                                    remaining_count, element_type.size_bytes(),
                                    size_bytes),
                    },
                    .offset{listed_count * element_type.size_bytes()},
                    .value_type{&element_type},
                    .size_bytes{size_bytes},
                    .alignment{element_type.alignment()},
                },
            },
            .covered{
                {at.elements_path(listed_count, array_count)},
            },
        });
    }

  private:
    auto assert_call_type(const type& tp) const -> void;

    auto assert_copy_source_shape(const toc& tc) const -> void;

    // the '}' that ends the fields, when it is the next character, e.g. the '}'
    // of '{x, y}'
    [[nodiscard]] auto at_closing_brace(tokenizer& tz) -> bool {
        const std::optional<token> close_brace_tk{tz.is_next_char_token('}')};

        if (not close_brace_tk) {
            return false;
        }

        std::get<field_list>(value_).close_brace_tk = *close_brace_tk;

        return true;
    }

    // the call whose result is the value, null for a literal or a copy
    [[nodiscard]] auto call_source() const -> const stmt_call* {
        const std::unique_ptr<stmt_call>* const call{
            std::get_if<std::unique_ptr<stmt_call>>(&value_),
        };

        return call != nullptr ? call->get() : nullptr;
    }

    auto compile_identifier_copy(toc& tc, const size_t indent,
                                 const destination& dst) const -> void;

    // creates the 'stmt_call' or 'stmt_identifier'
    auto parse_copy_source(toc& tc, tokenizer& tz, const type& tp,
                           const term_kind tm) -> void;

    // creates an 'expr_any' for each field
    auto parse_fields(toc& tc, tokenizer& tz, const type& tp) -> void;

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

    // the ',' before the value of the field 'next', e.g. the ',' of '{x, y}'
    // before the value of 'y'
    auto read_field_delimiter(tokenizer& tz, const type& tp,
                              const type_field& next) -> void {

        const token delimiter_tk{
            tz.expect_char_token(
                ',', std::format("expected ',' followed by a value for field "
                                 "{} in type {}",
                                 in_quotes(next.name), in_quotes(tp.name()))),
        };

        std::get<field_list>(value_).delims_tk.emplace_back(delimiter_tk);
    }
};
