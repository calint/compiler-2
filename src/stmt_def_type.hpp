#pragma once

#include <cassert>
#include <cstddef>
#include <format>
#include <memory>
#include <ostream>
#include <print>
#include <ranges>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "generics.hpp"
#include "ident_info.hpp"
#include "machine.hpp"
#include "statement.hpp"
#include "stmt_def_func.hpp"
#include "stmt_def_type_field.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"

// the braces, fields and delimiters of 'type point { x, y }'
class type_fields final {
    token open_brace_tk_;
    std::vector<stmt_def_type_field> fields_;
    std::vector<token> field_delims_tk_;
    token close_brace_tk_;

  public:
    auto add_to(toc& tc, type& tp, const token& name_tk) const -> void {
        tp.set_name(name_tk.text());

        for (const stmt_def_type_field& fld : fields_) {
            toc::assert_valid_name(fld.tok());

            if (tp.has_field(fld.name())) {
                throw compile_error{
                    fld.tok(),
                    std::format("field {} is declared twice in type {}",
                                in_quotes(fld.name()),
                                in_quotes(name_tk.text()))};
            }

            // a field without a type name has the default type, e.g. 'x' of
            // 'type point { x, y }'
            const type& fld_tp{
                fld.type_str().empty() ? tc.builtins().default_type()
                                       : tc.get_type_or_throw(fld.type_token()),
            };

            tp.add_field(fld.tok(), fld.name(), fld_tp, fld.array_count());
        }
    }

    // e.g. 'x, y }' after the '{' of 'type point { x, y }', 'name_tk' names
    // the type in the errors
    auto parse(toc& tc, tokenizer& tz, const token& open_brace_tk,
               const token& name_tk) -> void {

        open_brace_tk_ = open_brace_tk;

        while (true) {
            // a field starts with its name
            // e.g. 'items point[4]'
            fields_.emplace_back(tc, tz.next_token(), tz);

            if (is_next_close_brace(tz)) {
                break;
            }

            const token delimiter_tk{
                tz.expect_char_token(
                    ',', std::format("expected ',' followed by another field "
                                     "in type {}",
                                     in_quotes(name_tk.text()))),
            };

            field_delims_tk_.emplace_back(delimiter_tk);

            // a trailing ',' lets each field end its line the same way
            // e.g. 'type point { x, y, }'
            if (is_next_close_brace(tz)) {
                break;
            }
        }
    }

    auto source_to(std::ostream& os) const -> void {
        open_brace_tk_.source_to(os);

        // note: a type has at least one field
        assert(not fields_.empty());

        statement::source_delimited_to(os, fields_, field_delims_tk_);

        close_brace_tk_.source_to(os);
    }

  private:
    // e.g. the '}' of 'type point { x, y }', kept for 'source_to'
    auto is_next_close_brace(tokenizer& tz) -> bool {
        close_brace_tk_ = tz.is_next_char_token('}');
        return not close_brace_tk_.is_empty();
    }
};

// the field layout of the type 'tp' as comments
inline auto comment_type_layout(toc& tc, const size_t indent, const token& tk,
                                const type& tp) -> void {

    machine& x{tc.machine()};

    x.comment(tk, indent, "{} : {} B    fields:", tp.name(), tp.size_bytes());

    x.comment(tk, indent, "{:>10} : {:>7} : {:>7} : {:>7} : {:>10}", "name",
              "offset", "size", "array?", "array size");

    for (const type_field& f : tp.fields()) {
        x.comment(tk, indent, "{:>10} : {:>7} : {:>7} : {:>7} : {:>10}", f.name,
                  f.offset, f.size_bytes, f.is_array() ? "yes" : "no",
                  f.array_count ? std::format("{}", *f.array_count) : "");
    }

    x.comment({}, 0, "");
}

inline auto assert_arg_count(const token& generic_tk,
                             const generic_type_info& generic,
                             const std::span<const token> arg_tks) -> void {

    if (arg_tks.size() == generic.params.size()) {
        return;
    }

    throw compile_error{generic_tk,
                        std::format("generic type {} takes {}, got {}",
                                    in_quotes(generic_tk.text()),
                                    counted(generic.params.size(), "argument"),
                                    arg_tks.size())};
}

// e.g. 'text' of 'type str = text<8>', 'user' is what must come after the
// generic type
inline auto assert_generic_type(const toc& tc, const token& generic_tk,
                                const std::string_view user) -> void {

    if (tc.types().has(generic_tk.text())) {
        throw compile_error{generic_tk,
                            std::format("{} is a type, not a generic type",
                                        in_quotes(generic_tk.text()))};
    }

    if (not tc.generics().has_type(generic_tk.text())) {
        throw compile_error{
            generic_tk,
            std::format("{} is not a generic type, a generic type is "
                        "defined before {}",
                        in_quotes(generic_tk.text()), user)};
    }
}

