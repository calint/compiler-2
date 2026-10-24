#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "compile_error.hpp"
#include "expr_any.hpp"
#include "ident_info.hpp"
#include "statement.hpp"
#include "stmt_block.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_assigned.hpp"
#include "unary_ops.hpp"

// 'foo arr' or 'foo arr, count': the body runs once per element of the array,
// or of its first 'count' elements, with 'e' the element, 'i' its index and
// 'n' the length of the array; the body may run zero times
class stmt_builtin_foo final : public statement {
    stmt_identifier ident_;
    // the ',' before the count, none when every element is visited
    std::optional<token> count_delim_tk_;
    expr_any count_;
    stmt_block code_;

  public:
    stmt_builtin_foo(toc& tc, const token src_loc_tk, tokenizer& tz)
        : statement{src_loc_tk} {

        set_type(tc.builtins().void_type());

        ident_ = stmt_identifier::parse_array_argument(tc, tz);

        if (not ident_.is_array()) {
            throw compile_error{
                ident_.tok(),
                std::format("{} is not an array",
                            in_quotes(statement::trimmed_source(ident_)))};
        }

        // e.g. 'foo arr, len' visits the first 'len' elements
        count_delim_tk_ = tz.is_next_char_token(',');

        if (has_count()) {
            count_ = {tc, tz, tc.builtins().default_type(),
                      literal_typing::from_destination};
        }

        // the element and the counter are declared during the parse so the
        // block can name them
        const ident_info array_info{make_ident_info(tc, ident_)};
        const std::string label{tc.create_unique_label(tok(), "foo")};
        const toc::frame_scope foo{tc, frame::foo(label)};

        add_loop_names(tc, 0, src_loc_tk, array_info, operand{}, operand{});

        code_ = {tc, tz};
    }

    stmt_builtin_foo() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        ident_.source_to(os);

        if (count_delim_tk_) {
            count_delim_tk_->source_to(os);
            count_.source_to(os);
        }

        code_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        machine& x{tc.machine()};

        comment_header(x, indent);

        const std::string loop_label{tc.create_unique_label(tok(), "foo")};
        const std::string end_label{toc::end_label(loop_label)};

        // the array and the count are resolved before 'e', 'i' and 'n' can
        // hide names they use
        const ident_info array_info{make_ident_info(tc, ident_)};

        const operand reg_iter{
            x.alloc_scratch_register(ident_.tok(), indent,
                                     tc.builtins().address()),
        };

        x.comment(ident_.tok(), indent, "initialize iterator e");

        load_array_address(tc, indent, array_info, reg_iter);

        std::vector<operand> allocated_registers;

        const operand limit{
            compile_limit(tc, indent, array_info.element_count(),
                          allocated_registers),
        };

        const toc::frame_scope foo{tc, frame::foo(loop_label)};

        const operand counter{
            declare_loop_names(tc, indent, array_info, reg_iter,
                               allocated_registers),
        };

        x.comment(tok(), indent, "initialize counter i");

        x.copy_value(tok(), indent, counter,
                     operand::imm(0, tc.builtins().default_type()));

        skip_empty_loop(x, tc, indent, limit, end_label);

        x.label(indent, loop_label);
        code_.compile(tc, indent, destination{});
        x.label(indent + 1, toc::continue_label(loop_label));

        x.foo_advance_iteration(tok(), indent + 2, reg_iter, counter,
                                array_info.type_ref().size_bytes(), limit,
                                loop_label);

        x.label(indent, end_label);

        // note: the end is reached when the count runs out or at a 'break'
        tc.set_path_open(true);

