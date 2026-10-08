#pragma once
// reviewed: 2025-09-28

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <ostream>
#include <print>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "generics.hpp"
#include "machine.hpp"
#include "stmt_block.hpp"
#include "stmt_def_func_param.hpp"
#include "toc.hpp"
#include "token.hpp"

class stmt_def_func final : public statement {
    // what a function has from generics: a generic definition is only text and
    // the method of a generic type has the constants of its instance
    struct generic_part {
        enum class kind : uint8_t {
            // not from generics
            none,
            // kept as text for its instances
            definition,
            // the definition parsed again with the type arguments bound
            instance,
        };

        kind mode{kind::none};
        // the generic type of the method that an instance is of
        std::string type_name;
        // the text after 'func', only the instances of the definition parse it
        std::string_view text;
        std::vector<generic_binding> constants;
    };

    // where the definition continues after 'func', an instance starts here
    token start_tk_;
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
    generic_part generic_;

    // the types a signature names, as written
    struct signature_types {
        // one per parameter, empty for an array
        std::vector<std::string> parameters;

        std::string result;
    };

  public:
    // parses the function definition in the user code
    stmt_def_func(toc& tc, const token tk, tokenizer& tz)
        : stmt_def_func{tc, tk, tz, {}, nullptr} {}

    // 'type_args' are the arguments of an instance of a generic definition, the
    // tokenizer is then at the start of that definition. 'generic_instance' is
    // the instance of a generic type that an instance of its method is for,
    // 'make_instance' makes an instance
    stmt_def_func(toc& tc, const token tk, tokenizer& tz,
                  const std::span<const type* const> type_args,
                  const generic_type_instance* const generic_instance)
        : statement{tk}, start_tk_{tz.cur_position_token()},
          name_tk_{tz.next_token()},
          open_paren_tk_{tz.is_next_char_token('(')} {

        parse_modifiers(tz);

        toc::assert_valid_name(name_tk_);

        // e.g. 'func text.print()' of the generic type 'text'
        if (generic_instance == nullptr and is_generic_method_head(tc, tz)) {
            define_generic_method(tc, tk, tz);
            return;
        }

        // the type names of an instance end with the constructor
        type_alias_scope aliases{tc};

        // e.g. 'text' in 'func text.print()' is the instance of the type
        if (generic_instance != nullptr) {
            generic_.mode = generic_part::kind::instance;
            generic_.type_name = generic_instance->generic_name;

            bind_generic_instance(aliases, *generic_instance);
        }

        parse_receiver(tc, tz);

        // e.g. 'func list.add(x)' or 'func f(x)'
        if (not is_generic_head(tz)) {
            assert(type_args.empty());

            parse_function(tc, tz);

            return;
        }

        // e.g. 'func tokenizer.to<T type>()' is kept for its instances
        const std::vector<token> param_tks{parse_type_params(tz)};

        if (type_args.empty()) {
            define_generic(tc, tk, tz, param_tks, generic_instance);
            return;
        }

        // an instance is the definition parsed with the type parameters bound
        assert(type_args.size() == param_tks.size());

        generic_.mode = generic_part::kind::instance;

        name_ = generic_registry::instance_name(name_, type_args);
        bind_type_args(aliases, param_tks, type_args);
        open_paren_tk_ = tz.is_next_char_token('(');

        parse_function(tc, tz);
    }

    stmt_def_func() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        if (generic_.mode == generic_part::kind::definition) {
            statement::source_to(os);
            std::print(os, "{}", generic_.text);
            return;
        }

