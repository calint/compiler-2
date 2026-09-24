#pragma once
// reviewed: 2025-09-29

#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "decouple.hpp"
#include "statement.hpp"
#include "toc.hpp"

class expr_type_value final : public statement {
    std::shared_ptr<stmt_identifier> stmt_ident_;
    std::shared_ptr<stmt_call> stmt_call_;
    // note: 'shared_ptr' because 'unique_ptr' poses compilation issues
    //       regarding circular references

    token open_brace_tk_;
    std::vector<std::unique_ptr<expr_any>> exprs_;
    std::vector<token> expr_delims_tk_;
    token close_brace_tk_;

  public:
    expr_type_value(toc& tc, tokenizer& tz, const type& tp);
    // note: constructor and destructor are implemented in 'decouple_impl.hpp'
    //       where the 'expr_any' definition is known. clang++ -std=c++23 has
    //       required it since changes to handling of unique_ptr to
    //       incomplete types

    expr_type_value() = default;
    // note: copy and assignment constructor will not compile if used

    // implemented in 'decouple_impl.hpp' due to circular reference:
    // expr_type_value -> expr_any -> expr_type_value
    auto source_to(std::ostream& os) const -> void override;

    [[nodiscard]] auto is_make_copy() const -> bool {
        return not tok().text().empty();
        // note: if token is empty then it is an expression of a type '{ ... }'
        //       otherwise e.g. 'p = pt'
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override;

    // implemented in 'decouple_impl.hpp' due to circular reference:
    // expr_type_value -> expr_any -> expr_type_value
    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override;

    [[nodiscard]] auto is_identifier() const -> bool override {
        return stmt_ident_ != nullptr;
    }

    // implemented in 'decouple_impl.hpp' due to circular reference
    [[nodiscard]] auto is_indexed() const -> bool override;

    [[nodiscard]] auto is_array_element() const -> bool override;

    // implemented in 'decouple_impl.hpp' due to circular reference
    [[nodiscard]] auto identifier() const -> std::string_view override;

    // implemented in 'decouple_impl.hpp' due to circular reference
    [[nodiscard]] auto compile_lea(toc& tc, const size_t indent,
                                   const token& src_loc_tk,
                                   std::vector<operand>& allocated_registers,
                                   const operand& reg_count,
                                   const std::span<const operand> lea_path,
                                   const operand& address_register) const
        -> operand override;

    // bytes of the variable a record value is written into
    struct record_destination {
        std::string_view root;
        field_coverage::range range;

        // false when a runtime index leaves the element unknown
        bool is_exact{};
    };

    // implemented in 'decouple_impl.hpp'
    // the fields are written in order so later items must not read earlier
    // fields of the destination
    auto assert_not_reading(const record_destination& dst) const -> void;

  private:
    // implemented in 'decouple_impl.hpp'
    auto assert_items_not_reading(const record_destination& dst,
                                  const size_t record_offset) const -> void;

    // implemented in 'decouple_impl.hpp'
    static auto assert_item_not_reading(const statement& item,
                                        const record_destination& dst,
                                        const size_t written_size_bytes)
        -> void;

    // implemented in 'decouple_impl.hpp' due to circular reference:
    // expr_type_value -> expr_any -> expr_type_value
    auto compile_assign(toc& tc, const size_t indent, const type& dst_type,
                        const ident_info& dst_info, operand& dst_op) const
        -> void;

    // implemented in 'decouple_impl.hpp'
    static auto validate_array_assignment(const token& src_loc_tk,
                                          const type_field& fld,
                                          const ident_info& src_info) -> void;
};