// e.g. '<8>' after 'text'
[[nodiscard]] inline auto parse_generic_args(tokenizer& tz,
                                             const token& generic_tk)
    -> generic_arg_tokens {

    const token open_tk{tz.is_next_char_token('<')};

    if (open_tk.is_empty()) {
        throw compile_error{
            generic_tk,
            std::format("generic type {} needs arguments, e.g. '{}<...>'",
                        in_quotes(generic_tk.text()), generic_tk.text())};
    }

    return generic_arg_tokens::parse(tz, open_tk);
}

// e.g. 'type point { x, y }'
class stmt_def_type final : public statement {
    token name_tk_;
    type_fields fields_;
    type type_;

  public:
    stmt_def_type(toc& tc, const token tk, const token name_tk, tokenizer& tz)
        : statement{tk}, name_tk_{name_tk} {

        // e.g. the '{' of 'type point { x, y }'
        const token open_brace_tk{tz.is_next_char_token('{')};

        fields_.parse(tc, tz, open_brace_tk, name_tk_);

        fields_.add_to(tc, type_, name_tk_);

        tc.add_type(name_tk_, type_);
    }

    stmt_def_type() = default;
    // 'toc' keeps the address of the type
    stmt_def_type(const stmt_def_type&) = delete;
    stmt_def_type(stmt_def_type&&) = delete;
    auto operator=(const stmt_def_type&) -> stmt_def_type& = delete;
    auto operator=(stmt_def_type&&) -> stmt_def_type& = delete;

    ~stmt_def_type() override = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);
        fields_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        comment_type_layout(tc, indent, tok(), type_);
    }
};

// e.g. 'type text<size> = list<i8, size>'
class stmt_def_type_generic final : public statement {
    // where the definition continues after 'type', an alias parses it again
    token start_tk_;
    token name_tk_;
    // the text after 'type' of a generic definition, only its aliases parse it
    std::string_view generic_text_;

  public:
    stmt_def_type_generic(toc& tc, const token tk, const token start_tk,
                          const token name_tk, tokenizer& tz)
        : statement{tk}, start_tk_{start_tk}, name_tk_{name_tk} {

        define_generic(tc, tk, tz);
    }

    stmt_def_type_generic() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        std::print(os, "{}", generic_text_);
    }

    // only its aliases are types
    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {}

  private:
    // the fields and the methods are those of the base, the arguments of the
    // base are resolved when an alias names this type
    auto define_based_generic(toc& tc, const token& type_tk, tokenizer& tz,
                              std::vector<generic_param> params) -> void {

        // e.g. '=' and 'list' of '= list<i8, size>'
        const token equals_tk{tz.is_next_char_token('=')};
        const token base_tk{tz.next_token()};

        // note: the '=' is the place when the source ends after it

        if (not base_tk.is_word()) {
            throw compile_error{base_tk.is_none() ? equals_tk : base_tk,
                                "expected a generic type after '='"};
        }

        assert_generic_type(tc, base_tk, "the type based on it");

        // e.g. 'i8' and 'size' of '<i8, size>'
        generic_arg_tokens args{parse_generic_args(tz, base_tk)};

        assert_arg_count(base_tk, tc.generics().get_type(base_tk.text()),
                         args.arg_tks);

        tc.add_generic_type(name_tk_.text(),
                            {
                                .src_loc_tk{name_tk_},
                                .start_tk{start_tk_},
                                .params{std::move(params)},
                                .base_tk{base_tk},
                                .base_arg_tks{std::move(args.arg_tks)},
                                .definition{tc.current_definition()},
                            });

        // e.g. 'text<size> = list<i8, size>'
        generic_text_ = tz.text_after(type_tk);
    }

    // the text after 'type' is kept for 'source_to' and for the aliases, errors
    // in it are found when an alias parses it
    auto define_generic(toc& tc, const token& type_tk, tokenizer& tz) -> void {
        // e.g. 'capacity' of '<capacity>'
        std::vector<generic_param> params{generic_param::parse(tz)};

        for (const generic_param& param : params) {
            tc.assert_generic_param_free(param.name_tk);
        }

        // e.g. 'type text<size> = list<i8, size>'
        if (tz.peek_char_after_whitespace() == '=') {
            define_based_generic(tc, type_tk, tz, std::move(params));
            return;
        }

        // the fields follow the parameters
        if (tz.peek_char_after_whitespace() != '{') {
            throw compile_error{tz,
                                "expected '{' to begin declaration of type"};
        }

        tc.add_generic_type(name_tk_.text(),
                            {
                                .src_loc_tk{name_tk_},
                                .start_tk{start_tk_},
                                .params{std::move(params)},
                                .base_tk{},
                                .base_arg_tks{},
                                .definition{tc.current_definition()},
                            });

        // e.g. 'text<capacity> { items i8[capacity] }'
        generic_text_ = tz.skip_braced_block_after(type_tk);
    }
};

