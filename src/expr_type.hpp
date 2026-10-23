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
#include <variant>
#include <vector>

#include "compile_error.hpp"
#include "decouple.hpp"
#include "ident_info.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "type.hpp"
#include "ub_alias.hpp"
#include "ub_reads.hpp"

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

            value->item->assert_not_narrowed(tc, part_dst.type_ref());
            value->item->compile(tc, indent, part_dst);
            continue;
        }

        if (const bytes_write* const bytes{
                std::get_if<bytes_write>(&step.action),
            }) {

            x.copy_bytes(
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
// the type name alone as its zero value, a copy of a name or a call; exactly
// one of 'stmt_call_', 'stmt_ident_' and the list 'exprs_' is used
class expr_type final : public statement {
    std::unique_ptr<stmt_identifier> stmt_ident_;
    std::unique_ptr<stmt_call> stmt_call_;

    token open_brace_tk_;
    std::vector<std::unique_ptr<expr_any>> exprs_;
    std::vector<token> expr_delims_tk_;
    token close_brace_tk_;

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
    expr_type();

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

    [[nodiscard]] auto accessed_path() const -> access_path override;

    [[nodiscard]] auto compile_lea(toc& tc, const size_t indent,
                                   const token& src_loc_tk,
                                   std::vector<operand>& allocated_registers,
                                   const lea_request& request) const
        -> operand override;

    [[nodiscard]] auto is_array_element() const -> bool override;

    [[nodiscard]] auto is_indexed() const -> bool override;

    [[nodiscard]] auto is_name_or_literal() const -> bool override {
        return stmt_ident_ != nullptr;
    }

    [[nodiscard]] auto name_or_literal() const -> std::string_view override;

    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override;

    //
    // class methods
    //

    // the values of the listed fields of a literal, e.g. '1' and '2' of
    // 'point{1, 2}'
    [[nodiscard]] auto field_values() const
        -> std::span<const std::unique_ptr<expr_any>> {

        return exprs_;
    }

    // e.g. 'point{1, 2}' or 'point', not a copy of 'p' or of a call result
    [[nodiscard]] auto is_literal() const -> bool {
        return stmt_ident_ == nullptr and stmt_call_ == nullptr;
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
            .covered{},
        });
    }

  private:
    auto assert_call_type(const type& tp) const -> void;

    // the '}' that ends the fields, when it is the next character, e.g. the '}'
    // of '{x, y}'
    [[nodiscard]] auto at_closing_brace(tokenizer& tz) -> bool {
        close_brace_tk_ = tz.is_next_char_token('}');
        return not close_brace_tk_.is_empty();
    }

    auto compile_call_field(toc& tc, const size_t indent,
                            const destination& dst) const -> void;

    auto compile_identifier_copy(toc& tc, const size_t indent,
                                 const destination& dst) const -> void;

    // creates the 'stmt_call' or 'stmt_identifier'
    auto parse_copy_source(toc& tc, tokenizer& tz, const type& tp) -> void;

    // the ',' before the value of the field 'next', e.g. the ',' of '{x, y}'
    // before the value of 'y'
    auto parse_field_delimiter(tokenizer& tz, const type& tp,
                               const type_field& next) -> void {

        const token delimiter_tk{
            tz.expect_char_token(
                ',', std::format("expected ',' followed by a value for field "
                                 "{} in type {}",
                                 in_quotes(next.name), in_quotes(tp.name()))),
        };

        expr_delims_tk_.emplace_back(delimiter_tk);
    }

    // creates an 'expr_any' for each field
    auto parse_fields(toc& tc, tokenizer& tz, const type& tp) -> void;

    // padding is zeroed too so that instances compare equal byte by byte,
    // 'offset' bytes into the instance at 'at'
    auto plan_unwritten(const part_location& at, const std::string_view what,
                        const size_t offset, const size_t size_bytes,
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
            .covered{},
        });
    }

    // a call writes the fields only, so the padding is zeroed before it and
    // '==' can compare instances byte by byte; the backend decides whether
    // gap by gap or the whole instance
    auto zero_padding(toc& tc, const size_t indent, const type& dst_type,
                      const operand& dst) const -> void {

        machine& x{tc.machine()};

        const std::vector<machine::padding_gap> gaps{
            padding_gaps(dst_type, x.max_padding_gaps_zeroed_one_by_one()),
        };

        if (gaps.empty()) {
            return;
        }

        x.zero_padding(tok(), indent, dst, dst_type.size_bytes(),
                       dst_type.alignment(), gaps);
    }

    //
    // statics
    //

    // the bytes from 'start' to 'end' are a gap unless there are none,
    // 'alignment' is that of the instance
    static auto append_gap(std::vector<machine::padding_gap>& gaps,
                           const size_t start, const size_t end,
                           const size_t alignment) -> void {

        if (start == end) {
            return;
        }

        gaps.push_back({
            .offset{start},
            .size_bytes{end - start},
            .alignment{offset_alignment(start, alignment)},
        });
    }

    // the bytes that no field covers, between the data ranges and after the
    // last one; the first of them when there are more than 'max_count'
    [[nodiscard]] static auto padding_gaps(const type& tp,
                                           const size_t max_count)
        -> std::vector<machine::padding_gap> {

        // note: + 1 because a gap is between two ranges, so more than
        //       'max_count' + 1 ranges give more than 'max_count' gaps

        const size_t max_range_count{max_count + 1};

        const std::vector<byte_range> ranges{tp.data_ranges(max_range_count)};

        std::vector<machine::padding_gap> gaps;
        size_t covered_bytes{};

        for (const byte_range& r : ranges) {
            append_gap(gaps, covered_bytes, r.offset, tp.alignment());
            covered_bytes = r.offset + r.size_bytes;
        }

        // the last range of a stopped search may be incomplete
        if (ranges.size() > max_range_count) {
            return gaps;
        }

        append_gap(gaps, covered_bytes, tp.size_bytes(), tp.alignment());

        return gaps;
    }

    // appends the steps that write 'src' of the field at 'at'
    static auto plan_field(const toc& tc, const expr_any& src,
                           const type_field& field, const part_location& at,
                           write_plan& plan) -> void;

    static auto validate_array_assignment(const token& src_loc_tk,
                                          const type_field& fld,
                                          const ident_info& src_info) -> void {

        if (not src_info.is_array()) {
            throw compile_error{src_loc_tk, "source must be an array"};
        }

        // 'expr_any' validates the source element type before entering here
        assert(fld.type().is_same(src_info.type_ref()));

        const std::optional<size_t> src_len{src_info.array_len()};

        if (src_len and fld.array_count != *src_len) {
            throw compile_error{
                src_loc_tk,
                std::format("destination array size {} does not match source "
                            "size {}",
                            fld.array_count, *src_len)};
        }
    }
};
