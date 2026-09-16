#pragma once
// out-of-line definitions for functions/methods declared in 'decouple.hpp',
// 'expr_type_value.hpp' and 'unary_ops.hpp' to break circular includes
// between 'expr_any', 'expr_type_value', 'stmt_block', 'stmt_call',
// 'unary_ops' and 'toc'.

#include <memory>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "expr_array_assign.hpp"
#include "expr_type_value.hpp"
#include "stmt_builtin_address_of.hpp"
#include "stmt_builtin_array_size_of.hpp"
#include "stmt_builtin_arrays_equal.hpp"
#include "stmt_builtin_equal.hpp"
#include "stmt_builtin_foo.hpp"
#include "stmt_builtin_mov.hpp"
#include "stmt_builtin_syscall.hpp"
#include "stmt_call.hpp"
#include "stmt_identifier.hpp"
#include "stmt_if.hpp"
#include "stmt_loop.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"
#include "unary_ops.hpp"

// definitions are intentionally not 'inline': single translation unit build
// NOLINTBEGIN(misc-definitions-in-headers)

// declared in 'decouple.hpp'
// called from 'stmt_block' to solve circular dependencies with 'loop',
// 'if', 'mov', 'syscall'
[[nodiscard]] auto create_statement_in_stmt_block(toc& tc, tokenizer& tz,
                                                  const token tk)
    -> std::unique_ptr<statement> {

    // note: no 'std:move' on 'tk' because it is trivially copyable
    if (tk.is_text("loop")) {
        return std::make_unique<stmt_loop>(tc, tk, tz);
    }
    if (tk.is_text("if")) {
        return std::make_unique<stmt_if>(tc, tk, tz);
    }
    if (tk.is_text("mov")) {
        return std::make_unique<stmt_builtin_mov>(tc, tk, tz);
    }
    if (tk.is_text("syscall")) {
        return std::make_unique<stmt_builtin_syscall>(tc, tk, tz);
    }
    if (tk.is_text("foo")) {
        return std::make_unique<stmt_builtin_foo>(tc, tk, tz);
    }

    std::unreachable();
}

// declared in 'decouple.hpp'
// called from 'stmt_block'
[[nodiscard]] auto create_stmt_call(toc& tc, tokenizer& tz,
                                    const stmt_identifier& si,
                                    token open_paren_tk)
    -> std::unique_ptr<statement> {
    return std::make_unique<stmt_call>(tc, si.get_unary_ops(), si.first_token(),
                                       open_paren_tk, tz);
}

// declared in 'decouple.hpp'
// called from 'expr_ops_list' to solve circular dependencies with function
// calls
[[nodiscard]] auto create_statement_in_expr_ops_list(toc& tc, tokenizer& tz)
    -> std::unique_ptr<statement> {

    // note: no 'std::move' on 'tk' because it is trivially copyable
    unary_ops uops{tz};
    const token tk{tz.next_token()};
    if (tk.text().empty()) {
        throw compiler_exception{
            tk, "expected constant, identifier, or function call"};
    }
    if (tk.text().starts_with("#")) {
        throw compiler_exception{tk, "unexpected comment in expression"};
    }
    if (tk.is_text("address_of")) {
        return std::make_unique<stmt_builtin_address_of>(tc, std::move(uops),
                                                         tk, tz);
    }
    if (tk.is_text("array_size_of")) {
        return std::make_unique<stmt_builtin_array_size_of>(tc, std::move(uops),
                                                            tk, tz);
    }
    if (tk.is_text("arrays_equal")) {
        return std::make_unique<stmt_builtin_arrays_equal>(tc, std::move(uops),
                                                           tk, tz);
    }
    if (tk.is_text("equal")) {
        return std::make_unique<stmt_builtin_equal>(tc, std::move(uops), tk,
                                                    tz);
    }

    if (const token t{tz.is_next_char_token('(')}; not t.is_empty()) {
        // e.g.  foo(...)
        return std::make_unique<stmt_call>(tc, std::move(uops), tk, t, tz);
    }
    // e.g. 0x80, rax, identifiers, constants
    return std::make_unique<stmt_identifier>(tc, std::move(uops), tk, tz);
}

