#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

#include "compile_error.hpp"
#include "expr_any.hpp"
#include "ident_info.hpp"
#include "statement.hpp"
#include "stmt_block.hpp"
#include "stmt_identifier.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_assigned.hpp"

// 'foo arr' or 'foo arr, count': the body runs once per element of the array,
// or of its first 'count' elements, with 'e' the element, 'i' its index and
// 'n' the length of the array; the body may run zero times
class stmt_builtin_foo final : public statement {
    // e.g. the ', len' of 'foo arr, len'
    struct count_part {
        token delim_tk;
        expr_any count;
    };

    stmt_identifier ident_;
    // none when every element is visited
    std::optional<count_part> count_;
    stmt_block code_;

  public:
    stmt_builtin_foo(toc& tc, const token src_loc_tk, tokenizer& tz)
        : statement{src_loc_tk},
          ident_{stmt_identifier::parse_whole_array(tc, tz)},
          count_{parse_count(tc, tz)}, code_{parse_code(tc, tz)} {}

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        ident_.source_to(os);

        if (count_) {
            count_->delim_tk.source_to(os);
            count_->count.source_to(os);
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

        machine::held_registers held{x, tok(), indent};

        const operand reg_iter{
            x.alloc_scratch_register(ident_.tok(), indent,
                                     tc.builtins().address()),
        };

        held.add(reg_iter);

        x.comment(ident_.tok(), indent, "initialize iterator e");

        ident_.load_address(tc, indent, tok(), array_info, reg_iter);

        const operand limit{
            compile_limit(tc, indent, array_info.element_count(), held),
        };

        // e.g. not after 'foo a, stop() { }' with 'stop' an inlined 'exit'
        const bool count_returns{tc.is_reachable()};

        // a constant count above 0 runs the body, so the loop runs out only
        // where the body reaches its counter step, e.g. not after
        // 'foo a, 2 { stop() }'
        const bool runs_body{limit.is_immediate() and limit.immediate() != 0};

        const toc::scope_entry foo{
            tc, scope::loop(loop_label,
                            runs_body ? loop_end::when_stepped
                                      : loop_end::also_when_done,
                            toc::continue_label(loop_label))};

        const operand counter{
            declare_loop_names(tc, indent, array_info, reg_iter, held),
        };

        x.comment(tok(), indent, "initialize counter i");

        x.copy_value(tok(), indent, counter,
                     operand::imm(0, tc.builtins().default_type()));

        // a count that folds to 0 skips the body like a branch that folds to
        // false, see 'toc::check_skipped'
        if (limit.is_immediate() and limit.immediate() == 0) {
            tc.check_skipped(
                [&] -> void { code_.compile(tc, indent, destination{}); });

            tc.join_loop();

            return;
        }

        skip_empty_loop(tc, indent, limit, end_label);

        x.label(indent, loop_label);
        code_.compile(tc, indent, destination{});
        x.label(indent + 1, toc::continue_label(loop_label));

        x.foo_advance_iteration(tok(), indent + 2, reg_iter, counter,
                                array_info.type_ref().size_bytes(), limit,
                                loop_label);

        x.label(indent, end_label);

        tc.join_loop();

        if (not count_returns) {
            tc.end_path();
        }
    }

    auto trace_assignment(assignment_flow& flow) const -> void override {
        ident_.assert_result_not_read(flow);

        if (count_) {
            count_->count.assert_result_not_read(flow);
        }

        flow.trace_loop(code_, loop_end::also_when_done);
    }

