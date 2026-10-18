#pragma once
// reviewed: 2025-09-28

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

// the head of a function definition, 'func [noinline] [mut] name[.method]
// [<T type>] (', read before the kind of the definition is known
//
// e.g. 'func f(x)' and 'func list.add(x)' are functions, 'func f<T type>(x T)'
// is a generic function and 'func text.print()' a method of the generic type
// 'text'
//
// an instance reads the head again where the parameters name the arguments,
// e.g. 'text' names 'str' and 'func text.print()' is the function 'str.print'
struct func_head {
    // where the definition continues after 'func', an instance starts here
    token start_tk;
    token noinline_tk;
    token mut_tk;
    // e.g. 'f', or the receiver type of a method, e.g. 'list' of 'list.add'
    token name_tk;
    token method_dot_tk;
    token method_name_tk;
    // e.g. 'f' or 'list.add', the receiver named as the type it is bound to,
    // e.g. 'str.print' for 'text.print' when 'text' names 'str'
    std::string name;
    // e.g. 'T' of '<T type>'
    std::vector<token> type_param_tks;
    token open_paren_tk;
    // e.g. 'func text.print()' of the generic type 'text', read up to 'text'
    bool is_generic_type_method{};

    //
    // statics
    //

    // 'tz' is after 'func'
    [[nodiscard]] static auto read(const toc& tc, tokenizer& tz) -> func_head {
        func_head head;
        head.start_tk = tz.cur_position_token();
        head.name_tk = tz.next_token();
        head.open_paren_tk = tz.is_next_char_token('(');

        head.read_modifiers(tz);

        toc::assert_valid_function_name(head.name_tk);

        // e.g. 'func text.print()', each instance of 'text' reads the rest
        if (head.names_generic_type(tc, tz)) {
            head.is_generic_type_method = true;
            return head;
        }

        head.read_receiver(tc, tz);

        // e.g. 'func tokenizer.to<T type>()'
        if (head.open_paren_tk.is_empty() and
            tz.peek_char_after_whitespace() == '<') {

            head.type_param_tks = read_type_params(tz);
            head.open_paren_tk = tz.is_next_char_token('(');
        }

        return head;
    }

  private:
    // e.g. the 'text' of 'func text.print()'; in an instance 'text' names a
    // type and is a receiver
    [[nodiscard]] auto names_generic_type(const toc& tc, tokenizer& tz) const
        -> bool {

        return open_paren_tk.is_empty() and
               tc.generics().has_type(name_tk.text()) and
               not tc.has_type(name_tk.text()) and
               tz.peek_char_after_whitespace() == '.';
    }

    // 'name_tk' is the receiver type, a method gets an implicit first
    // parameter 'self' of that type and a constructor builds a 'self' of it
    auto read_method_name(const toc& tc, tokenizer& tz) -> void {
        const type& receiver_type{
            tc.get_type_or_throw(name_tk, name_tk.text()),
        };

        if (receiver_type.is_builtin()) {
            throw compiler_exception{
                name_tk, std::format("methods on built-in type '{}' are not "
                                     "supported",
                                     receiver_type.name())};
        }

        method_name_tk = tz.next_token();

        if (method_name_tk.text().empty()) {
            throw compiler_exception{tz, "expected method name after '.'"};
        }

        toc::assert_valid_name(method_name_tk);

        name =
            std::format("{}.{}", receiver_type.name(), method_name_tk.text());
    }

    // 'noinline' and 'mut' before the name; a function can be named like them
    // when '(' follows
    auto read_modifiers(tokenizer& tz) -> void {
        if (name_tk.is_text("noinline") and open_paren_tk.is_empty()) {
            noinline_tk = name_tk;
            name_tk = tz.next_token();
            open_paren_tk = tz.is_next_char_token('(');
        }

        if (name_tk.is_text("mut") and open_paren_tk.is_empty()) {
            mut_tk = name_tk;
            name_tk = tz.next_token();
            open_paren_tk = tz.is_next_char_token('(');
        }

        name = name_tk.text();
    }

    // e.g. 'func list.add(x)'
    auto read_receiver(const toc& tc, tokenizer& tz) -> void {
        if (not open_paren_tk.is_empty()) {
            return;
        }

        method_dot_tk = tz.is_next_char_token('.');

        if (method_dot_tk.is_empty()) {
            return;
        }

        read_method_name(tc, tz);
        open_paren_tk = tz.is_next_char_token('(');
    }

    //
    // statics
    //

