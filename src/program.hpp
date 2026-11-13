#pragma once

#include <cstddef>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <system_error>
#include <tuple>
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
#include "stmt_include.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"

//
// the compile of a program, the types below in reading order:
//
//   'report_entry', 'report_section', 'report_renderer'
//                            the report after the code
//   'program'                parses the source, compiles the globals,
//                            'main', the noinline bodies it reaches, checks
//                            every function without its calls
//                            ('check_body'), writes the output
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
            x.comment({token{}, 0}, "{:>28}:", title);

            // note: 28 is the length of the longest name, 'removed same target
            //       branches', the entries below align with it
        }

        for (const report_entry& entry : entries) {
            if (entry.value.empty()) {
                x.comment({token{}, 0}, "{:>28}", entry.name);
                continue;
            }

            x.comment({token{}, 0}, "{:>28}: {}", entry.name, entry.value);
        }

        if (not title.empty()) {
            x.comment({token{}, 0}, "");
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
            x.comment({token{}, 0}, "");
        }

        for (const std::string& line : register_lines) {
            x.comment({token{}, 0}, "{}", line);
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
                    std::format("{} {}, {}, {}{}", f.body_count,
                                f.body_count == 1 ? "body" : "bodies",
                                counted(f.call_count, "call"),
                                counted(f.instruction_count, "instruction"),
                                f.call_count <= f.body_count ? ", no reuse"
                                                             : ""),
                },
            });
        }

        return section;
    }

    [[nodiscard]] static auto
    optimization_report(const machine::output_statistics& stats)
        -> report_section {

        const assembler::optimization_counts& counts{stats.optimizations};

        return {
            .title{},
            .entries{
                {
                    .name{"removed jumps to next code"},
                    .value{std::format("{}", counts.jumps_to_next)},
                },
                {
                    .name{"removed unreachable jumps"},
                    .value{std::format("{}", counts.unreachable_jumps)},
                },
                {
                    .name{"removed same target branches"},
                    .value{std::format("{}", counts.same_target_branches)},
                },
                {
                    .name{"inverted branches over jumps"},
                    .value{std::format("{}", counts.inverted_branches)},
                },
                {
                    .name{"removed reloads after stores"},
                    .value{std::format("{}", counts.removed_reloads)},
                },
                {
                    .name{"removed register copies"},
                    .value{std::format("{}", counts.removed_copies)},
                },
                {
                    .name{"removed tests after setcc"},
                    .value{std::format("{}", counts.removed_retests)},
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
// includes into 'parsed_files_' through a 'parse_toc', an included file before
// the file that includes it; 'build' compiles from the parsed statements
// through a 'compile_toc', the globals then 'main', see 'etc/docs/README.md'
// section 1.2
class program final {
    // the statements of one file: its includes, then its definitions
    struct parsed_file {
        size_t file_ix{};
        std::vector<std::unique_ptr<statement>> statements;
        // the end of the file, its whitespace holds the comments and blank
        // lines after the last statement
        token end_tk;
    };

    // a file whose includes are being parsed, see 'parse_files'
    struct open_file {
        tokenizer tz;
        parsed_file parsed;
    };

    std::reference_wrapper<source_files> files_;
    std::vector<parsed_file> parsed_files_;
    // the names and the storage of the program, see 'toc_tables'
    toc_tables tables_;

  public:
    // the main file is the first of 'files', the included ones are added
    program(machine& backend, const builtin_type_set& builtins,
            source_files& files, const size_t vars_size_bytes)
        : files_{files}, tables_{backend, builtins, files, vars_size_bytes} {

        parse_toc tc{tables_};

        {
            const parse_toc::scope_entry root{tc, scope::block()};

            parse_files(tc);
        }

        // the program starts at 'main', its signature is checked where it is
        // defined
        std::ignore = tc.get_main_or_throw();
    }

    auto build(std::ostream& os) -> void {
        compile_toc tc{tables_};

        machine& x{tc.machine()};

        x.start();
        compile(tc);
        x.finish();
        report_renderer::write(x, tc.usage());
        tc.assert_released();

        x.write_assembly(os);
    }

    // writes the source of the statements parsed from a file
    auto source_to(const size_t file_ix, std::ostream& os) const -> void {
        for (const parsed_file& parsed : parsed_files_) {
            if (parsed.file_ix != file_ix) {
                continue;
            }

            for (const std::unique_ptr<statement>& s : parsed.statements) {
                s->source_to(os);
            }

            parsed.end_tk.source_to(os);
        }
    }

  private:
    auto compile(compile_toc& tc) -> void {
        {
            const compile_toc::scope_entry root{tc, scope::block()};

            // a global 'var' or 'dat' is declared again with its storage, a
            // type writes its layout comment, a function emits nothing here
            for (const parsed_file& parsed : parsed_files_) {
                for (const std::unique_ptr<statement>& s : parsed.statements) {
                    s->compile(tc, destination{});
                }
            }

            tc.get_main_or_throw().compile_main(tc);
            emit_noinline_bodies(tc);
            check_function_bodies(tc);
            check_noinline_bodies(tc);
        }

        machine& x{tc.machine()};

        x.emit_runtime();

        emit_read_only_data(tc);
        emit_data_section(tc);
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

        return files_.get().add(path.string(), std::move(text),
                                source_span::at(path_tk));
    }

    [[nodiscard]] auto opened_file(const size_t file_ix) const -> open_file {
        tokenizer::assert_source_well_formed(file_ix,
                                             files_.get().text(file_ix));

        return {
            .tz{file_ix, files_.get().text(file_ix)},
            .parsed{
                .file_ix{file_ix},
                .statements{},
                .end_tk{},
            },
        };
    }

    // the definitions after the includes, 'tk' is the first of them; the
    // statements of the file follow those of the files it included
    auto parse_definitions(parse_toc& tc, open_file& file, token tk) -> void {
        while (not tk.is_none()) {
            // a name is seen only after its definition, see
            // 'definition_order'
            tc.begin_definition();

            file.parsed.statements.emplace_back(
                parse_definition(tc, file.tz, tk));

            tk = file.tz.next_token();
        }

        // a delimiter is not a token, only the end of the source ends the
        // definitions
        file.tz.assert_not_at_delimiter();

        file.parsed.end_tk = tk;
        parsed_files_.push_back(std::move(file.parsed));
    }

    // the includes of a file come before its definitions, so an included
    // file is parsed before the definitions of the file that includes it:
    // the last open file reads on, an include of a new file opens it, a
    // file without more includes parses its definitions and is closed; the
    // files are a stack, not nested calls, so an include is no level of the
    // nesting limit
    auto parse_files(parse_toc& tc) -> void {
        std::vector<open_file> open_files;
        open_files.push_back(opened_file(0));

        while (not open_files.empty()) {
            open_file& file{open_files.back()};
            const token tk{file.tz.next_token()};

            if (not tk.is_text("include")) {
                parse_definitions(tc, file, tk);
                open_files.pop_back();
                continue;
            }

            const std::optional<size_t> loaded_ix{
                parse_include(file, tk),
            };

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

        const std::filesystem::path includer{
            files_.get().name(file.parsed.file_ix),
        };

        // relative to the directory of the file with the 'include'
        const std::filesystem::path joined{
            includer.parent_path() / include->file_name(),
        };

        const std::filesystem::path path{include_path(joined)};

        const token path_tk{include->path_token()};

        file.parsed.statements.emplace_back(std::move(include));

        if (files_.get().contains(path)) {
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
    // compiled whole
    static auto check_function_bodies(compile_toc& tc) -> void {
        const stmt_def_func& func_main{tc.get_main_or_throw()};

        for (const stmt_def_func* const f : tc.funcs().defs()) {
            // a noinline body without array lengths was checked as emitted
            if (f != &func_main and not tc.emitted_bodies().contains(*f, {})) {

                f->check_body(tc);
            }
        }
    }

    // a noinline body that was not emitted, asked for only by skipped code
    // or by 'check_function_bodies', is checked with its array lengths, as
    // an inlined call there is
    static auto check_noinline_bodies(compile_toc& tc) -> void {

        // note: the calls in a body can request more bodies, so the count
        //       is read on every pass

        for (size_t i{}; i < tc.checked_bodies().count(); ++i) {
            const noinline_body body{tc.checked_bodies().at(i)};

            if (tc.emitted_bodies().contains(*body.func, body.array_lengths)) {

                continue;
            }

            body.func->check_noinline_body(tc, body);
        }
    }

    static auto emit_data_section(compile_toc& tc) -> void {
        machine& x{tc.machine()};

        x.begin_data();

        // the zero padding places each dat at the offset the data table gave it
        for (const placed_dat& dat : tc.data().dats()) {
            if (dat.padding_bytes != 0) {
                x.comment({token{}, 0}, "padding {} B", dat.padding_bytes);
                x.emit_zero_data(dat.padding_bytes);
            }

            dat.def->compile_data(tc);
        }

        x.reserve_variables(tc.layout().vars_capacity_bytes());
    }

    // each noinline function that is called has one body per array lengths,
    // after 'main'; a function nothing calls leaves no code
    static auto emit_noinline_bodies(compile_toc& tc) -> void {

        // note: the calls in a body can request more bodies, so the count
        //       is read on every pass

        for (size_t i{}; i < tc.emitted_bodies().count(); ++i) {
            const noinline_body body{tc.emitted_bodies().at(i)};

            body.func->emit_noinline_body(tc, body);
        }
    }

    static auto emit_read_only_data(compile_toc& tc) -> void {
        // no section switches without read-only constants
        if (tc.data().constants().empty()) {
            return;
        }

        machine& x{tc.machine()};

        x.comment({token{}, 0}, "");
        x.emit_read_only_constants(tc.data().constants());
    }

    // the directory as the file system resolves it ('resolved_path'), so
    // that a chain of includes does not grow its paths and the includes of
    // the file are relative to the directory it was opened from; a path that
    // does not name a file stays as it is, its open tells why
    [[nodiscard]] static auto include_path(const std::filesystem::path& joined)
        -> std::filesystem::path {

        std::error_code error;

        if (not std::filesystem::exists(joined, error)) {
            return joined;
        }

        // note: relative stays relative, as the other names of the files; a
        //       relative path that was not resolved is returned as it is

        if (joined.is_absolute()) {
            return source_files::resolved_path(joined);
        }

        const std::filesystem::path resolved{
            source_files::resolved_path(joined),
        };

        return resolved.lexically_proximate(std::filesystem::current_path());
    }
};
