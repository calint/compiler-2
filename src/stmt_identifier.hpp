#pragma once

#include <algorithm>

#include "compiler_exception.hpp"
#include "decouple2.hpp"
#include "expr_any.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "unary_ops.hpp"

class stmt_identifier : public statement {
    std::vector<identifier_elem> elems_;
    token ws1_;
    token ws2_;
    std::string path_as_string_;

  public:
    stmt_identifier(toc& tc, tokenizer& tz, token tk, unary_ops uops = {})
        : statement{tz.next_whitespace_token(), std::move(uops)} {

        while (true) {
            // has array index?
            if (tz.is_next_char('[')) {
                // yes, read expression

                // note: array index expression always results into an
                //       integer, thus default type
                elems_.emplace_back(identifier_elem{
                    .name_tk = tk,
                    .array_index_expr{std::make_unique<expr_any>(
                        tc, tz, tc.get_type_default(), false)},
                    .ws1{tz.next_whitespace_token()},
                    .has_array_index_expr = true});

                if (not tz.is_next_char(']')) {
                    throw compiler_exception{
                        tz, "expected '[' to close array index expression"};
                }
            } else {
                // no array indexing
                elems_.emplace_back(
                    identifier_elem{.name_tk = tk,
                                    .array_index_expr{},
                                    .ws1{tz.next_whitespace_token()},
                                    .has_array_index_expr = false});
            }

            if (tz.is_next_char('.')) {
                tk = tz.next_token();
                continue;
            }

            ws2_ = tz.next_whitespace_token();

            // get the type at the end of path
            for (const identifier_elem& e : elems_) {
                if (not path_as_string_.empty()) {
                    path_as_string_ = path_as_string_ + ".";
                }
                path_as_string_ = path_as_string_ + e.name_tk.name();
            }

            if (tc.is_func(path_as_string_)) {
                break;
            }

            const ident_info ii{
                tc.make_ident_info(tok(), path_as_string_, false)};

            set_type(ii.type_ref);

            // done
            break;
        }
    }

    [[nodiscard]] auto first_token() const -> const token& {
        return elems_.at(0).name_tk;
    }

    [[nodiscard]] auto identifier() const -> const std::string& override {
        return path_as_string_;
    }

    [[nodiscard]] auto elems() const -> const std::vector<identifier_elem>& {
        return elems_;
    }

    stmt_identifier() = default;
    stmt_identifier(const stmt_identifier&) = default;
    stmt_identifier(stmt_identifier&&) = default;
    auto operator=(const stmt_identifier&) -> stmt_identifier& = default;
    auto operator=(stmt_identifier&&) -> stmt_identifier& = default;

    ~stmt_identifier() override = default;

