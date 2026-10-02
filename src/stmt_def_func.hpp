#pragma once
// reviewed: 2025-09-28

#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "stmt_block.hpp"
#include "stmt_def_func_param.hpp"

class stmt_def_func final : public statement {
    token noinline_tk_;
    token mut_tk_;
    token name_tk_;
    token method_dot_tk_;
    token method_name_tk_;
    std::string name_;
    token open_paren_tk_;
    std::vector<stmt_def_func_param> params_;
    std::vector<token> param_delims_tk_;
    token close_paren_tk_;
    std::optional<func_return_info> returns_;
    stmt_block code_;

  public:
    stmt_def_func(toc& tc, const token tk, tokenizer& tz)
        : statement{tk}, name_tk_{tz.next_token()},
          open_paren_tk_{tz.is_next_char_token('(')} {

        if (name_tk_.is_text("noinline") and open_paren_tk_.is_empty()) {
            noinline_tk_ = name_tk_;
            name_tk_ = tz.next_token();
            open_paren_tk_ = tz.is_next_char_token('(');
        }

        // a function named 'mut' is followed by '('
        if (name_tk_.is_text("mut") and open_paren_tk_.is_empty()) {
            mut_tk_ = name_tk_;
            name_tk_ = tz.next_token();
            open_paren_tk_ = tz.is_next_char_token('(');
        }

        name_ = name_tk_.text();

        toc::assert_name_not_reserved(name_tk_);

        // e.g. 'func list.add(x)'
        if (open_paren_tk_.is_empty()) {
            method_dot_tk_ = tz.is_next_char_token('.');
            if (not method_dot_tk_.is_empty()) {
                parse_method_name(tc, tz);
                open_paren_tk_ = tz.is_next_char_token('(');
            }
        }

        if (open_paren_tk_.is_empty()) {
            throw compiler_exception{name_tk_,
                                     "expected '(' after function name"};
        }

        parse_params(tc, tz);

        parse_returns(tc, tz);

        assert_mut_has_receiver();

        // known only after the result: a constructor builds 'self' instead
        if (is_method()) {
            add_self_param(tc);
        }

        tc.add_func(name_tk_, name_, statement::get_type(), this);

        // establish the function scope before parsing its body
        tc.enter_func(name(), {}, {}, is_inlined());

        // register variables without emitting output so that the function body
        // can be parsed

        // note: 'parse_returns' only creates a return with a name
        assert(not returns_ or not returns_->ident_tk.text().empty());

        add_signature_vars(tc, 0, false);

        code_ = {tc, tz, true};

        tc.exit_func(name());
    }

