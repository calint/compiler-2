#pragma once

#include <array>
#include <utility>

#include "decouple.hpp"
#include "stmt_call.hpp"

class stmt_builtin_io final : public stmt_call {
  public:
    stmt_builtin_io(toc& tc, unary_ops uops, const token tk, tokenizer& tz)
        : stmt_call{tc, std::move(uops), tk, tz.is_next_char_token('('), tz} {
        if (argument_count() != 3) {
            throw compiler_exception{tok(), "expected 3 arguments"};
        }
    }

    auto compile(toc& tc, const size_t indent, const ident_info& dst_info) const
        -> void override {
        machine& x{tc.machine()};

        x.comment(tok(), indent, statement::trimmed_source(*this));
        std::array<operand, 3> args;
        for (size_t index{}; index < args.size(); ++index) {
            args.at(index) =
                x.alloc_scratch_register(tok(), indent, tc.get_type_default());
            argument(index).compile(
                tc, indent, toc::make_ident_info_from_register(args.at(index)));
        }
        const operand result{
            x.alloc_scratch_register(tok(), indent, tc.get_type_default())};

        if (tok().is_text("read")) {
            x.read(tok(), indent, result, args.at(0), args.at(1), args.at(2));
        } else {
            x.write(tok(), indent, result, args.at(0), args.at(1), args.at(2));
        }
        get_unary_ops().compile(tc, indent, result);
        if (not dst_info.is_empty()) {
            x.copy_value(tok(), indent, dst_info.operand, result);
        }
        x.free_scratch_register(tok(), indent, result);
        x.free_scratch_registers(tok(), indent, args);
    }
};
