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

                // note: array index expression always results into an integer,
                // thus default type
                elems_.emplace_back(identifier_elem{
                    .name_tk = tk,
                    .array_index_expr =
                        new expr_any(tc, tz, tc.get_type_default(), false),
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

        const identifier_elem& el{elems_.at(0)};

        if (el.has_array_index_expr) {
            const ident_info& dst_info{tc.make_ident_info(*this, false)};
            const std::string& reg{
                tc.alloc_scratch_register(tok(), os, indent)};
            el.array_index_expr->compile(tc, os, indent, reg);
            const size_t dst_type_size{dst_info.type_ref.size()};
            std::string reg_scaled;
            switch (dst_type_size) {
            case 1:
                reg_scaled = reg;
                break;
            case 2:
                reg_scaled = reg + " * 2";
                break;
            case 4:
                reg_scaled = reg + " * 4";
                break;
            case 8:
                reg_scaled = reg + " * 8";
                break;
            default:
                tc.asm_cmd(tok(), os, indent, "imul", reg,
                           std::to_string(dst_type_size));
                reg_scaled = reg;
                break;
            }
            tc.asm_cmd(tok(), os, indent, "lea", reg,
                       "[rsp + " + reg_scaled + " - " +
                           std::to_string(-dst_info.stack_ix) + "]");
            // note: -dst_info.stack_ix for nicer source formatting
            const std::string& memsize{
                type::get_memory_operand_for_size(tok(), dst_type_size)};
            const std::string src_nasm{memsize + " [" + reg + "]"};
            tc.asm_cmd(tok(), os, indent, "mov", dst, src_nasm);
            get_unary_ops().compile(tc, os, indent, dst);
            tc.free_scratch_register(os, indent, reg);
            return;
        }

        // not an index into array

        const ident_info& src_info{tc.make_ident_info(*this, false)};
        tc.asm_cmd(tok(), os, indent, "mov", dst, src_info.id_nasm);
    }
};
