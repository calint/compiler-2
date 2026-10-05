#pragma once
// reviewed: 2025-09-28

#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "expr_any.hpp"
#include "statement.hpp"
#include "stmt_block.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "unary_ops.hpp"

class stmt_builtin_foo final : public statement {
    stmt_identifier ident_;
    token count_delim_tk_;
    expr_any count_;
    stmt_block code_;

  public:
    stmt_builtin_foo(toc& tc, const token src_loc_tk, tokenizer& tz)
        : statement{src_loc_tk, unary_ops{}} {

        set_type(tc.get_type_void());

        ident_ = {tc, unary_ops{}, tz.next_token(), tz};

        if (not ident_.is_array()) {
            throw compiler_exception{ident_.tok(), "expected an array"};
        }

        // e.g. 'foo arr, len' visits the first 'len' elements
        count_delim_tk_ = tz.is_next_char_token(',');

        if (has_count()) {
            count_ = {tc, tz, tc.get_type_default(), false, false, 0};
        }

        // add vars to toc without emitting output so that the code block can be
        // parsed
        const ident_info ii{tc.make_ident_info(ident_)};
        tc.enter_foo("");
        add_loop_names(tc, 0, token{}, token{}, ii, operand{}, operand{});

        code_ = {tc, tz};

        tc.exit_foo("");
    }

    stmt_builtin_foo() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        ident_.source_to(os);

        if (has_count()) {
            count_delim_tk_.source_to(os);
            count_.source_to(os);
        }

        code_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        machine& x{tc.machine()};

        // a one-line comment for the definition
        std::string header{statement::trimmed_source(ident_)};

        if (has_count()) {
            header += ", " + statement::trimmed_source(count_);
        }

        x.comment(tok(), indent, "foo {}", header);

        const std::string loop_label{tc.create_unique_label(tok(), "foo")};
        const std::string end_label{toc::end_label(loop_label)};

        // the array and the count are resolved before 'e', 'i' and 'n' can
        // hide names they use
        const ident_info ii{tc.make_ident_info(ident_)};

        const operand reg_iter{
            x.alloc_scratch_register(ident_.tok(), indent,
                                     tc.get_type_default()),
        };

        x.comment(ident_.tok(), indent, "initiate iterator e");

        load_array_address(tc, indent, ii, reg_iter);

        std::vector<operand> allocated_registers;

        const operand limit{
            compile_limit(tc, indent, ii.array_len, allocated_registers),
        };

        tc.enter_foo(loop_label);

        const operand reg_counter{
            x.alloc_scratch_register(tok(), indent, tc.get_type_default()),
        };

        allocated_registers.push_back(reg_counter);

        add_loop_names(tc, indent, ident_.tok(), tok(), ii, reg_iter,
                       reg_counter);

        x.comment(tok(), indent, "initiate counter i");

        x.copy_value(tok(), indent, reg_counter,
                     operand::imm("0", tc.get_type_default()));

        // the loop is tested at the end, so a count of zero or less skips it
        if (has_count()) {
            x.compare_and_branch(
                count_.tok(), indent, limit,
                operand::imm("0", tc.get_type_default()),
                {
                    .operation{machine::comparison_operator::less_equal},
                    .inverted{},
                    .destination{},
                    .target{end_label},
                    .branch_on_true{true},
                },
                {});
        }

        x.label(indent, loop_label);
        code_.compile(tc, indent, ident_info::make_empty());
        x.label(indent + 1, toc::continue_label(loop_label));

        x.advance_array_iteration(tok(), indent + 2, reg_iter, reg_counter,
                                  ii.type_ref().size_bytes(), limit,
                                  loop_label);

        x.label(indent, end_label);

        x.free_scratch_registers(tok(), indent, allocated_registers);
        x.free_scratch_register(tok(), indent, reg_iter);

        tc.exit_foo(loop_label);
    }

    // the body may run zero times so its assignments do not count
    auto trace_assignment(assignment_flow& flow) const -> void override {
        ident_.assert_var_not_used(flow.var, flow.assigned);

        if (has_count()) {
            count_.assert_var_not_used(flow.var, flow.assigned);
        }

        std::ignore = code_.trace_loop_body(flow);
    }

  private:
    // a count register is checked against the array size like the count of
    // 'array_copy'
    [[nodiscard]] auto
    compile_limit(toc& tc, const size_t indent, const size_t array_len,
                  std::vector<operand>& allocated_registers) const -> operand {

        if (not has_count()) {
            return operand::imm(std::format("{}", array_len),
                                tc.get_type_default());
        }

        machine& x{tc.machine()};

        const operand reg_count{
            x.alloc_scratch_register(count_.tok(), indent,
                                     tc.get_type_default()),
        };

        allocated_registers.push_back(reg_count);

        x.comment(count_.tok(), indent, "count {}",
                  statement::trimmed_source(count_));

        count_.compile(tc, indent,
                       toc::make_ident_info_from_register(reg_count));

        x.check_bounds(count_.tok(), indent, reg_count, array_len, true, {},
                       tc.bounds_check_options());

        return reg_count;
    }

    [[nodiscard]] auto has_count() const -> bool {
        return not count_delim_tk_.is_empty();
    }

    // an indexed or forwarded array needs its address computed
    auto load_array_address(toc& tc, const size_t indent, const ident_info& ii,
                            const operand& reg_iter) const -> void {

        machine& x{tc.machine()};

        if (not ii.has_lea() and not ii.is_pointer and
            not ident_.is_indexed()) {

            x.address_of(tok(), indent, reg_iter, ii.operand);
            return;
        }

        std::vector<operand> allocated_registers;

        const operand op{
            ident_.compile_lea(
                tc, indent, tok(), allocated_registers,
                {.reg_count{}, .lea_path{ii.lea_path}, .address_register{}}),
        };

        x.address_of(tok(), indent, reg_iter, op);

        x.free_scratch_registers(tok(), indent, allocated_registers);
    }

    //
    // statics
    //

    // 'e' is the element at the iterator, 'i' the counter and 'n' the array
    // size, parsing registers them without iterator and counter registers
    static auto add_loop_names(toc& tc, const size_t indent,
                               const token& src_loc_tk, const token& decl_tk,
                               const ident_info& array_info,
                               const operand& iterator, const operand& counter)
        -> void {

        tc.add_var(
            src_loc_tk, indent,
            {
                .name{"e"},
                .type_ptr{&array_info.type_ref()},
                .src_loc_tk{decl_tk},
                .read_only_why{
                    array_info.is_read_only() ? read_only_cause::foo_element
                                              : read_only_cause::none,
                },
                .pointer_register{iterator},
                .base_register{},
                .value_register{},
            },
            var_kind::var);

        tc.add_var(src_loc_tk, indent,
                   {
                       .name{"i"},
                       .type_ptr{&tc.get_type_default()},
                       .src_loc_tk{decl_tk},
                       .read_only_why{read_only_cause::foo_counter},
                       .pointer_register{},
                       .base_register{},
                       .value_register{counter},
                   },
                   var_kind::var);

        tc.add_const(src_loc_tk, indent, "n",
                     static_cast<int64_t>(array_info.array_len));
    }
};
