#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "lut.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"

// a parameter of a generic definition, e.g. 'T type' is a type and 'capacity'
// is a constant
struct generic_param {
    token name_tk;
    bool is_type{};

    //
    // statics
    //

    // e.g. '<T type, capacity>', the '<' is next
    [[nodiscard]] static auto parse(tokenizer& tz)
        -> std::vector<generic_param> {

        const token open_tk{tz.is_next_char_token('<')};

        assert(not open_tk.is_empty());

        std::vector<generic_param> params;

        while (true) {
            const token name_tk{tz.next_token()};

            if (name_tk.text().empty()) {
                throw compiler_exception{
                    tz, "expected a name for the generic parameter"};
            }

            const auto is_same_name{
                [&](const generic_param& param) -> bool {
                    return param.name_tk.text() == name_tk.text();
                },
            };

            if (std::ranges::any_of(params, is_same_name)) {
                throw compiler_exception{
                    name_tk, std::format("generic parameter '{}' is declared "
                                         "twice",
                                         name_tk.text())};
            }

            const char next{tz.peek_char_after_whitespace()};
            const bool is_type{next != ',' and next != '>'};

            if (is_type) {
                const token kind_tk{tz.next_token()};

                if (not kind_tk.is_text("type")) {
                    throw compiler_exception{
                        kind_tk,
                        std::format(
                            "expected 'type' after generic parameter '{}'",
                            name_tk.text())};
                }
            }

            params.push_back({.name_tk{name_tk}, .is_type{is_type}});

            if (not tz.is_next_char_token('>').is_empty()) {
                return params;
            }

            if (tz.is_next_char_token(',').is_empty()) {
                throw compiler_exception{
                    tz, std::format("expected ',' or '>' after generic "
                                    "parameter '{}'",
                                    name_tk.text())};
            }
        }
    }
};

// e.g. '<127, name>' of 'text<127, name>'
struct generic_arguments {
    std::vector<token> arg_tks;
    // the '<', the arguments, the delimiters and the '>' in source order
    std::vector<token> list_tks;

    //
    // statics
    //

    // 'open_tk' is the '<' that has been read
    [[nodiscard]] static auto parse(tokenizer& tz, const token& open_tk)
        -> generic_arguments {

        generic_arguments args;
        args.list_tks.emplace_back(open_tk);

        while (true) {
            const token arg_tk{tz.next_token()};

            if (arg_tk.text().empty()) {
                throw compiler_exception{tz, "expected a generic argument"};
            }

            args.arg_tks.emplace_back(arg_tk);
            args.list_tks.emplace_back(arg_tk);

            const token close_tk{tz.is_next_char_token('>')};

            if (not close_tk.is_empty()) {
                args.list_tks.emplace_back(close_tk);
                return args;
            }

            const token delim_tk{tz.is_next_char_token(',')};

            if (delim_tk.is_empty()) {
                throw compiler_exception{
                    tz, "expected ',' or '>' after generic argument"};
            }

            args.list_tks.emplace_back(delim_tk);
        }
    }
};

// an argument of an instance of a generic type, a type or a constant
struct generic_binding {
    std::string name;
    // null for a constant
    const type* type_ptr{};
    int64_t value{};
};

// the methods of a generic type see the arguments of their instance
struct generic_type_instance {
    // the name of the alias
    token src_loc_tk;
    std::string generic_name;
    const type* type_ptr{};
    std::vector<generic_binding> bindings;

    [[nodiscard]] auto alias_text() const -> std::string {
        return alias_text(type_ptr->name(), generic_name, bindings);
    }

    //
    // statics
    //

    // e.g. 'type str = text<8>', for the errors found in the instance
    [[nodiscard]] static auto
    alias_text(const std::string_view alias_name,
               const std::string_view generic_name,
               const std::span<const generic_binding> bindings) -> std::string {

        std::string text{
            std::format("type {} = {}<", alias_name, generic_name),
        };

        for (const generic_binding& binding : bindings) {
            if (text.back() != '<') {
                text += ", ";
            }

            text += binding.type_ptr != nullptr
                        ? std::string{binding.type_ptr->name()}
                        : std::format("{}", binding.value);
        }

        text += '>';

        return text;
    }
};

// how a call without type arguments can tell a type parameter
struct generic_deduction {
    // the first parameter declared with exactly that type, e.g. 's T' in
    // 'func text.append<T type>(s T)', not an array
    std::optional<size_t> param_index;
    // the result is declared with exactly that type, e.g. 'res T', the
    // destination of the call tells it
    bool is_result{};
};

