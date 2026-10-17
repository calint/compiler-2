#pragma once
// reviewed: 2025-09-28

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <print>
#include <ranges>
#include <span>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "decouple.hpp"
#include "generics.hpp"
#include "machine.hpp"
#include "statement.hpp"
#include "stmt_def_type_field.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"

class stmt_def_type final : public statement {
    enum class kind : uint8_t {
        plain,
        // kept as text for its aliases
        generic,
        // e.g. 'type str = text<127>', an instance of a generic type
        alias,
    };

    kind kind_{kind::plain};
    // where the definition continues after 'type', an alias parses it again
    token start_tk_;
    token name_tk_;
    token open_brace_tk_;
    std::vector<stmt_def_type_field> fields_;
    std::vector<token> field_delims_tk_;
    token close_brace_tk_;
    type type_;
    // the tokens of 'type str = text<127>' after the name
    std::vector<token> alias_tks_;
    // the text after 'type' of a generic definition, only its aliases parse it
    std::string_view generic_text_;

  public:
    stmt_def_type(toc& tc, const token tk, tokenizer& tz)
        : statement{tk}, start_tk_{tz.cur_position_token()},
          name_tk_{tz.next_token()},
          open_brace_tk_{tz.is_next_char_token('{')} {

        toc::assert_valid_name(name_tk_);

        // e.g. 'type point { x, y }'
        if (not open_brace_tk_.is_empty()) {
            parse_fields(tc, tz);

            add_type(tc);

            return;
        }

        const char next{tz.peek_char_after_whitespace()};

        // e.g. 'type text<capacity> {...}' or 'type text<capacity> = list<i8,
        // capacity>'
        if (next == '<') {
            define_generic(tc, tk, tz);
            return;
        }

        // e.g. 'type str = text<127>'
        if (next == '=') {
            parse_alias(tc, tz);
            return;
        }

        throw compiler_exception{tz,
                                 "expected '{' to begin declaration of type"};
    }

