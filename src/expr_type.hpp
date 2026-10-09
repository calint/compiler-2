#pragma once
// reviewed: 2025-09-29

#include <cassert>
#include <cstddef>
#include <format>
#include <memory>
#include <optional>
#include <ostream>
#include <string_view>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "type.hpp"
#include "ub_reads.hpp"

// note: members that use 'expr_any', 'stmt_identifier' or 'stmt_call' are
//       implemented in 'decouple_impl.hpp', those headers include this one
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
    // bytes of the variable an instance value is written into
    struct constructor_target {
        std::string_view root;
        byte_range range;

        // false when a runtime index leaves the element unknown
        bool is_exact{};
    };

    // out-of-line: parses 'expr_any' items and creates the 'stmt_call' or
    // 'stmt_identifier'; the type name alone is its zero value, e.g. 'var p =
    // point' is 'point{}'
    expr_type(toc& tc, tokenizer& tz, const type& tp,
              const bool is_array_destination);

    // out-of-line: a method receiver that has already been parsed
    explicit expr_type(std::unique_ptr<stmt_identifier> receiver);

    // out-of-line: a 'unique_ptr' of a class that is incomplete here needs
    // the special members where 'stmt_identifier' and 'stmt_call' are complete
    expr_type();

    expr_type(expr_type&&) noexcept;

    expr_type(const expr_type&) = delete;
    auto operator=(const expr_type&) -> expr_type& = delete;

    ~expr_type() override;

    //
    // overridden methods
    //

    // out-of-line: calls 'stmt_call', 'stmt_identifier' and 'expr_any'
    auto source_to(std::ostream& os) const -> void override;

    // out-of-line: calls 'stmt_call'
    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override;

    // out-of-line: calls 'stmt_identifier'
    [[nodiscard]] auto accessed_path() const
        -> std::optional<access_path> override;

    // out-of-line: calls 'stmt_identifier'
    [[nodiscard]] auto compile_lea(toc& tc, const size_t indent,
                                   const token& src_loc_tk,
                                   std::vector<operand>& allocated_registers,
                                   const lea_request& request) const
        -> operand override;

    // out-of-line: calls 'stmt_identifier'
    [[nodiscard]] auto identifier() const -> std::string_view override;

    // out-of-line: calls 'stmt_identifier'
    [[nodiscard]] auto is_array_element() const -> bool override;

    [[nodiscard]] auto is_identifier() const -> bool override {
        return stmt_ident_ != nullptr;
    }

    // out-of-line: calls 'stmt_identifier'
    [[nodiscard]] auto is_indexed() const -> bool override;

    // out-of-line: calls 'stmt_call', 'stmt_identifier' and 'expr_any'
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

        // todo: a call may write its result into an argument, see etc/todo.txt
        if (stmt_call_) {
            return;
        }

        assert_items_not_reading(dst, 0);
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
    // out-of-line: calls 'stmt_call'
    auto assert_call_type(const type& tp) const -> void;

    // out-of-line: calls 'expr_any'
    auto assert_items_not_reading(const constructor_target& dst,
                                  const size_t record_offset) const -> void;

    // the '}' that ends the fields, when it is the next character, e.g. the '}'
    // of '{x, y}'
    [[nodiscard]] auto at_closing_brace(tokenizer& tz) -> bool {
        close_brace_tk_ = tz.is_next_char_token('}');
        return not close_brace_tk_.is_empty();
    }

    // out-of-line: calls 'expr_any'
    auto compile_assign(toc& tc, const size_t indent, const type& dst_type,
                        const ident_info& dst_info, operand& dst_op) const
        -> void;

    // out-of-line: calls 'stmt_call'
    auto compile_call_field(toc& tc, const size_t indent, const type& dst_type,
                            const ident_info& dst_info, operand& dst_op) const
        -> void;

    // out-of-line: calls 'expr_any'
    auto compile_field(toc& tc, const size_t indent, const expr_any& src,
                       const type_field& field, ident_info& dst_info,
                       operand& dst_op) const -> void;

    // out-of-line: calls 'expr_any'
    auto compile_field_list(toc& tc, const size_t indent, const type& dst_type,
                            const ident_info& dst_info, operand& dst_op) const
        -> void;

    // out-of-line: calls 'stmt_identifier'
    auto compile_identifier_copy(toc& tc, const size_t indent,
                                 const type& dst_type, operand& dst_op) const
        -> void;

    // out-of-line: creates the 'stmt_call' or 'stmt_identifier'
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

    // out-of-line: creates an 'expr_any' for each field
    auto parse_fields(toc& tc, tokenizer& tz, const type& tp) -> void;

    // out-of-line: calls 'expr_any'
    auto write_builtin_field(toc& tc, const size_t indent, const expr_any& src,
                             const type_field& field, ident_info& dst_info,
                             const operand& dst_op) const -> void;

    // a call writes the fields only, so the padding is zeroed before it and
    // '==' can compare instances byte by byte
    auto zero_padding(toc& tc, const size_t indent, const type& dst_type,
                      const operand& dst_op) const -> void {

        operand cursor{dst_op};

        // bytes of the instance before 'cursor', fields in order then padding
        size_t covered_bytes{};

        for (const byte_range& r : dst_type.data_ranges()) {
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

        // with a runtime index any element of the array may already be written
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

                // a runtime index may or may not refer to the written element
                if (not dst.is_exact) {
                    throw compiler_exception{
                        src_loc_tk,
                        std::format(
                            "'{}' may have been overwritten in the same "
                            "statement",
                            read_text)};
                }

                throw compiler_exception{
                    src_loc_tk,
                    std::format("'{}' is read but has been overwritten in the "
                                "same statement",
                                read_text)};
            });
    }

    // out-of-line: calls 'expr_any'
    static auto assert_record_field_not_reading(const expr_any& src,
                                                const type_field& field,
                                                const constructor_target& dst,
                                                const size_t field_offset)
        -> void;

    // out-of-line: calls 'expr_any'
    static auto compile_builtin_field(toc& tc, const size_t indent,
                                      const expr_any& src,
                                      const operand& src_op, const operand& dst)
        -> void;

    // out-of-line: calls 'expr_any'
    static auto compile_record_field(toc& tc, const size_t indent,
                                     const expr_any& src,
                                     const type_field& field,
                                     const ident_info& dst_info,
                                     operand& dst_op) -> void;

    static auto validate_array_assignment(const token& src_loc_tk,
                                          const type_field& fld,
                                          const ident_info& src_info) -> void {

        if (not src_info.is_array) {
            throw compiler_exception{src_loc_tk, "source must be an array"};
        }

        // 'expr_any' validates the source element type before entering here

        assert(fld.type().is_same(src_info.type_ref()));

        if (fld.array_count != src_info.array_len) {
            throw compiler_exception{
                src_loc_tk,
                std::format("destination array size {} does not match source "
                            "size {}",
                            fld.array_count, src_info.array_len)};
        }
    }
};