        statement::source_to(os);
        source_signature_to(os);
        code_.source_to(os);
    }

    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {}

    //
    // class methods
    //

    // the constant arguments of the generic type of a method, in the frame of
    // the body
    auto add_constants(toc& tc, const size_t indent) const -> void {
        for (const generic_binding& constant : generic_.constants) {
            tc.add_const(name_tk_, indent, constant.name, constant.value);
        }
    }

    [[nodiscard]] auto array_param_count() const -> size_t {
        return static_cast<size_t>(
            std::ranges::count_if(params_, &stmt_def_func_param::is_array));
    }

    // keeps the labels of the body unique per instance: instances of one
    // generic definition have the same source locations
    [[nodiscard]] auto
    body_call_path(const std::span<const size_t> array_lengths) const
        -> std::string {

        const bool is_instance{generic_.mode == generic_part::kind::instance};

        if (array_lengths.empty()) {
            return is_instance ? label_name(name()) : std::string{};
        }

        if (not is_instance) {
            return instance_path(array_lengths);
        }

        return std::format("{}.{}", label_name(name()),
                           instance_path(array_lengths));
    }

    // 'func' is a keyword so no user name or internal label starts with 'func.'
    // e.g. 'func.sum.len.4.8' for array lengths 4 and 8
    [[nodiscard]] auto
    body_label(const std::span<const size_t> array_lengths = {}) const
        -> std::string {

        if (array_lengths.empty()) {
            return std::format("func.{}", label_name(name()));
        }

        return std::format("func.{}.{}", label_name(name()),
                           instance_path(array_lengths));
    }

    [[nodiscard]] auto code() const -> const stmt_block& { return code_; }

    // the lengths are those of the array arguments, one per array parameter
    [[nodiscard]] auto
    compile_body(toc& tc, const size_t indent,
                 const std::span<const size_t> array_lengths) const -> size_t {

        assert(not is_inlined());
        assert(array_lengths.size() == array_param_count());

        machine& x{tc.machine()};

        x.reserve_frame_base();

        tc.enter_noninline_func(name(), body_call_path(array_lengths),
                                x.frame_base_register());

        // the register is held while the body runs
        operand slot_register;

        if (has_slot_register(tc)) {

            slot_register = x.alloc_named_register(
                tok(), indent + 1, x.slot_register(), tc.get_type_address());
        }

        add_constants(tc, indent + 1);

        add_signature_vars(tc, indent + 1, true, array_lengths, slot_register);

        code_.compile(tc, indent, ident_info::make_empty());
        x.return_function(indent + 1);
        const size_t frame_size_bytes{tc.peak_frame_size_bytes()};
        tc.exit_func(name());

        if (not slot_register.is_empty()) {
            x.free_named_register(tok(), indent + 1, slot_register);
        }

        x.release_frame_base();

        return frame_size_bytes;
    }

    // the slots of a call are the result, when there is one, then the
    // parameters
    [[nodiscard]] auto first_param_slot() const -> size_t {
        return returns_ ? size_t{1} : size_t{};
    }

    // a suffix could collide with a method body, e.g. 'func.list.size' of
    // function 'list' and method 'list.size'
    [[nodiscard]] auto
    frame_size_label(const std::span<const size_t> array_lengths = {}) const
        -> std::string {

        return std::format("size.{}", body_label(array_lengths));
    }

    // the address of that slot is passed in a register
    [[nodiscard]] auto has_slot_register(const toc& tc) const -> bool {
        return register_slot_index() and tc.has_slot_register();
    }

    // e.g. 'func point.at(x, y) self'
    [[nodiscard]] auto is_constructor() const -> bool {
        return returns_.has_value() and
               returns_->ident_tk.is_text(reserved_names::self);
    }

    [[nodiscard]] auto is_inlined() const -> bool {
        return noinline_tk_.is_empty();
    }

    // has the implicit first parameter 'self'
    [[nodiscard]] auto is_method() const -> bool {
        return not method_dot_tk_.is_empty() and not is_constructor();
    }

    [[nodiscard]] auto name() const -> std::string_view { return name_; }

    [[nodiscard]] auto noinline_token() const -> const token& {
        return noinline_tk_;
    }

    [[nodiscard]] auto param(const size_t ix) const
        -> const stmt_def_func_param& {

        return params_.at(ix);
    }

    [[nodiscard]] auto params() const -> std::span<const stmt_def_func_param> {
        return params_;
    }

    // the slot of a call whose address may be passed in a register, the
    // receiver of a method
    [[nodiscard]] auto register_slot_index() const -> std::optional<size_t> {
        if (not is_method()) {
            return std::nullopt;
        }

        // note: the receiver is the first parameter
        return first_param_slot();
    }

    [[nodiscard]] auto returns() const
        -> const std::optional<func_return_info>& {

        return returns_;
    }

    auto source_def_comment_to(machine& x, const size_t indent) const -> void {
        std::stringstream ss;
        source_signature_to(ss);
        x.comment(name_tk_, indent, "{}", statement::trimmed_source(ss.view()));
    }

    // the definition after its first token, without the body
    auto source_signature_to(std::ostream& os) const -> void {
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

                // note: +1 because the first parameter is printed above

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

    //
    // statics
    //

    // the instance of a generic definition, parsed again from 'start_tk'
    [[nodiscard]] static auto
    make_instance(toc& tc, const token& func_tk, const token& start_tk,
                  const std::span<const type* const> type_args,
                  const generic_type_instance* const generic_instance)
        -> std::shared_ptr<stmt_def_func> {

        tokenizer tz{tc.source_of(start_tk), start_tk};

        return std::make_shared<stmt_def_func>(tc, func_tk, tz, type_args,
                                               generic_instance);
    }

  private:
    // located at the method name for diagnostics
    auto add_self_param(const toc& tc) -> void {
        const token self_tk{
            token::synthetic(method_name_tk_.file_index(), reserved_names::self,
                             method_name_tk_.start_index(),
                             method_name_tk_.at_line()),
        };

        params_.insert(
            params_.begin(),
            stmt_def_func_param{self_tk,
                                tc.get_type_or_throw(name_tk_, name_tk_.text()),
                                mut_tk_.is_empty()});
    }

    // a non-inline body reaches the result and the arguments through pointer
    // slots in its frame. the array lengths are those of the body instance,
    // empty where the body is only parsed. the parameter of the slot at
    // 'register_slot_index' is at 'address_register' instead of the frame,
    // when given
    auto add_signature_vars(toc& tc, const size_t indent, const bool is_pointer,
                            const std::span<const size_t> array_lengths,
                            const operand& address_register) const -> void {

        assert(array_lengths.empty() or
               array_lengths.size() == array_param_count());

        size_t next_array_length{};

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
                       var_kind::var);
        }

        for (const auto [param_index, param] : std::views::enumerate(params_)) {
            const size_t array_len{
                param.is_array() and not array_lengths.empty()
                    ? array_lengths.at(next_array_length++)
                    : size_t{},
            };

            // the register is the address of the variable, it has no slot in
            // the frame
            const bool is_in_register{
                not address_register.is_empty() and
                    register_slot_index() ==
                        first_param_slot() + static_cast<size_t>(param_index),
            };

            tc.add_var(param.tok(), indent,
                       {
                           .name{param.name()},
                           .type_ptr{&param.get_type()},
                           .src_loc_tk{param.tok()},
                           .is_array{param.is_array()},
                           .is_pointer{is_pointer and not is_in_register},
                           .read_only_why{
                               param.is_read_only() ? read_only_cause::param
                                                    : read_only_cause::none,
                           },
                           .array_len{array_len},
                           .pointer_register{
                               is_in_register ? address_register : operand{},
                           },
                           .base_register{},
                           .value_register{},
                       },
                       var_kind::var);
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

    // the constants of the instance are added to the frame of the body
    auto bind_generic_instance(type_alias_scope& aliases,
                               const generic_type_instance& instance) -> void {

        aliases.bind_instance(name_tk_, instance);

        for (const generic_binding& binding : instance.bindings) {
            if (binding.type_ptr == nullptr) {
                generic_.constants.push_back(binding);
            }
        }
    }

    // the text after 'func' is kept for 'source_to' and for the instances,
    // errors in it are found when an instance is parsed. a method of a generic
    // type keeps the instance it was made for
    auto define_generic(toc& tc, const token& func_tk, tokenizer& tz,
                        const std::span<const token> param_tks,
                        const generic_type_instance* const generic_instance)
        -> void {

        std::vector<std::string> param_names;
        for (const token& param_tk : param_tks) {
            tc.assert_generic_param_free(param_tk, param_tk.text());

            param_names.emplace_back(param_tk.text());
        }

        std::optional<generic_type_instance> receiver_instance{
            generic_instance != nullptr ? std::optional{*generic_instance}
                                        : std::nullopt,
        };

        // e.g. 'text.append' for the instance 'str.append' of 'text'
        std::string report_name{
            generic_instance != nullptr
                ? std::format("{}{}", generic_instance->generic_name,
                              name_.substr(name_.find('.')))
                : name_,
        };

        tc.add_generic_func(name_, {
                                       .src_loc_tk{name_tk_},
                                       .func_tk{func_tk},
                                       .start_tk{start_tk_},
                                       .report_name{std::move(report_name)},
                                       .param_names{std::move(param_names)},
                                       .deductions{deductions(param_tks, tz)},
                                       .receiver_instance{
                                           std::move(receiver_instance),
                                       },
                                   });

        generic_.mode = generic_part::kind::definition;
        generic_.text = tz.skip_braced_block_after(func_tk);
    }

    // e.g. 'func text.print()' of the generic type 'text': every instance of
    // the type gets the method, also those made after this definition
    auto define_generic_method(toc& tc, const token& func_tk, tokenizer& tz)
        -> void {

        const std::string_view type_name{name_tk_.text()};

        tc.generics().add_method(type_name, func_tk, start_tk_);

        generic_.mode = generic_part::kind::definition;
        generic_.text = tz.skip_braced_block_after(func_tk);

        for (const generic_type_instance& instance :
             tc.generics().get_type_instances(type_name)) {

            instantiate_generic_method(tc, func_tk, start_tk_, instance);
        }
    }

    // e.g. 'text.size' for the instance 'str.size', empty if the function is
    // not an instance of a method of a generic type
    [[nodiscard]] auto generic_method_name() const -> std::string {
        if (generic_.type_name.empty() or method_name_tk_.is_empty()) {
            return {};
        }

        return std::format("{}.{}", generic_.type_name, method_name_tk_.text());
    }

    // e.g. the '<' of 'func tokenizer.to<T type>()'
    [[nodiscard]] auto is_generic_head(tokenizer& tz) const -> bool {
        return open_paren_tk_.is_empty() and
               tz.peek_char_after_whitespace() == '<';
    }

    // e.g. the 'text' of 'func text.print()'
    [[nodiscard]] auto is_generic_method_head(const toc& tc,
                                              tokenizer& tz) const -> bool {

        return open_paren_tk_.is_empty() and
               tc.generics().has_type(name_tk_.text()) and
               tz.peek_char_after_whitespace() == '.';
    }

    // the function scope is established before the body is parsed, its
    // variables are registered without emitting output
    auto parse_body(toc& tc, tokenizer& tz) -> void {
        if (is_inlined()) {
            tc.enter_func(name());
        } else {
            tc.enter_noninline_func(name(), {}, {});
        }

        add_constants(tc, 0);

        // note: 'parse_returns' only creates a return with a name
        assert(not returns_ or not returns_->ident_tk.text().empty());

        add_signature_vars(tc, 0, false, {}, {});

        code_ = {tc, tz, true};

        tc.exit_func(name());
    }

    auto parse_function(toc& tc, tokenizer& tz) -> void {
        parse_signature(tc, tz);

        parse_body(tc, tz);
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

        toc::assert_valid_name(method_name_tk_);

        name_ =
            std::format("{}.{}", receiver_type.name(), method_name_tk_.text());
    }

    // 'noinline' and 'mut' before the name; a function can be named like them
    // when '(' follows
    auto parse_modifiers(tokenizer& tz) -> void {
        if (name_tk_.is_text("noinline") and open_paren_tk_.is_empty()) {
            noinline_tk_ = name_tk_;
            name_tk_ = tz.next_token();
            open_paren_tk_ = tz.is_next_char_token('(');
        }

        if (name_tk_.is_text("mut") and open_paren_tk_.is_empty()) {
            mut_tk_ = name_tk_;
            name_tk_ = tz.next_token();
            open_paren_tk_ = tz.is_next_char_token('(');
        }

        name_ = name_tk_.text();
    }

    auto parse_param_delimiter(tokenizer& tz) -> void {
        const token delimiter_tk{tz.is_next_char_token(',')};

        if (delimiter_tk.is_empty()) {
            throw compiler_exception{
                tz, std::format("expected ',' or ')' after parameter '{}'",
                                params_.back().tok().text())};
        }

        param_delims_tk_.emplace_back(delimiter_tk);
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

    // e.g. 'func list.add(x)'
    auto parse_receiver(const toc& tc, tokenizer& tz) -> void {
        if (not open_paren_tk_.is_empty()) {
            return;
        }

        method_dot_tk_ = tz.is_next_char_token('.');

        if (method_dot_tk_.is_empty()) {
            return;
        }

        parse_method_name(tc, tz);
        open_paren_tk_ = tz.is_next_char_token('(');
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

        if (ident_tk.is_text(reserved_names::self)) {
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

    // '(parameters) [result]', then the calls after it know the function
    auto parse_signature(toc& tc, tokenizer& tz) -> void {
        if (open_paren_tk_.is_empty()) {
            throw compiler_exception{tz, "expected '(' after function name"};
        }

        parse_params(tc, tz);

        parse_returns(tc, tz);

        assert_mut_has_receiver();

        // known only after the result: a constructor builds 'self' instead
        if (is_method()) {
            add_self_param(tc);
        }

        tc.add_func(name_tk_, name_, statement::get_type(), this,
                    generic_method_name());
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

    //
    // statics
    //

    // the type parameters name their arguments while the instance is parsed
    static auto bind_type_args(type_alias_scope& aliases,
                               const std::span<const token> param_tks,
                               const std::span<const type* const> type_args)
        -> void {

        for (const auto [param_tk, type_arg] :
             std::views::zip(param_tks, type_args)) {

            aliases.bind(param_tk, param_tk.text(), *type_arg);
        }
    }

    [[nodiscard]] static auto deduction_of(const std::string_view param_name,
                                           const signature_types& types)
        -> generic_deduction {

        const auto it{std::ranges::find(types.parameters, param_name)};

        std::optional<size_t> param_index;

        if (it != types.parameters.end()) {
            param_index = static_cast<size_t>(it - types.parameters.begin());
        }

        return {
            .param_index{param_index},
            .is_result{types.result == param_name},
        };
    }

    // how a call can tell each type parameter: the first parameter declared
    // with exactly its type, e.g. 's T' in 'func text.append<T type>(s T)',
    // and the result, e.g. 'res T'. 'tz' is before the '(' of the parameters
    // and is a copy, the caller does not move
    [[nodiscard]] static auto deductions(const std::span<const token> param_tks,
                                         tokenizer tz)
        -> std::vector<generic_deduction> {

        const signature_types types{read_signature_types(tz)};

        std::vector<generic_deduction> result;

        for (const token& param_tk : param_tks) {
            result.push_back(deduction_of(param_tk.text(), types));
        }

        return result;
    }

    // e.g. 'len.4.8', unlike the 'L.C' pairs of inlined calls
    [[nodiscard]] static auto
    instance_path(const std::span<const size_t> array_lengths) -> std::string {

        std::string path{"len"};

        for (const size_t length : array_lengths) {
            path += std::format(".{}", length);
        }

        return path;
    }

    // a label has no '<', '>' or ',', e.g. 'tokenizer.to<name>' is
    // 'tokenizer.to.name'
    [[nodiscard]] static auto label_name(const std::string_view name)
        -> std::string {

        std::string label;

        for (const char ch : name) {
            if (ch == '<' or ch == ',') {
                label.push_back('.');
            } else if (ch != '>') {
                label.push_back(ch);
            }
        }

        return label;
    }

    // e.g. '<T type, U type>', a generic function has type parameters only
    [[nodiscard]] static auto parse_type_params(tokenizer& tz)
        -> std::vector<token> {

        std::vector<token> param_tks;

        for (const generic_param& param : generic_param::parse(tz)) {
            if (not param.is_type) {
                throw compiler_exception{
                    param.name_tk,
                    std::format("generic parameter '{}' of a function must "
                                "have the kind 'type', e.g. '{} type'",
                                param.name_tk.text(), param.name_tk.text())};
            }

            param_tks.emplace_back(param.name_tk);
        }

        return param_tks;
    }

    // the same reading as the parameters of an instance, without resolving the
    // types, e.g. '(s T, n int) res T' gives the parameter types 'T' and 'int'
    // and the result type 'T'
    [[nodiscard]] static auto read_signature_types(tokenizer& tz)
        -> signature_types {

        signature_types types;

        if (tz.is_next_char_token('(').is_empty()) {
            return types;
        }

        bool is_closed{not tz.is_next_char_token(')').is_empty()};

        while (not is_closed) {
            if (not types.parameters.empty() and
                tz.is_next_char_token(',').is_empty()) {

                break;
            }

            const stmt_def_func_param::syntax param{
                stmt_def_func_param::syntax::read(tz),
            };

            // an array is not the type itself
            types.parameters.emplace_back(
                param.is_array() ? "" : param.type_tk.text());

            is_closed = not tz.is_next_char_token(')').is_empty();
        }

        if (is_closed) {
            types.result = result_type_text(tz);
        }

        return types;
    }

    // the type of 'name [type]' after the parameters, empty without one
    [[nodiscard]] static auto result_type_text(tokenizer& tz) -> std::string {
        const token name_tk{tz.next_token()};

        if (name_tk.text().empty() or tz.peek_char_after_whitespace() == '{') {
            return {};
        }

        return std::string{tz.next_token().text()};
    }
};
