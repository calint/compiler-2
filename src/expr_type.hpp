#pragma once

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

    // bytes of the variable an instance value is written into
    struct constructor_target {
        std::string_view root;
        byte_range range;

        // false when a run-time index leaves the element unknown
        bool is_exact{};
    };

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

    // the fields are written in order so later items must not read earlier
    // fields of the destination
    auto assert_not_reading(const constructor_target& dst) const -> void {
        // same-type copies are either the same bytes or separate bytes
        if (stmt_ident_) {
            return;
        }

        // a call that writes its result into an argument is rejected by the
        // alias check of the call
        if (stmt_call_) {
            return;
        }

        assert_items_not_reading(dst, 0);
    }

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

    //
    // statics
    //

    // unlisted elements are zero like unlisted fields, returns the zeroed size
    static auto zero_remaining_elements(toc& tc, const size_t indent,
                                        const token& src_loc_tk,
                                        const operand& dst,
                                        const type& element_type,
                                        const size_t remaining_count)
        -> size_t {

        if (remaining_count == 0) {
            return 0;
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

        return size_bytes;
    }

  private:
    auto assert_call_type(const type& tp) const -> void;

    auto assert_items_not_reading(const constructor_target& dst,
                                  const size_t record_offset) const -> void;

    // the '}' that ends the fields, when it is the next character, e.g. the '}'
    // of '{x, y}'
    [[nodiscard]] auto at_closing_brace(tokenizer& tz) -> bool {
        close_brace_tk_ = tz.is_next_char_token('}');
        return not close_brace_tk_.is_empty();
    }

    auto compile_assign(toc& tc, const size_t indent, const type& dst_type,
                        const destination& dst, operand& dst_op) const -> void;

    auto compile_call_field(toc& tc, const size_t indent, const type& dst_type,
                            const destination& dst, operand& dst_op) const
        -> void;

    auto compile_field(toc& tc, const size_t indent, const expr_any& src,
                       const type_field& field, const destination& dst,
                       operand& dst_op) const -> void;

    auto compile_field_list(toc& tc, const size_t indent, const type& dst_type,
                            const destination& dst, operand& dst_op) const
        -> void;

    auto compile_identifier_copy(toc& tc, const size_t indent,
                                 const type& dst_type, operand& dst_op) const
        -> void;

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

    auto write_builtin_field(toc& tc, const size_t indent, const expr_any& src,
                             const type_field& field, const destination& dst,
                             const operand& dst_op) const -> void;

    // a call writes the fields only, so the padding is zeroed before it and
    // '==' can compare instances byte by byte; padding in the elements of an
    // array is many gaps, then the whole instance is zeroed in one go
    auto zero_padding(toc& tc, const size_t indent, const type& dst_type,
                      const operand& dst_op) const -> void {

        // note: each gap takes one to three stores, the whole instance at most
        //       a loop of 4 instructions on x86_64 and 5 on rv32i, or a store
        //       per word when it is small
        constexpr size_t max_gaps_zeroed_one_by_one{8};

        const std::vector<byte_range> ranges{dst_type.data_ranges()};

        // note: a gap follows each range except, maybe, the last
        if (ranges.size() > max_gaps_zeroed_one_by_one) {
            zero_whole_instance(tc, indent, dst_type, dst_op);
            return;
        }

        operand cursor{dst_op};

        // bytes of the instance before 'cursor', fields in order then padding
        size_t covered_bytes{};

        for (const byte_range& r : ranges) {
            zero_unwritten(
                tc, indent, "padding", r.offset - covered_bytes,
                offset_alignment(covered_bytes, dst_type.alignment()), cursor);

            cursor.increment_offset(address_offset(r.size_bytes));
            covered_bytes = r.offset + r.size_bytes;
        }

        zero_unwritten(
            tc, indent, "padding", dst_type.size_bytes() - covered_bytes,
            offset_alignment(covered_bytes, dst_type.alignment()), cursor);
    }

    // padding is zeroed too so that instances compare equal byte by byte
    auto zero_unwritten(toc& tc, const size_t indent,
                        const std::string_view what, const size_t size_bytes,
                        const size_t alignment, operand& dst_op) const -> void {

        if (size_bytes == 0) {
            return;
        }

        machine& x{tc.machine()};

        x.comment(tok(), indent, "zero {}: {} B", what, size_bytes);
        x.zero(tok(), indent, dst_op, size_bytes, alignment);
        dst_op.increment_offset(address_offset(size_bytes));
    }

    // the call writes the fields over the zeroes
    auto zero_whole_instance(toc& tc, const size_t indent, const type& dst_type,
                             const operand& dst_op) const -> void {

        machine& x{tc.machine()};

        x.comment(tok(), indent, "zero instance for its padding: {} B",
                  dst_type.size_bytes());

        x.zero(tok(), indent, dst_op, dst_type.size_bytes(),
               dst_type.alignment());
    }

    //
    // statics
    //

    static auto assert_item_not_reading(const statement& item,
                                        const constructor_target& dst,
                                        const size_t written_size_bytes)
        -> void {

        if (written_size_bytes == 0) {
            return;
        }

        const byte_range exact{
            .offset{dst.range.offset},
            .size_bytes{written_size_bytes},
        };

        // with a run-time index any element of the array may already be written
        const byte_range& written{dst.is_exact ? exact : dst.range};

        item.visit_reads(
            dst.root,
            [&dst, &written](
                const token& src_loc_tk, const std::string_view read_text,
                const byte_range& accessed_bytes,
                [[maybe_unused]] const access_path& accessed_path) -> void {
                if (not accessed_bytes.overlaps(written)) {
                    return;
                }

                // a run-time index may or may not refer to the written element
                if (not dst.is_exact) {
                    throw compile_error{
                        src_loc_tk,
                        std::format(
                            "'{}' may have been overwritten in the same "
                            "statement",
                            read_text)};
                }

                throw compile_error{
                    src_loc_tk,
                    std::format("'{}' is read but has been overwritten in the "
                                "same statement",
                                read_text)};
            });
    }

    static auto assert_record_field_not_reading(const expr_any& src,
                                                const type_field& field,
                                                const constructor_target& dst,
                                                const size_t field_offset)
        -> void;

    // the type of the value of the built-in at 'tk', e.g. 'i8' of 'i8(1)',
    // none when 'tk' names no built-in with a value
    [[nodiscard]] static auto builtin_value_type(const toc& tc, const token& tk,
                                                 tokenizer& tz) -> const type*;

    static auto compile_builtin_field(toc& tc, const size_t indent,
                                      const expr_any& src,
                                      const operand& src_op, const operand& dst)
        -> void;

    static auto compile_record_field(toc& tc, const size_t indent,
                                     const expr_any& src,
                                     const type_field& field,
                                     const destination& dst, operand& dst_op)
        -> void;

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
