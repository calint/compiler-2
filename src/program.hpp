#pragma once
// reviewed: 2025-09-28

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "assembler.hpp"
#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "stmt_def_const.hpp"
#include "stmt_def_dat.hpp"
#include "stmt_def_func.hpp"
#include "stmt_def_type.hpp"
#include "stmt_def_var.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"
#include "ub_unset_var.hpp"
#include "unary_ops.hpp"

// a line of a report section, e.g. 'removed unreachable jumps: 0', without a
// value only the name is written
struct report_entry {
    std::string name;
    std::string value;
};

// lines of the report after the code: an optional title, the entries aligned
// and a separator after a titled section
struct report_section {
    std::string title;
    std::vector<report_entry> entries;

    // written as comments, nothing for a section without entries
    auto write_to(machine& x) const -> void {
        if (entries.empty()) {
            return;
        }

        if (not title.empty()) {
            x.comment(token{}, 0, "{:>28}:", title);
        }

        for (const report_entry& entry : entries) {
            if (entry.value.empty()) {
                x.comment(token{}, 0, "{:>28}", entry.name);
                continue;
            }

            x.comment(token{}, 0, "{:>28}: {}", entry.name, entry.value);
        }

        if (not title.empty()) {
            x.comment(token{}, 0, "");
        }
    }
};

// definite assignment: every path through a function with a return variable
// sets it, and the paths that reach the end of a body
class assignment_analysis final {
  public:
    //
    // statics
    //

    static auto assert_functions_set_return_value(
        const std::span<const stmt_def_func* const>& funcs) -> void {

        for (const stmt_def_func* f : funcs) {
            const std::optional<func_return_info> ret_info{f->returns()};

            if (not ret_info) {
                continue;
            }

            assignment_flow flow{
                .var{ret_info->ident_tk.text()},
                .src_loc_tk{ret_info->ident_tk},
                .assigned{field_coverage{ret_info->type_ptr->size_bytes()}},
                .at_breaks{},
                .is_reachable{true},
            };

            f->code().trace_assignment(flow);

            // the end of the body returns like a 'return' statement
            flow.assert_set_at_return();
        }
    }

    // uses the definite-assignment walk for its reachability only
    [[nodiscard]] static auto is_end_reachable(const stmt_def_func& func)
        -> bool {

        assignment_flow flow{
            .var{},
            .src_loc_tk{func.tok()},
            .assigned{field_coverage{0}},
            .at_breaks{},
            .is_reachable{true},
        };

        func.code().trace_assignment(flow);

        return flow.is_reachable;
    }
};

// the statistics after the code, as comment lines
class report_renderer final {
  public:
    //
    // statics
    //

    static auto write(machine& x, const usage_statistics& usage) -> void {
        const machine::output_statistics stats{x.statistics()};

        x.separate_report();

        if (stats.is_counted) {
            noinline_report(stats).write_to(x);
        }

        generics_report(usage).write_to(x);

        if (stats.is_counted) {
            optimization_report(stats).write_to(x);
        }

        usage_report(stats, usage).write_to(x);

        const std::vector<std::string> register_lines{x.register_peak_report()};

        // set apart from the reports above
        if (not register_lines.empty()) {
            x.comment(token{}, 0, "");
        }

        for (const std::string& line : register_lines) {
            x.comment(token{}, 0, "{}", line);
        }
    }

  private:
    //
    // statics
    //

    [[nodiscard]] static auto generics_report(const usage_statistics& usage)
        -> report_section {

        report_section section{
            .title{"uninstantiated generics"},
            .entries{},
        };

        for (const std::string& name : usage.uninstantiated_generics) {
            section.entries.push_back({
                .name{name},
                .value{},
            });
        }

        return section;
    }

    [[nodiscard]] static auto
    noinline_report(const ::machine::output_statistics& stats)
        -> report_section {

        report_section section{
            .title{"noinline functions"},
            .entries{},
        };

        for (const assembler::function_summary& f : stats.noinline_functions) {
            section.entries.push_back({
                .name{f.function},
                .value{
                    std::format(
                        "{} {}, {} {}, {} instructions{}", f.body_count,
                        f.body_count == 1 ? "body" : "bodies", f.call_count,
                        f.call_count == 1 ? "call" : "calls",
                        f.instruction_count,
                        f.call_count <= f.body_count ? ", no reuse" : ""),
                },
            });
        }

        return section;
    }

    [[nodiscard]] static auto
    optimization_report(const ::machine::output_statistics& stats)
        -> report_section {

        const assembler::optimization_counts& o{stats.optimizations};

        return {
            .title{},
            .entries{
                {
                    .name = "removed jumps to next code",
                    .value = std::format("{}", o.jumps_to_next),
                },
                {
                    .name = "removed unreachable jumps",
                    .value = std::format("{}", o.unreachable_jumps),
                },
                {
                    .name = "removed same target branches",
                    .value = std::format("{}", o.same_outcome_branches),
                },
                {
                    .name = "inverted branches over jumps",
                    .value = std::format("{}", o.inverted_branches),
                },
            },
        };
    }

