#pragma once
// reviewed: 2025-09-28

#include <string_view>

#include "decouple.hpp"
#include "stmt_call.hpp"

class stmt_builtin_mov final : public stmt_call {
  public:
    stmt_builtin_mov(toc& tc, const token tk, tokenizer& tz)
        : stmt_call{tc, {}, tk, tz.is_next_char_token('('), tz} {

        set_type(tc.get_type_void());
    }

    stmt_builtin_mov() = default;

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));

        if (argument_count() != 2) {
            throw compiler_exception{tok(), "expected 2 arguments"};
        }

        // the assembler command might not need to resolve expressions
        const ident_info dst_arg_info{tc.make_ident_info(argument(0))};

        const statement& src_arg{argument(1)};
        src_arg.compile(tc, indent + 1, dst_arg_info);
    }

    auto visit_reads(const std::string_view var,
                     const read_visitor reader) const -> void override {

        argument(1).visit_reads(var, reader);
    }

    // a register destination is written whole
    auto trace_assignment(assignment_flow& flow) const -> void override {
        assert_var_not_used(flow.var, flow.assigned);

        if (argument(0).identifier() != flow.var) {
            return;
        }

        flow.assigned = field_coverage::full(flow.assigned.size_bytes());
    }
};
