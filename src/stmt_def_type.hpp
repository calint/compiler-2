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

        const token equals_tk{tz.is_next_char_token('=')};
        const token base_tk{tz.next_token()};

        if (base_tk.text().empty()) {
            throw compiler_exception{equals_tk,
                                     "expected a generic type after '='"};
        }

        assert_generic_type(tc, base_tk, "the type based on it");

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

        generic_text_ = tz.text_after(type_tk);
    }

    // the text after 'type' is kept for 'source_to' and for the aliases, errors
    // in it are found when an alias parses it
    auto define_generic(toc& tc, const token& type_tk, tokenizer& tz) -> void {
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

        generic_text_ = tz.skip_braced_block_after(type_tk);
    }

    // e.g. 'type str = text<127>' is the type 'str' with the fields of the
    // generic type 'text' for the arguments
    auto parse_alias(toc& tc, tokenizer& tz) -> void {
        kind_ = kind::alias;
        alias_tks_.emplace_back(tz.is_next_char_token('='));

        const token generic_tk{parse_alias_generic(tc, tz)};

        const generic_type_info generic{
            tc.generics().get_type(generic_tk.text()),
        };

        const std::vector<token> arg_tks{parse_alias_args(tz)};

        assert_arg_count(generic_tk, generic, arg_tks);

        add_instances(tc, parse_instance(tc, generic_tk, generic, arg_tks));
    }

    // appends the tokens after the name to 'alias_tks_' and returns the
    // arguments
    auto parse_alias_args(tokenizer& tz) -> std::vector<token> {
        generic_arguments args{parse_generic_args(tz, alias_tks_.back())};

        alias_tks_.append_range(args.list_tks);

        return std::move(args.arg_tks);
    }

    // appends the name of the generic type to 'alias_tks_'
    auto parse_alias_generic(const toc& tc, tokenizer& tz) -> token {
        const token generic_tk{tz.next_token()};
        alias_tks_.emplace_back(generic_tk);

        assert_generic_type(tc, generic_tk, "the alias that names it");

        return generic_tk;
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

    // the fields are those of the generic definition, parsed again
    auto parse_generic_fields(toc& tc, const generic_type_info& generic)
        -> void {

        tokenizer generic_tz{tc.source_of(generic.start_tk), generic.start_tk};

        // skips the name and the parameters of the generic definition
        std::ignore = generic_tz.next_token();
        std::ignore = generic_param::parse(generic_tz);

        open_brace_tk_ = generic_tz.is_next_char_token('{');

        assert(not open_brace_tk_.is_empty());

        parse_fields(tc, generic_tz);
    }

    // adds this type with the fields of the generic definition for the
    // arguments, an error in the fields is in the generic definition, the
    // frame names the alias. the result has an instance per generic type, e.g.
    // 'type str = text<8>' with 'type text<capacity> = list<i8, capacity>' is
    // an instance of 'text' and of 'list<i8, 8>'
    auto parse_instance(toc& tc, const token& generic_tk,
                        const generic_type_info& generic,
                        const std::span<const token> arg_tks)
        -> std::vector<generic_type_instance> {

        const std::vector<generic_binding> bindings{
            resolve_args(tc, generic_tk.text(), generic.params, arg_tks),
        };

        const std::string alias_source{
            generic_type_instance::alias_text(name_tk_.text(),
                                              generic_tk.text(), bindings),
        };

        std::vector<generic_type_instance> instances;

        try {
            parse_level(tc, generic_tk.text(), generic, arg_tks, bindings, {},
                        instances);
        } catch (compiler_exception& e) {
            e.add_call_frame(name_tk_, alias_source, "instantiated by");

            throw;
        }

        tc.add_type(name_tk_, type_);

        for (generic_type_instance& instance : instances) {
            instance.alias_source = alias_source;
        }

        return instances;
    }

    // the instance of 'generic' and of its bases, the fields are those of the
    // generic type without a base. 'base_frames' lead from 'generic' to the
    // generic type that the alias names
    auto parse_level(toc& tc, const std::string_view generic_name,
                     const generic_type_info& generic,
                     const std::span<const token> arg_tks,
                     const std::span<const generic_binding> bindings,
                     const std::span<const generic_base_frame> base_frames,
                     std::vector<generic_type_instance>& instances) -> void {

        instances.push_back({
            .src_loc_tk{name_tk_},
            .generic_name{generic_name},
            .type_ptr{&type_},
            .bindings{bindings.begin(), bindings.end()},
            .alias_source{},
            .base_frames{base_frames.begin(), base_frames.end()},
        });

        if (generic.base_tk.is_empty()) {
            parse_root_fields(tc, generic, arg_tks, bindings);
            return;
        }

        const std::string_view base_name{generic.base_tk.text()};
        const generic_type_info& base{tc.generics().get_type(base_name)};

        const std::vector<generic_binding> base_bindings{
            resolve_base_args(tc, generic, base, arg_tks, bindings),
        };

        // an error in the base is reported with the arguments it got
        const generic_base_frame base_frame{
            .src_loc_tk{generic.base_tk},
            .text{
                generic_type_instance::instance_text(base_name, base_bindings),
            },
        };

        std::vector<generic_base_frame> base_frames_of_base{base_frame};
        base_frames_of_base.append_range(base_frames);

        try {
            parse_level(tc, base_name, base, generic.base_arg_tks,
                        base_bindings, base_frames_of_base, instances);
        } catch (compiler_exception& e) {
            e.add_call_frame(base_frame.src_loc_tk, base_frame.text,
                             "instantiated by");

            throw;
        }
    }

    // the constant arguments live in a block of their own
    auto parse_root_fields(toc& tc, const generic_type_info& generic,
                           const std::span<const token> arg_tks,
                           const std::span<const generic_binding> bindings)
        -> void {

        tc.enter_block();

        type_alias_scope aliases{tc};

        bind_args(tc, aliases, generic.params, arg_tks, bindings);

        parse_generic_fields(tc, generic);

        add_fields(tc);

        tc.exit_block();
    }

    //
    // statics
    //

    // the methods of each generic type see the arguments of its instance, the
    // methods of a base are made first
    static auto
    add_instances(toc& tc,
                  const std::span<const generic_type_instance> instances)
        -> void {

        for (const generic_type_instance& instance :
             instances | std::views::reverse) {

            tc.generics().add_type_instance(instance);

            instantiate_generic_methods(tc, instance);
        }
    }

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
    static auto bind_args(toc& tc, type_alias_scope& aliases,
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
    [[nodiscard]] static auto
    resolve_base_args(toc& tc, const generic_type_info& generic,
                      const generic_type_info& base,
                      const std::span<const token> arg_tks,
                      const std::span<const generic_binding> bindings)
        -> std::vector<generic_binding> {

        tc.enter_block();

        std::vector<generic_binding> base_bindings{
            resolve_bound_base_args(tc, generic, base, arg_tks, bindings),
        };

        tc.exit_block();

        return base_bindings;
    }

    // the parameters of 'generic' are names of the current block until the
    // return, when 'aliases' unbinds the type parameters
    [[nodiscard]] static auto
    resolve_bound_base_args(toc& tc, const generic_type_info& generic,
                            const generic_type_info& base,
                            const std::span<const token> arg_tks,
                            const std::span<const generic_binding> bindings)
        -> std::vector<generic_binding> {

        type_alias_scope aliases{tc};

        bind_args(tc, aliases, generic.params, arg_tks, bindings);

        return resolve_args(tc, generic.base_tk.text(), base.params,
                            generic.base_arg_tks);
    }
};
