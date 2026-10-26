#pragma once

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "assembler.hpp"
#include "compile_error.hpp"
#include "ident_info.hpp"
#include "machine.hpp"
#include "parse_statement.hpp"
#include "source_files.hpp"
#include "statement.hpp"
#include "stmt_def_dat.hpp"
#include "stmt_def_func.hpp"
#include "stmt_def_type.hpp"
#include "stmt_def_var.hpp"
#include "stmt_include.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"

//
// the compile of a program, the types below in reading order:
//
//   'report_entry', 'report_section', 'report_renderer'
//                            the '--report' text
//   'program'                parses the source, compiles 'main', the
//                            noinline bodies it reaches, checks every
//                            function without its calls ('check_body'),
//                            writes the output
//

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
            x.comment({}, 0, "{:>28}:", title);

            // note: 28 is the length of the longest name, 'removed same target
            //       branches', the entries below align with it
        }

        for (const report_entry& entry : entries) {
            if (entry.value.empty()) {
                x.comment({}, 0, "{:>28}", entry.name);
                continue;
            }

            x.comment({}, 0, "{:>28}: {}", entry.name, entry.value);
        }

        if (not title.empty()) {
            x.comment({}, 0, "");
        }
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

        noinline_report(stats).write_to(x);
        generics_report(usage).write_to(x);
        optimization_report(stats).write_to(x);

        usage_report(stats, usage).write_to(x);

        const std::vector<std::string> register_lines{x.register_peak_report()};

        // set apart from the reports above
        if (not register_lines.empty()) {
            x.comment({}, 0, "");
        }

        for (const std::string& line : register_lines) {
            x.comment({}, 0, "{}", line);
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
    noinline_report(const machine::output_statistics& stats) -> report_section {

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
    optimization_report(const machine::output_statistics& stats)
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
                    .value{std::format("{}", o.same_target_branches)},
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

        return {
            .title{},
            .entries{
                {
                    .name{"max scratch registers in use"},
                    .value{std::format("{}", stats.max_scratch_registers)},
                },
                {
                    .name{"max scopes in use"},
                    .value{std::format("{}", usage.max_scope_count)},
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
                    .name{"max body vars size"},
                    .value{std::format("{} B", usage.max_body_vars_size_bytes)},
                },
                {
                    .name{"instructions"},
                    .value{std::format("{}", stats.instruction_count)},
                },
            },
        };
    }
};

// the whole program: the constructor parses the main file and the files it
// includes into 'statements_', an included file before the file that
// includes it; 'build' compiles from the parsed statements, 'main' first, see
// 'etc/docs/README.md' section 1.2
class program final {
    // the statements of one file, in 'statements_'
    struct file_statements {
        size_t file_ix{};
        size_t begin{};
        size_t end{};
        // the end of the file, its whitespace holds the comments and blank
        // lines after the last statement
        token end_tk;
    };

    // a file whose includes are being parsed, see 'parse_files'
    struct open_file {
        size_t file_ix{};
        tokenizer tz;
        // its includes, then its definitions
        std::vector<std::unique_ptr<statement>> parsed;
    };

    std::reference_wrapper<source_files> files_;
    std::vector<std::unique_ptr<statement>> statements_;
    std::vector<file_statements> file_statements_;
    std::set<std::filesystem::path> loaded_paths_;
    toc tc_; // table of contents

  public:
    // the main file is the first of 'files', the included ones are added
    program(machine& backend, const builtin_type_set& builtins,
            source_files& files, const size_t vars_size_bytes,
            const check_options& checks)
        : files_{files},
          tc_{backend, builtins, files, vars_size_bytes, checks} {

        {
            const toc::scope_entry root{tc_, scope::block()};

            loaded_paths_.insert(identity_path(files.name(0)));

            parse_files(0);

            tc_.assert_generic_params_free();
        }

        // the program starts at 'main', its signature is checked where it is
        // defined
        std::ignore = tc_.get_main_or_throw();
    }

