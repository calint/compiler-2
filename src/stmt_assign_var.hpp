#pragma once
// reviewed: 2025-09-28

#include <cstddef>
#include <format>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_unset_var.hpp"

class stmt_assign_var final : public statement {
    stmt_identifier stmt_ident_;
    expr_any expr_;
    token equals_tk_;
    size_t array_count_{};

    // a read of a variable in the value
    struct value_read {
        token tk;
        std::string text;
        // empty when the read is of the whole variable
        std::optional<access_span> span;
    };

  public:
    stmt_assign_var(toc& tc, tokenizer& tz, stmt_identifier si,
                    const token equals_tk)
        : statement{si.tok()}, stmt_ident_{std::move(si)},
          equals_tk_{equals_tk} {

        const size_t array_count{stmt_ident_.array_count()};

        const ident_info dst_info{tc.make_ident_info(stmt_ident_)};

        assert_not_read_only(tok(), "assign to", stmt_ident_.identifier(),
                             dst_info);

        set_type(dst_info.type_ref());

        // the value is parsed as the type of the destination, e.g. 'p1 = p2' or
        // 'a = {1, 2}' or 'x = y + 1'
        expr_ = {
            tc,         tz, dst_info.type_ref(), false, stmt_ident_.is_array(),
            array_count};

        if (array_count == 0) {
            array_count_ = expr_.array_count();
        }
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
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        expr_.assert_record_value_not_reading({
            .root{stmt_ident_.first_token().text()},
            .range{stmt_ident_.access_range()},
            .is_exact{stmt_ident_.is_exact_access()},
        });

        // get information about the destination of the compilation
        ident_info var_dst_info{tc.make_ident_info(stmt_ident_)};

        var_dst_info.accessed_span = stmt_ident_.span();
        var_dst_info.is_reference = is_reference_destination(tc, var_dst_info);

        assert_reference_not_read(tc, var_dst_info);

        if (var_dst_info.is_const()) {
            throw compiler_exception{
                tok(), std::format("cannot assign to constant '{}'",
                                   var_dst_info.const_value)};
        }

        if (expr_.is_array_identifier()) {
            if (const ident_info src_info{tc.make_ident_info(expr_)};

                src_info.is_array and
                src_info.array_len != var_dst_info.array_len) {

                throw compiler_exception{
                    expr_.tok(),
                    "source and destination array sizes do not match"};
            }
        }

        std::vector<operand> lea_registers;

        var_dst_info.use_operand = array_count_ > 0 or
                                   stmt_ident_.is_indexed() or
                                   var_dst_info.is_pointer;

        var_dst_info.operand = tc.get_lea_operand(indent, stmt_ident_,
                                                  var_dst_info, lea_registers);

        var_dst_info.is_pointer = false;

        expr_.compile(tc, indent, var_dst_info);
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
    // a destination that names storage under another name, e.g. a parameter or
    // the element of a 'foo', may be read by the value under that name, only
    // for '--checks=alias'
    auto assert_reference_not_read(const toc& tc,
                                   const ident_info& dst_info) const -> void {

        if (not tc.is_alias_check() or not dst_info.is_reference) {
            return;
        }

        const storage_target dst{
            tc.storage_target_of(stmt_ident_.first_token(),
                                 stmt_ident_.first_token().text(),
                                 stmt_ident_.span()),
        };

        for (const value_read& read : reads_of_value()) {
            if (not may_share_storage(tc, dst, read)) {
                continue;
            }

            throw compiler_exception{
                read.tk,
                std::format("'{}' may share storage with the destination '{}' "
                            "(both name '{}'), compute the value in a "
                            "temporary variable first",
                            read.text, stmt_ident_.identifier(), dst.root)};
        }
    }

    // a pointer, a parameter that names its argument or an element of a 'foo'
    [[nodiscard]] auto
    is_reference_destination(const toc& tc, const ident_info& dst_info) const
        -> bool {

        const std::string_view root{stmt_ident_.first_token().text()};

        return dst_info.is_pointer or tc.is_alias(root) or
               not tc.foo_array(root).root.empty();
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

        if (not tc.is_var_or_alias(read_name)) {
            return false;
        }

        const storage_target source{
            tc.storage_target_of(stmt_ident_.first_token(), read_name,
                                 read.span),
        };

        return source.may_overlap(dst);
    }

    // every read of a variable in the value, in source order
    [[nodiscard]] auto reads_of_value() const -> std::vector<value_read> {
        std::vector<value_read> reads;

        expr_.visit_reads(
            std::nullopt,
            [&reads](const token& tk, const std::string_view text,
                     const std::optional<access_span>& accessed) -> void {
                reads.push_back({
                    .tk{tk},
                    .text{text},
                    .span{accessed},
                });
            });

        return reads;
    }
};
