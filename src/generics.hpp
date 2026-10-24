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
#include <variant>
#include <vector>

#include "compile_error.hpp"
#include "lut.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "type.hpp"

//
// generics: a generic definition is kept as text with its parameters, an
// instance is that text parsed again in a 'generic_scope' (in 'toc.hpp')
// where each parameter names its argument
//
// e.g. 'type list<T type, capacity> {...}' is a 'generic_type_info', the alias
// 'type str = list<i8, 8>' makes the 'generic_type_instance' 'list<i8, 8>'
// with the 'generic_arg's 'T' 'i8' and 'capacity' 8, it parses the fields of
// 'list' again as those of 'str' (see 'stmt_def_type_alias')
//
// e.g. 'func list.add(x)' is a 'generic_method_info', every instance of
// 'list' parses it again as its own method, e.g. 'str.add'
//
// e.g. 'func f<T type>(x T)' is a 'generic_func_info', a call 'f<i8>(x)' or
// 'f(x)' parses it again as 'f<i8>' (see 'func_instantiator')
//
// an error in the text of an instance gets the 'generic_trace' that led to it
//

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

        bool is_last{};

        while (not is_last) {
            params.push_back(parse_parameter(tz, params));
            is_last = ends_parameters(tz, params.back());
        }

        return params;
    }

  private:
    //
    // statics
    //

    // the '>' ends the parameters, a ',' goes on with the next, e.g. the ','
    // after 'T type' of '<T type, capacity>'
    [[nodiscard]] static auto ends_parameters(tokenizer& tz,
                                              const generic_param& param)
        -> bool {

        if (not tz.is_next_char_token('>').is_empty()) {
            return true;
        }

        const token delimiter_tk{tz.is_next_char_token(',')};

        if (delimiter_tk.is_empty()) {
            throw compile_error{tz,
                                std::format("expected ',' or '>' after generic "
                                            "parameter {}",
                                            in_quotes(param.name_tk.text()))};
        }

        tz.assert_no_trailing_delimiter(delimiter_tk, '>', "parameters");

        return false;
    }

    // e.g. 'T type' or 'capacity', a name is declared once
    [[nodiscard]] static auto
    parse_parameter(tokenizer& tz,
                    const std::span<const generic_param> declared)
        -> generic_param {

        const token name_tk{tz.next_token()};

        if (not name_tk.is_word()) {
            throw compile_error{tz.position_of(name_tk),
                                "expected a name for the generic parameter"};
        }

        for (const generic_param& param : declared) {
            if (param.name_tk.text() == name_tk.text()) {
                throw compile_error{
                    name_tk, std::format("generic parameter {} is declared "
                                         "twice",
                                         in_quotes(name_tk.text()))};
            }
        }

        return {
            .name_tk{name_tk},
            .is_type{reads_type_kind(tz, name_tk)},
        };
    }

    // 'type' after the name makes the parameter a type, none makes it a
    // constant, e.g. 'T type' and 'capacity' of '<T type, capacity>'
    [[nodiscard]] static auto reads_type_kind(tokenizer& tz,
                                              const token& name_tk) -> bool {

        const char next{tz.peek_char_after_whitespace()};

        if (next == ',' or next == '>') {
            return false;
        }

        const token kind_tk{tz.next_token()};

        if (not kind_tk.is_text("type")) {
            throw compile_error{
                kind_tk,
                std::format("expected 'type' after generic parameter {}",
                            in_quotes(name_tk.text()))};
        }

        return true;
    }
};

// the tokens of '<127, name>' of 'text<127, name>' as written
struct generic_arg_tokens {
    std::vector<token> arg_tks;
    // the '<', the arguments, the delimiters and the '>' in source order
    std::vector<token> list_tks;

    //
    // statics
    //

    // 'open_tk' is the '<' that has been read
    [[nodiscard]] static auto parse(tokenizer& tz, const token& open_tk)
        -> generic_arg_tokens {

        generic_arg_tokens args;
        args.list_tks.emplace_back(open_tk);

        while (true) {
            const token arg_tk{tz.next_token()};

            if (arg_tk.is_none()) {
                throw compile_error{tz, "expected a generic argument"};
            }

            args.arg_tks.emplace_back(arg_tk);
            args.list_tks.emplace_back(arg_tk);

            const token close_tk{tz.is_next_char_token('>')};

            if (not close_tk.is_empty()) {
                args.list_tks.emplace_back(close_tk);
                return args;
            }

            const token delim_tk{
                tz.expect_char_token(
                    ',', "expected ',' or '>' after generic argument"),
            };

            tz.assert_no_trailing_delimiter(delim_tk, '>', "type arguments");

            args.list_tks.emplace_back(delim_tk);
        }
    }
};

