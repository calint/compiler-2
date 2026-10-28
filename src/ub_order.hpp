#pragma once

#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "access_path.hpp"
#include "compile_error.hpp"
#include "ident_info.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "ub_reads.hpp"

// reads against writes: a statement is checked in the order its code runs, a
// read only against what was written before it; each check names the order it
// relies on and where that order is emitted
//
//   'stmt_assign_var::assert_parts_not_clobbered'
//                  a value written in parts reads no part written before it;
//                  the order of the 'write_plan' steps, which the compile of
//                  the value follows ('expr_any::plan_writes')
//   'reaches_destination'
//                  an expression computed in its destination reads nothing of
//                  it, else it is computed in a scratch register first
//                  ('expr_arith', 'expr_bool')
//   'assert_not_written_later'
//                  the operands of a list read nothing a call in a later
//                  operand writes; the list order, in which the arguments and
//                  operands are compiled, of a node ('operands_of'): a call, a
//                  list, a comparison and the bulk and io built-ins
//
// the definite assignment ('ub_assigned.hpp') walks the statements in the same
// order by name, it runs before 'toc' resolves the names; the aliasing of the
// arguments of a call is not an order: 'stmt_call::assert_no_shared_storage';
// every check by the undefined behavior it rules out: README 8.1

// 'st' reads 'dst', or a call in it reads or writes it; a register has no
// storage and is reached under its own name, storage by the bytes a read
// reaches under any name, e.g. the 'p' of 'g = p + 1' with 'p' bound to 'g',
// but not the 'p.y' of 'p.x = p.y + 1', and by the storage the body of a call
// reaches, e.g. 'gw()' writing 'gx' in 'gx = k + gw()'
[[nodiscard]] inline auto reaches_destination(const toc& tc,
                                              const statement& st,
                                              const destination& dst) -> bool {

    const std::optional<storage_path> dst_storage{dst.storage()};

    if (not dst_storage) {
        return st.first_read_of(dst.root_name()) != nullptr;
    }

    bool is_read{};

    st.visit_reads([&](const stmt_identifier& read) -> void {
        if (read.may_reach(tc, *dst_storage)) {
            is_read = true;
        }
    });

    st.visit_call_effects(tc, [&](const call_effect& effect) -> void {
        if (effect.storage.may_overlap(*dst_storage)) {
            is_read = true;
        }
    });

    return is_read;
}

// 'earlier' reads nothing that 'write', an effect of an operand computed
// after it, reaches; 'writer' says where the call is in the message, e.g. "a
// later argument"
inline auto assert_not_read_before(const toc& tc, const list_operand& earlier,
                                   const call_effect& write,
                                   const std::string_view writer) -> void {

    visit_operand_reads(earlier, [&](const stmt_identifier& read) -> void {
        if (not read.may_reach(tc, write.storage)) {
            return;
        }

        throw compile_error{
            read.tok(),
            std::format("{} may be written by the call of {} in {}, use a "
                        "temporary variable",
                        in_quotes(read.path_text()), in_quotes(write.call_tk),
                        writer)};
    });
}

// evaluation is left to right, but the compile reads a name where it is
// used, so a name that a later operand of the same list may write would read
// the written value; such a list is rejected, e.g. the 'i' of 'f(arr[i],
// h(i))' or of 'i == h(i)' with 'h' writing 'i'; 'operand' names an operand
// of the list in the message, e.g. "argument"
// note: a call among the earlier operands has run before the later ones,
//       only the names those read are checked
inline auto
assert_not_written_later(const toc& tc,
                         const std::span<const list_operand> operands,
                         const std::string_view operand) -> void {

    const std::string writer{std::format("a later {}", operand)};

    for (size_t k{1}; k < operands.size(); ++k) {
        operands.at(k).node->visit_call_effects(
            tc, [&](const call_effect& effect) -> void {
                if (not effect.is_write) {
                    return;
                }

                for (const list_operand& earlier : operands.first(k)) {
                    assert_not_read_before(tc, earlier, effect, writer);
                }
            });
    }
}

// the operands of 'node' in its 'visit_operands' order; a 'statement' or a
// part of one that lists operands the same way, e.g. 'bulk_operands'
[[nodiscard]] inline auto operands_of(const auto& node)
    -> std::vector<list_operand> {

    std::vector<list_operand> operands;

    node.visit_operands(
        [&operands](const list_operand& o) -> void { operands.push_back(o); });

    return operands;
}