// declared in 'decouple.hpp'
// shared by 'expr_type_value' and 'expr_array_assign'
[[nodiscard]] auto parse_ident_or_call(toc& tc, token tk, tokenizer& tz,
                                       const type& tp) -> ident_or_call_parts {

    if (const token t{tz.is_next_char_token('(')}; not t.is_empty()) {
        auto call{std::make_shared<stmt_call>(tc, unary_ops{}, tk, t, tz)};

        if (tp.name() != call->get_type().name()) {
            throw compiler_exception{
                tk, std::format("expected return type '{}', got '{}'",
                                tp.name(), call->get_type().name())};
        }

        return {.ident{}, .call{std::move(call)}};
    }

    auto ident{std::make_shared<stmt_identifier>(tc, unary_ops{}, tk, tz)};

    // check that an identifier type matches the expected type
    const ident_info src_info{tc.make_ident_info(*ident)};

    if (tp.name() != src_info.type().name()) {
        // note: checked a source location report ok
        throw compiler_exception{tk, std::format("expected type '{}', got '{}'",
                                                 tp.name(),
                                                 src_info.type().name())};
    }

    return {.ident{std::move(ident)}, .call{}};
}

// declared in 'decouple.hpp'
// shared by 'expr_type_value' and 'expr_array_assign'
auto copy_ident_bytes(toc& tc, std::ostream& os, size_t indent,
                      const statement& self, const type& dst_type,
                      operand& dst_op) -> void {

    const ident_info src_info{tc.make_ident_info(self)};

    // caller validates the source type before entering here
    assert(dst_type.name() == src_info.type().name());

    std::vector<std::string> allocated_registers;
    operand src_op;
    if (self.is_indexed() or src_info.has_lea()) {
        src_op = self.compile_lea(self.tok(), tc, os, indent,
                                  allocated_registers, "", src_info.lea_path);
    } else {
        src_op = src_info.operand;
    }

    const size_t nbytes{src_info.is_array
                            ? src_info.array_size * dst_type.size()
                            : dst_type.size()};

    // todo: validate dst array size fits src array size

    tc.rep_movs(self.tok(), os, indent, src_op.address_str(),
                dst_op.address_str(), nbytes);

    dst_op.displacement += static_cast<int32_t>(nbytes);

    for (const std::string& reg : allocated_registers | std::views::reverse) {
        tc.free_scratch_register(self.tok(), os, indent, reg);
    }
}

// declared in 'expr_type_value.hpp'
// note: constructor is implemented here (rather than in the header) because
//       it needs the 'expr_any' definition, which would otherwise create a
//       circular include between 'expr_type_value.hpp' and 'expr_any.hpp'
expr_type_value::expr_type_value(toc& tc, tokenizer& tz, const type& tp)
    : statement{tz.next_token()} {

    set_type(tp);

    // is it an identifier or a function call?
    // note: token name would be empty at the '{x, y}' type of statement

    if (not tok().text().empty()) {
        // yes, e.g. obj.pos = p, or obj.pos = f()
        ident_or_call_parts parts{parse_ident_or_call(tc, tok(), tz, tp)};
        stmt_ident_ = std::move(parts.ident);
        stmt_call_ = std::move(parts.call);
        return;
    }

    // e.g. obj.pos = {x, y}
    open_brace_tk_ = tz.is_next_char_token('{');
    if (open_brace_tk_.is_empty()) {
        throw compiler_exception{
            tz, std::format("expected '{{' to begin a value of type '{}'",
                            tp.name())};
    }

    const std::span<const type_field> flds{tp.fields()};
    const size_t nflds{flds.size()};
    size_t counter{};
    while (true) {
        close_brace_tk_ = tz.is_next_char_token('}');
        if (not close_brace_tk_.is_empty()) {
            break;
        }

        if (counter == nflds) {
            throw compiler_exception{
                tz, std::format("too many fields specified for type '{}'",
                                tp.name())};
        }
        const type_field& tf{flds[counter]};
        if (counter++) {
            const token t{tz.is_next_char_token(',')};
            if (t.is_empty()) {
                throw compiler_exception{
                    tz, std::format(
                            "expected ',' followed by a value for field '{}' "
                            "in type '{}'",
                            flds[counter - 1].name, tp.name())};
            }
            exprs_delims_tk_.emplace_back(t);
        }
        // create an expression that assigns to field
        // might recurse creating 'expr_type_value'
        exprs_.emplace_back(std::make_unique<expr_any>(
            tc, tz, tf.type(), true, tf.is_array, tf.array_size));
    }
}

