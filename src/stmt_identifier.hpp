#pragma once

#include <algorithm>

#include "compiler_exception.hpp"
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

            // build a path of element identifiers later used to get the type of
            // the target element
            for (const identifier_elem& e : elems_) {
                if (not path_as_string_.empty()) {
                    path_as_string_ = path_as_string_ + ".";
                }
                path_as_string_ = path_as_string_ + e.name_tk.name();
            }

            if (tc.is_func(path_as_string_)) {
                //? todo: should get the type? not necessary right now but for
                // consistency
                break;
            }

            const ident_info ii{
                tc.make_ident_info(tok(), path_as_string_, false)};

            set_type(ii.type_ref);

            // done
            break;
        }
    }

    stmt_identifier() = default;
    stmt_identifier(const stmt_identifier&) = default;
    stmt_identifier(stmt_identifier&&) = default;
    auto operator=(const stmt_identifier&) -> stmt_identifier& = default;
    auto operator=(stmt_identifier&&) -> stmt_identifier& = default;

    ~stmt_identifier() override = default;

    [[nodiscard]] auto first_token() const -> const token& {
        return elems_.at(0).name_tk;
    }

    [[nodiscard]] auto identifier() const -> const std::string& override {
        return path_as_string_;
    }

    [[nodiscard]] auto elems() const -> const std::vector<identifier_elem>& {
        return elems_;
    }

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

        const ident_info dst_info{tc.make_ident_info(tok(), dst, false)};

        if (not is_expression()) {
            const ident_info src_info{
                tc.make_ident_info(tok(), identifier(), false)};
            tc.asm_cmd(tok(), os, indent, "mov", dst_info.id_nasm,
                       src_info.id_nasm);
            get_unary_ops().compile(tc, os, indent, dst_info.id_nasm);
            return;
        }

        const std::string& reg_offset{
            tc.alloc_scratch_register(tok(), os, indent)};

        stmt_identifier::compile_address_calculation(tok(), tc, os, indent,
                                                     elems(), reg_offset);

        const ident_info src_info{
            tc.make_ident_info(tok(), identifier(), false)};
        const std::string& memsize{
            type::get_memory_operand_for_size(tok(), src_info.type_ref.size())};
        tc.asm_cmd(tok(), os, indent, "mov", dst_info.id_nasm,
                   memsize + " [" + reg_offset + "]");
        get_unary_ops().compile(tc, os, indent, dst_info.id_nasm);
        tc.free_scratch_register(os, indent, reg_offset);
    }

    static auto
    compile_address_calculation(const token& tok, toc& tc, std::ostream& os,
                                size_t indent,
                                const std::vector<identifier_elem>& elems,
                                const std::string& reg_offset) -> void {

        const identifier_elem& base_elem{elems.front()};
        std::string path{base_elem.name_tk.name()};
        const ident_info base_info{tc.make_ident_info(tok, path, false)};

        tc.asm_cmd(tok, os, indent, "lea", reg_offset,
                   "[rsp - " + std::to_string(-base_info.stack_ix) + "]");

        size_t accum_offset = 0;

        for (size_t i{}; i < elems.size(); ++i) {
            const identifier_elem& elem{elems.at(i)};
            const ident_info curr_info{tc.make_ident_info(tok, path, false)};

            if (elem.has_array_index_expr) {
                const std::string& reg_index{
                    tc.alloc_scratch_register(tok, os, indent)};
                elem.array_index_expr->compile(tc, os, indent, reg_index);
                tc.asm_cmd(tok, os, indent, "imul", reg_index,
                           std::to_string(curr_info.type_ref.size()));
                tc.asm_cmd(tok, os, indent, "add", reg_offset, reg_index);
                tc.free_scratch_register(os, indent, reg_index);
            }

            if (i + 1 < elems.size()) {
                const identifier_elem& next_elem{elems[i + 1]};
                accum_offset += toc::get_field_offset_in_type(
                    tok, curr_info.type_ref, next_elem.name_tk.name());
                path += "." + next_elem.name_tk.name();
            }
        }

        if (accum_offset != 0) {
            tc.asm_cmd(tok, os, indent, "add", reg_offset,
                       std::to_string(accum_offset));
        }
    }
};
