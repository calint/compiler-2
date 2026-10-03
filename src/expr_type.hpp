#pragma once
// reviewed: 2025-09-29

#include <format>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "toc.hpp"

// note: members that use 'expr_any', 'stmt_identifier' or 'stmt_call' are
//       implemented in 'decouple_impl.hpp', those headers include this one
//       through 'expr_any.hpp', so their definitions are incomplete here
class expr_type final : public statement {
    std::shared_ptr<stmt_identifier> stmt_ident_;
    std::shared_ptr<stmt_call> stmt_call_;
    // note: 'shared_ptr' because 'unique_ptr' poses compilation issues
    //       regarding circular references

    token open_brace_tk_;
    std::vector<std::unique_ptr<expr_any>> exprs_;
    std::vector<token> expr_delims_tk_;
    token close_brace_tk_;

    // only a whole array destination may copy a whole array source
    bool is_array_destination_{};

  public:
    // bytes of the variable a record value is written into
    struct record_destination {
        std::string_view root;
        field_coverage::range range;

        // false when a runtime index leaves the element unknown
        bool is_exact{};
    };

    // out-of-line: parses 'expr_any' items and creates the 'stmt_call' or
    // 'stmt_identifier'
    expr_type(toc& tc, tokenizer& tz, const type& tp,
              const bool is_array_destination);

    // out-of-line: a method receiver that has already been parsed
    explicit expr_type(std::shared_ptr<stmt_identifier> receiver);

    expr_type() = default;

    //
    // overridden methods
    //

    // note: copy and assignment constructor will not compile if used

    // out-of-line: calls 'stmt_call', 'stmt_identifier' and 'expr_any'
    auto source_to(std::ostream& os) const -> void override;

    // out-of-line: calls 'stmt_call'
    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override;

    // out-of-line: calls 'stmt_identifier'
    [[nodiscard]] auto accessed_range() const
        -> std::optional<field_coverage::range> override;

    // out-of-line: calls 'stmt_identifier'
    [[nodiscard]] auto compile_lea(toc& tc, const size_t indent,
                                   const token& src_loc_tk,
                                   std::vector<operand>& allocated_registers,
                                   const operand& reg_count,
                                   const std::span<const operand> lea_path,
                                   const operand& address_register) const
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
    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override;

    //
    // class methods
    //

    // the fields are written in order so later items must not read earlier
    // fields of the destination
    auto assert_not_reading(const record_destination& dst) const -> void {
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
    auto assert_items_not_reading(const record_destination& dst,
                                  const size_t record_offset) const -> void;

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

    // out-of-line: calls 'expr_any'
    auto write_builtin_field(toc& tc, const size_t indent, const expr_any& src,
                             const type_field& field, ident_info& dst_info,
                             const operand& dst_op) const -> void;

    // a call writes the fields only, so the padding is zeroed before it and
    // 'equal' can compare records byte by byte
    auto zero_padding(toc& tc, const size_t indent, const type& dst_type,
                      const operand& dst_op) const -> void {

        operand cursor{dst_op};

        // bytes of the record before 'cursor', fields in order then padding
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

    // padding is zeroed too so that records compare equal byte by byte
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
                                        const record_destination& dst,
                                        const size_t written_size_bytes)
        -> void {

        if (written_size_bytes == 0) {
            return;
        }

        // with a runtime index any element of the array may already be written
        const field_coverage::range written{
            dst.is_exact
                ? field_coverage::range{.offset{dst.range.offset},
                                        .size_bytes{written_size_bytes},}
                : dst.range,};

        item.visit_reads(
            dst.root,
            [&dst, &written](
                const token& use_tk, const std::string_view read_text,
                const std::optional<field_coverage::range>& accessed) -> void {
                if (accessed and not accessed->overlaps(written)) {
                    return;
                }

                // a runtime index may or may not refer to the written element
                if (not dst.is_exact) {
                    throw compiler_exception{
                        use_tk,
                        std::format(
                            "'{}' may have been overwritten in the same "
                            "statement",
                            read_text)};
                }

                throw compiler_exception{
                    use_tk,
                    std::format("'{}' is read but has been overwritten in the "
                                "same statement",
                                read_text)};
            });
    }

    // out-of-line: calls 'expr_any'
    static auto assert_record_field_not_reading(const expr_any& src,
                                                const type_field& field,
                                                const record_destination& dst,
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

        assert(fld.type().name() == src_info.type_ref().name());

        if (fld.array_count != src_info.array_len) {
            throw compiler_exception{
                src_loc_tk,
                std::format("destination array size {} does not match source "
                            "size {}",
                            fld.array_count, src_info.array_len)};
        }
    }
};