// e.g. 'type str = text<127>', an instance of a generic type
class stmt_def_type_alias final : public statement {
    token name_tk_;
    type type_;
    // the tokens of 'type str = text<127>' after the name
    std::vector<token> alias_tks_;

  public:
    stmt_def_type_alias(toc& tc, const token tk, const token name_tk,
                        tokenizer& tz)
        : statement{tk}, name_tk_{name_tk} {

        parse_alias(tc, tz);
    }

    stmt_def_type_alias() = default;
    // 'toc' keeps the address of the type
    stmt_def_type_alias(const stmt_def_type_alias&) = delete;
    stmt_def_type_alias(stmt_def_type_alias&&) = delete;
    auto operator=(const stmt_def_type_alias&) -> stmt_def_type_alias& = delete;
    auto operator=(stmt_def_type_alias&&) -> stmt_def_type_alias& = delete;

    ~stmt_def_type_alias() override = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        name_tk_.source_to(os);

        for (const token& t : alias_tks_) {
            t.source_to(os);
        }
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {

        comment_type_layout(tc, indent, tok(), type_);
    }

  private:
    // e.g. 'type str = text<8>' is the type 'str' with the fields of the
    // generic type 'text' for the arguments
    //
    // e.g. with 'type text<size> = list<i8, size>' and
    // 'type list<T type, capacity> { array T[capacity], len }' the steps are:
    //   1. read '= text<8>', 'size' of 'text' is 8
    //   2. follow the bases, 'text<8>' then 'list<i8, 8>'
    //   3. parse the fields of 'list' as those of 'str'
    //   4. add 'str' and make the methods of 'list<i8, 8>' and 'text<8>'
    auto parse_alias(toc& tc, tokenizer& tz) -> void {
        // 'alias_tks_' keeps the tokens after the name for 'source_to'
        const token equals_tk{tz.is_next_char_token('=')};
        alias_tks_.emplace_back(equals_tk);

        const token generic_tk{tz.next_token()};
        alias_tks_.emplace_back(generic_tk);

        if (not generic_tk.is_word()) {
            throw compile_error{generic_tk.is_none() ? equals_tk : generic_tk,
                                "expected a generic type after '='"};
        }

        assert_generic_type(tc, generic_tk, "the alias that names it");

        const generic_type_info& generic{
            tc.generics().get_type(generic_tk.text()),
        };

        const generic_arg_tokens arg_tokens{parse_generic_args(tz, generic_tk)};
        alias_tks_.append_range(arg_tokens.list_tks);

        assert_arg_count(generic_tk, generic, arg_tokens.arg_tks);

        // step 1, an error here is in the alias itself, so it gets no frame
        std::vector<generic_arg> args{
            resolve_generic_args(tc, generic_tk.text(), generic.params,
                                 arg_tokens.arg_tks),
        };

        // an error in the generic code names the alias, e.g. 'type str =
        // text<8>' at 'str'
        const generic_frame alias_frame{
            .src_loc_tk{name_tk_},
            .text{
                std::format("type {} = {}", name_tk_.text(),
                            generic_instance_text(generic_tk.text(), args)),
            },
        };

        // step 2, the instance then its bases
        std::vector<generic_type_instance> instances{
            {
                .generic_name{generic_tk.text()},
                .type_ptr{&type_},
                .args{std::move(args)},
                .frames{alias_frame},
            },
        };

        try {
            while (has_base(tc, instances.back())) {
                instances.push_back(base_instance(tc, instances.back()));
            }

            // step 3
            parse_root_fields(tc, instances.back());
        } catch (compile_error& e) {
            // the frames name the instances that led to the error, the alias
            // last
            instances.back().add_frames_to(e);
            throw;
        }

        // step 4, the methods of each generic type see the arguments of its
        // instance, the methods of a base are made first
        tc.add_type(name_tk_, type_);

        for (const generic_type_instance& instance :
             instances | std::views::reverse) {

            tc.add_generic_type_instance(instance);

            func_instantiator::instantiate_methods(tc, instance);
        }
    }