// declared in 'expr_type_value.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
auto expr_type_value::source_to(std::ostream& os) const -> void {
    if (stmt_call_) {
        stmt_call_->source_to(os);
        return;
    }

    // is it an identifier? because statement printed that
    if (is_make_copy()) {
        stmt_ident_->source_to(os);
        return;
    }

    statement::source_to(os);

    // not an identifier
    open_brace_tk_.source_to(os);
    if (not exprs_.empty()) {
        exprs_.front()->source_to(os);
        for (const auto [d, e] :
             std::views::zip(exprs_delims_tk_, exprs_ | std::views::drop(1))) {
            d.source_to(os);
            e->source_to(os);
        }
    }
    close_brace_tk_.source_to(os);
}

// declared in 'expr_type_value.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
auto expr_type_value::compile(toc& tc, std::ostream& os, size_t indent,
                              const ident_info& dst_info) const -> void {

    if (stmt_call_) {
        stmt_call_->compile(tc, os, indent, dst_info);
        return;
    }

    const type& tp{dst_info.type()};
    operand op{dst_info.operand};
    compile_assign(tc, os, indent, tp, op);
}

// declared in 'expr_type_value.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
auto expr_type_value::compile_assign(toc& tc, std::ostream& os, size_t indent,
                                     const type& dst_type,
                                     operand& dst_op) const -> void {

    // is it e.g. pt1 = pt2, or pt1 = f()?
    if (is_identifier()) {
        copy_ident_bytes(tc, os, indent, *this, dst_type, dst_op);
        return;
    }

    // initialize fields
    size_t counter{};
    const std::span<const type_field>& flds{dst_type.fields()};
    for (const std::unique_ptr<expr_any>& ea : exprs_) {
        tc.comment_start(tok(), os, indent);
        const type_field& tf{flds[counter]};
        std::println(os, "copy field '{}'", tf.name);

        if (not tf.type().is_built_in()) {
            // a not-builtin statement is 'expr_type_value'
            const expr_type_value& e{ea->as_expr_type_value()};
            e.compile_assign(tc, os, indent, tf.type(), dst_op);
            ++counter;
            continue;
        }

        // built-in

        const expr_any& src{*exprs_[counter]};

        if (src.is_array() and src.is_empty()) {
            // special case when empty array
            // e.g.:
            //   type msgpoint {  msg : i8[128], pt : point }
            //   var mp : msgpoint[3] = { { {}, { x, y } } }
            tc.comment_start(tok(), os, indent);
            std::println(os, "zero empty field: {} * {} B = {} B",
                         tf.array_size, tf.type().size(), tf.size);
            tc.rep_stos_zero(tok(), os, indent, dst_op.address_str(), tf.size);
            dst_op.displacement += static_cast<int32_t>(tf.size);
            ++counter;
            continue;
        }

        if (tf.is_array and src.is_array_identifier()) {
            validate_array_assignment(src.tok(), tf, tc.make_ident_info(src));
        }

        const std::string dst_accessor{dst_op.str(tf.type().size())};

        if (src.is_expression() or (src.is_identifier() and tc.has_lea(src))) {
            // built-in, expression
            const ident_info dst_info{
                tc.make_ident_info(src.tok(), dst_accessor)};
            src.compile(tc, os, indent, dst_info);
        } else {
            // built-in, not expression
            const ident_info src_info{tc.make_ident_info(src)};
            if (src_info.is_const()) {
                // built-in, not expression, constant
                tc.asm_cmd(src.tok(), os, indent, "mov", dst_accessor,
                           std::format("{}{}", src.get_unary_ops().to_string(),
                                       src_info.const_value));
            } else {
                // built-in, not expression, not constant
                if (tf.is_array) {
                    // todo: this code is not covered by the tests, find how to
                    //       trigger it
                    // built-in, not expression, not constant, array
                    validate_array_assignment(src.tok(), tf, src_info);
                    tc.rep_movs(src.tok(), os, indent,
                                src_info.operand.address_str(),
                                dst_op.address_str(), tf.size);
                } else {
                    // built-in, not expression, not constant, not array
                    tc.asm_cmd(src.tok(), os, indent, "mov", dst_accessor,
                               src_info.operand.str());
                    src.get_unary_ops().compile(tc, os, indent, dst_accessor);
                }
            }
        }
        dst_op.displacement += static_cast<int32_t>(tf.size);
        ++counter;
    }

    // zero out the remaining fields

    const size_t diff{dst_type.fields().size() - counter};

    if (diff == 0) {
        // all fields have been assigned
        return;
    }

    // calculate remaining bytes of the type to zero

    const size_t n{flds.size()};
    size_t nbytes{};
    for (size_t i{counter}; i < n; ++i) {
        nbytes += flds[i].size;
    }

    tc.comment_start(tok(), os, indent);
    std::println(os, "zero remaining fields: {} B", nbytes);
    tc.rep_stos_zero(tok(), os, indent, dst_op.address_str(), nbytes);
    dst_op.displacement += static_cast<int32_t>(nbytes);
}