        x.free_scratch_registers(tok(), indent, allocated_registers);
        x.free_scratch_register(tok(), indent, reg_iter);
    }

    auto trace_assignment(assignment_flow& flow) const -> void override {
        ident_.assert_var_not_used(flow);

        if (has_count()) {
            count_.assert_var_not_used(flow);
        }

        // the loop names hide the traced variable only in the body
        const assignment_flow::block_scope scope{flow};

        flow.declare("e");
        flow.declare("i");
        flow.declare("n");

        flow.trace_foo(code_);
    }

  private:
    // 'e' is the element at the iterator, 'i' the counter and 'n' the array
    // size, declared at the 'foo'; parsing registers them without iterator
    // and counter registers; 'n' is a constant when the size is known, else
    // a read-only variable that is never read at run time, see
    // 'ident_info::is_length_unknown'
    auto add_loop_names(toc& tc, const size_t indent, const token& src_loc_tk,
                        const ident_info& array_info, const operand& iterator,
                        const operand& counter) const -> void {

        // note: before 'e' is added, see 'foo_array'
        const std::optional<storage_path> array{foo_array(tc)};

        tc.add_var(
            src_loc_tk, indent,
            {
                .name{"e"},
                .type_ptr{&array_info.type_ref()},
                .src_loc_tk{tok()},
                .shape{},
                .read_only_why{
                    array_info.is_read_only() ? read_only_cause::foo_element
                                              : read_only_cause::none,
                },
                .placement{
                    iterator.is_empty()
                        ? var_placement{}
                        : var_placement{at_address_register{iterator}},
                },
                .foo_array{array},
            },
            var_kind::var);

        // an empty counter is a variable in memory
        tc.add_var(src_loc_tk, indent,
                   {
                       .name{"i"},
                       .type_ptr{&tc.builtins().default_type()},
                       .src_loc_tk{tok()},
                       .shape{},
                       .read_only_why{read_only_cause::foo_counter},
                       .placement{
                           counter.is_empty()
                               ? var_placement{}
                               : var_placement{in_value_register{counter}},
                       },
                       .foo_array{},
                   },
                   var_kind::var);

        const std::optional<size_t> array_len{array_info.array_len()};

        // note: a constant 0 would fold, e.g. 'let m = n' would keep it
        if (not array_len) {
            tc.add_var(src_loc_tk, indent,
                       {
                           .name{"n"},
                           .type_ptr{&tc.builtins().default_type()},
                           .src_loc_tk{tok()},
                           .shape{},
                           .read_only_why{read_only_cause::foo_length},
                           .placement{},
                           .foo_array{},
                       },
                       var_kind::var);

            return;
        }

        tc.add_const(src_loc_tk, indent, "n",
                     folded_value::untyped(static_cast<int64_t>(*array_len)));
    }

    // the 'foo' header as one comment line, e.g. 'foo a, 2'
    auto comment_header(machine& x, const size_t indent) const -> void {
        std::string header{statement::trimmed_source(ident_)};

        if (has_count()) {
            header += ", " + statement::trimmed_source(count_);
        }

        x.comment(tok(), indent, "foo {}", header);
    }

    // a count register is checked against the array size like the count of
    // 'array_copy', a constant count is checked at compile time
    [[nodiscard]] auto
    compile_limit(toc& tc, const size_t indent, const size_t array_len,
                  std::vector<operand>& allocated_registers) const -> operand {

        if (not has_count()) {
            return operand::imm(array_len, tc.builtins().default_type());
        }

        if (const std::optional<int64_t> count{count_.constant_value(tc)}) {
            ident_.assert_count_in_bounds(tc, count_.tok(), *count);
            return operand::imm(*count, tc.builtins().default_type());
        }

        machine& x{tc.machine()};

        const operand reg_count{
            x.alloc_scratch_register(count_.tok(), indent,
                                     tc.builtins().default_type()),
        };

        allocated_registers.push_back(reg_count);

        x.comment(count_.tok(), indent, "count {}",
                  statement::trimmed_source(count_));

        count_.compile(tc, indent, destination::of_register(reg_count));

        x.check_bounds(count_.tok(), indent,
                       machine::bounds_range::count_alone(reg_count, array_len),
                       tc.checks().bounds_options());

        return reg_count;
    }

    // 'e', 'i' and 'n' are declared, the counter is a register or a variable in
    // memory, as the machine prefers
    auto declare_loop_names(toc& tc, const size_t indent,
                            const ident_info& array_info,
                            const operand& reg_iter,
                            std::vector<operand>& allocated_registers) const
        -> operand {

        machine& x{tc.machine()};

        const bool counter_in_memory{x.foo_counter_in_memory()};

        operand reg_counter;

        if (not counter_in_memory) {
            reg_counter = x.alloc_scratch_register(
                tok(), indent, tc.builtins().default_type());

            allocated_registers.push_back(reg_counter);
        }

        add_loop_names(tc, indent, ident_.tok(), array_info, reg_iter,
                       reg_counter);

        return counter_in_memory ? tc.make_ident_info(tok(), "i").operand()
                                 : reg_counter;
    }

    // the root of the iterated array and the bytes of it that 'e' ranges over,
    // found before this 'foo' adds its own 'e' so that a name that is the 'e'
    // of an outer 'foo' is that one
    [[nodiscard]] auto foo_array(const toc& tc) const
        -> std::optional<storage_path> {

        return ident_.storage(tc);
    }

    [[nodiscard]] auto has_count() const -> bool {
        return count_delim_tk_.has_value();
    }

    // an indexed or forwarded array needs its address computed
    auto load_array_address(toc& tc, const size_t indent,
                            const ident_info& array_info,
                            const operand& reg_iter) const -> void {

        machine& x{tc.machine()};

        if (not ident_.has_computed_address(array_info)) {
            x.address_of(tok(), indent, reg_iter, array_info.operand());
            return;
        }

        std::vector<operand> allocated_registers;

        const operand op{
            ident_.compile_lea(
                tc, indent, tok(), allocated_registers,
                {
                    .reg_count{},
                    .address_by_call{array_info.address_by_call()},
                    .address_register{},
                }),
        };

        x.address_of(tok(), indent, reg_iter, op);

        x.free_scratch_registers(tok(), indent, allocated_registers);
    }

    // the loop is tested at the end, so a count of zero or less skips it
    auto skip_empty_loop(machine& x, const toc& tc, const size_t indent,
                         const operand& limit,
                         const std::string_view end_label) const -> void {

        if (not has_count()) {
            return;
        }

        // note: a constant count is not negative, see 'compile_limit'
        if (limit.is_immediate()) {
            if (limit.immediate() == 0) {
                x.branch(indent, end_label);
            }

            return;
        }

        x.compare_and_branch(
            count_.tok(), indent, limit,
            operand::imm(0, tc.builtins().default_type()),
            {
                .operation{machine::comparison_operator::less_equal},
                .inverted{},
                .destination{},
                .target{end_label},
                .branch_on_true{true},
            });
    }
};