    // the fields are those of the generic definition, parsed again with its
    // parameters naming the arguments
    // e.g. 'array T[capacity], len' of 'type list<T type, capacity> {...}'
    // with 'T' 'i8' and 'capacity' 8
    auto parse_root_fields(toc& tc, const generic_type_instance& instance)
        -> void {

        const generic_type_info& generic{
            tc.generics().get_type(instance.generic_name),
        };

        // the parameters name the arguments until the return
        generic_scope scope{tc, generic.definition};
        scope.add_args(instance.args);

        tokenizer generic_tz{tc.source_of(generic.start_tk), generic.start_tk};

        // note: the instance is parsed inside the parse that needs it
        const toc::parse_nesting_scope nesting{tc, generic_tz};

        // the name and the parameters, e.g. 'list' and '<T type, capacity>'
        std::ignore = generic_tz.next_token();
        std::ignore = generic_param::parse(generic_tz);

        const token open_brace_tk{generic_tz.is_next_char_token('{')};

        // note: 'define_generic' checked the '{' of a generic without a base
        assert(not open_brace_tk.is_empty());

        // only read to fill 'type_', not printed by 'source_to'
        type_fields fields;
        fields.parse(tc, generic_tz, open_brace_tk, name_tk_);
        fields.add_to(tc, type_, name_tk_);
    }

    //
    // statics
    //

    // e.g. 'list<i8, 8>' for 'text<8>' with 'type text<size> = list<i8, size>'
    [[nodiscard]] static auto
    base_instance(toc& tc, const generic_type_instance& instance)
        -> generic_type_instance {

        const generic_type_info& generic{
            tc.generics().get_type(instance.generic_name),
        };

        const token& base_tk{generic.base_tk};

        std::vector<generic_arg> base_args{
            resolve_base_args(tc, generic, instance.args),
        };

        // an error in the base is reported with the arguments it got
        // e.g. 'list<i8, 8>' at the 'list' of 'list<i8, size>'
        const generic_frame base_frame{
            .src_loc_tk{base_tk},
            .text{generic_instance_text(base_tk.text(), base_args)},
        };

        // followed by the frames that lead to it, the alias last
        std::vector<generic_frame> frames{base_frame};
        frames.append_range(instance.frames);

        return {
            .generic_name{base_tk.text()},
            .type_ptr{instance.type_ptr},
            .args{std::move(base_args)},
            .frames{std::move(frames)},
        };
    }

    // e.g. 'text' has the base 'list', 'list' has none
    [[nodiscard]] static auto has_base(const toc& tc,
                                       const generic_type_instance& instance)
        -> bool {

        return not tc.generics()
                       .get_type(instance.generic_name)
                       .base_tk.is_empty();
    }

    // the argument tokens of a base name the parameters of 'generic', e.g.
    // 'size' of 'list<i8, size>' is 8 for 'text<8>'; the scope ends before
    // the base binds its own parameters, which may have the same names
    [[nodiscard]] static auto
    resolve_base_args(toc& tc, const generic_type_info& generic,
                      const std::span<const generic_arg> args)
        -> std::vector<generic_arg> {

        generic_scope scope{tc, generic.definition};
        scope.add_args(args);

        const generic_type_info& base{
            tc.generics().get_type(generic.base_tk.text()),
        };

        return resolve_generic_args(tc, generic.base_tk.text(), base.params,
                                    generic.base_arg_tks);
    }
};

// reads the name and makes the statement of the kind that follows it
inline auto make_stmt_def_type(toc& tc, const token tk, tokenizer& tz)
    -> std::unique_ptr<statement> {

    const token start_tk{tz.cur_position_token()};
    const token name_tk{tz.next_token()};

    // e.g. 'type read {x}' could have no method, 'read.get' reads as the
    // built-in
    tc.assert_valid_unqualified_name(name_tk);

    const char next{tz.peek_char_after_whitespace()};

    // e.g. 'type point { x, y }'
    if (next == '{') {
        return std::make_unique<stmt_def_type>(tc, tk, name_tk, tz);
    }

    // e.g. 'type text<size> = list<i8, size>'
    if (next == '<') {
        return std::make_unique<stmt_def_type_generic>(tc, tk, start_tk,
                                                       name_tk, tz);
    }

    // e.g. 'type str = text<127>'
    if (next == '=') {
        return std::make_unique<stmt_def_type_alias>(tc, tk, name_tk, tz);
    }

    throw compile_error{tz, "expected '{' to begin declaration of type"};
}
