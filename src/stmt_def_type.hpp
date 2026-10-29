#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <format>
#include <memory>
#include <optional>
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

//
// a type definition, e.g. 'type point { x, y }', is the classes below, in
// reading order:
//
//   'type_fields'            the braces, fields and delimiters of a body
//   'comment_type_layout'    the layout comment of a plain type and an alias
//   'generic_type_use'       the generic type named after an '='
//   'stmt_def_type'          a plain type, 'type point { x, y }'
//   'stmt_def_type_generic'  a generic definition, kept as text
//   'type_instantiator'      makes the instance of a generic type
//   'stmt_def_type_alias'    'type str = text<8>', an instance
//   'make_stmt_def_type'     reads the name and picks the statement (last in
//                            the file, it uses the others)
//

// the braces, fields and delimiters of 'type point { x, y }'
class type_fields final {
    token open_brace_tk_;
    std::vector<stmt_def_type_field> fields_;
    std::vector<token> field_delims_tk_;
    token close_brace_tk_;

  public:
    auto add_to(type& tp, const token& name_tk) const -> void {
        tp.set_name(name_tk.text());

        // note: 'parse' checked the names

        for (const stmt_def_type_field& fld : fields_) {
            tp.add_field(fld.tok(), fld.name(), fld.get_type(),
                         fld.array_count());
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
            const token field_tk{tz.next_token()};

            // checked where it is read, before the errors of later fields
            const auto existing{
                std::ranges::find(fields_, field_tk.text(),
                                  &stmt_def_type_field::name),
            };

            if (existing != fields_.end()) {
                throw compile_error{
                    field_tk, std::format("field {} already defined at {}",
                                          in_quotes(field_tk.text()),
                                          tc.where(existing->tok()))};
            }

            fields_.emplace_back(tc, field_tk, tz);

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
        const std::optional<token> close_brace_tk{tz.is_next_char_token('}')};

        if (not close_brace_tk) {
            return false;
        }

        close_brace_tk_ = *close_brace_tk;

        return true;
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
                  f.shape.length() ? std::format("{}", *f.shape.length()) : "");
    }

    x.comment({}, 0, "");
}

// the generic type a definition names after its '=', e.g. '= list<i8, size>'
// of 'type text<size> = list<i8, size>' or '= text<8>' of 'type str =
// text<8>'
struct generic_type_use {
    token equals_tk;
    token name_tk;
    generic_arg_tokens args{};

    //
    // statics
    //

    // 'user' is what must come after the generic type
    // note: the callers dispatch on the '='
    [[nodiscard]] static auto parse(const toc& tc, tokenizer& tz,
                                    const std::string_view user)
        -> generic_type_use {

        const token equals_tk{tz.seen_char_token('=')};
        const token name_tk{tz.next_token()};

        // note: the '=' is the place when the source ends after it

        if (not name_tk.is_word()) {
            throw compile_error{name_tk.is_none() ? equals_tk : name_tk,
                                "expected a generic type after '='"};
        }

        assert_generic_type(tc, name_tk, user);

        generic_arg_tokens args{read_generic_args(tz, name_tk)};

        assert_arg_count(name_tk, tc.generics().get_type(name_tk.text()),
                         args.arg_tks);

        return {
            .equals_tk{equals_tk},
            .name_tk{name_tk},
            .args{std::move(args)},
        };
    }

  private:
    //
    // statics
    //

    // a generic type is given one argument for each of its parameters
    static auto assert_arg_count(const token& generic_tk,
                                 const generic_type_info& generic,
                                 const std::span<const token> arg_tks) -> void {

        if (arg_tks.size() == generic.params.size()) {
            return;
        }

        throw compile_error{
            generic_tk, std::format("generic type {} takes {}, got {}",
                                    in_quotes(generic_tk.text()),
                                    counted(generic.params.size(), "argument"),
                                    arg_tks.size())};
    }

