#pragma once

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
#include <tuple>
#include <utility>
#include <vector>

#include "compile_error.hpp"
#include "generics.hpp"
#include "ident_info.hpp"
#include "machine.hpp"
#include "stmt_block.hpp"
#include "stmt_def_func_param.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "ub_assigned.hpp"

//
// a function definition, e.g. 'func f(x)', is the classes below, in reading
// order:
//
//   'make_stmt_def_func'     reads the head and picks the statement (last in
//                            the file, it uses the others)
//   'func_head'              'func [noinline] [mut] name[.method] [<T type>] ('
//   'func_result_syntax'     the tokens of the result 'name [type]'
//   'func_return_info'       the result of a function, its name and type
//   'stmt_def_func'          a function, of the user code or an instance
//   'func_instantiator'      records the generic definitions and makes their
//                            instances, each a 'stmt_def_func'
//   'stmt_def_func_generic'  a generic definition, kept as text
//
// the formatter sorts the functions of a class by name, so the comment above
// 'func_instantiator' lists its functions in the order they run
//

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
    // the type of 'name_tk' of a method, null for a function
    const type* receiver_type{};
    // e.g. 'f' or 'list.add', the receiver named as the type it is bound to,
    // e.g. 'str.print' for 'text.print' when 'text' names 'str'
    std::string name;
    // e.g. 'T' of '<T type>'
    std::vector<token> type_param_tks;
    token open_paren_tk;
    // e.g. 'func text.print()' of the generic type 'text', read up to 'text'
    bool is_generic_type_method{};

    // kept as text, its instances are the functions
    [[nodiscard]] auto is_generic() const -> bool {
        return is_generic_type_method or not type_param_tks.empty();
    }

    //
    // statics
    //

    // 'tz' is after 'func'
    //
    // the '(' is looked for after each word: a word followed by '(' is the
    // name, e.g. 'func noinline()' reads 'noinline' as the name, which is
    // then rejected as a keyword
    [[nodiscard]] static auto read(const toc& tc, tokenizer& tz) -> func_head {
        func_head head;
        head.start_tk = tz.cur_position_token();
        head.name_tk = tz.next_token();
        head.open_paren_tk = tz.is_next_char_token('(');

        head.read_modifiers(tz);

        // a receiver names a type, e.g. 'point' of 'func point.at()', which
        // 'read_method_name' checks
        if (tz.peek_char_after_whitespace() == '.') {
            toc::assert_valid_name(head.name_tk);
        } else {
            tc.assert_valid_unqualified_name(head.name_tk);
        }

        // e.g. 'func text.print()': each instance of 'text' reads the head
        // again from 'start_tk', where 'text' names a type, e.g. 'str', and
        // this returns false
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

    // the name after the '.' of a method, e.g. 'print' of 'func text.print()'
    [[nodiscard]] static auto read_method_name_token(tokenizer& tz) -> token {
        const token name_tk{tz.next_token()};

        if (not name_tk.is_word()) {
            throw compile_error{tz.position_of(name_tk),
                                "expected method name after '.'"};
        }

        toc::assert_valid_name(name_tk);

        return name_tk;
    }

  private:
    // e.g. the 'text' of 'func text.print()'; in an instance 'text' names a
    // type and is a receiver
    [[nodiscard]] auto names_generic_type(const toc& tc, tokenizer& tz) const
        -> bool {

        return open_paren_tk.is_empty() and
               tc.generics().has_type(name_tk.text()) and
               not tc.types().has(name_tk.text()) and
               tz.peek_char_after_whitespace() == '.';
    }

    // 'name_tk' is the receiver type, a method gets an implicit first
    // parameter 'self' of that type and a constructor builds a 'self' of it
    auto read_method_name(const toc& tc, tokenizer& tz) -> void {
        receiver_type = &tc.get_type_or_throw(name_tk);

        if (receiver_type->is_builtin()) {
            throw compile_error{
                name_tk, std::format("methods on built-in type '{}' are not "
                                     "supported",
                                     receiver_type->name())};
        }

        method_name_tk = read_method_name_token(tz);

        name =
            std::format("{}.{}", receiver_type->name(), method_name_tk.text());
    }

    // 'noinline' and 'mut' before the name; a word followed by '(' is read as
    // the name
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
                throw compile_error{
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

// the tokens of the result 'name [type]' after the parameters, before the type
// is resolved; read the same way by a definition and by the deduction of a
// generic function that has no instance yet
struct func_result_syntax {
    // e.g. 'res', or 'self' of a constructor, empty without a result
    token name_tk;
    // empty for the default type
    token type_tk;

    // e.g. 'func point.at(x, y) self'
    [[nodiscard]] auto is_self() const -> bool {
        return name_tk.is_text(reserved_names::self);
    }

    //
    // statics
    //

    // 'tz' is after the ')' of the parameters
    [[nodiscard]] static auto read(const toc& tc, tokenizer& tz)
        -> func_result_syntax {

        const token name_tk{tz.next_token()};

        if (not name_tk.is_word()) {
            tz.put_back_token(name_tk);
            return {};
        }

        token type_tk{tz.next_token()};

        // a word before the '{' of the body names the type, else a missing
        // '{' is reported by the body, e.g. 'func f() res number {'
        // note: the '{' also tells the type of a generic definition, where
        //       'T' of 'res T {' is not a type yet
        const bool is_type_before_body{
            type_tk.is_word() and tz.peek_char_after_whitespace() == '{',
        };

        if (not tc.types().has(type_tk.text()) and not is_type_before_body) {
            tz.put_back_token(type_tk);
            type_tk = {};
        }

        return {
            .name_tk{name_tk},
            .type_tk{type_tk},
        };
    }
};

// the result of a function, e.g. 'res i8': its name and its type
struct func_return_info {
    token type_tk;
    token ident_tk;
    const type* type_ptr{};
};

class stmt_def_func final : public statement {
    func_head head_;
    std::vector<stmt_def_func_param> params_;
    std::vector<token> param_delims_tk_;
    token close_paren_tk_;
    std::optional<func_return_info> returns_;
    stmt_block code_;
    // the definition parsed again with the parameters naming the arguments
    std::optional<func_instance> instance_;
    // what the body reaches that its callers name too, see 'body_uses'
    body_uses body_uses_;

    // how the body reaches the result and the parameters: a parsed body only
    // names them, a compiled noinline body has pointers to them in its frame
    enum class var_access : uint8_t { direct, through_pointer };

  public:
    // a function of the user code, or an instance of a generic definition,
    // e.g. 'tokenizer.to<name>' or 'str.print', see 'func_instantiator'.
    // 'tz' is after 'head'
    stmt_def_func(toc& tc, const token tk, const func_head& head, tokenizer& tz,
                  std::optional<func_instance> instance)
        : statement{tk}, head_{head}, instance_{std::move(instance)} {

        // note: a generic definition is a 'stmt_def_func_generic', a head
        //       with type parameters is read here only for an instance

        assert(not head.is_generic_type_method);
        assert(instance_ or head.type_param_tks.empty());

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

    // the body is compiled by its calls: in place by
    // 'inline_call::compile_body', else once per array lengths by
    // 'emit_noinline_body'
    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] const size_t indent,
                 [[maybe_unused]] const destination& dst) const
        -> void override {}

    //
    // class methods
    //

    // the constant arguments of the generic type of a method, in the frame of
    // the body
    auto add_constants(toc& tc, const size_t indent) const -> void {
        if (not instance_) {
            return;
        }

        for (const generic_arg& constant : instance_->constants) {
            tc.add_const(head_.name_tk, indent, constant.param_tk.text(),
                         folded_value::untyped(constant.value));
        }
    }

    // 'func' is a keyword so no user name or internal label starts with 'func.'
    // e.g. 'func.sum.len.4.8' for array lengths 4 and 8
    [[nodiscard]] auto
    body_label(const std::span<const size_t> array_lengths = {}) const
        -> std::string {

        return std::format("{}{}", machine::body_label_prefix,
                           name_with_lengths(array_lengths));
    }

    // for the errors of a function that no compile reached, compiled like a
    // noinline body and discarded; an unsized array parameter has no length
    // ('ident_info::is_length_unknown'). an error that depends on the
    // arguments of a call, e.g. an inlined recursion that a constant argument
    // ends, cannot be told and ends the check; a body that takes nothing from
    // a call, a noinline one without unsized arrays or an inlined one without
    // parameters and result, has no such error; an inlined result is the
    // destination of the call, checked here through an address
    auto check_body(toc& tc, const size_t indent) const -> void {
        const bool takes_from_call{
            array_param_count() != 0 or
                (is_inlined() and (not params().empty() or returns())),
        };

        tc.dry_run([&] -> void {
            machine& x{tc.machine()};

            const machine::call_frame_scope call_frame{
                x,
                token{},
                std::string{name()},
                machine::frame_kind::noinline_body,
            };

            try {
                std::ignore = compile_in_frame(tc, indent, {});
            } catch (compile_error& e) {
                if (not takes_from_call or not e.depends_on_call()) {
                    add_instance_frames(e);
                    throw;
                }
            }
        });
    }

    // in place or in a body of its own, the function is reached
    auto compile_statements(toc& tc, const size_t indent) const -> void {
        tc.mark_func_reached(*this);
        code_.compile(tc, indent, destination{});
    }

    // the body of a noinline function for one set of array lengths, after
    // 'main'; the lengths are those of the array arguments, one per array
    // parameter
    auto emit_noinline_body(toc& tc,
                            const std::span<const size_t> array_lengths) const
        -> void {

        assert(not is_inlined());
        assert(array_lengths.size() == array_param_count());

        machine& x{tc.machine()};

        const machine::call_frame_scope call_frame{
            x,
            token{},
            std::string{name()},
            machine::frame_kind::noinline_body,
        };

        x.begin_noinline_body(std::string{name()}, body_label(array_lengths));

        x.comment({}, 0, "");
        source_def_comment_to(x, 0);

        if (not array_lengths.empty()) {
            x.comment({}, 0, "array parameter lengths: {:n}", array_lengths);
        }

        x.label(0, body_label(array_lengths));

        x.reserve_frame_base();

        const size_t frame_size_bytes{compile_in_frame(tc, 0, array_lengths)};

        x.release_frame_base();

        x.define_constant(frame_size_label(array_lengths), frame_size_bytes);

        x.end_noinline_body();
    }

    // the slots of a call are the result, when there is one, then the
    // parameters
    [[nodiscard]] auto first_param_slot() const -> size_t {
        return returns_ ? size_t{1} : size_t{};
    }

    // a suffix could collide with a method body, e.g. 'func.list.size' of
    // function 'list' and method 'list.size'
    [[nodiscard]] auto
    frame_size_label(const std::span<const size_t> array_lengths) const
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

    // uses the definite-assignment walk for its reachability only
    [[nodiscard]] auto is_end_reachable() const -> bool {
        assignment_flow flow{std::nullopt, tok(), 0};

        code_.trace_assignment(flow);

        return flow.is_reachable();
    }

    [[nodiscard]] auto is_inlined() const -> bool {
        return head_.noinline_tk.is_empty();
    }

    // has the implicit first parameter 'self'
    [[nodiscard]] auto is_method() const -> bool {
        return not head_.method_dot_tk.is_empty() and not is_constructor();
    }

    // e.g. 'f', 'list.add', or the name of an instance, e.g.
    // 'tokenizer.to<name>'
    [[nodiscard]] auto name() const -> std::string_view {
        return instance_ ? instance_->name : head_.name;
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

        const std::string signature{statement::trimmed_source(ss.view())};

        // the signature of a generic definition names its parameters, the
        // instance names the arguments, e.g. 'pt.put<i16>'
        if (instance_) {
            x.comment(head_.name_tk, indent, "{}, instance '{}'", signature,
                      instance_->name);

            return;
        }

        x.comment(head_.name_tk, indent, "{}", signature);
    }

    // the definition after its first token, without the body
    auto source_signature_to(std::ostream& os) const -> void {
        head_.noinline_tk.source_to(os);
        head_.mut_tk.source_to(os);
        head_.name_tk.source_to(os);
        head_.method_dot_tk.source_to(os);
        head_.method_name_tk.source_to(os);
        head_.open_paren_tk.source_to(os);

        // the implicit 'self' has no source and no delimiter after it
        const size_t first_param{is_method() ? size_t{1} : size_t{0}};

        source_delimited_to(os, params_ | std::views::drop(first_param),
                            param_delims_tk_);

        close_paren_tk_.source_to(os);

        if (returns_) {
            returns_->ident_tk.source_to(os);
            returns_->type_tk.source_to(os);
        }
    }

    [[nodiscard]] auto uses() const -> const body_uses& { return body_uses_; }

  private:
    // the frames of what made the instance, see 'func_instance::frames'
    auto add_instance_frames(compile_error& e) const -> void {
        if (not instance_) {
            return;
        }

        for (const generic_frame& frame : instance_->frames) {
            e.add_call_frame(frame.src_loc_tk, frame.text, "instantiated by");
        }
    }

    // located at the method name for diagnostics
    auto add_self_param() -> void {
        const token self_tk{
            token::synthetic(head_.method_name_tk.file_index(),
                             reserved_names::self,
                             head_.method_name_tk.start_index(),
                             head_.method_name_tk.at_line()),
        };

        assert(head_.receiver_type != nullptr);

        params_.insert(params_.begin(),
                       stmt_def_func_param{self_tk, *head_.receiver_type,
                                           head_.mut_tk.is_empty()});
    }

    // a noinline body reaches the result and the arguments through pointer
    // slots in its frame. the array lengths are those of the body instance,
    // empty where no call gives them: in the parse and in 'check_body'. the
    // parameter of the slot at 'register_slot_index' is at 'address_register'
    // instead of the frame, when given
    auto add_signature_vars(toc& tc, const size_t indent,
                            const var_access access,
                            const std::span<const size_t> array_lengths,
                            const operand& address_register) const -> void {

        assert(array_lengths.empty() or
               array_lengths.size() == array_param_count());

        const bool is_pointer{access == var_access::through_pointer};

        size_t next_array_index{};

        if (returns_) {
            tc.add_var(returns_->ident_tk, indent,
                       {
                           .name{returns_->ident_tk.text()},
                           .type_ptr{&get_type()},
                           .src_loc_tk{returns_->ident_tk},
                           .is_pointer{is_pointer},
                           .array_len{},
                           .placement{},
                           .foo_array{},
                       },
                       var_kind::result);
        }

        for (const auto [param_index, param] : std::views::enumerate(params_)) {
            const std::optional<size_t> array_len{
                param.is_array() and not array_lengths.empty()
                    ? std::optional{array_lengths.at(next_array_index++)}
                    : std::nullopt,
            };

            // the register is the address of the variable, it has no slot in
            // the frame
            const bool is_in_register{
                not address_register.is_empty() and
                    register_slot_index() ==
                        first_param_slot() + static_cast<size_t>(param_index),
            };

            const at_pointer_register address_in_register{address_register};

            const var_placement placement{
                is_in_register ? var_placement{address_in_register}
                               : var_placement{},
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
                           .placement{placement},
                           .foo_array{},
                       },
                       var_kind::param);
        }
    }

    [[nodiscard]] auto array_param_count() const -> size_t {
        return static_cast<size_t>(
            std::ranges::count_if(params_, &stmt_def_func_param::is_array));
    }

    // no caller gives 'main' arguments or takes its result
    auto assert_main_signature() const -> void {
        if (name() != reserved_names::main) {
            return;
        }

        if (not is_inlined()) {
            throw compile_error{head_.noinline_tk,
                                "'main' cannot be declared 'noinline'"};
        }

        if (not params().empty()) {
            throw compile_error{params().front().tok(),
                                "'main' cannot have parameters"};
        }

        if (returns()) {
            throw compile_error{returns()->ident_tk,
                                "'main' cannot have a result"};
        }
    }

    // e.g. 'func mut point.move(dx)', a constructor writes its 'self' anyway
    auto assert_mut_has_receiver() const -> void {
        if (head_.mut_tk.is_empty() or is_method()) {
            return;
        }

        if (is_constructor()) {
            throw compile_error{
                head_.mut_tk,
                "a constructor builds 'self', 'mut' is not needed"};
        }

        throw compile_error{head_.mut_tk,
                            "'mut' requires a method 'type.name'"};
    }

    // the result is declared before the parameters, slot 0 is the result,
    // so the duplicate is found here at the result
    auto assert_result_not_named_like_param() const -> void {
        if (not returns_) {
            return;
        }

        for (const stmt_def_func_param& param : params_) {
            if (param.name() == returns_->ident_tk.text()) {
                throw compile_error{
                    returns_->ident_tk,
                    std::format("result '{}' is named like a parameter",
                                returns_->ident_tk.text()),
                };
            }
        }
    }

    // every path through the body sets the result, also the end, which
    // returns like a 'return' statement
    auto assert_sets_result() const -> void {
        if (not returns_) {
            return;
        }

        assignment_flow flow{
            returns_->ident_tk.text(),
            returns_->ident_tk,
            returns_->type_ptr->size_bytes(),
        };

        code_.trace_assignment(flow);
        flow.assert_set_at_return();
    }

    // keeps the labels of the body unique: the instances of one generic
    // definition have the same source locations, and a noinline body is
    // compiled once per array lengths
    [[nodiscard]] auto
    body_call_path(const std::span<const size_t> array_lengths) const
        -> std::string {

        if (instance_) {
            return name_with_lengths(array_lengths);
        }

        if (array_lengths.empty()) {
            return {};
        }

        return array_lengths_path(array_lengths);
    }

    // the body in the frame of the function; returns the size of the frame
    [[nodiscard]] auto
    compile_frame_body(toc& tc, const size_t indent,
                       const std::span<const size_t> array_lengths,
                       const operand& slot_register) const -> size_t {

        machine& x{tc.machine()};

        const toc::frame_scope body{
            tc,
            frame::noinline_func(name(), body_call_path(array_lengths),
                                 x.frame_base_register()),
        };

        add_constants(tc, indent + 1);

        add_signature_vars(tc, indent + 1, var_access::through_pointer,
                           array_lengths, slot_register);

        compile_statements(tc, indent);
        x.return_function(indent + 1);

        return tc.storage().peak_frame_size_bytes();
    }

    // the body in a frame of its own at the frame base, which the caller
    // holds; returns the size of the frame
    [[nodiscard]] auto
    compile_in_frame(toc& tc, const size_t indent,
                     const std::span<const size_t> array_lengths) const
        -> size_t {

        machine& x{tc.machine()};

        // the register is held while the body runs
        operand slot_register;

        if (has_slot_register(tc)) {
            slot_register = x.alloc_named_register(
                tok(), indent + 1, x.slot_register(), tc.builtins().address());
        }

        const size_t frame_size_bytes{
            compile_frame_body(tc, indent, array_lengths, slot_register),
        };

        if (not slot_register.is_empty()) {
            x.free_named_register(tok(), indent + 1, slot_register);
        }

        return frame_size_bytes;
    }

    // e.g. 'sum.len.4.8' for array lengths 4 and 8, 'sum' without lengths
    [[nodiscard]] auto
    name_with_lengths(const std::span<const size_t> array_lengths) const
        -> std::string {

        if (array_lengths.empty()) {
            return label_name(name());
        }

        return std::format("{}.{}", label_name(name()),
                           array_lengths_path(array_lengths));
    }

    // the function scope is established before the body is parsed, its
    // variables are registered without emitting output
    auto parse_body(toc& tc, tokenizer& tz) -> void {
        const toc::frame_scope body{
            tc,
            is_inlined() ? frame::inlined_func(name(), {}, {})
                         : frame::noinline_func(name(), {}, {}),
        };

        add_constants(tc, 0);

        // note: 'parse_returns' only creates a return with a name
        assert(not returns_ or returns_->ident_tk.is_word());

        add_signature_vars(tc, 0, var_access::direct, {}, {});

        code_ = {tc, tz, block_braces::required};

        // checked here so that the error of an instance gets its frames, a
        // signature that 'main' cannot have first
        assert_main_signature();
        assert_sets_result();

        body_uses_ = tc.scopes().current_func_frame().body_uses_of_func();
    }

    // e.g. 'a i32, b i32' up to the closing parenthesis
    auto parse_params(toc& tc, tokenizer& tz) -> void {
        stmt_def_func_param::syntax_list list{
            stmt_def_func_param::syntax_list::read(tz),
        };

        for (const stmt_def_func_param::syntax& param : list.params) {
            params_.emplace_back(tc, param);
        }

        param_delims_tk_ = std::move(list.delimiter_tks);
        close_paren_tk_ = list.close_paren_tk;
    }

    // 'name [type]' of the returned value, without a name the type is void
    auto parse_returns(toc& tc, tokenizer& tz) -> void {
        const func_result_syntax result{func_result_syntax::read(tc, tz)};

        if (result.name_tk.is_empty()) {
            set_type(tc.builtins().void_type());
            return;
        }

        // a name before the body is the result, 'self' names the value a
        // constructor builds
        if (tz.peek_char_after_whitespace() == '{' and not result.is_self()) {
            tc.assert_valid_unqualified_name(result.name_tk);
        }

        if (result.is_self()) {
            set_constructor_result(result.name_tk, result.type_tk);
            return;
        }

        const type& tp{
            result.type_tk.is_empty() ? tc.builtins().default_type()
                                      : tc.get_type_or_throw(result.type_tk),
        };

        returns_.emplace(result.type_tk, result.name_tk, &tp);
        set_type(tp);
    }

    // '(parameters) [result]', then the calls after it know the function
    auto parse_signature(toc& tc, tokenizer& tz) -> void {
        if (head_.open_paren_tk.is_empty()) {
            throw compile_error{tz, "expected '(' after function name"};
        }

        parse_params(tc, tz);

        parse_returns(tc, tz);

        assert_result_not_named_like_param();

        assert_mut_has_receiver();

        // known only after the result: a constructor builds 'self' instead
        if (is_method()) {
            add_self_param();
        }

        // e.g. 'text.size' for the instance 'str.size', empty if the function
        // is not an instance of a method of a generic type
        const std::string generic_method{
            instance_ ? instance_->generic_method : std::string{},
        };

        // see 'definition_order'
        const bool is_ordered{not instance_ and head_.method_dot_tk.is_empty()};

        tc.add_func(head_.name_tk, std::string{name()}, statement::get_type(),
                    *this, generic_method, is_ordered);
    }

    // e.g. 'func point.at(x, y) self' builds a 'point'
    auto set_constructor_result(const token& self_tk, const token& type_tk)
        -> void {

        if (head_.method_dot_tk.is_empty()) {
            throw compile_error{
                self_tk, "result 'self' requires a constructor 'type.name'"};
        }

        // one spelling, the type is already named before the '.'
        if (not type_tk.is_empty()) {
            throw compile_error{
                type_tk, "result 'self' has the type of the constructor"};
        }

        assert(head_.receiver_type != nullptr);

        const type& tp{*head_.receiver_type};

        returns_.emplace(type_tk, self_tk, &tp);
        set_type(tp);
    }

    //
    // statics
    //

    // e.g. 'len.4.8', unlike the 'L.C' pairs of inlined calls
    [[nodiscard]] static auto
    array_lengths_path(const std::span<const size_t> array_lengths)
        -> std::string {

        std::string path{"len"};

        for (const size_t length : array_lengths) {
            path += std::format(".{}", length);
        }

        return path;
    }

    // a label has no '<', '>' or ',', e.g. 'tokenizer.to<name>' is
    // 'tokenizer.to..name'; a name has no '..', so 'f<a,b>' ('f..a.b') and
    // the method instance 'f.a<b>' ('f.a..b') differ
    [[nodiscard]] static auto label_name(const std::string_view name)
        -> std::string {

        std::string label;

        for (const char ch : name) {
            if (ch == '<') {
                label += "..";
            } else if (ch == ',') {
                label.push_back('.');
            } else if (ch != '>') {
                label.push_back(ch);
            }
        }

        return label;
    }
};