    stmt_def_func() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        source_def_to(os, false);
        code_.source_to(os);
    }

    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {}

    //
    // class methods
    //

    // 'func' is a keyword so no user name or internal label starts with 'func.'
    [[nodiscard]] auto body_label() const -> std::string {
        return std::format("func.{}", name());
    }

    [[nodiscard]] auto code() const -> const stmt_block& { return code_; }

    [[nodiscard]] auto compile_body(toc& tc, const size_t indent) const
        -> size_t {

        assert(not is_inlined());

        for (const stmt_def_func_param& param : params_) {
            if (param.is_array()) {
                throw compiler_exception{
                    param.tok(),
                    "non-inline functions require non-array parameters"};
            }
        }

        machine& x{tc.machine()};

        x.reserve_frame_base();
        tc.enter_func(name(), {}, {}, false, x.frame_base_register());
        add_signature_vars(tc, indent + 1, true);
        code_.compile(tc, indent, ident_info::make_empty());
        x.return_function(indent + 1);
        const size_t frame_size_bytes{tc.peak_frame_size_bytes()};
        tc.exit_func(name());
        x.release_frame_base();

        return frame_size_bytes;
    }

    // a suffix could collide with a method body, e.g. 'func.list.size' of
    // function 'list' and method 'list.size'
    [[nodiscard]] auto frame_size_label() const -> std::string {
        return std::format("size.{}", body_label());
    }

    // e.g. 'func point.at(x, y) self'
    [[nodiscard]] auto is_constructor() const -> bool {
        return returns_.has_value() and returns_->ident_tk.is_text("self");
    }

    [[nodiscard]] auto is_inlined() const -> bool {
        return noinline_tk_.is_empty();
    }

    // has the implicit first parameter 'self'
    [[nodiscard]] auto is_method() const -> bool {
        return not method_dot_tk_.is_empty() and not is_constructor();
    }

    [[nodiscard]] auto name() const -> std::string_view { return name_; }

    [[nodiscard]] auto param(const size_t ix) const
        -> const stmt_def_func_param& {

        return params_.at(ix);
    }

    [[nodiscard]] auto params() const -> std::span<const stmt_def_func_param> {
        return params_;
    }

    [[nodiscard]] auto returns() const
        -> const std::optional<func_return_info>& {

        return returns_;
    }

    auto source_def_comment_to(machine& x, const size_t indent) const -> void {
        std::stringstream ss;
        source_def_to(ss, true);
        x.comment(name_tk_, indent, "{}", statement::trimmed_source(ss.view()));
    }

    auto source_def_to(std::ostream& os, const bool summary) const -> void {
        if (not summary) {
            statement::source_to(os);
        }
        noinline_tk_.source_to(os);
        mut_tk_.source_to(os);
        name_tk_.source_to(os);
        if (not method_dot_tk_.is_empty()) {
            method_dot_tk_.source_to(os);
            method_name_tk_.source_to(os);
        }
        open_paren_tk_.source_to(os);

        // the implicit 'self' has no source and no delimiter after it
        const size_t first_param{is_method() ? size_t{1} : size_t{0}};
        if (params_.size() > first_param) {
            params_.at(first_param).source_to(os);
            for (const auto [d, e] :
                 std::views::zip(param_delims_tk_,
                                 params_ | std::views::drop(first_param + 1))) {

                d.source_to(os);
                e.source_to(os);
            }
        }
        close_paren_tk_.source_to(os);

        if (returns_) {
            returns_->ident_tk.source_to(os);
            if (not returns_->type_tk.is_empty()) {
                returns_->type_tk.source_to(os);
            }
        }
    }

  private:
    // located at the method name for diagnostics
    auto add_self_param(const toc& tc) -> void {
        const token self_tk{
            "",     method_name_tk_.start_index(),
            "self", method_name_tk_.start_index(),
            "",     method_name_tk_.at_line(),
            false,
        };

        params_.insert(
            params_.begin(),
            stmt_def_func_param{self_tk,
                                tc.get_type_or_throw(name_tk_, name_tk_.text()),
                                mut_tk_.is_empty()});
    }

    // a non-inline body reaches the result and the arguments through pointer
    // slots in its frame
    auto add_signature_vars(toc& tc, const size_t indent,
                            const bool is_pointer) const -> void {

        if (returns_) {
            tc.add_var(returns_->ident_tk, indent,
                       {
                           .name{returns_->ident_tk.text()},
                           .type_ptr{&get_type()},
                           .src_loc_tk{returns_->ident_tk},
                           .is_pointer{is_pointer},
                           .pointer_register{},
                           .base_register{},
                           .value_register{},
                       },
                       false);
        }

        for (const stmt_def_func_param& param : params_) {
            tc.add_var(param.tok(), indent,
                       {
                           .name{param.name()},
                           .type_ptr{&param.get_type()},
                           .src_loc_tk{param.tok()},
                           .is_array{param.is_array()},
                           .is_pointer{is_pointer},
                           .read_only_why{
                               param.is_read_only() ? read_only_cause::PARAM
                                                    : read_only_cause::NONE,
                           },
                           .pointer_register{},
                           .base_register{},
                           .value_register{},
                       },
                       false);
        }
    }

    // e.g. 'func mut point.move(dx)', a constructor writes its 'self' anyway
    auto assert_mut_has_receiver() const -> void {
        if (mut_tk_.is_empty() or is_method()) {
            return;
        }

        if (is_constructor()) {
            throw compiler_exception{
                mut_tk_, "a constructor builds 'self', 'mut' is not needed"};
        }

        throw compiler_exception{mut_tk_,
                                 "'mut' requires a method 'type.name'"};
    }

    // 'name_tk_' is the receiver type, a method gets an implicit first
    // parameter 'self' of that type and a constructor builds a 'self' of it
    auto parse_method_name(const toc& tc, tokenizer& tz) -> void {
        const type& receiver_type{
            tc.get_type_or_throw(name_tk_, name_tk_.text()),
        };

        if (receiver_type.is_builtin()) {
            throw compiler_exception{
                name_tk_, std::format("methods on built-in type '{}' are not "
                                      "supported",
                                      receiver_type.name())};
        }

        method_name_tk_ = tz.next_token();
        if (method_name_tk_.text().empty()) {
            throw compiler_exception{tz, "expected method name after '.'"};
        }

        toc::assert_name_not_reserved(method_name_tk_);

        name_ =
            std::format("{}.{}", receiver_type.name(), method_name_tk_.text());
    }

    auto parse_param_delimiter(tokenizer& tz) -> void {
        const token t{tz.is_next_char_token(',')};
        if (t.is_empty()) {
            throw compiler_exception{
                tz, std::format("expected ',' or ')' after parameter '{}'",
                                params_.back().tok().text())};
        }

        param_delims_tk_.emplace_back(t);
    }

    // e.g. 'a i32, b i32' up to the closing parenthesis
    auto parse_params(toc& tc, tokenizer& tz) -> void {
        close_paren_tk_ = tz.is_next_char_token(')');

        while (close_paren_tk_.is_empty()) {
            if (not params_.empty()) {
                parse_param_delimiter(tz);
            }

            params_.emplace_back(tc, tz);

            close_paren_tk_ = tz.is_next_char_token(')');
        }
    }

    // 'name [type]' of the returned value, without a name the type is void
    auto parse_returns(toc& tc, tokenizer& tz) -> void {
        const token ident_tk{tz.next_token()};
        if (ident_tk.text().empty()) {
            tz.put_back_token(ident_tk);
            set_type(tc.get_type_void());

            return;
        }

        token type_tk{tz.next_token()};
        if (not tc.has_type(type_tk.text())) {
            tz.put_back_token(type_tk);
            type_tk = {};
        }

        if (ident_tk.is_text("self")) {
            set_constructor_result(tc, ident_tk, type_tk);
            return;
        }

        const type& tp{
            type_tk.is_empty() ? tc.get_type_default()
                               : tc.get_type_or_throw(type_tk, type_tk.text()),
        };

        returns_.emplace(type_tk, ident_tk, &tp);
        set_type(tp);
    }

    // e.g. 'func point.at(x, y) self' builds a 'point'
    auto set_constructor_result(const toc& tc, const token& self_tk,
                                const token& type_tk) -> void {

        if (method_dot_tk_.is_empty()) {
            throw compiler_exception{
                self_tk, "result 'self' requires a constructor 'type.name'"};
        }

        // one spelling, the type is already named before the '.'
        if (not type_tk.is_empty()) {
            throw compiler_exception{
                type_tk, "result 'self' has the type of the constructor"};
        }

        const type& tp{tc.get_type_or_throw(name_tk_, name_tk_.text())};

        returns_.emplace(type_tk, self_tk, &tp);
        set_type(tp);
    }
};