    [[nodiscard]] auto is_expression() const -> bool override {
        return std::ranges::any_of(elems_, [](const identifier_elem& e) {
            return e.has_array_index_expr;
        });
    }

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        int sep{0};
        for (const identifier_elem& e : elems_) {
            if (sep++) {
                os << '.';
            }
            e.name_tk.source_to(os);
            if (e.has_array_index_expr) {
                os << '[';
                e.array_index_expr->source_to(os);
                os << ']';
            }
            e.ws1.source_to(os);
        }
        ws2_.source_to(os);
    }

    auto compile(toc& tc, std::ostream& os, size_t indent,
                 const std::string& dst = "") const -> void override {

        if (dst.empty()) {
            return;
        }

        tc.comment_source(*this, os, indent);

        if (not is_expression()) {
            // no index calculations
            const ident_info di{tc.make_ident_info(tok(), dst, false)};
            const ident_info ii{tc.make_ident_info(tok(), identifier(), false)};
            tc.asm_cmd(tok(), os, indent, "mov", di.id_nasm, ii.id_nasm);
            get_unary_ops().compile(tc, os, indent, di.id_nasm);
            return;
        }

        if (elems().size() == 1) {
            const identifier_elem& ie{elems().at(0)};
            const ident_info ii{tc.make_ident_info(tok(), identifier(), false)};
            const ident_info di{tc.make_ident_info(tok(), dst, false)};
            if (not ie.has_array_index_expr) {
                tc.asm_cmd(tok(), os, indent, "mov", di.id_nasm, ii.id_nasm);
                get_unary_ops().compile(tc, os, indent, di.id_nasm);
                return;
            }

            const std::string& reg_accum{
                tc.alloc_scratch_register(tok(), os, indent)};
            tc.asm_cmd(tok(), os, indent, "lea", reg_accum,
                       "[rsp - " + std::to_string(-ii.stack_ix) + "]");
            const std::string& reg_index{
                tc.alloc_scratch_register(tok(), os, indent)};
            ie.array_index_expr->compile(tc, os, indent, reg_index);
            tc.asm_cmd(tok(), os, indent, "imul", reg_index,
                       std::to_string(ii.type_ref.size()));
            tc.asm_cmd(tok(), os, indent, "add", reg_accum, reg_index);
            tc.free_scratch_register(os, indent, reg_index);
            const std::string& memsize{
                type::get_memory_operand_for_size(tok(), ii.type_ref.size())};
            tc.asm_cmd(tok(), os, indent, "mov", dst,
                       memsize + " [" + reg_accum + "]");
            tc.free_scratch_register(os, indent, reg_accum);
            get_unary_ops().compile(tc, os, indent, di.id_nasm);
            return;
        }

        // identifier contains index offsets

        const identifier_elem& ie_base{elems_.at(0)};
        const std::string& reg_accum{
            tc.alloc_scratch_register(tok(), os, indent)};
        std::string path = ie_base.name_tk.name();
        const ident_info ii_base{tc.make_ident_info(tok(), path, false)};
        tc.asm_cmd(tok(), os, indent, "lea", reg_accum,
                   "[rsp - " + std::to_string(-ii_base.stack_ix) + "]");
        size_t i{0};
        size_t accum_offset{};
        while (true) {
            if (i + 1 == elems_.size()) {
                break;
            }
            const identifier_elem& ie{elems_.at(i)};
            const identifier_elem& ie_nxt{elems_.at(i + 1)};
            const ident_info ii{tc.make_ident_info(tok(), path, false)};
            const std::string path_nxt{path + "." + ie_nxt.name_tk.name()};
            const ident_info ii_nxt{tc.make_ident_info(tok(), path_nxt, false)};
            if (not ie.has_array_index_expr) {
                accum_offset += toc::get_field_offset_in_type(
                    tok(), ii.type_ref, ie_nxt.name_tk.name());
                path = path_nxt;
                i++;
                continue;
            }
            const std::string& reg_index{
                tc.alloc_scratch_register(tok(), os, indent)};
            ie.array_index_expr->compile(tc, os, indent, reg_index);
            tc.asm_cmd(tok(), os, indent, "imul", reg_index,
                       std::to_string(ii.type_ref.size()));
            tc.asm_cmd(tok(), os, indent, "add", reg_accum, reg_index);
            tc.free_scratch_register(os, indent, reg_index);
            accum_offset += toc::get_field_offset_in_type(
                tok(), ii.type_ref, ie_nxt.name_tk.name());
            path = path_nxt;
            i++;
        }
        if (accum_offset != 0) {
            tc.asm_cmd(tok(), os, indent, "add", reg_accum,
                       std::to_string(accum_offset));
        }
        const ident_info dst_info{tc.make_ident_info(tok(), dst, false)};
        const ident_info ii{tc.make_ident_info(tok(), identifier(), false)};
        const std::string& memsize{
            type::get_memory_operand_for_size(tok(), ii.type_ref.size())};
        tc.asm_cmd(tok(), os, indent, "mov", dst_info.id_nasm,
                   memsize + " [" + reg_accum + "]");
        get_unary_ops().compile(tc, os, indent, dst_info.id_nasm);
        tc.free_scratch_register(os, indent, reg_accum);
    }
};