// the generic functions and methods of the program: a definition is recorded
// in the 'generic_registry' and its instances are its text parsed again, see
// 'generics.hpp'
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
//
// the steps:
//
//   1. a definition: 'define_func' records a generic function with how a call
//      deduces its types ('read_signature_types', 'deductions'),
//      'define_method' records a method and makes it for the instances of the
//      type made so far
//   2. an alias: 'instantiate_methods' makes each method for the new instance
//      ('instantiate_method', 'parse_method_instance'), a method with type
//      parameters of its own goes to 'define_func'
//   3. a call: 'instantiate_func' makes the instance for the type arguments
//      once ('parse_func_instance')
class func_instantiator final {
    // the types a signature names, as written
    struct signature_param {
        token name_tk;
        // empty for an array
        std::string type_name;
    };

    struct signature_types {
        std::vector<signature_param> parameters;

        func_result_syntax result;
    };

  public:
    //
    // statics
    //

    // records a generic function for its calls, e.g. 'func f<T type>(x T)'; a
    // method of an instance of a generic type keeps that instance, e.g.
    // 'str.append' of 'func text.append<T type>(s T)' keeps 'text<8>'. 'tz'
    // is after the '(' of 'head'; 'definition' is the last one the body sees,
    // that of the generic method for a method of a generic type
    static auto define_func(toc& tc, const token& func_tk,
                            const func_head& head, const tokenizer& tz,
                            std::optional<generic_type_instance> receiver,
                            const size_t definition) -> void {

        for (const token& param_tk : head.type_param_tks) {
            tc.assert_generic_param_free(param_tk);
        }

        // e.g. 'text.append' for the instance 'str.append' of 'text'
        std::string report_name{
            receiver ? receiver->method_name(head.method_name_tk.text())
                     : head.name,
        };

        // note: the definition checked that the '(' follows
        assert(not head.open_paren_tk.is_empty());

        const signature_types types{read_signature_types(tc, tz)};

        // a constructor is called as 'point.at(...)', there is no place for
        // type arguments
        if (types.result.is_self()) {
            throw compile_error{
                types.result.name_tk,
                std::format("constructor '{}' cannot have type parameters",
                            report_name)};
        }

        tc.add_generic_func(head.name,
                            {
                                .src_loc_tk{head.name_tk},
                                .func_tk{func_tk},
                                .start_tk{head.start_tk},
                                .report_name{std::move(report_name)},
                                .param_tks{head.type_param_tks},
                                .deductions{deductions(head, types)},
                                .receiver{std::move(receiver)},
                                .definition{definition},
                                .is_method{not head.method_dot_tk.is_empty()},
                            });
    }