    // e.g. 'text' of 'type str = text<8>', 'user' is what must come after the
    // generic type
    static auto assert_generic_type(const toc& tc, const token& generic_tk,
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
    [[nodiscard]] static auto read_generic_args(tokenizer& tz,
                                                const token& generic_tk)
        -> generic_arg_tokens {

        const std::optional<token> open_tk{tz.is_next_char_token('<')};

        if (not open_tk) {
            throw compile_error{
                generic_tk,
                std::format("generic type {} needs arguments, e.g. '{}<...>'",
                            in_quotes(generic_tk.text()), generic_tk.text())};
        }

        return generic_arg_tokens::parse(tz, *open_tk, true);
    }
};

// e.g. 'type point { x, y }'
class stmt_def_type final : public statement {
    token name_tk_;
    type_fields fields_;
    type type_;

  public:
    stmt_def_type(toc& tc, const token tk, const token name_tk, tokenizer& tz)
        : statement{tk}, name_tk_{name_tk} {

        // e.g. the '{' of 'type point { x, y }'
        // note: the definition is dispatched on the '{'
        const token open_brace_tk{tz.seen_char_token('{')};

        fields_.parse(tc, tz, open_brace_tk, name_tk_);

        fields_.add_to(type_, name_tk_);

        tc.add_type(name_tk_, type_);
    }

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
    token name_tk_;
    // the text after 'type' of a generic definition, kept for 'source_to'
    std::string_view generic_text_;

  public:
    stmt_def_type_generic(toc& tc, const token tk, const token name_tk,
                          tokenizer& tz)
        : statement{tk}, name_tk_{name_tk} {

        define_generic(tc, tk, tz);
    }

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

        // e.g. '= list<i8, size>'
        generic_type_use base{
            generic_type_use::parse(tc, tz, "the type based on it"),
        };

        tc.add_generic_type(name_tk_.text(),
                            {
                                .src_loc_tk{name_tk_},
                                .params{std::move(params)},
                                .fields_tk{std::nullopt},
                                .base_tk{base.name_tk},
                                .base_arg_tks{std::move(base.args.arg_tks)},
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
                                .params{std::move(params)},
                                .fields_tk{tz.cur_position_token()},
                                .base_tk{std::nullopt},
                                .base_arg_tks{},
                                .definition{tc.current_definition()},
                            });

        // e.g. 'text<capacity> { items i8[capacity] }'
        generic_text_ = tz.skip_braced_block_after(type_tk);
    }
};

// makes the type that an alias names, an instance of a generic type, e.g.
// 'type str = text<8>' is the type 'str' with the fields of 'text' for the
// arguments; with 'type text<size> = list<i8, size>' and
// 'type list<T type, capacity> { array T[capacity], len }' the steps of
// 'instantiate' are:
//
//   1. resolve the arguments of '= text<8>', 'size' of 'text' is 8
//   2. follow the bases ('has_base', 'base_instance', 'resolve_base_args'),
//      'text<8>' then 'list<i8, 8>'
//   3. parse the fields of 'list' as those of 'str' ('parse_root_fields')
//   4. add 'str' and make the methods of 'list<i8, 8>' and 'text<8>', see
//      'func_instantiator'
class type_instantiator final {
  public:
    //
    // statics
    //

    // 'tp' becomes the type of the alias 'alias_tk', 'toc' keeps its address
    static auto instantiate(toc& tc, const token& alias_tk,
                            const generic_type_use& use, type& tp) -> void {

        const generic_type_info& generic{
            tc.generics().get_type(use.name_tk.text()),
        };

        // step 1, an error here is in the alias itself, so it gets no frame
        std::vector<generic_arg> args{
            resolve_generic_args(tc, use.name_tk.text(), generic.params,
                                 use.args.arg_tks),
        };

        // an error in the generic code names the alias, e.g. 'type str =
        // text<8>' at 'str'
        const generic_trace alias_trace{
            alias_tk,
            std::format("type {} = {}", alias_tk.text(),
                        generic_instance_text(use.name_tk.text(), args)),
        };

        // step 2, the instance then its bases
        std::vector<generic_type_instance> instances{
            {
                .generic_name{use.name_tk.text()},
                .type_ptr{&tp},
                .args{std::move(args)},
                .trace{alias_trace},
            },
        };

        try {
            while (has_base(tc, instances.back())) {
                instances.push_back(base_instance(tc, instances.back()));
            }

            // step 3
            parse_root_fields(tc, instances.back(), alias_tk, tp);
        } catch (compile_error& e) {
            // the frames name the instances that led to the error, the alias
            // last
            instances.back().trace.add_to(e);
            throw;
        }

        // step 4, the methods of each generic type see the arguments of its
        // instance
        tc.add_type(alias_tk, tp);

        for (const generic_type_instance& instance :
             instances | std::views::reverse) {

            tc.add_generic_type_instance(instance);
        }

        func_instantiator::instantiate_methods(tc, instances);
    }