// a function with type parameters, e.g. 'func tokenizer.to<T type>()'. every
// distinct list of type arguments is the definition parsed again from
// 'start_tk' as an instance. the method of a generic type with type parameters
// of its own is one generic function per instance of the type
struct generic_func_info {
    token src_loc_tk;
    token func_tk;
    token start_tk;
    // the name in the report, a method is one generic function for all the
    // instances of its type, e.g. 'text.append' for 'str.append' and
    // 'name.append'
    std::string report_name;
    std::vector<std::string> param_names;
    // for each type parameter
    std::vector<generic_deduction> deductions;
    std::optional<generic_type_instance> receiver_instance;
};

// a type with parameters, e.g. 'type text<capacity> {...}'. 'type str =
// text<127>' parses the fields again from 'start_tk' as the type 'str'
struct generic_type_info {
    token src_loc_tk;
    token start_tk;
    std::vector<generic_param> params;
};

// a method of a generic type, e.g. 'func text.print()', every instance of the
// type gets its own method
struct generic_method_info {
    std::string type_name;
    token func_tk;
    token start_tk;
};

// the generic functions and types of the program, the methods of the generic
// types and the instances made of them
class generic_registry {
    lut<generic_func_info> funcs_;
    lut<generic_type_info> types_;
    std::vector<generic_method_info> methods_;
    std::vector<generic_type_instance> type_instances_;
    std::set<std::string, std::less<>> instantiated_funcs_;

  public:
    auto add_func(std::string name, generic_func_info info) -> void {
        funcs_.put(std::move(name), std::move(info));
    }

    auto add_method(const std::string_view type_name, const token& func_tk,
                    const token& start_tk) -> void {

        methods_.push_back({
            .type_name{type_name},
            .func_tk{func_tk},
            .start_tk{start_tk},
        });
    }

    auto add_type(const token& src_loc_tk, const std::string_view name,
                  const token& start_tk, std::vector<generic_param> params)
        -> void {

        types_.put(std::string{name}, {
                                          .src_loc_tk{src_loc_tk},
                                          .start_tk{start_tk},
                                          .params{std::move(params)},
                                      });
    }

    auto add_type_instance(generic_type_instance instance) -> void {
        type_instances_.emplace_back(std::move(instance));
    }

    [[nodiscard]] auto get_func(const std::string_view name) const
        -> const generic_func_info& {

        return funcs_.get_const_ref(name);
    }

    [[nodiscard]] auto get_methods(const std::string_view type_name) const
        -> std::vector<generic_method_info> {

        std::vector<generic_method_info> methods;

        for (const generic_method_info& method : methods_) {
            if (method.type_name == type_name) {
                methods.push_back(method);
            }
        }

        return methods;
    }

    [[nodiscard]] auto get_type(const std::string_view name) const
        -> const generic_type_info& {

        return types_.get_const_ref(name);
    }

    [[nodiscard]] auto
    get_type_instances(const std::string_view generic_name) const
        -> std::vector<generic_type_instance> {

        std::vector<generic_type_instance> instances;

        for (const generic_type_instance& instance : type_instances_) {
            if (instance.generic_name == generic_name) {
                instances.push_back(instance);
            }
        }

        return instances;
    }

    [[nodiscard]] auto has_func(const std::string_view name) const -> bool {
        return funcs_.has(name);
    }

    [[nodiscard]] auto has_type(const std::string_view name) const -> bool {
        return types_.has(name);
    }

    auto mark_func_instantiated(const std::string_view name) -> void {
        instantiated_funcs_.emplace(funcs_.get_const_ref(name).report_name);
    }

    // the names of the generic functions without an instance, in the order of
    // definition
    [[nodiscard]] auto uninstantiated_func_names() const
        -> std::vector<std::string> {

        std::vector<std::string> names;

        for (const std::string& key : funcs_.keys()) {
            const std::string& name{funcs_.get_const_ref(key).report_name};

            if (not instantiated_funcs_.contains(name) and
                not std::ranges::contains(names, name)) {

                names.push_back(name);
            }
        }

        return names;
    }

    //
    // statics
    //

    // e.g. 'tokenizer.to<name>' for the generic 'tokenizer.to'
    [[nodiscard]] static auto
    instance_name(const std::string_view generic_name,
                  const std::span<const type* const> type_args) -> std::string {

        std::string name{generic_name};
        name += '<';

        for (const type* const type_arg : type_args) {
            if (name.back() != '<') {
                name += ',';
            }

            name += type_arg->name();
        }

        name += '>';

        return name;
    }
};