    [[nodiscard]] static auto
    usage_report(const machine::output_statistics& stats,
                 const usage_statistics& usage) -> report_section {

        report_section section{
            .title{},
            .entries{
                {
                    .name = "max scratch registers in use",
                    .value = std::format("{}", stats.max_scratch_registers),
                },
                {
                    .name = "max frames in use",
                    .value = std::format("{}", usage.max_frame_count),
                },
                {
                    .name = "dat size",
                    .value = std::format("{} B", usage.dat_size_bytes),
                },
                {
                    .name = "dat var padding",
                    .value = std::format("{} B", usage.dat_var_padding_bytes),
                },
                {
                    .name = "max vars size",
                    .value = std::format("{} B", usage.max_vars_size_bytes),
                },
            },
        };

        if (stats.is_counted) {
            section.entries.push_back({
                .name = "instructions",
                .value = std::format("{}", stats.instruction_count),
            });
        }

        return section;
    }
};

class program final {
    // built-in types
    type type_void_{"void", 0, type_kind::builtin};
    type type_i64_{"i64", sizeof(int64_t), type_kind::builtin};
    type type_i32_{"i32", sizeof(int32_t), type_kind::builtin};
    type type_i16_{"i16", sizeof(int16_t), type_kind::builtin};
    type type_i8_{"i8", sizeof(int8_t), type_kind::builtin};
    type type_bool_{"bool", type_i8_.size_bytes(), type_kind::boolean};

    std::vector<std::unique_ptr<statement>> statements_;
    toc tc_; // table of contents
    size_t vars_size_bytes_{};

  public:
    program(machine& backend, const std::string_view source,
            const size_t vars_size_bytes, const check_options& checks)
        : tc_{backend, source, vars_size_bytes, checks},
          vars_size_bytes_{vars_size_bytes} {

        if (vars_size_bytes > backend.max_storage_bytes()) {
            throw compiler_exception::file_level(
                std::format("variable storage of {} B exceeds the {} B that "
                            "the target addresses",
                            vars_size_bytes, backend.max_storage_bytes()));
        }

        // create a placeholder token to use with 'toc' functions
        const token src_loc_tk{};

        // add built-in calls
        tc_.add_func(src_loc_tk, "exit", type_void_, nullptr, {});

        // add built-in types
        tc_.add_type(src_loc_tk, type_i64_);
        tc_.add_type(src_loc_tk, type_i32_);
        tc_.add_type(src_loc_tk, type_i16_);
        tc_.add_type(src_loc_tk, type_i8_);
        tc_.add_type(src_loc_tk, type_bool_);
        tc_.add_type(src_loc_tk, type_void_);

        // the types the front end needs by role
        tc_.set_type_void(type_void_);
        tc_.set_type_bool(type_bool_);

        machine& x{tc_.machine()};

        tc_.set_builtin_types(type_i64_, type_i32_, type_i16_, type_i8_);
        x.set_builtin_types(type_i64_, type_i32_, type_i16_, type_i8_);

        // the default type has a name of its own so that it is portable
        tc_.add_type_alias(src_loc_tk, "int", tc_.get_type_default());

        tc_.add_func(src_loc_tk, "read", tc_.get_type_default(), nullptr, {});
        tc_.add_func(src_loc_tk, "write", tc_.get_type_default(), nullptr, {});

        tc_.enter_block();

        tokenizer tz{source};
        while (true) {
            const token tk{tz.next_token()};

            if (tk.text().empty() and not tk.is_string()) {
                // a delimiter is not a token, only the end of the source ends
                // the definitions
                tz.assert_not_at_delimiter();

                break;
            }

            statements_.emplace_back(parse_definition(tc_, tz, tk));
        }

        tc_.exit_block();

        assignment_analysis::assert_functions_set_return_value(
            tc_.get_func_defs());
    }

    auto build(std::ostream& os) -> void {
        machine& x{tc_.machine()};

        x.start();
        compile(tc_, 0);
        x.finish();
        report_renderer::write(x, tc_.usage());
        tc_.finish();

        x.write_assembly(os);
    }

    auto source_to(std::ostream& os) const -> void {
        for (const std::unique_ptr<statement>& s : statements_) {
            s->source_to(os);
        }
    }

    //
    // statics
    //

    // only definitions are allowed at the top level
    [[nodiscard]] static auto parse_definition(toc& tc, tokenizer& tz,
                                               const token tk)
        -> std::unique_ptr<statement> {

        if (tk.is_text("func")) {
            return std::make_unique<stmt_def_func>(tc, tk, tz);
        }

        if (tk.is_text("type")) {
            return std::make_unique<stmt_def_type>(tc, tk, tz);
        }

        if (tk.is_text("let")) {
            return stmt_def_const::parse_let(tc, tz, tk);
        }

        if (tk.is_text("dat")) {
            return std::make_unique<stmt_def_dat>(tc, tk, tz);
        }

        if (tk.is_text("var")) {
            return std::make_unique<stmt_def_var>(tc, tk, tz);
        }

        throw compiler_exception{
            tk, std::format("unexpected keyword '{}'", tk.text())};
    }

