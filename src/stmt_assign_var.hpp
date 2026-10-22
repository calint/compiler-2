#pragma once

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

        assert_not_read_only(tok(), "assign to", stmt_ident_.name_or_literal(),
                             dst_info);

        tc.record_use(ident_info::root_of(stmt_ident_.name_or_literal()),
                      stmt_ident_.accessed_path(), true);

        set_type(dst_info.type_ref());

        // the value is parsed as the type of the destination, which gives the
        // type of a literal, e.g. 'a = {1, 2}'
        const std::optional<size_t> array_count{
            stmt_ident_.is_array()
                ? std::optional{stmt_ident_.array_count().value_or(
                      unsized_array_count)}
                : std::nullopt,
        };

        expr_ = {tc, tz, dst_info.type_ref(), literal_typing::from_destination,
                 array_count};
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
            tc.storage_of(stmt_ident_.first_token().text(),
                          stmt_ident_.accessed_path()),
        };

        assert_parts_not_clobbered(tc, dst_storage);

        const ident_info var_info{make_ident_info(tc, stmt_ident_)};

        // a parameter names its argument only here, e.g. a constant
        assert_assignable(tc, var_info);

        std::vector<operand> lea_registers;

        const operand address{
            stmt_ident_.get_lea_operand(tc, indent, var_info, lea_registers),
        };

        // an indexed element or a loaded pointer is written at the address
        // computed here
        const bool is_address_known{
            stmt_ident_.is_indexed() or var_info.is_pointer(),
        };

        const destination var_dst{
            destination::of_name(var_info,
                                 {
                                     .address{address},
                                     .is_address_known{is_address_known},
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
    // the destination is not a constant and an array copy has the size of
    // the destination; an unsized array parameter is checked when its call
    // gives it a size
    auto assert_assignable(const toc& tc, const ident_info& dst_info) const
        -> void {

        if (dst_info.is_const()) {
            throw compile_error{tok(),
                                std::format("cannot assign to constant '{}'",
                                            stmt_ident_.name_or_literal())};
        }

        if (not expr_.is_array_identifier()) {
            return;
        }

        const ident_info src_info{make_ident_info(tc, expr_)};

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

    // 'item' reads none of the 'written' parts, under any name
    auto assert_no_read_of_written_part(
        const toc& tc, const statement& item,
        const std::span<const written_place> written) const -> void {

        item.visit_reads(
            std::nullopt,
            [&](const token& src_loc_tk, const std::string_view read_text,
                [[maybe_unused]] const byte_range& accessed_bytes,
                const access_path& read_path) -> void {
                const std::optional<storage_path> read{
                    tc.storage_of(src_loc_tk.text(), read_path),
                };

                // note: a constant or a value in a register is no storage
                if (not read) {
                    return;
                }

                for (const written_place& place : written) {
                    if (not read->may_overlap(place.storage)) {
                        continue;
                    }

                    if (is_certain_overwrite(src_loc_tk, *read, read_path,
                                             place)) {

                        throw compile_error{
                            src_loc_tk,
                            std::format("'{}' is read but has been "
                                        "overwritten in the same statement",
                                        read_text)};
                    }

                    throw compile_error{
                        src_loc_tk,
                        std::format("'{}' may have been overwritten in the "
                                    "same statement",
                                    read_text)};
                }
            });
    }

    // the value is written part by part, so a part must not read what is
    // already written, e.g. the 'p.x' of 'p = {p.y, p.x}'
    auto assert_parts_not_clobbered(
        const toc& tc, const std::optional<storage_path>& dst) const -> void {

        // note: a value in a register is written as one part that no other
        //       name reaches

        if (not dst) {
            return;
        }

        std::vector<written_part> parts;
        expr_.list_written_parts(access_path::whole_variable(), parts);

        // the parts before the current one
        std::vector<written_place> written;

        for (const written_part& part : parts) {
            const written_place place{
                .storage{
                    .root{dst->root},
                    .path{dst->path.extended_by(part.suffix)},
                },
                .path{stmt_ident_.accessed_path().extended_by(part.suffix)},
            };

            assert_no_read_of_written_part(tc, *part.item, written);
            written.push_back(place);
        }
    }

    // the read is of the written bytes, not only maybe: no run-time index
    // on either side, or none under the destination's name, which reaches
    // the bytes through the same binding, e.g. 'p.x' of 'p = {1, p.x}' with
    // 'p' bound to 'ps[i]'
    [[nodiscard]] auto is_certain_overwrite(const token& read_tk,
                                            const storage_path& read,
                                            const access_path& read_path,
                                            const written_place& place) const
        -> bool {

        if (not read.path.has_run_time_index() and
            not place.storage.path.has_run_time_index()) {

            return true;
        }

        return read_tk.text() == stmt_ident_.first_token().text() and
               not read_path.has_run_time_index() and
               not place.path.has_run_time_index();
    }
};
