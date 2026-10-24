#pragma once

#include <cassert>
#include <cstddef>
#include <format>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compile_error.hpp"
#include "expr_any.hpp"
#include "ident_info.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_assigned.hpp"
#include "ub_order.hpp"
#include "ub_reads.hpp"

// 'dst = value' and the initializer of a declaration: the value is parsed as
// the type of the destination, which types its literals, and is written
// into the destination part by part, so a part that reads a part already
// written is rejected ('assert_parts_not_clobbered')
class stmt_assign_var final : public statement {
    stmt_identifier stmt_ident_;
    expr_any expr_;
    token equals_tk_;

    // a part of the value: its storage and its path under the destination's
    // name
    struct written_place {
        storage_path storage;
        access_path path;
    };

  public:
    stmt_assign_var(toc& tc, tokenizer& tz, stmt_identifier si,
                    const token equals_tk)
        : statement{si.tok()}, stmt_ident_{std::move(si)},
          equals_tk_{equals_tk} {

        const ident_info dst_info{make_ident_info(tc, stmt_ident_)};

        stmt_ident_.record_write(tc, tok(), "assign to", dst_info);

        set_type(dst_info.type_ref());

        // the value is parsed as the type of the destination, which gives the
        // type of a literal, e.g. 'a = {1, 2}'
        expr_ = {tc, tz, dst_info.type_ref(), literal_typing::from_destination,
                 stmt_ident_.shape()};
    }

    // the initializer of a declaration, parsed before the variable was
    // declared; a new variable is writable, so the read-only check of an
    // assignment does not apply
    stmt_assign_var(const toc& tc, stmt_identifier si, const token equals_tk,
                    expr_any value)
        : statement{si.tok()}, stmt_ident_{std::move(si)},
          expr_{std::move(value)}, equals_tk_{equals_tk} {

        set_type(make_ident_info(tc, stmt_ident_).type_ref());
    }

    stmt_assign_var() = default;

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

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        const std::optional<storage_path> dst_storage{
            stmt_ident_.storage(tc),
        };

        const ident_info var_info{make_ident_info(tc, stmt_ident_)};

        assert_parts_not_clobbered(tc, dst_storage, var_info.array_len());

        // note: the parse rejected a constant destination
        assert(not var_info.is_const());

        // an unsized array parameter has a size only here
        assert_array_size_matches(tc, var_info);

        std::vector<operand> lea_registers;

        const operand address{
            stmt_ident_.get_lea_operand(tc, indent, var_info, lea_registers),
        };

        // the destination is written at the address computed here
        const bool is_final_address{
            stmt_ident_.has_computed_address(var_info),
        };

        const destination var_dst{
            destination::of_name(var_info,
                                 {
                                     .address{address},
                                     .is_final_address{is_final_address},
                                     .storage{dst_storage},
                                     .text{stmt_ident_.path_text()},
                                 }),
        };

        expr_.compile(tc, indent, var_dst);
        x.free_scratch_registers(tok(), indent, lea_registers);
    }

    // the value is read before the destination is written
    auto trace_assignment(assignment_flow& flow) const -> void override {
        assert_var_not_used(flow);
        stmt_ident_.record_assignment(flow);
    }

    auto visit_reads(const read_filter var, const read_visitor reader) const
        -> void override {

        expr_.visit_reads(var, reader);
        stmt_ident_.visit_index_reads(var, reader);
    }

    //
    // class methods
    //

    [[nodiscard]] auto expression() const -> const expr_any& { return expr_; }

  private:
    // an array copy has the size of the destination; an unsized array
    // parameter is checked when its call gives it a size
    auto assert_array_size_matches(const toc& tc,
                                   const ident_info& dst_info) const -> void {

        if (not expr_.is_array_copy()) {
            return;
        }

        const ident_info src_info{make_ident_info(tc, *expr_.place_path())};

        const std::optional<size_t> src_len{src_info.array_len()};
        const std::optional<size_t> dst_len{dst_info.array_len()};

        if (not src_len or not dst_len or *src_len == *dst_len) {
            return;
        }

        throw compile_error{
            expr_.tok(),
            std::format("destination array size {} does not match source "
                        "size {}",
                        *dst_len, *src_len)};
    }

    // 'item' reads none of the 'written' parts, under any name, nor does a call
    // in it read or write one
    auto assert_no_read_of_written_part(
        const toc& tc, const statement& item,
        const std::span<const written_place> written) const -> void {

        visit_storage_reads(tc, item, [&](const storage_read& read) -> void {
            for (const written_place& place : written) {
                if (not read.storage.may_overlap(place.storage)) {
                    continue;
                }

                if (is_certain_overwrite(read, place)) {
                    throw compile_error{
                        read.src_loc_tk,
                        std::format("{} is read but has been overwritten in "
                                    "the same statement",
                                    in_quotes(read.read_text))};
                }

                throw compile_error{
                    read.src_loc_tk,
                    std::format("{} may have been overwritten in the same "
                                "statement",
                                in_quotes(read.read_text))};
            }
        });

        // e.g. 'gr()' of 'g = {1, gr()}' with 'gr' reading 'g.x'
        item.visit_call_effects(tc, [&](const call_effect& effect) -> void {
            for (const written_place& place : written) {
                if (not effect.storage.may_overlap(place.storage)) {
                    continue;
                }

                throw compile_error{
                    effect.call_tk,
                    std::format("the call of {} reaches {}, which has been "
                                "written in the same statement, use a "
                                "temporary variable",
                                in_quotes(effect.call_tk),
                                in_quotes(effect.name))};
            }
        });
    }

    // the value is written part by part, so a part must not read what is
    // already written, e.g. the 'p.x' of 'p = {p.y, p.x}'; 'unsized_count'
    // is the size of an unsized array destination
    auto assert_parts_not_clobbered(
        const toc& tc, const std::optional<storage_path>& dst,
        const std::optional<size_t> unsized_count) const -> void {

        // note: a value in a register is written as one part that no other
        //       name reaches

        if (not dst) {
            return;
        }

        write_plan plan;
        expr_.plan_writes(tc, part_location{}, unsized_count, plan);

        // the parts written by the steps before the current one
        std::vector<written_place> written;

        for (const write_step& step : plan) {
            if (const value_write* const value{
                    std::get_if<value_write>(&step.action),
                }) {

                assert_no_read_of_written_part(tc, *value->item, written);
            }

            for (const access_path& part : step.covered) {
                written.push_back({
                    .storage{
                        .root{dst->root},
                        .path{dst->path.extended_by(part)},
                    },
                    .path{stmt_ident_.accessed_path().extended_by(part)},
                });
            }
        }
    }

    // the read is of the written bytes, not only maybe: no run-time index
    // on either side, or none under the destination's name, which reaches
    // the bytes through the same binding, e.g. 'p.x' of 'p = {1, p.x}' with
    // 'p' bound to 'ps[i]'
    [[nodiscard]] auto is_certain_overwrite(const storage_read& read,
                                            const written_place& place) const
        -> bool {

        if (not read.storage.path.has_run_time_index() and
            not place.storage.path.has_run_time_index()) {

            return true;
        }

        return read.src_loc_tk.text() == stmt_ident_.first_token().text() and
               not read.read_path.has_run_time_index() and
               not place.path.has_run_time_index();
    }
};
