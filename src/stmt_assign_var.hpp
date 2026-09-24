#pragma once
// reviewed: 2025-09-28

#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"

class stmt_assign_var final : public statement {
    stmt_identifier stmt_ident_;
    expr_any expr_;
    token equals_tk_;
    size_t array_count_{};

  public:
    stmt_assign_var(toc& tc, tokenizer& tz, stmt_identifier si,
                    const token equals_tk, const bool is_array,
                    const size_t array_count)
        : statement{si.tok()}, stmt_ident_{std::move(si)},
          equals_tk_{equals_tk} {

        const ident_info& dst_info{tc.make_ident_info(stmt_ident_)};

        set_type(dst_info.type_ref());

        expr_ = {tc, tz, dst_info.type_ref(), false, is_array, array_count};

        if (array_count == 0) {
            array_count_ = expr_.array_count();
        }
    }

    stmt_assign_var() = default;

    auto source_to(std::ostream& os) const -> void override {
        // note: all the source info is in 'stmt_ident_'
        // statement::source_to(os);
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

        if (var_dst_info.is_const()) {
            throw compiler_exception{
                tok(), std::format("cannot assign to constant '{}'",
                                   var_dst_info.const_value)};
        }

        if (expr_.is_array_identifier()) {
            if (const ident_info src_info{tc.make_ident_info(expr_)};

                src_info.is_array and var_dst_info.is_array and
                src_info.array_len != var_dst_info.array_len) {

                throw compiler_exception{
                    tok(), "source and destination array sizes do not match"};
            }
        }

        std::vector<operand> lea_registers;
        var_dst_info.use_operand = array_count_ > 0 or
                                   stmt_ident_.is_indexed() or
                                   var_dst_info.is_pointer;
        var_dst_info.operand = tc.get_lea_operand(indent, stmt_ident_,
                                                  var_dst_info, lea_registers);
        var_dst_info.is_pointer = false;
        if (stmt_ident_.is_array_element()) {
            var_dst_info.is_array = false;
            var_dst_info.array_len = 0;
        }

        expr_.compile(tc, indent, var_dst_info);
        x.free_scratch_registers(tok(), indent, lea_registers);
    }

    [[nodiscard]] auto expression() const -> const expr_any& { return expr_; }

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

        expr_.visit_reads(var, reader);
        stmt_ident_.visit_index_reads(var, reader);
    }

    // the value is read before the destination is written
    auto trace_assignment(assignment_flow& flow) const -> void override {
        assert_var_not_used(flow.var, flow.assigned);
        stmt_ident_.record_assignment(flow);
    }

    [[nodiscard]] auto array_count() const -> size_t { return array_count_; }
};