    stmt_def_type() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);

        if (kind_ == kind::generic) {
            std::print(os, "{}", generic_text_);
            return;
        }

        name_tk_.source_to(os);

        if (kind_ == kind::alias) {
            for (const token& t : alias_tks_) {
                t.source_to(os);
            }

            return;
        }

        open_brace_tk_.source_to(os);

        // note: a type has at least one field
        assert(not fields_.empty());

        fields_.front().source_to(os);
        for (const auto [d, e] :
             std::views::zip(field_delims_tk_, fields_ | std::views::drop(1))) {

            d.source_to(os);
            e.source_to(os);
        }

        if (field_delims_tk_.size() == fields_.size()) {
            field_delims_tk_.back().source_to(os);
        }

        close_brace_tk_.source_to(os);
    }

    auto compile(toc& tc, const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {

        // only its aliases are types
        if (kind_ == kind::generic) {
            return;
        }

        const type& tp{tc.get_type_or_throw(tok(), name_tk_.text())};

        machine& x{tc.machine()};

        x.comment(tok(), indent, "{} : {} B    fields:", name_tk_.text(),
                  tp.size_bytes());

        x.comment(tok(), indent, "{:>10} : {:>7} : {:>7} : {:>7} : {:>10}",
                  "name", "offset", "size", "array?", "array size");

        for (const type_field& f : tp.fields()) {
            x.comment(tok(), indent, "{:>10} : {:>7} : {:>7} : {:>7} : {:>10}",
                      f.name, f.offset, f.size_bytes, f.is_array ? "yes" : "no",
                      f.is_array ? std::format("{}", f.array_count) : "");
        }

        x.comment({}, 0, "");
    }

  private:
    auto add_fields(toc& tc) -> void {
        type_.set_name(name_tk_.text());

        for (const stmt_def_type_field& fld : fields_) {
            toc::assert_valid_name(fld.tok());

            if (type_.has_field(fld.name())) {
                throw compiler_exception{
                    fld.tok(),
                    std::format("field '{}' is declared twice in type '{}'",
                                fld.name(), name_tk_.text())};
            }

            // get the type of field. no type name means default
            const type& tp{
                fld.type_str().empty()
                    ? tc.get_type_default()
                    : tc.get_type_or_throw(fld.type_token(), fld.type_str()),
            };

            type_.add_field(fld.tok(), fld.name(), tp, fld.is_array(),
                            fld.array_count());
        }
    }

    auto add_type(toc& tc) -> void {
        add_fields(tc);

        tc.add_type(name_tk_, type_);
    }

    // the fields and the methods are those of the base, the arguments of the
    // base are resolved when an alias names this type
    auto define_based_generic(toc& tc, const token& type_tk, tokenizer& tz,
                              std::vector<generic_param> params) -> void {

        // e.g. '=' and 'list' of '= list<i8, capacity>'
        const token equals_tk{tz.is_next_char_token('=')};
        const token base_tk{tz.next_token()};

        if (base_tk.text().empty()) {
            throw compiler_exception{equals_tk,
                                     "expected a generic type after '='"};
        }

        assert_generic_type(tc, base_tk, "the type based on it");

        // e.g. 'i8' and 'capacity' of '<i8, capacity>'
        generic_arguments args{parse_generic_args(tz, base_tk)};

        assert_arg_count(base_tk, tc.generics().get_type(base_tk.text()),
                         args.arg_tks);

        tc.add_generic_type(name_tk_.text(),
                            {
                                .src_loc_tk{name_tk_},
                                .start_tk{start_tk_},
                                .params{std::move(params)},
                                .base_tk{base_tk},
                                .base_arg_tks{std::move(args.arg_tks)},
                            });

        // e.g. 'text<capacity> = list<i8, capacity>'
        generic_text_ = tz.text_after(type_tk);
    }

    // the text after 'type' is kept for 'source_to' and for the aliases, errors
    // in it are found when an alias parses it
    auto define_generic(toc& tc, const token& type_tk, tokenizer& tz) -> void {
        // e.g. 'capacity' of '<capacity>'
        std::vector<generic_param> params{generic_param::parse(tz)};

        for (const generic_param& param : params) {
            tc.assert_generic_param_free(param.name_tk, param.name_tk.text());
        }

        kind_ = kind::generic;

        // e.g. 'type text<capacity> = list<i8, capacity>'
        if (tz.peek_char_after_whitespace() == '=') {
            define_based_generic(tc, type_tk, tz, std::move(params));
            return;
        }

        // the fields follow the parameters
        if (tz.peek_char_after_whitespace() != '{') {
            throw compiler_exception{
                tz, "expected '{' to begin declaration of type"};
        }

        tc.add_generic_type(name_tk_.text(), {
                                                 .src_loc_tk{name_tk_},
                                                 .start_tk{start_tk_},
                                                 .params{std::move(params)},
                                                 .base_tk{},
                                                 .base_arg_tks{},
                                             });

        // e.g. 'text<capacity> { items i8[capacity] }'
        generic_text_ = tz.skip_braced_block_after(type_tk);
    }

    // an instance of a generic type for this alias
    auto make_generic_type_instance(const std::string_view generic_name,
                                    std::vector<generic_binding> bindings,
                                    std::vector<generic_base_frame> base_frames,
                                    const std::string& alias_source)
        -> generic_type_instance {

        return {
            // e.g. 'str' of 'type str = text<8>'
            .src_loc_tk{name_tk_},
            // e.g. 'text'
            .generic_name{generic_name},
            // e.g. the type 'str'
            .type_ptr{&type_},
            // e.g. 'capacity' is 8
            .bindings{std::move(bindings)},
            // e.g. 'type str = text<8>'
            .alias_source{alias_source},
            // e.g. none, 'text' is the generic type that the alias names
            .base_frames{std::move(base_frames)},
        };
    }

    // e.g. 'type str = text<8>' is the type 'str' with the fields of the
    // generic type 'text' for the arguments
    //
    // e.g. with 'type text<capacity> = list<i8, capacity>' and
    // 'type list<T type, capacity> { array T[capacity], len }' the steps are:
    //   1. read '= text<8>'
    //   2. bind 'capacity' of 'text' to 8
    //   3. follow the bases, 'text<8>' then 'list<i8, 8>', and parse the
    //      fields of 'list' as those of 'str'
    //   4. add 'str' and make the methods of 'list<i8, 8>' and 'text<8>'
    auto parse_alias(toc& tc, tokenizer& tz) -> void {
        kind_ = kind::alias;

        // 'alias_tks_' keeps the tokens after the name for 'source_to'
        // e.g. the '=' of 'type str = text<8>'
        alias_tks_.emplace_back(tz.is_next_char_token('='));

        // e.g. 'text'
        const token generic_tk{tz.next_token()};
        alias_tks_.emplace_back(generic_tk);

        // e.g. 'type str = i32<8>' or 'type str = undefined<8>' are errors
        assert_generic_type(tc, generic_tk, "the alias that names it");

        // e.g. 'type text<capacity> = list<i8, capacity>'
        const generic_type_info generic{
            tc.generics().get_type(generic_tk.text()),
        };

        // e.g. '8' of '<8>', the '<', '8' and '>' go to 'alias_tks_'
        generic_arguments args{parse_generic_args(tz, generic_tk)};
        alias_tks_.append_range(args.list_tks);

        // e.g. 'text<8, 9>' is an error, 'text' has one parameter
        assert_arg_count(generic_tk, generic, args.arg_tks);

        // an error here is in the alias itself, so it gets no frame
        // e.g. 'capacity' is 8 for 'text<8>'
        const std::vector<generic_binding> bindings{
            resolve_args(tc, generic_tk.text(), generic.params, args.arg_tks),
        };

        // e.g. 'type str = text<8>', names the alias in the errors found in
        // the generic code
        const std::string alias_source{
            generic_type_instance::alias_text(name_tk_.text(),
                                              generic_tk.text(), bindings),
        };

        // e.g. 'text<8>' then 'list<i8, 8>', 'type_' gets the fields of 'list'
        const std::vector<generic_type_instance> instances{
            parse_instances(tc, generic_tk.text(), generic, args.arg_tks,
                            bindings, alias_source),
        };

        // e.g. 'str' with the fields of 'list<i8, 8>'
        tc.add_type(name_tk_, type_);

        // the methods of each generic type see the arguments of its instance,
        // the methods of a base are made first
        //
        // e.g. the methods of 'list<i8, 8>' then of 'text<8>' for 'str'
        for (const generic_type_instance& instance :
             instances | std::views::reverse) {

            tc.generics().add_type_instance(instance);

            instantiate_generic_methods(tc, instance);
        }
    }

    auto parse_fields(toc& tc, tokenizer& tz) -> void {
        while (true) {
            // read field definition with the next token being the name
            // e.g. 'x', 'x i32' or 'items point[4]'
            fields_.emplace_back(tc, tz.next_token(), tz);
            close_brace_tk_ = tz.is_next_char_token('}');

            if (not close_brace_tk_.is_empty()) {
                break;
            }

            const token delimiter_tk{
                tz.expect_char_token(
                    ',', std::format("expected ',' followed by another field "
                                     "in type '{}'",
                                     name_tk_.text())),
            };

            field_delims_tk_.emplace_back(delimiter_tk);

            // a trailing ',' lets each field end its line the same way
            // e.g. 'type point { x, y, }'
            close_brace_tk_ = tz.is_next_char_token('}');

            if (not close_brace_tk_.is_empty()) {
                break;
            }
        }
    }

    // follows the bases from the generic type that the alias names to the one
    // with the fields and parses those fields as the fields of this type. the
    // result has an instance per generic type, the named one first
    //
    // e.g. 'type str = text<8>' with 'type text<capacity> = list<i8, capacity>'
    // and 'type list<T type, capacity> { array T[capacity], len }':
    //   level 1: 'text' with the argument '8', 'capacity' is 8
    //   level 2: 'list' with the arguments 'i8' and 'capacity', 'T' is 'i8' and
    //            'capacity' is 8, has no base so 'str' gets its fields
    //
    // an error in a level is in the generic definition, it gets a frame per
    // level before it and one that names the alias, e.g. an error in the
    // fields of 'list' has the frames 'list<i8, 8>' and 'type str = text<8>'
    auto parse_instances(toc& tc, const std::string_view generic_name,
                         const generic_type_info& generic,
                         const std::span<const token> arg_tks,
                         const std::span<const generic_binding> bindings,
                         const std::string& alias_source)
        -> std::vector<generic_type_instance> {

        // the generic type of the current level and its argument tokens
        // e.g. 'text' and '8' of 'text<8>'
        const generic_type_info* level{&generic};
        std::vector<token> level_arg_tks{arg_tks.begin(), arg_tks.end()};

        // e.g. 'text<8>' then its base 'list<i8, 8>'
        std::vector<generic_type_instance> instances;

        // e.g. 'text<8>', the named generic type has no frames of bases
        instances.push_back(make_generic_type_instance(
            generic_name, {bindings.begin(), bindings.end()}, {},
            alias_source));

        try {
            // e.g. 'text' has the base 'list', 'list' has none
            while (not level->base_tk.is_empty()) {
                // e.g. 'list' of 'type text<capacity> = list<i8, capacity>'
                const token& base_tk{level->base_tk};

                // e.g. 'type list<T type, capacity> { array T[capacity], len }'
                const generic_type_info& base{
                    tc.generics().get_type(base_tk.text()),
                };

                // e.g. 'T' is 'i8' and 'capacity' is 8 for 'list<i8,
                // capacity>' with 'capacity' 8 of 'text'
                std::vector<generic_binding> base_bindings{
                    resolve_base_args(tc, *level, base, level_arg_tks,
                                      instances.back().bindings),
                };

                // an error in the base is reported with the arguments it got
                // e.g. 'list<i8, 8>' at the 'list' of 'list<i8, capacity>'
                const generic_base_frame base_frame{
                    .src_loc_tk{base_tk},
                    .text{
                        generic_type_instance::instance_text(base_tk.text(),
                                                             base_bindings),
                    },
                };

                // followed by the frames that lead to the named generic type
                // e.g. none when the level before is 'text'
                std::vector<generic_base_frame> base_frames{base_frame};
                base_frames.append_range(instances.back().base_frames);

                // e.g. 'list<i8, 8>'
                instances.push_back(make_generic_type_instance(
                    base_tk.text(), std::move(base_bindings),
                    std::move(base_frames), alias_source));

                // the base is the next level
                // e.g. 'list' with the tokens 'i8' and 'capacity'
                level_arg_tks = level->base_arg_tks;
                level = &base;
            }

            // e.g. 'str' gets 'array i8[8]' and 'len'
            parse_root_fields(tc, *level, level_arg_tks,
                              instances.back().bindings);

        } catch (compiler_exception& e) {
            // e.g. an error in 'list' gets the frame 'list<i8, 8>' at the
            // 'list' of 'type text<capacity> = list<i8, capacity>'
            for (const generic_base_frame& frame :
                 instances.back().base_frames) {

                e.add_call_frame(frame.src_loc_tk, frame.text,
                                 "instantiated by");
            }

            // e.g. the frame 'type str = text<8>' at 'str'
            e.add_call_frame(name_tk_, alias_source, "instantiated by");

            throw;
        }

        return instances;
    }

    // the fields are those of the generic definition, parsed again with its
    // parameters declared in a block of their own
    // e.g. 'array T[capacity], len' of 'type list<T type, capacity> {...}'
    // with 'T' 'i8' and 'capacity' 8
    auto parse_root_fields(toc& tc, const generic_type_info& generic,
                           const std::span<const token> arg_tks,
                           const std::span<const generic_binding> bindings)
        -> void {

        // the constant arguments live in a block of their own
        tc.enter_block();

        // unbinds the type parameters at return
        type_alias_scope aliases{tc};

        // e.g. 'T' names the type 'i8' and 'capacity' is the constant 8
        declare_params(tc, aliases, generic.params, arg_tks, bindings);

        // e.g. reads from the 'list' after 'type' of the generic definition
        tokenizer generic_tz{tc.source_of(generic.start_tk), generic.start_tk};

        // skips the name and the parameters of the generic definition
        // e.g. 'list' and '<T type, capacity>'
        std::ignore = generic_tz.next_token();
        std::ignore = generic_param::parse(generic_tz);

        // e.g. the '{' before 'array T[capacity], len }'
        open_brace_tk_ = generic_tz.is_next_char_token('{');

        // note: 'define_generic' checked the '{' of a generic without a base
        assert(not open_brace_tk_.is_empty());

        // e.g. 'array T[capacity]' and 'len'
        parse_fields(tc, generic_tz);

        // e.g. 'array' is 'i8[8]' because 'T' and 'capacity' are declared
        add_fields(tc);

        tc.exit_block();
    }

    //
    // statics
    //

    static auto assert_arg_count(const token& generic_tk,
                                 const generic_type_info& generic,
                                 const std::span<const token> arg_tks) -> void {

        if (arg_tks.size() == generic.params.size()) {
            return;
        }

        throw compiler_exception{
            generic_tk,
            std::format("generic type '{}' takes {} argument(s), got {}",
                        generic_tk.text(), generic.params.size(),
                        arg_tks.size())};
    }

    // e.g. 'text' of 'type str = text<8>', 'user' is what must come after the
    // generic type
    static auto assert_generic_type(const toc& tc, const token& generic_tk,
                                    const std::string_view user) -> void {

        if (tc.has_type(generic_tk.text())) {
            throw compiler_exception{
                generic_tk, std::format("'{}' is a type, not a generic type",
                                        generic_tk.text())};
        }

        if (not tc.generics().has_type(generic_tk.text())) {
            throw compiler_exception{
                generic_tk,
                std::format("'{}' is not a generic type, a generic type is "
                            "defined before {}",
                            generic_tk.text(), user)};
        }
    }

    // a type parameter names its argument, a constant parameter is a constant
    // of the current block
    static auto declare_params(toc& tc, type_alias_scope& aliases,
                               const std::span<const generic_param> params,
                               const std::span<const token> arg_tks,
                               const std::span<const generic_binding> bindings)
        -> void {

        for (const auto [param, arg_tk, binding] :
             std::views::zip(params, arg_tks, bindings)) {

            if (binding.type_ptr != nullptr) {
                aliases.bind(param.name_tk, binding.name, *binding.type_ptr);
                continue;
            }

            tc.add_const(arg_tk, 0, binding.name, binding.value);
        }
    }

    // e.g. '<8>' after 'text'
    [[nodiscard]] static auto parse_generic_args(tokenizer& tz,
                                                 const token& generic_tk)
        -> generic_arguments {

        const token open_tk{tz.is_next_char_token('<')};

        if (open_tk.is_empty()) {
            throw compiler_exception{
                generic_tk,
                std::format("generic type '{}' needs arguments, e.g. '{}<...>'",
                            generic_tk.text(), generic_tk.text())};
        }

        return generic_arguments::parse(tz, open_tk);
    }

    // a type parameter needs a type, a constant parameter a constant
    [[nodiscard]] static auto
    resolve_args(const toc& tc, const std::string_view generic_name,
                 const std::span<const generic_param> params,
                 const std::span<const token> arg_tks)
        -> std::vector<generic_binding> {

        std::vector<generic_binding> bindings;

        for (const auto [param, arg_tk] : std::views::zip(params, arg_tks)) {

            const std::string_view name{param.name_tk.text()};
            const std::optional<int64_t> value{tc.constant_value_of(arg_tk)};

            if (param.is_type) {
                if (value) {
                    throw compiler_exception{
                        arg_tk,
                        std::format("generic parameter '{}' of '{}' needs a "
                                    "type, got the constant '{}'",
                                    name, generic_name, arg_tk.text())};
                }

                const type& type_arg{
                    tc.get_type_or_throw(arg_tk, arg_tk.text()),
                };

                bindings.push_back({
                    .name{name},
                    .type_ptr{&type_arg},
                });

                continue;
            }

            if (not value) {
                throw compiler_exception{
                    arg_tk,
                    std::format("generic parameter '{}' of '{}' needs a "
                                "constant, got {}'{}'",
                                name, generic_name,
                                tc.has_type(arg_tk.text()) ? "the type " : "",
                                arg_tk.text())};
            }

            bindings.push_back({
                .name{name},
                .value{*value},
            });
        }

        return bindings;
    }

    // the arguments of the base with the parameters of 'generic' bound, e.g.
    // 'i8' and '8' of 'list<i8, capacity>' for 'capacity' 8. the block is
    // closed before the base binds its parameters, which may have the same
    // names
    //
    // the argument tokens of a base are written with the parameter names of
    // 'generic', so they have values only for one instance of 'generic'.
    // 'bindings' are the values of that instance, 'arg_tks' its argument
    // tokens, used as the source locations of the constants
    //
    // e.g. 'text<8>' with 'type text<capacity> = list<i8, capacity>':
    //   'generic' is 'text', 'arg_tks' is '8', 'bindings' has 'capacity' 8
    //   'base' is 'list', 'generic.base_arg_tks' are 'i8' and 'capacity'
    //   in the block 'capacity' is the constant 8
    //   the result binds 'T' of 'list' to 'i8' and 'capacity' of 'list' to 8
    [[nodiscard]] static auto
    resolve_base_args(toc& tc, const generic_type_info& generic,
                      const generic_type_info& base,
                      const std::span<const token> arg_tks,
                      const std::span<const generic_binding> bindings)
        -> std::vector<generic_binding> {

        // the parameters of 'generic' are visible only while the base
        // arguments are resolved
        tc.enter_block();

        // the parameters of 'generic' are names of the current block until the
        // return, when 'aliases' unbinds the type parameters
        type_alias_scope aliases{tc};

        declare_params(tc, aliases, generic.params, arg_tks, bindings);

        // matches the parameters of the base with its argument tokens, a
        // token that names a parameter of 'generic' gets its value
        // e.g. 'T' with 'i8' and 'capacity' with 'capacity' that is 8
        std::vector<generic_binding> base_bindings{
            resolve_args(tc, generic.base_tk.text(), base.params,
                         generic.base_arg_tks),
        };

        tc.exit_block();

        return base_bindings;
    }
};