// what a parameter is in one instance, a type or a constant, e.g. 'T' is 'i8'
// and 'capacity' is 8 in 'list<i8, 8>'
struct generic_arg {
    // e.g. 'T' of 'type list<T type, capacity>'
    token param_tk;
    // e.g. 'i8' of 'list<i8, 8>', empty for a type argument of a function
    token arg_tk;
    // the type, or the value of a constant
    std::variant<const type*, int64_t> value;

    [[nodiscard]] auto constant() const -> int64_t {
        return std::get<int64_t>(value);
    }

    [[nodiscard]] auto is_type() const -> bool {
        return std::holds_alternative<const type*>(value);
    }

    [[nodiscard]] auto type_ref() const -> const type& {
        return *std::get<const type*>(value);
    }
};

// the two spellings of an instance: a function instance is a function named
// by 'generic_func_instance_name', e.g. 'tokenizer.to<name>'; a type instance
// is a type named by its alias, the errors in it show it as
// 'generic_instance_text', e.g. 'list<i8, 8>'

// e.g. 'tokenizer.to<name>' for the generic 'tokenizer.to'
[[nodiscard]] inline auto
generic_func_instance_name(const std::string_view generic_name,
                           const std::span<const type* const> type_args)
    -> std::string {

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

// e.g. 'list<i8, 8>', how the errors found in an instance name it
[[nodiscard]] inline auto
generic_instance_text(const std::string_view generic_name,
                      const std::span<const generic_arg> args) -> std::string {

    std::string text{std::format("{}<", generic_name)};

    for (const generic_arg& arg : args) {
        if (text.back() != '<') {
            text += ", ";
        }

        text += arg.is_type() ? std::string{arg.type_ref().name()}
                              : std::format("{}", arg.constant());
    }

    text += '>';

    return text;
}

// the order of the top-level definitions: a generic body sees, like a plain
// body, only what is defined before its generic, so while an instance is
// parsed the globals, types, functions and generics defined after it are
// hidden; the built-ins come before every definition, and a method, a
// constructor and an instance have no place in the order: they are reached
// through their type or their generic, e.g. the methods of a type argument
class definition_order final {
    // the definition being parsed, 0 before the first
    size_t current_{};
    // set while an instance is parsed, the last definition it sees
    std::optional<size_t> visible_until_;

  public:
    // an instance parse sees the definitions up to 'last'
    class instance_scope final {
        definition_order& order_;
        std::optional<size_t> saved_;

      public:
        instance_scope(definition_order& order, const size_t last)
            : order_{order}, saved_{order.visible_until_} {

            order_.visible_until_ = last;
        }

        instance_scope(const instance_scope&) = delete;
        instance_scope(instance_scope&&) = delete;
        auto operator=(const instance_scope&) -> instance_scope& = delete;
        auto operator=(instance_scope&&) -> instance_scope& = delete;

        ~instance_scope() { order_.visible_until_ = saved_; }
    };

    auto begin_definition() -> void { ++current_; }

    [[nodiscard]] auto current() const -> size_t { return current_; }

    // what definition 'definition' made is seen where the name is used
    [[nodiscard]] auto is_visible(const size_t definition) const -> bool {
        return not visible_until_ or definition <= *visible_until_;
    }
};

// the instances of generic definitions that led to an error in generic code,
// innermost first, e.g. 'list<i8, 8>' at the 'list' of 'type text<size> =
// list<i8, size>', then 'type str = text<8>' at 'str'
class generic_trace final {
    std::vector<compile_error::call_frame> frames_;

  public:
    // the instance 'text' made at 'src_loc_tk'
    generic_trace(const token& src_loc_tk, std::string text)
        : frames_{
              compile_error::call_frame::at(src_loc_tk, std::move(text),
                                            "instantiated by"),
          } {}

    generic_trace() = default;

    auto add_to(compile_error& e) const -> void {
        e.call_frames.append_range(frames_);
    }

    // this instance made inside the instances of 'outer'
    [[nodiscard]] auto inside(const generic_trace& outer) const
        -> generic_trace {

        generic_trace trace{*this};
        trace.frames_.append_range(outer.frames_);
        return trace;
    }
};

// a method name as written, e.g. 'text.print' for the method 'print' of
// 'text'
[[nodiscard]] inline auto method_name_of(const std::string_view type_name,
                                         const std::string_view method)
    -> std::string {

    return std::format("{}.{}", type_name, method);
}

// a type with parameters, e.g. 'type list<T type, capacity> {...}'. an alias
// 'type str = list<i8, 8>' parses the fields again from 'start_tk' with the
// parameters naming the arguments
struct generic_type_info {
    // e.g. 'list'
    token src_loc_tk;
    // where the definition continues after 'type', e.g. at 'list'
    token start_tk;
    std::vector<generic_param> params;
    // e.g. 'list' of 'type text<size> = list<i8, size>', empty when
    // the fields follow the parameters
    token base_tk;
    // e.g. 'i8' and 'size' of 'list<i8, size>'
    std::vector<token> base_arg_tks;
    // see 'definition_order'
    size_t definition{};
};

// the instance of a generic type that an alias names, or a base of it, e.g.
// 'type str = text<8>' makes 'text<8>' and its base 'list<i8, 8>', both are
// the type 'str'
struct generic_type_instance {
    // e.g. 'text'
    std::string generic_name;
    // e.g. the type 'str'
    const type* type_ptr{};
    // e.g. 'size' is 8
    std::vector<generic_arg> args;
    // e.g. 'list<i8, 8>' then 'type str = text<8>' for the base 'list', the
    // alias last
    generic_trace trace;

    // e.g. 'capacity' is 8
    [[nodiscard]] auto constants() const -> std::vector<generic_arg> {
        std::vector<generic_arg> result;

        for (const generic_arg& arg : args) {
            if (not arg.is_type()) {
                result.push_back(arg);
            }
        }

        return result;
    }

    // the name written in the source, e.g. 'text.print' for the method
    // 'print' of 'text<8>'
    [[nodiscard]] auto method_name(const std::string_view name) const
        -> std::string {

        return method_name_of(generic_name, name);
    }

    // e.g. 'T' is 'i8'
    [[nodiscard]] auto type_args() const -> std::vector<generic_arg> {
        std::vector<generic_arg> result;

        for (const generic_arg& arg : args) {
            if (arg.is_type()) {
                result.push_back(arg);
            }
        }

        return result;
    }
};

// how a call without type arguments can tell a type parameter
struct generic_deduction {
    // the first parameter declared with exactly that type, e.g. 's T' in
    // 'func text.append<T type>(s T)', not an array
    std::optional<size_t> param_index;
    // the name of that parameter, e.g. 's', for a missing argument
    std::string param_name;
    // the result is declared with exactly that type, e.g. 'res T', the
    // destination of the call tells it
    bool is_result{};
};

// a parameter of a generic function as written, e.g. 'x' of type 'U' in
// '(x U)'
struct generic_signature_param {
    token name_tk;
    // empty for an array, which is not of the type itself
    std::string type_name;
};

// what a generic function or a method of a generic type declares, read once
// at its definition, e.g. '<U type>(x U) res U' of 'func box.f<U type>(x U)
// res U'; the types are resolved by each instance
struct generic_signature {
    // its own, e.g. 'U', not the parameters of the generic type 'box'
    std::vector<token> type_param_tks;
    std::vector<generic_signature_param> params;
    // e.g. 'res', or 'self' of a constructor, empty without a result
    token result_name_tk;
    // empty for the default type
    token result_type_tk;
};

// a function with type parameters, e.g. 'func tokenizer.to<T type>()'. every
// distinct list of type arguments is the definition parsed again from
// 'start_tk' as an instance. the method of a generic type with type parameters
// of its own is one generic function per instance of the type
struct generic_func_info {
    // e.g. 'tokenizer'
    token src_loc_tk;
    // e.g. 'func'
    token func_tk;
    // where the definition continues after 'func', e.g. at 'tokenizer'
    token start_tk;
    // the name in the report, a method is one generic function for all the
    // instances of its type, e.g. 'text.append' for 'str.append' and
    // 'name.append'
    std::string report_name;
    // e.g. '<T type>()'
    generic_signature signature;
    // for each type parameter
    std::vector<generic_deduction> deductions;
    // the instance of the generic type of a method, e.g. 'text<8>' for
    // 'str.append'
    std::optional<generic_type_instance> receiver;
    // the last definition the body sees, see 'definition_order'
    size_t definition{};
    // reached through its type wherever it is defined
    bool is_method{};
};

// what a function made from a generic definition keeps of it
// e.g. 'tokenizer.to<name>' of 'func tokenizer.to<T type>()'
// e.g. 'str.print' of 'func text.print()' with 'size' 8 for 'text<8>'
struct func_instance {
    // e.g. 'tokenizer.to<name>' or 'str.print'
    std::string name;
    // e.g. 'text.print' for 'str.print', empty for a generic function without
    // a receiver
    std::string generic_method;
    // the constants of the generic type of a method, in the frame of the body
    std::vector<generic_arg> constants;
    // what made the instance, e.g. the call 'make<i8>' or the alias 'type str =
    // text<8>', for the errors that the check of an uncalled instance finds
    generic_trace trace;
};

// a method of a generic type, e.g. 'func text.print()', every instance of the
// type gets its own method
struct generic_method_info {
    // the generic type it is a method of, e.g. 'text'
    token type_tk;
    // e.g. 'print'
    token method_name_tk;
    // e.g. 'func'
    token func_tk;
    // where the definition continues after 'func', e.g. at 'text'
    token start_tk;
    // e.g. '()' of 'func text.print()'
    generic_signature signature;
    // see 'definition_order'
    size_t definition{};
};

// the generic functions and types of the program, the methods of the generic
// types and the instances made of them
class generic_registry final {
    lut<generic_func_info> funcs_;
    lut<generic_type_info> types_;
    std::vector<generic_method_info> methods_;
    std::vector<generic_type_instance> type_instances_;
    std::set<std::string, std::less<>> instantiated_funcs_;

    std::reference_wrapper<const definition_order> order_;

  public:
    explicit generic_registry(const definition_order& order) : order_{order} {}

    auto add_func(std::string name, generic_func_info info) -> void {
        funcs_.put(std::move(name), std::move(info));
    }

    auto add_method(const generic_method_info& info) -> void {
        methods_.push_back(info);
    }

    auto add_type(const std::string_view name, generic_type_info info) -> void {
        types_.put(std::string{name}, std::move(info));
    }

    auto add_type_instance(generic_type_instance instance) -> void {
        type_instances_.emplace_back(std::move(instance));
    }

    // also a generic that the current instance parse does not see, for the
    // checks of a name defined twice
    [[nodiscard]] auto exists_func(const std::string_view name) const -> bool {
        return funcs_.has(name);
    }

    [[nodiscard]] auto exists_type(const std::string_view name) const -> bool {
        return types_.has(name);
    }

    [[nodiscard]] auto get_func(const std::string_view name) const
        -> const generic_func_info& {

        return funcs_.get_const_ref(name);
    }

    [[nodiscard]] auto get_methods(const std::string_view type_name) const
        -> std::vector<generic_method_info> {

        std::vector<generic_method_info> methods;

        for (const generic_method_info& method : methods_) {
            if (method.type_tk.text() == type_name) {
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

    // a generic the current instance parse sees, see 'definition_order'
    [[nodiscard]] auto has_func(const std::string_view name) const -> bool {
        if (not funcs_.has(name)) {
            return false;
        }

        const generic_func_info& info{funcs_.get_const_ref(name)};

        return info.is_method or order_.get().is_visible(info.definition);
    }

    [[nodiscard]] auto has_type(const std::string_view name) const -> bool {
        return types_.has(name) and
               order_.get().is_visible(types_.get_const_ref(name).definition);
    }

    auto mark_func_instantiated(const std::string_view name) -> void {
        instantiated_funcs_.emplace(funcs_.get_const_ref(name).report_name);
    }

    // the parameters of every generic definition, e.g. 'T' of 'func f<T
    // type>()', 'U' of 'func box.f<U type>()' and 'n' of 'type list<T type,
    // n>'; not those of the functions that instances of a generic type make
    // of its methods, the definition holds them
    [[nodiscard]] auto param_tokens() const -> std::vector<token> {
        std::vector<token> tks;

        for (const generic_func_info& func : funcs_.values()) {
            if (not func.receiver) {
                tks.append_range(func.signature.type_param_tks);
            }
        }

        for (const generic_method_info& method : methods_) {
            tks.append_range(method.signature.type_param_tks);
        }

        for (const generic_type_info& type : types_.values()) {
            for (const generic_param& param : type.params) {
                tks.push_back(param.name_tk);
            }
        }

        return tks;
    }

    // the names of the generic functions without an instance, in the order of
    // definition; a method of a generic type with type parameters of its own
    // is one, also without an instance of its type, e.g. 'box.f' of 'func
    // box.f<U type>()'
    [[nodiscard]] auto uninstantiated_func_names() const
        -> std::vector<std::string> {

        struct named_definition {
            size_t definition{};
            std::string name;
        };

        std::vector<named_definition> generics;

        for (const generic_func_info& func : funcs_.values()) {
            if (not func.receiver) {
                generics.push_back({
                    .definition{func.definition},
                    .name{func.report_name},
                });
            }
        }

        for (const generic_method_info& method : methods_) {
            if (not method.signature.type_param_tks.empty()) {
                generics.push_back({
                    .definition{method.definition},
                    .name{
                        method_name_of(method.type_tk.text(),
                                       method.method_name_tk.text()),
                    },
                });
            }
        }

        std::ranges::stable_sort(generics, {}, &named_definition::definition);

        std::vector<std::string> names;

        for (const named_definition& generic : generics) {
            if (not instantiated_funcs_.contains(generic.name)) {
                names.push_back(generic.name);
            }
        }

        return names;
    }
};