    // e.g. 'func text.print()' of the generic type 'text': every instance of
    // the type gets the method, also those made after this definition
    static auto define_method(toc& tc, const token& func_tk,
                              const func_head& head,
                              const token& method_name_tk) -> void {

        const generic_method_info method{
            .type_tk{head.name_tk},
            .method_name_tk{method_name_tk},
            .func_tk{func_tk},
            .start_tk{head.start_tk},
            .definition{tc.current_definition()},
        };

        tc.add_generic_method(method);

        for (const generic_type_instance& instance :
             tc.generics().get_type_instances(head.name_tk.text())) {

            instantiate_method(tc, method, instance);
        }
    }

    // the instance of the generic function for the type arguments, e.g.
    // 'tokenizer.to<name>' of 'func tokenizer.to<T type>()', made at the first
    // call that needs it, returns its name
    [[nodiscard]] static auto instantiate_func(
        toc& tc, const token& call_tk, const std::string_view generic_name,
        const std::span<const type* const> type_args) -> std::string {

        // note: the reference stays valid while the instance is parsed, a body
        //       defines no generic function

        const generic_func_info& generic{tc.generics().get_func(generic_name)};

        // note: the call checked the count of written type arguments, a
        //       deduction gives one per parameter

        assert(type_args.size() == generic.param_tks.size());

        // the generic functions that no call instantiates are listed in the
        // usage report
        tc.mark_generic_func_instantiated(generic_name);

        std::string name{
            generic_func_instance_name(generic_name, type_args),
        };

        if (tc.is_func(name)) {
            return name;
        }

        // an error in the instance reports the call that needed it, like the
        // frames of a generic type instance
        const generic_frame call_frame{
            .src_loc_tk{call_tk},
            .text{name},
        };

        try {
            parse_func_instance(tc, generic, name, type_args, call_frame);
        } catch (compile_error& e) {
            e.add_call_frame(call_tk, name, "instantiated by");
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
    //
    // statics
    //

    [[nodiscard]] static auto deduction_of(const std::string_view param_name,
                                           const signature_types& types)
        -> generic_deduction {

        const auto it{
            std::ranges::find(types.parameters, param_name,
                              &signature_param::type_name),
        };

        std::optional<size_t> param_index;
        std::string param_name_text;

        if (it != types.parameters.end()) {
            param_index = static_cast<size_t>(it - types.parameters.begin());
            param_name_text = it->name_tk.text();
        }

        return {
            .param_index{param_index},
            .param_name{std::move(param_name_text)},
            .is_result{types.result.type_tk.is_text(param_name)},
        };
    }

    // how a call can tell each type parameter: the first parameter declared
    // with exactly its type, e.g. 's T' in 'func text.append<T type>(s T)',
    // and the result, e.g. 'res T'
    [[nodiscard]] static auto deductions(const func_head& head,
                                         const signature_types& types)
        -> std::vector<generic_deduction> {

        std::vector<generic_deduction> result;
        result.reserve(head.type_param_tks.size());

        for (const token& param_tk : head.type_param_tks) {
            result.push_back(deduction_of(param_tk.text(), types));
        }

        return result;
    }

    // an error in the method is in the generic code, the frames name the bases
    // and the alias
    static auto instantiate_method(toc& tc, const generic_method_info& method,
                                   const generic_type_instance& receiver)
        -> void {

        try {
            parse_method_instance(tc, method, receiver);
        } catch (compile_error& e) {
            receiver.add_frames_to(e);
            throw;
        }
    }

    // e.g. 'tokenizer.to<name>' of 'func tokenizer.to<T type>()', 'T' names
    // 'name' while the definition is parsed again
    static auto
    parse_func_instance(toc& tc, const generic_func_info& generic,
                        const std::string& name,
                        const std::span<const type* const> type_args,
                        const generic_frame& call_frame) -> void {

        generic_scope scope{tc, generic.definition};

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

        tokenizer tz{tc.source_of(generic.start_tk), generic.start_tk};

        // note: the instance is parsed inside the parse that needs it
        const toc::parse_nesting_scope nesting{tc, tz};

        const func_head head{func_head::read(tc, tz)};

        func_instance instance{
            .name{name},
            .generic_method{},
            .constants{},
            .frames{call_frame},
        };

        // e.g. 'str.append<i8>' keeps 'text.append' and the constants of
        // 'text<8>', like the instance of a method without type parameters
        if (generic.receiver) {
            instance.generic_method =
                generic.receiver->method_name(head.method_name_tk.text());

            instance.constants = generic.receiver->constants();
        }

        tc.add_func_instance(std::make_shared<stmt_def_func>(
            tc, generic.func_tk, head, tz, std::move(instance)));
    }

    // e.g. 'str.print' of 'func text.print()', 'text' names 'str' while the
    // definition is parsed again; a method with type parameters of its own is
    // a generic function of the instance, e.g. 'str.append' of
    // 'func text.append<T type>(s T)'
    static auto parse_method_instance(toc& tc,
                                      const generic_method_info& method,
                                      const generic_type_instance& receiver)
        -> void {

        generic_scope scope{tc, method.definition};
        scope.add_receiver(receiver);

        tokenizer tz{tc.source_of(method.start_tk), method.start_tk};

        // note: the instance is parsed inside the parse that needs it
        const toc::parse_nesting_scope nesting{tc, tz};

        // note: 'text' names 'str' now, so the head is the method 'str.print'
        const func_head head{func_head::read(tc, tz)};

        assert(not head.is_generic_type_method);

        if (not head.type_param_tks.empty()) {
            define_func(tc, method.func_tk, head, tz, receiver,
                        method.definition);

            return;
        }

        func_instance instance{
            .name{head.name},
            .generic_method{receiver.method_name(head.method_name_tk.text())},
            .constants{receiver.constants()},
            .frames{receiver.frames},
        };

        tc.add_func_instance(std::make_shared<stmt_def_func>(
            tc, method.func_tk, head, tz, std::move(instance)));
    }

    // the same reading as the signature of an instance, without resolving the
    // types, e.g. '(s T, n int) res T' gives the parameter types 'T' and 'int'
    // and the result type 'T'. 'tz' is after the '(' and is a copy, the caller
    // does not move
    [[nodiscard]] static auto read_signature_types(const toc& tc, tokenizer tz)
        -> signature_types {

        signature_types types;

        const stmt_def_func_param::syntax_list list{
            stmt_def_func_param::syntax_list::read(tz),
        };

        for (const stmt_def_func_param::syntax& param : list.params) {
            // an array is not the type itself
            types.parameters.push_back({
                .name_tk{param.name_tk},
                .type_name{param.is_array() ? "" : param.type_tk.text()},
            });
        }

        types.result = func_result_syntax::read(tc, tz);

        return types;
    }
};

// a generic definition, kept as text for its instances, which
// 'func_instantiator' makes
class stmt_def_func_generic final : public statement {
    // the text after 'func', errors in the types of its signature and in its
    // body are found when an instance is parsed
    std::string_view generic_text_;

  public:
    // 'tz' is after the head
    stmt_def_func_generic(toc& tc, const token tk, const func_head& head,
                          tokenizer& tz)
        : statement{tk} {

        // note: the signature is checked before the definition, which makes
        //       the instances of the types already defined, e.g. 'str.print'

        if (head.is_generic_type_method) {
            const token method_name_tk{skip_signature(tc, head, tz)};
            func_instantiator::define_method(tc, tk, head, method_name_tk);
        } else {
            // e.g. 'func f<T type> {'
            if (head.open_paren_tk.is_empty()) {
                throw compile_error{tz, "expected '(' after function name"};
            }

            func_instantiator::define_func(tc, tk, head, tz, std::nullopt,
                                           tc.current_definition());

            std::ignore = skip_signature(tc, head, tz);
        }

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
                 [[maybe_unused]] const destination& dst) const
        -> void override {}

  private:
    //
    // statics
    //

    // the '(' and the body must follow, else the skip of the body would take
    // the next definition, e.g. 'func g' after 'func f<T type>(x T)'; the
    // parameters are read for their syntax, the types are resolved when an
    // instance is parsed; 'tz' is after the head; returns the name of a method
    // of a generic type, e.g. 'print' of 'func text.print()', empty for a
    // generic function
    [[nodiscard]] static auto skip_signature(const toc& tc,
                                             const func_head& head,
                                             tokenizer& tz) -> token {

        token method_name_tk;

        // e.g. the '.print(' of 'func text.print()' or the '.to<T type>(' of
        // 'func text.to<T type>()'
        if (head.is_generic_type_method) {
            const token dot_tk{tz.is_next_char_token('.')};

            // note: 'func_head::names_generic_type' saw the '.'
            assert(not dot_tk.is_empty());

            method_name_tk = func_head::read_method_name_token(tz);

            // e.g. 'n' of 'func box.f<n type>()' of 'type box<n>'
            if (tz.peek_char_after_whitespace() == '<') {
                for (const generic_param& param : generic_param::parse(tz)) {
                    tc.assert_method_type_param_free(param.name_tk,
                                                     head.name_tk.text());
                }
            }

            std::ignore =
                tz.expect_char_token('(', "expected '(' after function name");
        }

        std::ignore = stmt_def_func_param::syntax_list::read(tz);
        std::ignore = func_result_syntax::read(tc, tz);

        const token open_brace_tk{
            tz.expect_char_token('{', "expected '{' to begin block"),
        };

        tz.put_back_token(open_brace_tk);

        return method_name_tk;
    }
};

// reads the head and makes the statement of the kind that follows it
inline auto make_stmt_def_func(toc& tc, const token tk, tokenizer& tz)
    -> std::unique_ptr<statement> {

    const func_head head{func_head::read(tc, tz)};

    // no call gives 'main' type arguments, the rest of its signature is
    // checked by 'stmt_def_func::assert_main_signature'
    if (head.name == reserved_names::main and not head.type_param_tks.empty()) {
        throw compile_error{head.type_param_tks.front(),
                            "'main' cannot have type parameters"};
    }

    // e.g. 'func text.print()' or 'func tokenizer.to<T type>()'
    if (head.is_generic()) {
        return std::make_unique<stmt_def_func_generic>(tc, tk, head, tz);
    }

    // e.g. 'func f(x)' or 'func list.add(x)'
    return std::make_unique<stmt_def_func>(tc, tk, head, tz, std::nullopt);
}
