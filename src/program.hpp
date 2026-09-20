#pragma once
// reviewed: 2025-09-28

#include <memory>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "stmt_def_const.hpp"
#include "stmt_def_func.hpp"
#include "stmt_def_type.hpp"
#include "toc.hpp"
#include "tokenizer.hpp"
#include "unary_ops.hpp"

class program final {
    // built-in types
    type type_void{"void", 0, true};
    type type_i64{"i64", operand::size_qword, true};
    type type_i32{"i32", operand::size_dword, true};
    type type_i16{"i16", operand::size_word, true};
    type type_i8{"i8", operand::size_byte, true};
    type type_bool{"bool", operand::size_byte, true};

    std::vector<std::unique_ptr<statement>> statements_;
    toc tc_; // table of contents
    size_t vars_size_bytes_{};

  public:
    program(machine& backend, const std::string_view source,
            const size_t vars_size_bytes, const bool bounds_check_upper,
            const bool bounds_check_lower, const bool bounds_check_with_line)
        : tc_{backend,
              source,
              vars_size_bytes,
              bounds_check_upper,
              bounds_check_lower,
              bounds_check_with_line},
          vars_size_bytes_{vars_size_bytes} {

        // create a placeholder token to use with 'toc' functions
        const token src_loc_tk{};

        // add built-in assembler calls
        tc_.add_func(src_loc_tk, "mov", type_void, nullptr);
        tc_.add_func(src_loc_tk, "syscall", type_void, nullptr);

        // add built-in types
        tc_.add_type(src_loc_tk, type_i64);
        tc_.add_type(src_loc_tk, type_i32);
        tc_.add_type(src_loc_tk, type_i16);
        tc_.add_type(src_loc_tk, type_i8);
        tc_.add_type(src_loc_tk, type_bool);
        tc_.add_type(src_loc_tk, type_void);

        // set defaults
        tc_.set_type_void(type_void);
        tc_.set_type_bool(type_bool);

        machine& x{tc_.machine()};

        x.set_builtin_types(type_i64, type_i32, type_i16, type_i8, type_bool,
                            type_void);

        tc_.enter_block();

        tokenizer tz{source};
        while (true) {
            const token tk{tz.next_token()};
            if (tk.is_empty()) {
                if (not tz.is_eos()) {
                    throw compiler_exception{tk, "unexpected character"};
                }
                break;
            }
            if (tk.is_text("func")) {
                statements_.emplace_back(
                    std::make_unique<stmt_def_func>(tc_, tk, tz));
            } else if (tk.is_text("type")) {
                statements_.emplace_back(
                    std::make_unique<stmt_def_type>(tc_, tk, tz));
            } else if (tk.is_text("const")) {
                statements_.emplace_back(
                    std::make_unique<stmt_def_const>(tc_, tk, tz));
            } else if (tk.is_text("dat")) {
                statements_.emplace_back(
                    std::make_unique<stmt_def_dat>(tc_, tk, tz));
            } else if (tk.is_text("var")) {
                statements_.emplace_back(
                    std::make_unique<stmt_def_var>(tc_, tk, tz));
            } else if (tk.text().starts_with("#")) {
                statements_.emplace_back(
                    std::make_unique<stmt_comment>(tc_, unary_ops{}, tk, tz));
            } else {
                throw compiler_exception{
                    tk, std::format("unexpected keyword '{}'", tk.text())};
            }
        }

        tc_.exit_block();

        assert_functions_set_return_value(tc_.get_func_defs());
    }

    auto source_to(std::ostream& os) const -> void {
        for (const std::unique_ptr<statement>& s : statements_) {
            s->source_to(os);
        }
    }

    auto compile(toc& tc, const size_t indent) const -> void {
        tc.reset_usage();

        machine& x{tc.machine()};

        x.program_start();

        tc.enter_block();

        for (const std::unique_ptr<statement>& s : statements_) {
            s->compile(tc, indent, ident_info::make_empty());
        }
        const stmt_def_func& func_main{tc.get_func_or_throw(token{}, "main")};

        x.println();

        x.label(0, "main");
        tc.enter_func("main", {});
        func_main.code().compile(tc, indent, ident_info::make_empty());
        tc.exit_func("main");

        tc.exit_block();

        x.program_end();
        if (tc.is_bounds_check_upper() or tc.is_bounds_check_lower()) {
            x.emit_bounds_failure_handler(tc.is_bounds_check_with_line());
        }

        // data section
        const size_t alignment{x.data_alignment()};
        x.begin_data(alignment);

        for (const statement* s : tc.get_data()) {
            s->compile_data(tc);
        }

        x.reserve_variables(alignment, vars_size_bytes_);
    }

    auto build(std::ostream& os) -> void {
        machine& x{tc_.machine()};

        std::ostream& previous{x.use_stream(os)};
        x.reserve_variables_base();
        compile(tc_, 0);
        x.release_variables_base();
        x.finish();
        tc_.finish();
        x.use_stream(previous);
    }

    static auto assert_functions_set_return_value(
        const std::span<const stmt_def_func* const>& funcs) -> void {

        for (const stmt_def_func* f : funcs) {
            const std::optional<func_return_info> ret_info{f->returns()};
            if (not ret_info) {
                continue;
            }
            if (not f->code().is_var_set(ret_info->ident_tk.text())) {
                throw compiler_exception{
                    f->tok(),
                    "function may return without setting its return value"};
            }
            f->code().assert_no_ub_for_var(ret_info->ident_tk.text());
        }
    }
};