  private:
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

        // note: only a type with a base asks, see 'has_base'
        assert(generic.base_tk);

        const token& base_tk{*generic.base_tk};

        std::vector<generic_arg> base_args{
            resolve_base_args(tc, generic, instance.args),
        };

        // an error in the base is reported with the arguments it got
        // e.g. 'list<i8, 8>' at the 'list' of 'list<i8, size>'
        // followed by the frames that lead to it, the alias last
        const generic_trace trace{
            generic_trace{
                base_tk,
                generic_instance_text(base_tk.text(), base_args),
            }
                .inside(instance.trace),
        };

        return {
            .generic_name{base_tk.text()},
            .type_ptr{instance.type_ptr},
            .args{std::move(base_args)},
            .trace{trace},
        };
    }

    // e.g. 'text' has the base 'list', 'list' has none
    [[nodiscard]] static auto has_base(const toc& tc,
                                       const generic_type_instance& instance)
        -> bool {

        return tc.generics()
            .get_type(instance.generic_name)
            .base_tk.has_value();
    }

    // the fields are those of the generic definition, parsed again with its
    // parameters naming the arguments
    // e.g. 'array T[capacity], len' of 'type list<T type, capacity> {...}'
    // with 'T' 'i8' and 'capacity' 8
    static auto parse_root_fields(toc& tc,
                                  const generic_type_instance& instance,
                                  const token& alias_tk, type& tp) -> void {

        const generic_type_info& generic{
            tc.generics().get_type(instance.generic_name),
        };

        // note: only the root of the bases, a type without a base, asks
        assert(generic.fields_tk);

        // the parameters name the arguments until the return
        generic_reparse reparse{tc, generic.definition, *generic.fields_tk};
        reparse.scope().add_args(instance.args);

        tokenizer& generic_tz{reparse.tz()};

        // note: 'define_generic' checked the '{' of a generic without a base
        const token open_brace_tk{generic_tz.seen_char_token('{')};

        // only read to fill 'type_', not printed by 'source_to'
        type_fields fields;
        fields.parse(tc, generic_tz, open_brace_tk, alias_tk);
        fields.add_to(tp, alias_tk);
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

        // note: only a type with a base asks, see 'has_base'
        assert(generic.base_tk);

        const std::string_view base_name{generic.base_tk->text()};

        const generic_type_info& base{tc.generics().get_type(base_name)};

        return resolve_generic_args(tc, base_name, base.params,
                                    generic.base_arg_tks);
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

        const generic_type_use generic{
            generic_type_use::parse(tc, tz, "the alias that names it"),
        };

        // kept for 'source_to'
        alias_tks_.emplace_back(generic.equals_tk);
        alias_tks_.emplace_back(generic.name_tk);
        alias_tks_.append_range(generic.args.list_tks);

        type_instantiator::instantiate(tc, name_tk_, generic, type_);
    }

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
};

// reads the name and makes the statement of the kind that follows it
inline auto make_stmt_def_type(toc& tc, const token tk, tokenizer& tz)
    -> std::unique_ptr<statement> {

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
        return std::make_unique<stmt_def_type_generic>(tc, tk, name_tk, tz);
    }

    // e.g. 'type str = text<127>'
    if (next == '=') {
        return std::make_unique<stmt_def_type_alias>(tc, tk, name_tk, tz);
    }

    throw compile_error{tz, "expected '{' to begin declaration of type"};
}
