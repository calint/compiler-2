#pragma once

#include <cassert>
#include <optional>
#include <ostream>
#include <utility>

#include "expr_any.hpp"
#include "ident_info.hpp"
#include "machine.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_assigned.hpp"
#include "ub_reads.hpp"

// 'dst = value' and the initializer of a declaration: the value is parsed as
// the type of the destination, which types its literals, and is written
// into the destination part by part, so a part that reads a part already
// written is rejected ('expr_any::checked_plan')
class stmt_assign_var final : public statement {
    stmt_identifier stmt_ident_;
    expr_any expr_;
    token equals_tk_;

  public:
    stmt_assign_var(parse_toc& tc, tokenizer& tz, stmt_identifier ident,
                    const token& equals_tk)
        : statement{ident.tok()}, stmt_ident_{std::move(ident)},
          expr_{parse_value(tc, tz, stmt_ident_)}, equals_tk_{equals_tk} {}

    // the initializer of a declaration, parsed before the variable was
    // declared; a new variable is writable, so the read-only check of an
    // assignment does not apply
    stmt_assign_var(stmt_identifier ident, const token& equals_tk,
                    expr_any value)
        : statement{ident.tok()}, stmt_ident_{std::move(ident)},
          expr_{std::move(value)}, equals_tk_{equals_tk} {}

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        // all the source info is in 'stmt_ident_', not in this statement's
        // token
        stmt_ident_.source_to(os);
        equals_tk_.source_to(os);
        expr_.source_to(os);
    }

    auto compile(compile_toc& tc, [[maybe_unused]] const destination& dst) const
        -> void override {

        comment_source(tc);

        const std::optional<storage_path> dst_storage{
            stmt_ident_.storage(tc),
        };

        const ident_info dst_info{make_ident_info(tc, stmt_ident_)};

        const write_plan plan{
            expr_.checked_plan(tc, dst_storage, dst_info.array_len()),
        };

        // note: the parse rejected a constant destination
        assert(not dst_info.is_const());

        machine& x{tc.machine()};

        machine::held_registers held{x, {tok(), tc.indent()}};

        // the destination is written at the address computed here
        const ident_info written{
            stmt_ident_.info_at_address(tc, dst_info, held),
        };

        const destination var_dst{
            destination::of_name(written,
                                 {
                                     .storage{dst_storage},
                                     .text{stmt_ident_.path_text()},
                                 }),
        };

        emit_write_plan(tc, plan, var_dst);
    }

    // the value is read before the destination is written
    auto trace_assignment(assignment_flow& flow) const -> void override {
        assert_result_set_before_read(flow);
        stmt_ident_.record_assignment(flow);
    }

    // as 'compile' emits it: the indexes of the destination, its address
    // computed first, then the value; the destination is written last
    auto visit_operands(const operand_visitor visitor) const -> void override {
        visitor({
            .node{&stmt_ident_},
            .index_only_place{&stmt_ident_},
        });

        visitor(node_operand::read(expr_));
    }

    //
    // class methods
    //

    [[nodiscard]] auto value() const -> const expr_any& { return expr_; }

  private:
    //
    // statics
    //

    // the write is recorded before the value parses; the value is parsed as
    // the type of the destination, which gives the type of a literal, e.g.
    // 'a = {1, 2}'
    [[nodiscard]] static auto parse_value(parse_toc& tc, tokenizer& tz,
                                          const stmt_identifier& dst)
        -> expr_any {

        dst.record_write(tc, "assign to");

        return {tc, tz, dst.get_type(), literal_typing::from_destination,
                dst.shape()};
    }
};
