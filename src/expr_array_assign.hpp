#pragma once
// reviewed: 2026-09-16

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "decouple.hpp"
#include "statement.hpp"
#include "toc.hpp"

// whole-array assignment from an identifier or a function call, e.g.
// 'arr2 = arr1' or 'arr2 = f()'. shares its parsing and byte-copy logic with
// 'expr_type_value' (see 'parse_ident_or_call'/'copy_ident_bytes' in
// 'decouple.hpp') since neither cares whether the expected type is a
// built-in array or a user type; unlike 'expr_type_value' it never parses a
// '{ field, ... }' literal, so it does not carry that responsibility.
class expr_array_assign final : public statement {
    std::shared_ptr<stmt_identifier> stmt_ident_;
    std::shared_ptr<stmt_call> stmt_call_;
    // note: 'shared_ptr' because 'unique_ptr' poses compilation issues
    //       regarding circular references

  public:
    // note: implemented in 'decouple_impl.hpp' where 'stmt_identifier' and
    //       'stmt_call' definitions are known
    expr_array_assign(toc& tc, tokenizer& tz, token tk, const type& tp);

    expr_array_assign() = default;

    // implemented in 'decouple_impl.hpp'
    auto source_to(std::ostream& os) const -> void override;

    // implemented in 'decouple_impl.hpp'
    auto compile(toc& tc, std::ostream& os, size_t indent,
                 const ident_info& dst_info) const -> void override;

    // no-op: matches 'expr_type_value's identifier-or-call case, which does
    // not check whether 'var' is used
    auto assert_var_not_used([[maybe_unused]] std::string_view var) const
        -> void override {}

    [[nodiscard]] auto is_identifier() const -> bool override {
        return stmt_ident_ != nullptr;
    }

    // implemented in 'decouple_impl.hpp'
    [[nodiscard]] auto is_indexed() const -> bool override;

    // implemented in 'decouple_impl.hpp'
    [[nodiscard]] auto identifier() const -> std::string_view override;

    // implemented in 'decouple_impl.hpp'
    auto compile_lea(const token& src_loc_tk, toc& tc, std::ostream& os,
                     size_t indent,
                     std::vector<std::string>& allocated_registers,
                     const std::string& reg_size,
                     std::span<const std::string> lea) const
        -> operand override;
};
