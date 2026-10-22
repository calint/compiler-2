#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <ostream>
#include <span>
#include <string_view>
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
        type_tk, std::format("the destination gives the type '{}', write '{}'",
                             type_name, value)};
}

// a part of an assigned value and where in the destination it is written,
// e.g. 'q.y' at the field 'y' of 'p = {1, q.y}'
struct written_part {
    const statement* item{};
    // the path from the destination to the part
    access_path suffix;
};

// note: members that use 'expr_any', 'stmt_identifier' or 'stmt_call' are
//       implemented in 'expr_type_impl.hpp', those headers include this one
//       through 'expr_any.hpp', so their definitions are incomplete here

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

    // the parts of the value at 'at' in the destination, in the order
    // 'compile' writes them
    auto list_written_parts(const access_path& at,
                            std::vector<written_part>& parts) const -> void;

    auto operator=(expr_type&&) noexcept -> expr_type&;

    //
    // statics
    //

    // unlisted elements are zero like unlisted fields
    static auto zero_remaining_elements(toc& tc, const size_t indent,
                                        const token& src_loc_tk,
                                        const operand& dst,
                                        const type& element_type,
                                        const size_t remaining_count) -> void {

        if (remaining_count == 0) {
            return;
        }

        machine& x{tc.machine()};

        const size_t size_bytes{
            multiply_storage_size(src_loc_tk, element_type.size_bytes(),
                                  remaining_count),
        };

        x.comment(src_loc_tk, indent,
                  "zero remaining elements: {} * {} B = {} B", remaining_count,
                  element_type.size_bytes(), size_bytes);

        x.zero(src_loc_tk, indent, dst, size_bytes, element_type.alignment());
    }

  private:
    auto assert_call_type(const type& tp) const -> void;

    // the '}' that ends the fields, when it is the next character, e.g. the '}'
    // of '{x, y}'
    [[nodiscard]] auto at_closing_brace(tokenizer& tz) -> bool {
        close_brace_tk_ = tz.is_next_char_token('}');
        return not close_brace_tk_.is_empty();
    }

    auto compile_assign(toc& tc, const size_t indent,
                        const destination& dst) const -> void;

    auto compile_call_field(toc& tc, const size_t indent,
                            const destination& dst) const -> void;

    auto compile_field_list(toc& tc, const size_t indent,
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
                                 "'{}' in type '{}'",
                                 next.name, tp.name())),
        };

        expr_delims_tk_.emplace_back(delimiter_tk);
    }

    // creates an 'expr_any' for each field
    auto parse_fields(toc& tc, tokenizer& tz, const type& tp) -> void;

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

    // padding is zeroed too so that instances compare equal byte by byte
    auto zero_unwritten(toc& tc, const size_t indent,
                        const std::string_view what, const size_t size_bytes,
                        const size_t alignment, const operand& dst) const
        -> void {

        if (size_bytes == 0) {
            return;
        }

        machine& x{tc.machine()};

        x.comment(tok(), indent, "zero {}: {} B", what, size_bytes);
        x.zero(tok(), indent, dst, size_bytes, alignment);
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

    // the type of the value of the built-in at 'tk', e.g. 'i8' of 'i8(1)',
    // none when 'tk' names no built-in with a value
    [[nodiscard]] static auto builtin_value_type(const toc& tc, const token& tk,
                                                 tokenizer& tz) -> const type*;

    static auto compile_builtin_field(toc& tc, const size_t indent,
                                      const expr_any& src,
                                      const operand& src_op, const operand& dst)
        -> void;

    static auto compile_field(toc& tc, const size_t indent, const expr_any& src,
                              const type_field& field, const destination& dst)
        -> void;

    static auto compile_record_field(toc& tc, const size_t indent,
                                     const expr_any& src,
                                     const type_field& field,
                                     const destination& dst) -> void;

    // the parts of the value 'src' of 'field' at 'at', as
    // 'compile_field' writes them
    static auto list_field_parts(const expr_any& src, const type_field& field,
                                 const access_path& at,
                                 std::vector<written_part>& parts) -> void;

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

    static auto write_builtin_field(toc& tc, const size_t indent,
                                    const expr_any& src,
                                    const type_field& field,
                                    const destination& dst) -> void;
};