// declared in 'expr_type_value.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
auto expr_type_value::validate_array_assignment(const token& tok,
                                                const type_field& fld,
                                                const ident_info& src_info)
    -> void {

    if (not src_info.is_array) {
        throw compiler_exception{tok, "source must be an array"};
    }

    // 'expr_any' validates the source element type before entering here
    assert(fld.type().name() == src_info.type().name());

    if (fld.array_size != src_info.array_size) {
        throw compiler_exception{
            tok, std::format("destination array size {} does not match "
                             "source size {}",
                             fld.array_size, src_info.array_size)};
    }
}

// declared in 'expr_type_value.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
auto expr_type_value::assert_var_not_used(const std::string_view var) const
    -> void {

    for (const std::unique_ptr<expr_any>& e : exprs_) {
        e->assert_var_not_used(var);
    }
}

// declared in 'expr_type_value.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
[[nodiscard]] auto expr_type_value::compile_lea(
    const token& src_loc_tk, toc& tc, std::ostream& os, size_t indent,
    std::vector<std::string>& allocated_registers, const std::string& reg_size,
    const std::span<const std::string> lea_path) const -> operand {

    return stmt_ident_->compile_lea(src_loc_tk, tc, os, indent,
                                    allocated_registers, reg_size, lea_path);
}

// declared in 'expr_type_value.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
[[nodiscard]] auto expr_type_value::identifier() const -> std::string_view {
    if (stmt_ident_) {
        return stmt_ident_->identifier();
    }
    return statement::identifier();
}

// declared in 'expr_type_value.hpp'
// solves circular reference: expr_type_value -> expr_any -> expr_type_value
[[nodiscard]] auto expr_type_value::is_indexed() const -> bool {
    return stmt_ident_ and stmt_ident_->is_indexed();
}

// declared in 'expr_array_assign.hpp'
expr_array_assign::expr_array_assign(toc& tc, tokenizer& tz, token tk,
                                     const type& tp)
    : statement{tk} {

    set_type(tp);

    ident_or_call_parts parts{parse_ident_or_call(tc, tk, tz, tp)};
    stmt_ident_ = std::move(parts.ident);
    stmt_call_ = std::move(parts.call);
}

// declared in 'expr_array_assign.hpp'
auto expr_array_assign::source_to(std::ostream& os) const -> void {
    if (stmt_call_) {
        stmt_call_->source_to(os);
        return;
    }
    stmt_ident_->source_to(os);
}

// declared in 'expr_array_assign.hpp'
auto expr_array_assign::compile(toc& tc, std::ostream& os, size_t indent,
                                const ident_info& dst_info) const -> void {
    if (stmt_call_) {
        stmt_call_->compile(tc, os, indent, dst_info);
        return;
    }

    operand op{dst_info.operand};
    copy_ident_bytes(tc, os, indent, *this, dst_info.type(), op);
}

// declared in 'expr_array_assign.hpp'
[[nodiscard]] auto expr_array_assign::is_indexed() const -> bool {
    return stmt_ident_ and stmt_ident_->is_indexed();
}

// declared in 'expr_array_assign.hpp'
[[nodiscard]] auto expr_array_assign::identifier() const -> std::string_view {
    if (stmt_ident_) {
        return stmt_ident_->identifier();
    }
    return statement::identifier();
}

// declared in 'expr_array_assign.hpp'
[[nodiscard]] auto expr_array_assign::compile_lea(
    const token& src_loc_tk, toc& tc, std::ostream& os, size_t indent,
    std::vector<std::string>& allocated_registers, const std::string& reg_size,
    const std::span<const std::string> lea_path) const -> operand {

    return stmt_ident_->compile_lea(src_loc_tk, tc, os, indent,
                                    allocated_registers, reg_size, lea_path);
}

// declared in 'unary_ops.hpp'
// solves circular reference: unary_ops -> toc -> statement -> unary_ops
auto unary_ops::compile([[maybe_unused]] toc& tc, std::ostream& os,
                        const size_t indnt,
                        const std::string_view dst_info) const -> void {

    for (const char op : ops_ | std::views::reverse) {
        switch (op) {
        case '~':
            toc::asm_not(os, indnt, dst_info);
            break;
        case '-':
            toc::asm_neg(os, indnt, dst_info);
            break;
        default:
            std::unreachable();
        }
    }
}

// NOLINTEND(misc-definitions-in-headers)
