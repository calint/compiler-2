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
//   'assert_written_not_reached'
//                  a value written in parts reads no part written before it;
//                  the order of the 'write_plan' steps, the plan the compile
//                  emits ('expr_any::plan_writes'), walked by
//                  'stmt_assign_var::assert_parts_not_clobbered'
//   'reaches_destination'
//                  an expression computed in its destination reads nothing of
//                  it, else it is computed in a scratch register first
//                  ('expr_arith', 'expr_bool')
//   'assert_not_written_later'
//                  the operands of a list read nothing a call in a later
//                  operand writes; the list order, in which the arguments and
//                  operands are compiled, of a node ('visit_operands'): a call,
//                  a comparison and the bulk and io built-ins; an arithmetic
//                  list reads each operand at its place, a later write misses
//                  it
//
// the definite assignment ('ub_assigned.hpp') walks the statements in the same
// order by name, it runs before 'toc' resolves the names; the aliasing of the
// arguments of a call is not an order: 'stmt_call::assert_no_shared_storage';
// every check by the undefined behavior it rules out: README 8.1

// 'st' reads 'dst', or a call in it reads or writes it; a name in a register
// has no storage and is reached under its own name, a register the compiler
// allocated under none; storage by the bytes a read reaches under any name,
// e.g. the 'e' of 'a[0] = 7 - e' in a 'foo a', but not the 'p.y' of
// 'p.x = p.y + 1', and by the storage the body of a call reaches
[[nodiscard]] inline auto reaches_destination(const toc& tc,
                                              const statement& st,
                                              const destination& dst) -> bool {

    bool is_read{};

    if (dst.is_named_register()) {
        // a path such as 'p.y' reads its root variable
        st.visit_reads([&](const stmt_identifier& read) -> void {
            is_read = is_read or read.tok().is_text(dst.text());
        });

        return is_read;
    }

    const std::optional<storage_path> dst_storage{dst.storage()};

    if (not dst_storage) {
        return false;
    }

    st.visit_reads([&](const stmt_identifier& read) -> void {
        is_read = is_read or read.may_reach(tc, *dst_storage);
    });

    st.visit_call_effects(tc, [&](const call_effect& effect) -> void {
        is_read = is_read or effect.storage.may_overlap(*dst_storage);
    });

    return is_read;
}

// a read of the statement whose parts 'written' are reaches none of them
inline auto assert_read_not_written(const toc& tc, const stmt_identifier& read,
                                    const std::span<const storage_path> written)
    -> void {

    // a constant or a value in a register reaches no storage
    const std::optional<storage_path> reached{read.storage(tc)};

    if (not reached) {
        return;
    }

    for (const storage_path& place : written) {
        if (not reached->may_overlap(place)) {
            continue;
        }

        // note: without a run-time index on either side the read is of the
        //       written bytes, not only maybe

        if (not reached->path.has_run_time_index() and
            not place.path.has_run_time_index()) {

            throw compile_error{
                read.tok(),
                std::format("{} is read but has been overwritten in the same "
                            "statement, use a temporary variable",
                            in_quotes(read.path_text()))};
        }

        throw compile_error{
            read.tok(),
            std::format("{} may have been overwritten in the same statement, "
                        "use a temporary variable",
                        in_quotes(read.path_text()))};
    }
}

// a call in the statement whose parts 'written' are reaches none of them
inline auto
assert_call_effect_not_written(const call_effect& effect,
                               const std::span<const storage_path> written)
    -> void {

    for (const storage_path& place : written) {
        if (not effect.storage.may_overlap(place)) {
            continue;
        }

        throw compile_error{
            effect.call_tk,
            std::format("the call of {} reaches {}, which has been written in "
                        "the same statement, use a temporary variable",
                        in_quotes(effect.call_tk.text()),
                        in_quotes(effect.name))};
    }
}

// 'item', computed after the parts 'written' are, reaches none of them as
// 'reaches_destination' asks of a destination: it reads none under any name,
// nor does a call in it read or write one, e.g. the 'gr()' of 'g = {1, gr()}'
// with 'gr' reading 'g.x'
inline auto
assert_written_not_reached(const toc& tc, const statement& item,
                           const std::span<const storage_path> written)
    -> void {

    item.visit_reads([&](const stmt_identifier& read) -> void {
        assert_read_not_written(tc, read, written);
    });

    item.visit_call_effects(tc, [&](const call_effect& effect) -> void {
        assert_call_effect_not_written(effect, written);
    });
}

// 'earlier' reads nothing that 'write', an effect of an operand computed
// after it, reaches; 'writer' says where the call is in the message, e.g. "a
// later argument"
inline auto assert_not_read_before(const toc& tc, const node_operand& earlier,
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
                        in_quotes(read.path_text()),
                        in_quotes(write.call_tk.text()), writer)};
    });
}

// evaluation is left to right, but the compile reads a name where it is
// used, so a name that a later operand of the same list may write would read
// the written value; such a list is rejected, e.g. the 'i' of 'f(arr[i],
// h(i))' or of 'i == h(i)' with 'h' writing 'i'; 'node' lists its operands by
// 'visit_operands', a 'statement' or a part of one; 'operand' is the word the
// message uses for an operand of the list
// note: a call among the earlier operands has run before the later ones,
//       only the names those read are checked
inline auto assert_not_written_later(const toc& tc, const auto& node,
                                     const std::string_view operand) -> void {

    std::vector<node_operand> operands;

    node.visit_operands(
        [&operands](const node_operand& o) -> void { operands.push_back(o); });

    const std::string writer{std::format("a later {}", operand)};

    for (size_t k{1}; k < operands.size(); ++k) {
        operands.at(k).node->visit_call_effects(
            tc, [&](const call_effect& effect) -> void {
                if (not effect.is_write) {
                    return;
                }

                for (const node_operand& earlier :
                     std::span{operands}.first(k)) {

                    assert_not_read_before(tc, earlier, effect, writer);
                }
            });
    }
}