    auto build(std::ostream& os) -> void {
        tc_.begin_compile();

        machine& x{tc_.machine()};

        x.start();
        compile();
        x.finish();
        report_renderer::write(x, tc_.usage());
        tc_.assert_released();

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

            parsed.end_tk.source_to(os);
        }
    }

  private:
    auto compile() -> void {
        {
            const toc::scope_entry root{tc_, scope::block()};

            for (const std::unique_ptr<statement>& s : statements_) {
                s->compile(tc_, 0, destination{});
            }

            compile_main(tc_, tc_.get_main_or_throw());
            compile_noinline_functions(tc_);
            check_function_bodies(tc_);
        }

        machine& x{tc_.machine()};

        x.emit_runtime();

        emit_read_only_data(tc_);
        emit_data_section();
    }

    auto emit_data_section() -> void {
        machine& x{tc_.machine()};

        const size_t alignment{x.data_alignment()};
        x.begin_data(alignment);

        // the zero padding places each dat at the offset the data table gave it
        for (const placed_dat& dat : tc_.data().dats()) {
            if (dat.padding_bytes != 0) {
                x.comment({}, 0, "padding {} B", dat.padding_bytes);
                x.emit_zero_data(dat.padding_bytes);
            }

            dat.def->compile_data(tc_);
        }

        x.reserve_variables(alignment, tc_.layout().vars_capacity_bytes());
    }

    // the index of the file in 'files_'
    [[nodiscard]] auto load_file(const std::filesystem::path& path,
                                 const token& path_tk) -> size_t {

        std::string text;

        try {
            text = source_files::read_file(path.string());
        } catch (const unlocated_error& e) {
            throw compile_error{path_tk, e.what()};
        }

        return files_.get().add(path.string(), std::move(text));
    }

    [[nodiscard]] auto opened_file(const size_t file_ix) const -> open_file {
        return {
            .file_ix{file_ix},
            .tz{file_ix, files_.get().text(file_ix)},
            .parsed{},
        };
    }

    // the definitions after the includes, 'tk' is the first of them; the
    // statements of the file follow those of the files it included
    auto parse_definitions(open_file& file, token tk) -> void {
        while (true) {
            if (tk.is_none()) {
                // a delimiter is not a token, only the end of the source ends
                // the definitions
                file.tz.assert_not_at_delimiter();

                break;
            }

            tc_.begin_definition();
            file.parsed.emplace_back(parse_definition(tc_, file.tz, tk));
            tk = file.tz.next_token();
        }

        const size_t begin{statements_.size()};

        std::ranges::move(file.parsed, std::back_inserter(statements_));

        file_statements_.push_back({
            .file_ix{file.file_ix},
            .begin{begin},
            .end{statements_.size()},
            .end_tk{tk},
        });
    }

    // the includes of a file come before its definitions, so an included
    // file is parsed before the definitions of the file that includes it:
    // the last open file reads on, an include of a new file opens it, a
    // file without more includes parses its definitions and is closed; the
    // files are a stack, not nested calls, so an include is no level of the
    // nesting limit
    auto parse_files(const size_t main_file_ix) -> void {
        std::vector<open_file> open_files;
        open_files.push_back(opened_file(main_file_ix));

        while (not open_files.empty()) {
            open_file& file{open_files.back()};
            const token tk{file.tz.next_token()};

            if (not tk.is_text("include")) {
                parse_definitions(file, tk);
                open_files.pop_back();
                continue;
            }

            const std::optional<size_t> loaded_ix{parse_include(file, tk)};

            if (loaded_ix) {
                open_files.push_back(opened_file(*loaded_ix));
            }
        }
    }

    // a file is loaded once, a later 'include' of it, also from a file it
    // includes, changes nothing; the loaded file, none when it was loaded
    // before
    [[nodiscard]] auto parse_include(open_file& file, const token& include_tk)
        -> std::optional<size_t> {

        std::unique_ptr<stmt_include> include{
            std::make_unique<stmt_include>(include_tk, file.tz),
        };

        // relative to the directory of the file with the 'include'
        const std::filesystem::path path{
            (std::filesystem::path{files_.get().name(file.file_ix)}
                 .parent_path() /
             include->file_name())
                .lexically_normal(),
        };

        const token path_tk{include->path_token()};

        file.parsed.emplace_back(std::move(include));

        const bool is_new{loaded_paths_.insert(identity_path(path)).second};

        if (not is_new) {
            return std::nullopt;
        }

        return load_file(path, path_tk);
    }

    //
    // statics
    //

    // the rules checked while compiling, e.g. a constant index in bounds,
    // hold in every function without its calls too, also where a call folds
    // the code away, so whether a program is accepted does not depend on its
    // calls; this includes the instances that an alias of a generic type
    // makes of its methods; 'main' takes nothing from a call and was
    // compiled whole; the list is taken before the checks, a check compiles
    // the functions it calls without checking them on their own
    static auto check_function_bodies(toc& tc) -> void {
        const stmt_def_func& func_main{tc.get_main_or_throw()};

        std::vector<const stmt_def_func*> funcs;

        for (const stmt_def_func* const f : tc.funcs().defs()) {
            if (f != &func_main) {
                funcs.push_back(f);
            }
        }

        for (const stmt_def_func* const f : funcs) {
            f->check_body(tc, 0);
        }
    }

    // the main function is compiled where the program starts
    static auto compile_main(toc& tc, const stmt_def_func& func_main) -> void {
        machine& x{tc.machine()};

        const toc::scope_entry main{tc, scope::main_func()};

        x.comment({}, 0, "");

        x.label(0, reserved_names::main);

        func_main.compile_statements(tc, 0);

        // code after an 'exit' or 'return' on every path would never run;
        // the compile knows the conditions it folded, e.g. with constant
        // arguments of an inlined call
        if (tc.is_reachable()) {
            x.end_main();
        }
    }

    // each noinline function that is called has one body per array lengths,
    // after 'main'; a function nothing calls leaves no code
    static auto compile_noinline_functions(toc& tc) -> void {

        // note: the calls in a body can request more bodies, so the count
        //       is read on every pass

        for (size_t i{}; i < tc.noinline_worklist().noinline_body_count();
             ++i) {

            const noinline_body body{
                tc.noinline_worklist().noinline_body_at(i),
            };

            body.func->emit_noinline_body(tc, body.array_lengths);
        }
    }

    static auto emit_read_only_data(toc& tc) -> void {
        // no section switches without strings
        if (tc.data().constants().empty()) {
            return;
        }

        machine& x{tc.machine()};

        x.comment({}, 0, "");
        x.emit_string_constants(tc.data().constants());
    }

    // the path that tells loaded files apart; a file without a canonical
    // path, e.g. '/dev/stdin' of a pipe, is known by its own, a file that
    // cannot be read is reported when it is loaded
    [[nodiscard]] static auto identity_path(const std::filesystem::path& name)
        -> std::filesystem::path {

        std::error_code error;

        const std::filesystem::path path{
            std::filesystem::weakly_canonical(name, error),
        };

        return error ? name.lexically_normal() : path;
    }
};
