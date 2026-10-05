#pragma once
// reviewed: 2025-09-28

#include <cassert>
#include <cstdint>
#include <format>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <tuple>
#include <vector>

#include "generics.hpp"
#include "stmt_def_type_field.hpp"
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

        toc::assert_name_not_reserved(name_tk_);

        if (not open_brace_tk_.is_empty()) {
            parse_fields(tc, tz);

            add_type(tc);

            return;
        }

        const char next{tz.peek_char_after_whitespace()};

        // e.g. 'type text<capacity> {...}'
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
            toc::assert_name_not_reserved(fld.tok());

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

    // the methods of the generic type see the arguments of the alias
    auto add_instance(toc& tc, const token& generic_tk,
                      std::vector<generic_binding> bindings) -> void {

        const generic_type_instance instance{
            .src_loc_tk{name_tk_},
            .generic_name{generic_tk.text()},
            .type_ptr{&type_},
            .bindings{std::move(bindings)},
        };

        tc.generics().add_type_instance(instance);

        instantiate_generic_methods(tc, instance);
    }

    auto add_type(toc& tc) -> void {
        add_fields(tc);

        tc.add_type(name_tk_, type_);
    }

    // the text after 'type' is kept for 'source_to' and for the aliases, errors
    // in it are found when an alias parses it
    auto define_generic(toc& tc, const token& type_tk, tokenizer& tz) -> void {
        std::vector<generic_param> params{generic_param::parse(tz)};

        for (const generic_param& param : params) {
            tc.assert_generic_param_free(param.name_tk, param.name_tk.text());
        }

        tc.add_generic_type(name_tk_, name_tk_.text(), start_tk_,
                            std::move(params));

        kind_ = kind::generic;
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

        add_instance(tc, generic_tk,
                     parse_instance(tc, generic_tk, generic, arg_tks));
    }

    // appends the tokens after the name to 'alias_tks_' and returns the
    // arguments
    auto parse_alias_args(tokenizer& tz) -> std::vector<token> {
        const token open_tk{tz.is_next_char_token('<')};
        if (open_tk.is_empty()) {
            throw compiler_exception{
                alias_tks_.back(),
                std::format("generic type '{}' needs arguments, e.g. '{}<...>'",
                            alias_tks_.back().text(),
                            alias_tks_.back().text())};
        }

        generic_arguments args{generic_arguments::parse(tz, open_tk)};

        alias_tks_.append_range(args.list_tks);

        return std::move(args.arg_tks);
    }

    // appends the name of the generic type to 'alias_tks_'
    auto parse_alias_generic(const toc& tc, tokenizer& tz) -> token {
        const token generic_tk{tz.next_token()};
        alias_tks_.emplace_back(generic_tk);

        if (tc.has_type(generic_tk.text())) {
            throw compiler_exception{
                generic_tk, std::format("'{}' is a type, not a generic type",
                                        generic_tk.text())};
        }

        if (not tc.generics().has_type(generic_tk.text())) {
            throw compiler_exception{
                generic_tk,
                std::format("'{}' is not a generic type, a generic type is "
                            "defined before the alias that names it",
                            generic_tk.text())};
        }

        return generic_tk;
    }

    auto parse_fields(toc& tc, tokenizer& tz) -> void {
        while (true) {
            // read field definition with the next token being the name
            fields_.emplace_back(tc, tz.next_token(), tz);
            close_brace_tk_ = tz.is_next_char_token('}');
            if (not close_brace_tk_.is_empty()) {
                break;
            }
            const token t{tz.is_next_char_token(',')};
            if (t.is_empty()) {
                throw compiler_exception{
                    tz, std::format("expected ',' followed by another field "
                                    "in type '{}'",
                                    name_tk_.text())};
            }
            field_delims_tk_.emplace_back(t);

            // a trailing ',' lets each field end its line the same way
            close_brace_tk_ = tz.is_next_char_token('}');
            if (not close_brace_tk_.is_empty()) {
                break;
            }
        }
    }

    // the fields are those of the generic definition, parsed again
    auto parse_generic_fields(toc& tc, const generic_type_info& generic)
        -> void {

        tokenizer generic_tz{tc.source(), generic.start_tk};
        std::ignore = generic_tz.next_token();
        std::ignore = generic_param::parse(generic_tz);

        open_brace_tk_ = generic_tz.is_next_char_token('{');

        assert(not open_brace_tk_.is_empty());

        parse_fields(tc, generic_tz);
    }

    // adds this type with the fields of the generic definition for the
    // arguments, the constant arguments live in a block of their own. an error
    // in the fields is in the generic definition, the frame names the alias
    auto parse_instance(toc& tc, const token& generic_tk,
                        const generic_type_info& generic,
                        const std::span<const token> arg_tks)
        -> std::vector<generic_binding> {

        tc.enter_block();

        type_alias_scope aliases{tc};

        std::vector<generic_binding> bindings{
            bind_args(tc, aliases, generic_tk.text(), generic.params, arg_tks),
        };

        try {
            parse_generic_fields(tc, generic);

            add_fields(tc);
        } catch (compiler_exception& e) {
            e.add_call_frame(name_tk_,
                             generic_type_instance::alias_text(
                                 name_tk_.text(), generic_tk.text(), bindings),
                             "instantiated by");

            throw;
        }

        tc.add_type(name_tk_, type_);

        tc.exit_block();

        return bindings;
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

    // a type parameter names its argument, a constant parameter is a constant
    // of the current block
    [[nodiscard]] static auto bind_args(
        toc& tc, type_alias_scope& aliases, const std::string_view generic_name,
        const std::span<const generic_param> params,
        const std::span<const token> arg_tks) -> std::vector<generic_binding> {

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

                aliases.bind(param.name_tk, name, type_arg);
                bindings.push_back({.name{name}, .type_ptr{&type_arg}});

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

            tc.add_const(arg_tk, 0, name, *value);
            bindings.push_back({.name{name}, .value{*value}});
        }

        return bindings;
    }
};
