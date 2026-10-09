#pragma once
// reviewed: 2025-09-28

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
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
#include "stmt_include.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"
#include "ub_assigned.hpp"
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
                .assigned{assigned_bytes{ret_info->type_ptr->size_bytes()}},
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
            .assigned{assigned_bytes{0}},
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
                    .name{"removed jumps to next code"},
                    .value{std::format("{}", o.jumps_to_next)},
                },
                {
                    .name{"removed unreachable jumps"},
                    .value{std::format("{}", o.unreachable_jumps)},
                },
                {
                    .name{"removed same target branches"},
                    .value{std::format("{}", o.same_outcome_branches)},
                },
                {
                    .name{"inverted branches over jumps"},
                    .value{std::format("{}", o.inverted_branches)},
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
                    .name{"max scratch registers in use"},
                    .value{std::format("{}", stats.max_scratch_registers)},
                },
                {
                    .name{"max frames in use"},
                    .value{std::format("{}", usage.max_frame_count)},
                },
                {
                    .name{"dat size"},
                    .value{std::format("{} B", usage.dat_size_bytes)},
                },
                {
                    .name{"dat var padding"},
                    .value{std::format("{} B", usage.dat_var_padding_bytes)},
                },
                {
                    .name{"max vars size"},
                    .value{std::format("{} B", usage.max_vars_size_bytes)},
                },
            },
        };

        if (stats.is_counted) {
            section.entries.push_back({
                .name{"instructions"},
                .value{std::format("{}", stats.instruction_count)},
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

    // the statements of one file, in 'statements_'
    struct file_statements {
        size_t file_ix{};
        size_t begin{};
        size_t end{};
    };

    std::reference_wrapper<source_files> files_;
    std::vector<std::unique_ptr<statement>> statements_;
    std::vector<file_statements> file_statements_;
    std::set<std::filesystem::path> loaded_paths_;
    toc tc_; // table of contents

  public:
    // the main file is the first of 'files', the included ones are added
    program(machine& backend, source_files& files, const size_t vars_size_bytes,
            const check_options& checks)
        : files_{files}, tc_{backend, files, vars_size_bytes, checks} {

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

        loaded_paths_.insert(std::filesystem::weakly_canonical(files.name(0)));

        parse_file(0);

        tc_.exit_block();

        assignment_analysis::assert_functions_set_return_value(
            tc_.get_func_defs());
    }

    auto build(std::ostream& os) -> void {
        machine& x{tc_.machine()};

        x.start();
        compile(0);
        x.finish();
        report_renderer::write(x, tc_.usage());
        tc_.finish();

        x.write_assembly(os);
    }

    // writes the source of the statements parsed from a file
    auto source_to(const size_t file_ix, std::ostream& os) const -> void {
        for (const file_statements& parsed : file_statements_) {
            if (parsed.file_ix != file_ix) {
                continue;
            }

            for (size_t i{parsed.begin}; i < parsed.end; ++i) {
                statements_.at(i)->source_to(os);
            }
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

        if (tk.is_text("include")) {
            throw compiler_exception{
                tk, "'include' must be before the definitions of the file"};
        }

        throw compiler_exception{
            tk, std::format("unexpected keyword '{}'", tk.text())};
    }

  private:
    auto compile(const size_t indent) -> void {
        tc_.reset_usage();

        tc_.enter_block();

        for (const std::unique_ptr<statement>& s : statements_) {
            s->compile(tc_, indent, ident_info::make_empty());
        }

        compile_main(tc_, indent);
        compile_noninline_functions(tc_, indent);

        tc_.exit_block();

        emit_failure_handlers(tc_);
        emit_read_only_data(tc_);
        emit_data_section();
    }

    auto emit_data_section() -> void {
        machine& x{tc_.machine()};

        const size_t alignment{x.data_alignment()};
        x.begin_data(alignment);

        // zero padding places each dat at the offset 'toc::add_var' gave it
        size_t dat_offset{};
        for (const statement* s : tc_.get_data()) {
            const size_t padding_bytes{
                align_storage_size(dat_offset, s->get_type().alignment()) -
                    dat_offset,
            };

            if (padding_bytes != 0) {
                x.comment({}, 0, "padding {} B", padding_bytes);
                x.emit_zero_data(padding_bytes);
            }

            s->compile_data(tc_);

            dat_offset = add_storage_size(s->tok(), dat_offset + padding_bytes,
                                          s->dat_size_bytes());
        }

        x.reserve_variables(alignment, tc_.vars_capacity_bytes());
    }

    auto load_file(const std::filesystem::path& path, const token& path_tk)
        -> void {

        std::string text;

        try {
            text = source_files::read_file(path.string());
        } catch (const std::runtime_error& e) {
            throw compiler_exception{path_tk, e.what()};
        }

        const size_t file_ix{files_.get().add(path.string(), std::move(text))};

        parse_file(file_ix);
    }

    // the included files are parsed where their 'include' is, before the
    // definitions of the including file
    auto parse_file(const size_t file_ix) -> void {
        tokenizer tz{file_ix, files_.get().text(file_ix)};
        std::vector<std::unique_ptr<statement>> parsed;

        token tk{tz.next_token()};

        while (tk.is_text("include")) {
            parsed.emplace_back(parse_include(tz, tk, file_ix));
            tk = tz.next_token();
        }

        while (true) {
            if (tk.text().empty() and not tk.is_string()) {
                // a delimiter is not a token, only the end of the source ends
                // the definitions
                tz.assert_not_at_delimiter();

                break;
            }

            parsed.emplace_back(parse_definition(tc_, tz, tk));
            tk = tz.next_token();
        }

        const size_t begin{statements_.size()};

        std::ranges::move(parsed, std::back_inserter(statements_));

        file_statements_.push_back({
            .file_ix{file_ix},
            .begin{begin},
            .end{statements_.size()},
        });
    }

    // a file is parsed once, a later 'include' of it, also from a file it
    // includes, changes nothing
    [[nodiscard]] auto parse_include(tokenizer& tz, const token include_tk,
                                     const size_t file_ix)
        -> std::unique_ptr<statement> {

        const token path_tk{tz.next_token()};

        if (not path_tk.is_string()) {
            throw compiler_exception{
                path_tk, "expected the file name in quotes after 'include'"};
        }

        // relative to the directory of the file with the 'include'
        const std::filesystem::path path{
            (std::filesystem::path{files_.get().name(file_ix)}.parent_path() /
             std::string{path_tk.text()})
                .lexically_normal(),
        };

        const bool is_new{
            loaded_paths_.insert(std::filesystem::weakly_canonical(path))
                .second,
        };

        if (is_new) {
            load_file(path, path_tk);
        }

        return std::make_unique<stmt_include>(include_tk, path_tk);
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

        if (tc.is_division_check()) {
            x.comment({}, 0, "division failure handler (--checks=division)");
            x.emit_division_failure_handler(tc.is_bounds_check_with_line());
        }

        if (tc.is_shift_check()) {
            x.comment({}, 0, "shift failure handler (--checks=shift)");
            x.emit_shift_failure_handler(tc.is_bounds_check_with_line());
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
