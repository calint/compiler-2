#pragma once

#include <cstddef>
#include <format>
#include <optional>
#include <ostream>
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

class stmt_assign_var final : public statement {
    stmt_identifier stmt_ident_;
    expr_any expr_;
    token equals_tk_;

    // a read of a variable in the value
    struct value_read {
        token tk;
        std::string text;
        access_path path;
    };

  public:
    stmt_assign_var(toc& tc, tokenizer& tz, stmt_identifier si,
                    const token equals_tk)
        : statement{si.tok()}, stmt_ident_{std::move(si)},
          equals_tk_{equals_tk} {

        const ident_info dst_info{tc.make_ident_info(stmt_ident_)};

        assert_not_read_only(tok(), "assign to", stmt_ident_.name_or_literal(),
                             dst_info);

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

        set_type(tc.make_ident_info(stmt_ident_).type_ref());
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

        const type& dst_type{stmt_ident_.get_type()};

        const bool is_record{
            not stmt_ident_.is_array() and not dst_type.is_builtin(),
        };

        expr_.assert_value_not_reading({
            .root{stmt_ident_.first_token().text()},
            .range{stmt_ident_.accessed_bytes()},
            .is_exact{stmt_ident_.is_exact_access()},
            .path{stmt_ident_.accessed_path()},
            .record_type{is_record ? &dst_type : nullptr},
        });

        const ident_info var_info{tc.make_ident_info(stmt_ident_)};

        assert_reference_not_read(tc, var_info);

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
            destination::of_name(
                var_info,
                {
                    .address{address},
                    .is_address_known{is_address_known},
                    .accessed_path{stmt_ident_.accessed_path()},
                }),
        };

        expr_.compile(tc, indent, var_dst);
        x.free_scratch_registers(tok(), indent, lea_registers);
    }

    // the value is read before the destination is written
    auto trace_assignment(assignment_flow& flow) const -> void override {
        assert_var_not_used(flow.var, flow.assigned);
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

        const ident_info src_info{tc.make_ident_info(expr_)};

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

    // a destination that names storage under another name, e.g. a parameter or
    // the element of a 'foo', may be read by the value under that name, only
    // for '--checks=alias'
    auto assert_reference_not_read(const toc& tc,
                                   const ident_info& var_info) const -> void {

        if (not tc.checks().alias or
            not is_reference_destination(tc, var_info)) {

            return;
        }

        const storage_target dst{
            tc.storage_target_of(stmt_ident_.first_token(),
                                 stmt_ident_.first_token().text(),
                                 stmt_ident_.accessed_path()),
        };

        for (const value_read& read : reads_of_value()) {
            if (not may_share_storage(tc, dst, read)) {
                continue;
            }

            throw compile_error{
                read.tk,
                std::format("'{}' may share storage with the destination '{}' "
                            "(both name '{}'), compute the value in a "
                            "temporary variable first",
                            read.text, stmt_ident_.name_or_literal(),
                            dst.display_root())};
        }
    }

    // a pointer, a parameter that names its argument or an element of a 'foo'
    [[nodiscard]] auto
    is_reference_destination(const toc& tc, const ident_info& var_info) const
        -> bool {

        const std::string_view root{stmt_ident_.first_token().text()};

        return var_info.is_pointer() or tc.scopes().is_alias(root) or
               not tc.scopes().foo_array(root).root.empty();
    }

    // true when the read may be of bytes of the destination under another
    // name
    [[nodiscard]] auto may_share_storage(const toc& tc,
                                         const storage_target& dst,
                                         const value_read& read) const -> bool {

        const std::string_view read_name{read.tk.text()};

        // note: a read under the name of the destination is handled where the
        //       value is compiled, e.g. 'x = x + 1'
        if (read_name == stmt_ident_.first_token().text()) {
            return false;
        }

        if (not tc.scopes().lookup(read_name).is_var_or_alias()) {
            return false;
        }

        const storage_target source{
            tc.storage_target_of(read.tk, read_name, read.path),
        };

        return source.may_overlap(dst);
    }

    // every read of a variable in the value, in source order
    [[nodiscard]] auto reads_of_value() const -> std::vector<value_read> {
        std::vector<value_read> reads;

        expr_.visit_reads(
            std::nullopt,
            [&reads](const token& tk, const std::string_view text,
                     [[maybe_unused]] const byte_range& accessed_bytes,
                     const access_path& accessed_path) -> void {
                reads.push_back({
                    .tk{tk},
                    .text{text},
                    .path{accessed_path},
                });
            });

        return reads;
    }
};