  private:
    // 'e' is the element, 'i' the counter and 'n' the array size, declared
    // at the array and placed as 'element' and 'counter' say (the parse
    // declares the names only); 'n' is a constant when the size is known,
    // else a read-only variable that is never read at run time, see
    // 'ident_info::is_length_unknown'
    auto add_loop_names(toc& tc, const size_t indent,
                        const ident_info& array_info,
                        const var_placement& element,
                        const var_placement& counter) const -> void {

        // the root of the iterated array and the bytes of it that 'e' ranges
        // over, found before this 'foo' adds its own 'e' so that a name that is
        // the 'e' of an outer 'foo' is that one
        const std::optional<storage_path> array{ident_.storage(tc)};

        const token& src_loc_tk{ident_.tok()};

        tc.add_var(
            indent,
            {
                .name{"e"},
                .type_ptr{&array_info.type_ref()},
                .src_loc_tk{src_loc_tk},
                .shape{},
                .read_only_why{
                    array_info.is_read_only() ? read_only_cause::foo_element
                                              : read_only_cause::none,
                },
                .placement{element},
                .foo_array{array},
            },
            var_kind::var);

        tc.add_var(indent,
                   {
                       .name{"i"},
                       .type_ptr{&tc.builtins().default_type()},
                       .src_loc_tk{src_loc_tk},
                       .shape{},
                       .read_only_why{read_only_cause::foo_counter},
                       .placement{counter},
                       .foo_array{},
                   },
                   var_kind::var);

        const std::optional<size_t> array_len{array_info.array_len()};

        // note: a constant 0 would fold, e.g. 'let m = n' would keep it
        if (not array_len) {
            tc.add_var(indent,
                       {
                           .name{"n"},
                           .type_ptr{&tc.builtins().default_type()},
                           .src_loc_tk{src_loc_tk},
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

        if (count_) {
            header += ", " + statement::trimmed_source(count_->count);
        }

        x.comment(tok(), indent, "foo {}", header);
    }

    // a count register is checked against the array size like the count of
    // 'array_copy', a constant count is checked at compile time
    [[nodiscard]] auto compile_limit(toc& tc, const size_t indent,
                                     const size_t array_len,
                                     machine::held_registers& held) const
        -> operand {

        if (not count_) {
            return operand::imm(array_len, tc.builtins().default_type());
        }

        const expr_any& count{count_->count};

        if (const std::optional<int64_t> value{count.constant_value(tc)}) {
            ident_.assert_count_in_bounds(tc, count.tok(), *value);
            return operand::imm(*value, tc.builtins().default_type());
        }

        machine& x{tc.machine()};

        const operand reg_count{
            x.alloc_scratch_register(count.tok(), indent,
                                     tc.builtins().default_type()),
        };

        held.add(reg_count);

        x.comment(count.tok(), indent, "count {}",
                  statement::trimmed_source(count));

        count.compile(tc, indent, destination::of_register(reg_count));

        x.check_bounds(count.tok(), indent,
                       machine::bounds_range::up_to(reg_count, array_len),
                       tc.checks().bounds_options());

        return reg_count;
    }

    // 'e', 'i' and 'n' are declared, the counter is a register or a variable in
    // memory, as the machine prefers
    auto declare_loop_names(toc& tc, const size_t indent,
                            const ident_info& array_info,
                            const operand& reg_iter,
                            machine::held_registers& held) const -> operand {

        machine& x{tc.machine()};

        const bool counter_in_memory{x.foo_counter_in_memory()};

        if (counter_in_memory) {
            add_loop_names(tc, indent, array_info,
                           at_address_register{reg_iter}, var_placement{});

            return tc.resolve_declared(tok(), "i").operand();
        }

        const operand reg_counter{
            x.alloc_scratch_register(tok(), indent,
                                     tc.builtins().default_type()),
        };

        held.add(reg_counter);

        add_loop_names(tc, indent, array_info, at_address_register{reg_iter},
                       in_value_register{reg_counter});

        return reg_counter;
    }

    // the element and the counter are declared during the parse so the block
    // can name them, the labels are made at the compile
    // note: called by the constructor after 'ident_' is made
    [[nodiscard]] auto parse_code(toc& tc, tokenizer& tz) const -> stmt_block {
        const ident_info array_info{make_ident_info(tc, ident_)};

        const toc::scope_entry foo{
            tc,
            scope::loop({}, loop_end::also_when_done, {}),
        };

        add_loop_names(tc, 0, array_info, var_placement{}, var_placement{});

        return {tc, tz};
    }

    // the loop is tested at the end, so a count of zero or less skips it
    auto skip_empty_loop(toc& tc, const size_t indent, const operand& limit,
                         const std::string_view end_label) const -> void {

        if (not count_) {
            return;
        }

        // note: a constant count is not negative, see 'compile_limit', and
        //       one of 0 skips the body in 'compile'

        if (limit.is_immediate()) {
            return;
        }

        machine& x{tc.machine()};

        x.compare_and_branch(count_->count.tok(), indent, limit,
                             operand::imm(0, tc.builtins().default_type()),
                             {
                                 .operation{comparison_operator::less_equal},
                                 .inverted{},
                                 .destination{},
                                 .target{end_label},
                                 .branch_on_true{true},
                             });
    }

    //
    // statics
    //

    // e.g. 'foo arr, len' visits the first 'len' elements
    [[nodiscard]] static auto parse_count(toc& tc, tokenizer& tz)
        -> std::optional<count_part> {

        const std::optional<token> delim_tk{tz.is_next_char_token(',')};

        if (not delim_tk) {
            return std::nullopt;
        }

        // e.g. 'foo a, { e = 1 }', like an 'if' without a condition
        if (tz.peek_char_after_whitespace() == '{') {
            throw compile_error{tz, "expected a count before '{'"};
        }

        return count_part{
            .delim_tk{*delim_tk},
            .count{
                tc,
                tz,
                tc.builtins().default_type(),
                literal_typing::from_destination,
            },
        };
    }
};