    // e.g. '<T type, U type>', a generic function has type parameters only
    [[nodiscard]] static auto read_type_params(tokenizer& tz)
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
};

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
    // the definition parsed again with the type arguments bound
    bool is_instance_{};
    // the generic type of the method that an instance is of, e.g. 'text'
    std::string generic_type_name_;
    // the constants of that instance, added to the frame of the body
    std::vector<generic_arg> constants_;

  public:
    // a function of the user code, or an instance of a generic definition
    // parsed with 'instance', e.g. 'tokenizer.to<name>' or 'str.print', see
    // 'stmt_def_func_generic'. 'tz' is after 'head'
    stmt_def_func(toc& tc, const token tk, const func_head& head, tokenizer& tz,
                  const generic_func_instance* const instance)
        : statement{tk}, noinline_tk_{head.noinline_tk}, mut_tk_{head.mut_tk},
          name_tk_{head.name_tk}, method_dot_tk_{head.method_dot_tk},
          method_name_tk_{head.method_name_tk}, name_{head.name},
          open_paren_tk_{head.open_paren_tk} {

        // note: a generic definition is a 'stmt_def_func_generic', its
        //       instance has the type arguments
        assert(not head.is_generic_type_method);
        assert(instance != nullptr or head.type_param_tks.empty());

        if (instance != nullptr) {
            set_instance(head, *instance);
        }

        parse_signature(tc, tz);

        parse_body(tc, tz);
    }

    stmt_def_func() = default;
    // 'toc' keeps the address of the definition
    stmt_def_func(const stmt_def_func&) = delete;
    stmt_def_func(stmt_def_func&&) = delete;
    auto operator=(const stmt_def_func&) -> stmt_def_func& = delete;
    auto operator=(stmt_def_func&&) -> stmt_def_func& = delete;

    ~stmt_def_func() override = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
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
        for (const generic_arg& constant : constants_) {
            tc.add_const(name_tk_, indent, constant.param_tk.text(),
                         constant.value);
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

        if (array_lengths.empty()) {
            return is_instance_ ? label_name(name()) : std::string{};
        }

        if (not is_instance_) {
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
                           .foo_array{},
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
                           .foo_array{},
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

    // e.g. 'text.size' for the instance 'str.size', empty if the function is
    // not an instance of a method of a generic type
    [[nodiscard]] auto generic_method_name() const -> std::string {
        if (generic_type_name_.empty() or method_name_tk_.is_empty()) {
            return {};
        }

        return std::format("{}.{}", generic_type_name_, method_name_tk_.text());
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

        tc.parse_storage_unchecked([&] -> void {
            add_signature_vars(tc, 0, false, {}, {});

            code_ = {tc, tz, true};
        });

        tc.exit_func(name());
    }

    auto parse_param_delimiter(tokenizer& tz) -> void {
        const token delimiter_tk{
            tz.expect_char_token(
                ',', std::format("expected ',' or ')' after parameter '{}'",
                                 params_.back().tok().text())),
        };

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

    // 'name [type]' of the returned value, without a name the type is void
    auto parse_returns(toc& tc, tokenizer& tz) -> void {
        const token ident_tk{tz.next_token()};

        if (ident_tk.text().empty()) {
            tz.put_back_token(ident_tk);
            set_type(tc.get_type_void());
            return;
        }

        token type_tk{tz.next_token()};

        // a word before the '{' of the body names the type, else a missing
        // '{' is reported by the body, e.g. 'func f() res number {'
        const bool is_type_before_body{
            not type_tk.text().empty() and
                tz.peek_char_after_whitespace() == '{',
        };

        if (not tc.has_type(type_tk.text()) and not is_type_before_body) {
            tz.put_back_token(type_tk);
            type_tk = {};
        }

        // a name before the body is the result, 'self' names the value a
        // constructor builds
        if (tz.peek_char_after_whitespace() == '{' and
            not ident_tk.is_text(reserved_names::self)) {

            toc::assert_valid_variable_name(ident_tk);
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

    // e.g. 'str.print' of 'func text.print()' or 'tokenizer.to<name>' of
    // 'func tokenizer.to<T type>()'
    auto set_instance(const func_head& head,
                      const generic_func_instance& instance) -> void {

        assert(instance.type_args.size() == head.type_param_tks.size());

        is_instance_ = true;

        // e.g. 'text' of 'func text.print()' names 'str'
        if (instance.receiver != nullptr) {
            generic_type_name_ = instance.receiver->generic_name;
            constants_ = instance.receiver->constants();
        }

        // e.g. 'tokenizer.to<name>'
        if (not instance.type_args.empty()) {
            name_ =
                generic_registry::instance_name(head.name, instance.type_args);
        }
    }

    //
    // statics
    //

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
};

// a generic definition, kept as text for its instances, see 'generics.hpp'
//
// e.g. 'func tokenizer.to<T type>()' is a generic function, a call makes the
// instance 'tokenizer.to<name>' ('instantiate_func')
//
// e.g. 'func text.print()' is a method of the generic type 'text', every
// instance of 'text' makes its own, e.g. 'str.print' ('instantiate_method')
//
// e.g. 'func text.append<T type>(s T)' is both: every instance of 'text' makes
// a generic function, e.g. 'str.append', and a call makes its instance, e.g.
// 'str.append<i8>'
class stmt_def_func_generic final : public statement {
    // the text after 'func', errors in it are found when an instance is parsed
    std::string_view generic_text_;

    // the types a signature names, as written
    struct signature_types {
        // one per parameter, empty for an array
        std::vector<std::string> parameters;

        std::string result;
    };

  public:
    // 'tz' is after the head
    stmt_def_func_generic(toc& tc, const token tk, const func_head& head,
                          tokenizer& tz)
        : statement{tk} {

        if (head.is_generic_type_method) {
            define_method(tc, tk, head, tz);
            return;
        }

        define_func(tc, tk, head, tz, nullptr);

        generic_text_ = tz.skip_braced_block_after(tk);
    }

    stmt_def_func_generic() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        std::print(os, "{}", generic_text_);
    }

    // only its instances are functions
    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {}

    //
    // statics
    //

    // the instance of the generic function for the type arguments, e.g.
    // 'tokenizer.to<name>' of 'func tokenizer.to<T type>()', made at the first
    // call that needs it, returns its name
    [[nodiscard]] static auto instantiate_func(
        toc& tc, const token& call_tk, const std::string_view generic_name,
        const std::span<const type* const> type_args) -> std::string {

        const generic_func_info& generic{tc.generics().get_func(generic_name)};

        if (type_args.size() != generic.param_tks.size()) {
            throw compiler_exception{
                call_tk,
                std::format("generic function '{}' takes {} type argument(s), "
                            "got {}",
                            generic_name, generic.param_tks.size(),
                            type_args.size())};
        }

        tc.generics().mark_func_instantiated(generic_name);

        std::string name{
            generic_registry::instance_name(generic_name, type_args),
        };

        if (tc.is_func(name)) {
            return name;
        }

        // an error in the instance reports the call that needed it
        try {
            parse_func_instance(tc, generic, type_args);
        } catch (compiler_exception& e) {
            e.add_call_frame(call_tk, name);
            throw;
        }

        return name;
    }

    // the methods defined so far for a new instance of a generic type, e.g.
    // 'str.print' of 'func text.print()' for 'type str = text<8>'
    static auto instantiate_methods(toc& tc,
                                    const generic_type_instance& instance)
        -> void {

        for (const generic_method_info& method :
             tc.generics().get_methods(instance.generic_name)) {

            instantiate_method(tc, method, instance);
        }
    }

  private:
    // e.g. 'func text.print()' of the generic type 'text': every instance of
    // the type gets the method, also those made after this definition
    auto define_method(toc& tc, const token& func_tk, const func_head& head,
                       tokenizer& tz) -> void {

        const generic_method_info method{
            .src_loc_tk{head.name_tk},
            .func_tk{func_tk},
            .start_tk{head.start_tk},
        };

        tc.generics().add_method(method);

        generic_text_ = tz.skip_braced_block_after(func_tk);

        for (const generic_type_instance& instance :
             tc.generics().get_type_instances(head.name_tk.text())) {

            instantiate_method(tc, method, instance);
        }
    }

    //
    // statics
    //

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
    // and the result, e.g. 'res T'. 'tz' is after the '(' of 'head' and is a
    // copy, the caller does not move
    [[nodiscard]] static auto deductions(const func_head& head, tokenizer tz)
        -> std::vector<generic_deduction> {

        // note: a missing '(' is reported when an instance is parsed
        const signature_types types{
            head.open_paren_tk.is_empty() ? signature_types{}
                                          : read_signature_types(tz),
        };

        std::vector<generic_deduction> result;
        result.reserve(head.type_param_tks.size());

        for (const token& param_tk : head.type_param_tks) {
            result.push_back(deduction_of(param_tk.text(), types));
        }

        return result;
    }

    // a method of a generic type keeps the instance it was made for, e.g.
    // 'str.append' of 'func text.append<T type>(s T)' keeps 'text<8>'
    static auto define_func(toc& tc, const token& func_tk,
                            const func_head& head, const tokenizer& tz,
                            const generic_type_instance* const receiver)
        -> void {

        for (const token& param_tk : head.type_param_tks) {
            tc.assert_generic_param_free(param_tk, param_tk.text());
        }

        std::optional<generic_type_instance> receiver_copy{
            receiver != nullptr ? std::optional{*receiver} : std::nullopt,
        };

        // e.g. 'text.append' for the instance 'str.append' of 'text'
        std::string report_name{
            receiver != nullptr ? std::format("{}.{}", receiver->generic_name,
                                              head.method_name_tk.text())
                                : head.name,
        };

        tc.add_generic_func(head.name, {
                                           .src_loc_tk{head.name_tk},
                                           .func_tk{func_tk},
                                           .start_tk{head.start_tk},
                                           .report_name{std::move(report_name)},
                                           .param_tks{head.type_param_tks},
                                           .deductions{deductions(head, tz)},
                                           .receiver{std::move(receiver_copy)},
                                       });
    }

    // an error in the method is in the generic code, the frames name the bases
    // and the alias
    static auto instantiate_method(toc& tc, const generic_method_info& method,
                                   const generic_type_instance& receiver)
        -> void {

        try {
            parse_method_instance(tc, method, receiver);
        } catch (compiler_exception& e) {
            receiver.add_frames_to(e);
            throw;
        }
    }

    // e.g. 'tokenizer.to<name>' of 'func tokenizer.to<T type>()', 'T' names
    // 'name' while the definition is parsed again
    static auto
    parse_func_instance(toc& tc, const generic_func_info& generic,
                        const std::span<const type* const> type_args) -> void {

        generic_scope scope{tc};

        // e.g. 'text' names 'str' for 'func text.append<T type>(s T)'
        if (generic.receiver) {
            scope.add_receiver(*generic.receiver);
        }

        // e.g. 'T' names 'name'
        std::vector<generic_arg> args;

        for (const auto [param_tk, type_arg] :
             std::views::zip(generic.param_tks, type_args)) {

            args.push_back({
                .param_tk{param_tk},
                .arg_tk{},
                .type_ptr{type_arg},
                .value{},
            });
        }

        scope.add_args(args);

        const generic_func_instance instance{
            .receiver{generic.receiver ? &*generic.receiver : nullptr},
            .type_args{type_args},
        };

        tokenizer tz{tc.source_of(generic.start_tk), generic.start_tk};

        const func_head head{func_head::read(tc, tz)};

        tc.add_func_instance(std::make_shared<stmt_def_func>(
            tc, generic.func_tk, head, tz, &instance));
    }

    // e.g. 'str.print' of 'func text.print()', 'text' names 'str' while the
    // definition is parsed again; a method with type parameters of its own is
    // a generic function of the instance, e.g. 'str.append' of
    // 'func text.append<T type>(s T)'
    static auto parse_method_instance(toc& tc,
                                      const generic_method_info& method,
                                      const generic_type_instance& receiver)
        -> void {

        generic_scope scope{tc};
        scope.add_receiver(receiver);

        tokenizer tz{tc.source_of(method.start_tk), method.start_tk};

        const func_head head{func_head::read(tc, tz)};

        if (not head.type_param_tks.empty()) {
            define_func(tc, method.func_tk, head, tz, &receiver);
            return;
        }

        const generic_func_instance instance{
            .receiver{&receiver},
            .type_args{},
        };

        tc.add_func_instance(std::make_shared<stmt_def_func>(
            tc, method.func_tk, head, tz, &instance));
    }

    // the same reading as the parameters of an instance, without resolving the
    // types, e.g. '(s T, n int) res T' gives the parameter types 'T' and 'int'
    // and the result type 'T', 'tz' is after the '('
    [[nodiscard]] static auto read_signature_types(tokenizer& tz)
        -> signature_types {

        signature_types types;

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

// reads the head and makes the statement of the kind that follows it
inline auto make_stmt_def_func(toc& tc, const token tk, tokenizer& tz)
    -> std::unique_ptr<statement> {

    const func_head head{func_head::read(tc, tz)};

    // e.g. 'func text.print()' or 'func tokenizer.to<T type>()'
    if (head.is_generic_type_method or not head.type_param_tks.empty()) {
        return std::make_unique<stmt_def_func_generic>(tc, tk, head, tz);
    }

    // e.g. 'func f(x)' or 'func list.add(x)'
    return std::make_unique<stmt_def_func>(tc, tk, head, tz, nullptr);
}
