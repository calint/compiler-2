#pragma once

#include <format>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "compile_error.hpp"
#include "ident_info.hpp"
#include "statement.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_alias.hpp"
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
//                  operands are compiled
//   'assert_not_written_after'
//                  where a backend computes an operand before ones that come
//                  earlier in the source, e.g. the count of 'array_copy', the
//                  earlier ones write nothing it reads
//
// the definite assignment ('ub_assigned.hpp') walks the statements in the same
// order by name, it runs before 'toc' resolves the names

// a read resolved to the storage it reaches, e.g. 'p.x' with 'p' bound to
// 'ps[i]' reaches the field 'x' of any element of 'ps'
struct storage_read {
    // the root name of the read, where an error points
    token src_loc_tk;
    // the read as written
    std::string_view read_text;
    // the path from the root name as written
    access_path read_path;
    storage_path storage;
};

using storage_read_visitor = std::function_ref<void(const storage_read& read)>;

// a 'read_visitor' that hands each read that reaches storage to 'visitor', a
// constant or a value in a register reaches none
[[nodiscard]] inline auto storage_reader(const toc& tc,
                                         const storage_read_visitor visitor) {

    return [&tc, visitor](const token& src_loc_tk,
                          const std::string_view read_text,
                          [[maybe_unused]] const byte_range& accessed_bytes,
                          const access_path& read_path) -> void {
        const std::optional<storage_path> storage{
            tc.storage_of(src_loc_tk.text(), read_path),
        };

        if (not storage) {
            return;
        }

        visitor({
            .src_loc_tk{src_loc_tk},
            .read_text{read_text},
            .read_path{read_path},
            .storage{*storage},
        });
    };
}

// every read of 'st' that reaches storage
inline auto visit_storage_reads(const toc& tc, const statement& st,
                                const storage_read_visitor visitor) -> void {

    st.visit_reads(std::nullopt, storage_reader(tc, visitor));
}

// 'st' reads 'dst', or a call in it reads or writes it; a register has no
// storage and is reached under its own name, storage by the bytes a read
// reaches under any name, e.g. the 'p' of 'g = p + 1' with 'p' bound to 'g',
// but not the 'p.y' of 'p.x = p.y + 1', and by the storage the body of a call
// reaches, e.g. 'gw()' writing 'gx' in 'gx = k + gw()'
[[nodiscard]] inline auto reaches_destination(const toc& tc,
                                              const statement& st,
                                              const destination& dst) -> bool {

    if (not dst.storage()) {
        return st.reads_var(dst.root_id());
    }

    bool is_read{};

    visit_storage_reads(tc, st, [&](const storage_read& read) -> void {
        if (read.storage.may_overlap(*dst.storage())) {
            is_read = true;
        }
    });

    st.visit_call_effects(tc, [&](const call_effect& effect) -> void {
        if (effect.storage.may_overlap(*dst.storage())) {
            is_read = true;
        }
    });

    return is_read;
}

// an operand of a list that 'assert_not_written_later' checks, e.g. an
// argument of a call
struct list_operand {
    const statement* node{};
    // the storage the list writes when the operand is one, it reads only its
    // indexes, e.g. the buffer of 'read' or the destination of 'array_copy';
    // null when the operand is read
    const stmt_identifier* written_place{};

    // every read that reaches storage
    auto visit_storage_reads(const toc& tc,
                             const storage_read_visitor visitor) const -> void {

        if (written_place != nullptr) {
            written_place->visit_index_reads(std::nullopt,
                                             storage_reader(tc, visitor));

            return;
        }

        ::visit_storage_reads(tc, *node, visitor);
    }
};

// 'earlier' reads nothing that 'write', an effect of an operand computed
// after it, reaches; 'writer' says where the call is in the message, e.g. "a
// later argument"
inline auto assert_not_read_before(const toc& tc, const list_operand& earlier,
                                   const call_effect& write,
                                   const std::string_view writer) -> void {

    earlier.visit_storage_reads(tc, [&](const storage_read& read) -> void {
        if (not read.storage.may_overlap(write.storage)) {
            return;
        }

        throw compile_error{
            read.src_loc_tk,
            std::format("{} may be written by the call of {} in {}, use a "
                        "temporary variable",
                        in_quotes(read.read_text), in_quotes(write.call_tk),
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

// a backend that computes 'computed_first' before 'computed_after', which come
// earlier in the source, gives the left-to-right result only when no call in
// 'computed_after' writes what 'computed_first' reads, e.g. the 'n' of
// 'array_copy(a[g()], b, n)' with 'g' writing 'n'; 'writer' says where the
// call is in the message
inline auto
assert_not_written_after(const toc& tc,
                         const std::span<const list_operand> computed_first,
                         const std::span<const list_operand> computed_after,
                         const std::string_view writer) -> void {

    for (const list_operand& after : computed_after) {
        after.node->visit_call_effects(
            tc, [&](const call_effect& effect) -> void {
                if (not effect.is_write) {
                    return;
                }

                for (const list_operand& first : computed_first) {
                    assert_not_read_before(tc, first, effect, writer);
                }
            });
    }
}