  private:
    auto compile(toc& tc, const size_t indent) const -> void {
        tc.reset_usage();

        tc.enter_block();

        for (const std::unique_ptr<statement>& s : statements_) {
            s->compile(tc, indent, ident_info::make_empty());
        }

        compile_main(tc, indent);
        compile_noninline_functions(tc, indent);

        tc.exit_block();

        emit_failure_handlers(tc);
        emit_read_only_data(tc);
        emit_data_section(tc);
    }

    auto emit_data_section(toc& tc) const -> void {
        machine& x{tc.machine()};

        const size_t alignment{x.data_alignment()};
        x.begin_data(alignment);

        // zero padding places each dat at the offset 'toc::add_var' gave it
        size_t dat_offset{};
        for (const statement* s : tc.get_data()) {
            const size_t padding_bytes{
                align_storage_size(dat_offset, s->get_type().alignment()) -
                    dat_offset,
            };

            if (padding_bytes != 0) {
                x.comment({}, 0, "padding {} B", padding_bytes);
                x.emit_zero_data(padding_bytes);
            }

            s->compile_data(tc);

            dat_offset = add_storage_size(s->tok(), dat_offset + padding_bytes,
                                          s->dat_size_bytes());
        }

        x.reserve_variables(alignment, vars_size_bytes_);
    }

    //
    // statics
    //

    // the main function is compiled where the program starts
    static auto compile_main(toc& tc, const size_t indent) -> void {
        const stmt_def_func& func_main{tc.get_main_or_throw()};

        if (not func_main.is_inlined()) {
            throw compiler_exception{func_main.noinline_token(),
                                     "main cannot be declared noinline"};
        }

        machine& x{tc.machine()};

        const machine::call_frame_scope call_frame{
            x,
            token{},
            std::string{reserved_names::main},
        };

        x.comment({}, 0, "");

        x.label(0, reserved_names::main);
        tc.enter_func(reserved_names::main);
        func_main.code().compile(tc, indent, ident_info::make_empty());
        tc.exit_func(reserved_names::main);

        // code after an 'exit' or 'return' on every path would never run
        if (assignment_analysis::is_end_reachable(func_main)) {
            x.end_main();
        }
    }

    static auto compile_noninline_body(
        toc& tc, const size_t indent, const stmt_def_func& func,
        const std::span<const size_t> array_lengths) -> void {

        machine& x{tc.machine()};

        const machine::call_frame_scope call_frame{
            x,
            token{},
            std::string{func.name()},
            true,
        };

        x.begin_noinline_body(std::string{func.name()},
                              func.body_label(array_lengths));

        x.comment({}, 0, "");
        func.source_def_comment_to(x, 0);

        if (not array_lengths.empty()) {
            x.comment({}, 0, "array parameter lengths: {:n}", array_lengths);
        }

        x.label(indent, func.body_label(array_lengths));

        const size_t frame_size_bytes{
            func.compile_body(tc, indent, array_lengths),
        };

        x.define_constant(func.frame_size_label(array_lengths),
                          frame_size_bytes);

        x.end_noinline_body();
    }

    // each noninline function that is called has one body per kind of call,
    // after 'main'; a function nothing calls leaves no code
    static auto compile_noninline_functions(toc& tc, const size_t indent)
        -> void {

        // note: the calls in a body can request more instances, so the count
        //       is read on every pass
        for (size_t i{}; i < tc.noninline_instance_count(); ++i) {
            const noninline_instance instance{tc.noninline_instance_at(i)};

            compile_noninline_body(tc, indent, *instance.func,
                                   instance.array_lengths);
        }
    }

    // only the checks the user asked for have a handler
    static auto emit_failure_handlers(toc& tc) -> void {
        machine& x{tc.machine()};

        if (tc.is_frame_check()) {
            x.comment({}, 0, "frame overflow handler (--checks=frame)");
            x.emit_frame_overflow_handler();
        }

        if (tc.is_bounds_check_upper() or tc.is_bounds_check_lower()) {
            x.comment({}, 0,
                      "bounds failure handler (--checks=upper or "
                      "--checks=lower)");

            x.emit_bounds_failure_handler(tc.is_bounds_check_with_line());
        }
    }

    static auto emit_read_only_data(toc& tc) -> void {
        // no section switches without strings
        if (tc.get_string_constants().empty()) {
            return;
        }

        machine& x{tc.machine()};

        x.comment({}, 0, "");
        x.emit_string_constants(tc.get_string_constants());
    }
};
