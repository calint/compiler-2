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
#include <variant>
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
//   'func_result'            the result 'name [type]' as written
//   'stmt_def_func'          a function, of the user code or an instance
//   'func_instantiator'      records the generic definitions and makes their
//                            instances, each a 'stmt_def_func'
//   'stmt_def_func_generic'  a generic definition, kept as text
//
// the formatter sorts the functions of a class by name, so the comment above
// 'func_instantiator' lists its functions in the order they run
//

// the head of a function definition, 'func [noinline] [mut] name[.method]
// [<T type>] (', read the same way for every kind of definition, the kind
// follows from it: the receiver, resolved at its '.', and the type parameters
//
// e.g. 'func f(x)' and 'func list.add(x)' are functions, 'func f<T type>(x T)'
// is a generic function and 'func text.print()' a method of the generic type
// 'text'
//
// an instance reads the head again where the parameters name the arguments,
// e.g. 'text' names 'str' and 'func text.print()' is the function 'str.print'
struct func_head {
  private:
    // what 'name_tk' names before the '.' of a method
    struct receiver_none {};

    struct receiver_of_type {
        const type* tp{};
    };

    // e.g. 'text' of 'func text.print()', each instance of 'text' reads the
    // head again from 'start_tk', where 'text' names a type
    struct receiver_of_generic_type {};

  public:
    // where the definition continues after 'func', an instance starts here
    token start_tk;
    std::optional<token> noinline_tk;
    std::optional<token> mut_tk;
    // e.g. 'f', or the receiver type of a method, e.g. 'list' of 'list.add'
    token name_tk;
    // none for a function
    std::optional<token> method_dot_tk;
    std::optional<token> method_name_tk;

    std::variant<receiver_none, receiver_of_type, receiver_of_generic_type>
        receiver;

    // e.g. 'f' or 'list.add', the receiver named as the type it is bound to,
    // e.g. 'str.print' for 'text.print' when 'text' names 'str'
    std::string name;
    // e.g. 'T' of '<T type>'
    std::vector<token> type_param_tks;
    // none until it is read, e.g. after 'list.add' of 'func list.add('
    std::optional<token> open_paren_tk;

    // a constructor builds an instance of its receiver type, e.g. 'func
    // point.at(x, y) self'
    auto assert_receiver_for_self(const token& self_tk) const -> void {
        if (is_type_member()) {
            return;
        }

        throw compile_error{self_tk,
                            "result 'self' requires a constructor 'type.name'"};
    }

    // kept as text, its instances are the functions
    [[nodiscard]] auto is_generic() const -> bool {
        return is_generic_type_method() or not type_param_tks.empty();
    }

    // e.g. 'func text.print()' of the generic type 'text'
    [[nodiscard]] auto is_generic_type_method() const -> bool {
        return std::holds_alternative<receiver_of_generic_type>(receiver);
    }

    // reached through its type wherever it is defined, e.g. 'list.add' or
    // the constructor 'point.at'
    [[nodiscard]] auto is_type_member() const -> bool {
        return method_dot_tk.has_value();
    }

    // the type of 'name_tk' of a method of a type, null otherwise
    [[nodiscard]] auto receiver_type() const -> const type* {
        const receiver_of_type* const of_type{
            std::get_if<receiver_of_type>(&receiver),
        };

        return of_type == nullptr ? nullptr : of_type->tp;
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

        head.read_receiver(tc, tz);

        // e.g. 'func tokenizer.to<T type>()' or 'func text.to<T type>()'
        if (not head.open_paren_tk and tz.peek_char_after_whitespace() == '<') {

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

    // e.g. '<T type, U type>', a generic function has type parameters only
    [[nodiscard]] static auto read_type_params(tokenizer& tz)
        -> std::vector<token> {

        std::vector<token> param_tks;

        for (const generic_param& param : generic_param::parse(tz)) {
            if (not param.is_type) {
                throw compile_error{
                    param.name_tk,
                    std::format("generic parameter {} of a function must "
                                "have the kind 'type', e.g. '{} type'",
                                in_quotes(param.name_tk.text()),
                                param.name_tk.text())};
            }

            param_tks.emplace_back(param.name_tk);
        }

        return param_tks;
    }

  private:
    // 'name_tk' is the receiver: a type, whose method gets an implicit first
    // parameter 'self' and whose constructor builds a 'self', or a generic
    // type, whose instances get the method; the name of a method of a type
    // is that of the type it names, e.g. 'str.print' where 'text' names 'str'
    auto read_method_name(const toc& tc, tokenizer& tz) -> void {
        // e.g. the 'text' of 'func text.print()'; in an instance 'text' names
        // a type
        if (tc.generics().has_type(name_tk.text()) and
            not tc.types().has(name_tk.text())) {

            receiver = receiver_of_generic_type{};

            const token read_name_tk{read_method_name_token(tz)};

            method_name_tk = read_name_tk;
            name = method_name_of(name_tk.text(), read_name_tk.text());

            return;
        }

        const type& receiver_tp{tc.get_type_or_throw(name_tk)};

        if (receiver_tp.is_builtin()) {
            throw compile_error{
                name_tk, std::format("methods on built-in type {} are not "
                                     "supported",
                                     in_quotes(receiver_tp.name()))};
        }

        receiver = receiver_of_type{
            .tp{&receiver_tp},
        };

        const token read_name_tk{read_method_name_token(tz)};

        method_name_tk = read_name_tk;
        name = method_name_of(receiver_tp.name(), read_name_tk.text());
    }

    // 'noinline' and 'mut' before the name; a word followed by '(' is read as
    // the name
    auto read_modifiers(tokenizer& tz) -> void {
        if (name_tk.is_text("noinline") and not open_paren_tk) {
            noinline_tk = name_tk;
            name_tk = tz.next_token();
            open_paren_tk = tz.is_next_char_token('(');
        }

        if (name_tk.is_text("mut") and not open_paren_tk) {
            mut_tk = name_tk;
            name_tk = tz.next_token();
            open_paren_tk = tz.is_next_char_token('(');
        }

        name = name_tk.text();
    }

    // e.g. 'func list.add(x)'
    auto read_receiver(const toc& tc, tokenizer& tz) -> void {
        if (open_paren_tk) {
            return;
        }

        method_dot_tk = tz.is_next_char_token('.');

        if (not method_dot_tk) {
            return;
        }

        read_method_name(tc, tz);
        open_paren_tk = tz.is_next_char_token('(');
    }

    //
    // statics
    //
};

// the result 'name [type]' as written after the parameters, e.g. 'res i8';
// its type is the node's ('stmt_def_func::result_type'); a generic definition
// keeps it in its 'generic_signature'
struct func_result {
    // e.g. 'res', or 'self' of a constructor
    token name_tk;
    // none for the default type
    std::optional<token> type_tk;

    // e.g. 'func point.at(x, y) self'
    [[nodiscard]] auto is_self() const -> bool {
        return name_tk.is_text(reserved_names::self);
    }

    //
    // statics
    //

    // 'tz' is after the ')' of the parameters; none without a result
    [[nodiscard]] static auto read(const toc& tc, tokenizer& tz)
        -> std::optional<func_result> {

        const token name_tk{tz.next_token()};

        if (not name_tk.is_word()) {
            tz.put_back_token(name_tk);
            return std::nullopt;
        }

        const token type_tk{tz.next_token()};

        // a word before the '{' of the body names the type, else a missing
        // '{' is reported by the body, e.g. 'func f() res number {'
        // note: the '{' also tells the type of a generic definition, where
        //       'T' of 'res T {' is not a type yet
        const bool is_type_before_body{
            type_tk.is_word() and tz.peek_char_after_whitespace() == '{',
        };

        if (not tc.types().has(type_tk.text()) and not is_type_before_body) {
            tz.put_back_token(type_tk);

            return func_result{
                .name_tk{name_tk},
                .type_tk{std::nullopt},
            };
        }

        return func_result{
            .name_tk{name_tk},
            .type_tk{type_tk},
        };
    }
};

// a slot of the frame of a noinline call: the address of the result, or of
// the argument of the parameter at 'param_index'; the address of a receiver
// may be passed in a register instead
struct frame_slot {
    std::optional<size_t> param_index;
    bool is_in_register{};
};

// a function: its signature, its body and the checks of the body; 'compile'
// emits nothing, the body is compiled by its calls; the four ways a body is
// compiled:
//
//   when                  scope              the parameters
//   'parse_body'          'inlined_func' or  variables, named only
//                         'noinline_func'
//   an inlined call       'inlined_func'     bindings to the arguments
//   ('inline_call')
//   the dry run of a      'noinline_dry_run' bindings to the arguments
//   noinline call
//   a noinline body,      'noinline_func'    variables reached through the
//   'emit_noinline_body'                     addresses in the frame
//   and 'check_body'                         ('compile_in_frame')
//
// 'check_body' compiles a function that nothing compiled like a noinline
// body; 'toc' keeps the address of the definition, so it is not
// copied or moved
class stmt_def_func final : public statement {
    func_head head_;
    std::vector<stmt_def_func_param> params_;
    std::vector<token> param_delims_tk_;
    token close_paren_tk_;
    std::optional<func_result> result_;
    stmt_block code_;
    // the definition parsed again with the parameters naming the arguments
    std::optional<func_instance> instance_;
    // what the body reaches that its callers name too, see 'body_uses'
    body_uses body_uses_;

    // how the body reaches the result and the parameters: a parsed body only
    // names them, a compiled noinline body has their addresses in its frame
    enum class var_access : uint8_t { direct, through_address };

  public:
    // a function of the user code, or an instance of a generic definition,
    // e.g. 'tokenizer.to<name>' or 'str.print', see 'func_instantiator'.
    // 'tz' is after 'head'
    stmt_def_func(toc& tc, const token tk, const func_head& head, tokenizer& tz,
                  std::optional<func_instance> instance)
        : statement{tk}, head_{head}, instance_{std::move(instance)} {

        // note: a generic definition is a 'stmt_def_func_generic', a head
        //       with type parameters is read here only for an instance

        assert(not head.is_generic_type_method());
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

    // the constant arguments of the generic type of a method, in the scope of
    // the body
    auto add_constants(toc& tc, const size_t indent) const -> void {
        if (not instance_) {
            return;
        }

        for (const generic_arg& constant : instance_->constants) {
            tc.add_const(head_.name_tk, indent, constant.param_tk.text(),
                         folded_value::untyped(constant.constant()));
        }
    }

    // the definite-assignment walk without a variable, see
    // 'stmt_block::trace_assignment'
    auto assert_statements_reached() const -> void {
        assignment_flow flow{assignment_flow::reachability_only(tok())};
        code_.trace_assignment(flow);
    }

    // 'func' is a keyword so no user name or internal label starts with 'func.'
    // e.g. 'func.sum.len.4.8' for array lengths 4 and 8
    [[nodiscard]] auto
    body_label(const std::span<const size_t> array_lengths = {}) const
        -> std::string {

        return std::format("{}{}", machine::body_label_prefix,
                           name_with_lengths(array_lengths));
    }

    // for the errors of a function that nothing compiled, compiled like a
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
                (is_inlined() and (not params().empty() or has_result())),
        };

        std::optional<compile_error> error{
            tc.dry_run([&] -> void {
                const toc::scope_entry body{tc, frame_scope(tc, {})};

                std::ignore = compile_in_frame(tc, indent, {});
            }),
        };

        if (not error or (takes_from_call and error->depends_on_call())) {
            return;
        }

        add_instance_trace(*error);
        toc::throw_dry_run_error(std::move(*error));
    }

    // in place or in a body of its own, the function is compiled
    auto compile_statements(toc& tc, const size_t indent) const -> void {
        tc.mark_func_compiled(*this);
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

        const toc::scope_entry body{tc, frame_scope(tc, array_lengths)};

        machine& x{tc.machine()};

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

    // a suffix could collide with a method body, e.g. 'func.list.size' of
    // function 'list' and method 'list.size'
    [[nodiscard]] auto
    frame_size_label(const std::span<const size_t> array_lengths) const
        -> std::string {

        return std::format("size.{}", body_label(array_lengths));
    }

    // the slots of a noinline call, each the address of what it names: the
    // result, when there is one, then the parameters
    [[nodiscard]] auto frame_slots() const -> std::vector<frame_slot> {
        std::vector<frame_slot> slots;

        if (result_) {
            slots.push_back({
                .param_index{},
                .is_in_register{},
            });
        }

        for (size_t i{}; i < params_.size(); ++i) {
            slots.push_back({
                .param_index{i},
                .is_in_register{i == 0 and has_receiver_in_register()},
            });
        }

        return slots;
    }

    // the receiver of a method has its address in the slot register of the
    // backend, not in a frame slot
    [[nodiscard]] auto has_receiver_in_register() const -> bool {
        return has_self_param();
    }

    [[nodiscard]] auto has_result() const -> bool {
        return result_.has_value();
    }

    // a method of a type, a constructor builds its 'self' instead
    [[nodiscard]] auto has_self_param() const -> bool {
        return head_.method_dot_tk and not is_constructor();
    }

    // e.g. 'func point.at(x, y) self'
    [[nodiscard]] auto is_constructor() const -> bool {
        return result_ and result_->is_self();
    }

    [[nodiscard]] auto is_inlined() const -> bool {
        return not head_.noinline_tk;
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

    [[nodiscard]] auto result() const -> const std::optional<func_result>& {
        return result_;
    }

    // the type of the result, 'void' without one; the node's type
    [[nodiscard]] auto result_type() const -> const type& {
        return statement::get_type();
    }

    auto source_def_comment_to(machine& x, const size_t indent) const -> void {
        std::stringstream ss;
        source_signature_to(ss);

        const std::string signature{statement::trimmed_source(ss.view())};

        // the signature of a generic definition names its parameters, the
        // instance names the arguments, e.g. 'pt.put<i16>'
        if (instance_) {
            x.comment(head_.name_tk, indent, "{}, instance {}", signature,
                      in_quotes(instance_->name));

            return;
        }

        x.comment(head_.name_tk, indent, "{}", signature);
    }

    [[nodiscard]] auto uses() const -> const body_uses& { return body_uses_; }

    //
    // statics
    //

    // a parsed body names its result and parameters in place, a compiled
    // noinline body reaches each through the address in its frame slot, the
    // receiver through the address in 'slot_register'
    [[nodiscard]] static auto placement_of(const var_access access,
                                           const frame_slot& slot,
                                           const operand& slot_register)
        -> var_placement {

        if (access == var_access::direct) {
            return in_storage{
                .offset{},
                .base_register{},
                .holds_address{},
            };
        }

        if (slot.is_in_register) {
            assert(not slot_register.is_empty());

            return at_address_register{slot_register};
        }

        return in_storage{
            .offset{},
            .base_register{},
            .holds_address{true},
        };
    }

  private:
    // the frames of what made the instance, see 'func_instance::trace'
    auto add_instance_trace(compile_error& e) const -> void {
        if (instance_) {
            instance_->trace.add_to(e);
        }
    }

    // located at the method name for diagnostics
    auto add_self_param() -> void {
        // note: a method has its name after the '.'
        assert(head_.method_name_tk);

        const token self_tk{
            token::synthetic(head_.method_name_tk->file_index(),
                             reserved_names::self,
                             head_.method_name_tk->start_index(),
                             head_.method_name_tk->at_line()),
        };

        assert(head_.receiver_type() != nullptr);

        params_.insert(params_.begin(),
                       stmt_def_func_param{self_tk, *head_.receiver_type(),
                                           not head_.mut_tk});
    }

    // the result and the parameters in the order of 'frame_slots', placed by
    // 'placement_of'; the array lengths are those of the body instance, empty
    // where no call gives them: in the parse and in 'check_body'
    auto add_signature_vars(toc& tc, const size_t indent,
                            const var_access access,
                            const std::span<const size_t> array_lengths,
                            const operand& slot_register) const -> void {

        assert(array_lengths.empty() or
               array_lengths.size() == array_param_count());

        size_t next_array_index{};

        for (const frame_slot& slot : frame_slots()) {
            const var_placement placement{
                placement_of(access, slot, slot_register),
            };

            if (not slot.param_index) {
                // note: a slot without a parameter is the result
                assert(result_);

                tc.add_var(result_->name_tk, indent,
                           {
                               .name{result_->name_tk.text()},
                               .type_ptr{&result_type()},
                               .src_loc_tk{result_->name_tk},
                               .shape{},
                               .placement{placement},
                               .foo_array{},
                           },
                           var_kind::result);

                continue;
            }

            const stmt_def_func_param& param{params_.at(*slot.param_index)};

            const std::optional<size_t> array_len{
                param.is_array() and not array_lengths.empty()
                    ? std::optional{array_lengths.at(next_array_index++)}
                    : std::nullopt,
            };

            tc.add_var(param.tok(), indent,
                       {
                           .name{param.name()},
                           .type_ptr{&param.get_type()},
                           .src_loc_tk{param.tok()},
                           .shape{
                               param.is_array() ? array_shape::array(array_len)
                                                : array_shape{},
                           },
                           .read_only_why{
                               param.is_read_only() ? read_only_cause::param
                                                    : read_only_cause::none,
                           },
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

        if (head_.noinline_tk) {
            throw compile_error{*head_.noinline_tk,
                                "'main' cannot be declared 'noinline'"};
        }

        if (not params().empty()) {
            throw compile_error{params().front().tok(),
                                "'main' cannot have parameters"};
        }

        if (result_) {
            throw compile_error{result_->name_tk,
                                "'main' cannot have a result"};
        }
    }

    // e.g. 'func mut point.move(dx)', a constructor writes its 'self' anyway
    auto assert_mut_has_receiver() const -> void {
        if (not head_.mut_tk or has_self_param()) {
            return;
        }

        if (is_constructor()) {
            throw compile_error{
                *head_.mut_tk,
                "a constructor builds 'self', 'mut' is not needed"};
        }

        throw compile_error{*head_.mut_tk,
                            "'mut' requires a method 'type.name'"};
    }

    // the result is declared before the parameters, slot 0 is the result,
    // so the duplicate is found here at the result
    auto assert_result_not_named_like_param() const -> void {
        if (not result_) {
            return;
        }

        for (const stmt_def_func_param& param : params_) {
            if (param.name() == result_->name_tk.text()) {
                throw compile_error{
                    result_->name_tk,
                    std::format("result {} is named like a parameter",
                                in_quotes(result_->name_tk.text())),
                };
            }
        }
    }

    // every path through the body sets the result, also the end, which
    // returns like a 'return' statement
    auto assert_sets_result() const -> void {
        if (not result_) {
            return;
        }

        assignment_flow flow{
            result_->name_tk.text(),
            result_->name_tk,
            result_type().size_bytes(),
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

    // the body in a frame of its own at the frame base, which the caller
    // holds, inside its 'frame_scope'; returns the size of the frame
    [[nodiscard]] auto
    compile_in_frame(toc& tc, const size_t indent,
                     const std::span<const size_t> array_lengths) const
        -> size_t {

        machine& x{tc.machine()};

        // the register is held while the body runs
        operand slot_register;

        if (has_receiver_in_register()) {
            slot_register = x.alloc_named_register(
                tok(), indent + 1, x.slot_register(), tc.builtins().address());
        }

        add_constants(tc, indent + 1);

        add_signature_vars(tc, indent + 1, var_access::through_address,
                           array_lengths, slot_register);

        compile_statements(tc, indent);
        x.return_function(indent + 1);

        const size_t frame_size_bytes{tc.layout().peak_frame_size_bytes()};

        if (not slot_register.is_empty()) {
            x.free_named_register(tok(), indent + 1, slot_register);
        }

        return frame_size_bytes;
    }

    // the scope of the body in a frame of its own, entered before the frame
    // base and the slot register are taken
    [[nodiscard]] auto
    frame_scope(toc& tc, const std::span<const size_t> array_lengths) const
        -> scope {

        const machine& x{tc.machine()};

        return scope::noinline_func(name(), body_call_path(array_lengths),
                                    x.frame_base_register());
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
        const machine& x{tc.machine()};

        const toc::scope_entry body{
            tc,
            is_inlined()
                ? scope::inlined_func(name(), {}, {})
                : scope::noinline_func(name(), {}, x.frame_base_register()),
        };

        add_constants(tc, 0);

        // note: 'parse_result' only keeps a result with a name
        assert(not result_ or result_->name_tk.is_word());

        add_signature_vars(tc, 0, var_access::direct, {}, {});

        code_ = {tc, tz, block_braces::required};

        // checked here so that the error of an instance gets its frames, a
        // signature that 'main' cannot have first
        assert_main_signature();
        assert_statements_reached();
        assert_sets_result();

        body_uses_ = tc.scopes().current_func_scope().body_uses_of_func();
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

    // the result 'name [type]', without one the type is 'void'
    auto parse_result(toc& tc, tokenizer& tz) -> void {
        const std::optional<func_result> read_result{
            func_result::read(tc, tz),
        };

        if (not read_result) {
            set_type(tc.builtins().void_type());
            return;
        }

        const func_result& result{*read_result};

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
            result.type_tk ? tc.get_type_or_throw(*result.type_tk)
                           : tc.builtins().default_type(),
        };

        result_ = result;
        set_type(tp);
    }

    // '(parameters) [result]', then the calls after it know the function
    auto parse_signature(toc& tc, tokenizer& tz) -> void {
        if (not head_.open_paren_tk) {
            throw compile_error{tz, "expected '(' after function name"};
        }

        parse_params(tc, tz);

        parse_result(tc, tz);

        assert_result_not_named_like_param();

        assert_mut_has_receiver();

        // known only after the result: a constructor builds 'self' instead
        if (has_self_param()) {
            add_self_param();
        }

        // e.g. 'text.size' for the instance 'str.size', empty if the function
        // is not an instance of a method of a generic type
        const std::string generic_method{
            instance_ ? instance_->generic_method : std::string{},
        };

        // see 'definition_order'
        const bool is_ordered{not instance_ and not head_.is_type_member()};

        tc.add_func(head_.name_tk, std::string{name()}, result_type(), *this,
                    generic_method, is_ordered);
    }

    // e.g. 'func point.at(x, y) self' builds a 'point'
    auto set_constructor_result(const token& self_tk,
                                const std::optional<token>& type_tk) -> void {

        head_.assert_receiver_for_self(self_tk);

        // one spelling, the type is already named before the '.'
        if (type_tk) {
            throw compile_error{
                *type_tk, "result 'self' has the type of the constructor"};
        }

        assert(head_.receiver_type() != nullptr);

        const type& tp{*head_.receiver_type()};

        result_ = func_result{
            .name_tk{self_tk},
            .type_tk{std::nullopt},
        };

        set_type(tp);
    }

    // the definition after its first token, without the body
    auto source_signature_to(std::ostream& os) const -> void {
        token::source_to(os, head_.noinline_tk);
        token::source_to(os, head_.mut_tk);
        head_.name_tk.source_to(os);
        token::source_to(os, head_.method_dot_tk);
        token::source_to(os, head_.method_name_tk);
        token::source_to(os, head_.open_paren_tk);

        // the implicit 'self' has no source and no delimiter after it
        const size_t first_param{has_self_param() ? size_t{1} : size_t{0}};

        source_delimited_to(os, params_ | std::views::drop(first_param),
                            param_delims_tk_);

        close_paren_tk_.source_to(os);

        if (result_) {
            result_->name_tk.source_to(os);
            token::source_to(os, result_->type_tk);
        }
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
//      deduces its types ('deductions') from its 'generic_signature',
//      'define_method' records a method and makes it for the instances of the
//      type made so far
//   2. an alias: 'instantiate_methods' makes each method for the new instance
//      ('instantiate_method', 'parse_method_instance'), a method with type
//      parameters of its own goes to 'define_func'
//   3. a call: 'instantiate_func' makes the instance for the type arguments
//      once ('parse_func_instance')
class func_instantiator final {
  public:
    //
    // statics
    //

    // records a generic function for its calls, e.g. 'func f<T type>(x T)'; a
    // method of an instance of a generic type keeps that instance, e.g.
    // 'str.append' of 'func text.append<T type>(s T)' keeps 'text<8>';
    // 'definition' is the last one the body sees, that of the generic method
    // for a method of a generic type
    static auto define_func(toc& tc, const token& func_tk,
                            const func_head& head,
                            const generic_signature& signature,
                            std::optional<generic_type_instance> receiver,
                            const size_t definition) -> void {

        // note: the definition checked the type parameters and that it is no
        //       constructor, see 'assert_generic_signature'

        // e.g. 'text.append' for the instance 'str.append' of 'text'
        std::string report_name{head.name};

        if (receiver) {
            // note: the head of an instance names the receiver type
            assert(head.method_name_tk);

            report_name = receiver->method_name(head.method_name_tk->text());
        }

        // note: the definition checked that the '(' follows
        assert(head.open_paren_tk);

        assert(not signature.is_constructor());

        tc.add_generic_func(head.name,
                            {
                                .src_loc_tk{head.name_tk},
                                .func_tk{func_tk},
                                .start_tk{head.start_tk},
                                .report_name{std::move(report_name)},
                                .signature{signature},
                                .deductions{deductions(signature)},
                                .receiver{std::move(receiver)},
                                .definition{definition},
                                .is_type_member{head.is_type_member()},
                            });
    }

    // e.g. 'func text.print()' of the generic type 'text': every instance of
    // the type gets the method, also those made after this definition
    static auto define_method(toc& tc, const token& func_tk,
                              const func_head& head,
                              const generic_signature& signature) -> void {

        assert(head.method_name_tk);

        const generic_method_info method{
            .type_tk{head.name_tk},
            .method_name_tk{*head.method_name_tk},
            .func_tk{func_tk},
            .start_tk{head.start_tk},
            .signature{signature},
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

        assert(type_args.size() == generic.signature.type_param_tks.size());

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
        const generic_trace call_trace{call_tk, name};

        try {
            parse_func_instance(tc, generic, name, type_args, call_trace);
        } catch (compile_error& e) {
            call_trace.add_to(e);
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
                                           const generic_signature& signature)
        -> generic_deduction {

        const auto it{
            std::ranges::find(signature.params, param_name,
                              &generic_signature_param::type_name),
        };

        std::optional<size_t> param_index;
        std::string param_name_text;

        if (it != signature.params.end()) {
            param_index = static_cast<size_t>(it - signature.params.begin());
            param_name_text = it->name_tk.text();
        }

        return {
            .param_index{param_index},
            .param_name{std::move(param_name_text)},
            .is_result{
                signature.result_type_tk and
                    signature.result_type_tk->is_text(param_name),
            },
        };
    }

    // how a call can tell each type parameter: the first parameter declared
    // with exactly its type, e.g. 's T' in 'func text.append<T type>(s T)',
    // and the result, e.g. 'res T'
    [[nodiscard]] static auto deductions(const generic_signature& signature)
        -> std::vector<generic_deduction> {

        std::vector<generic_deduction> result;
        result.reserve(signature.type_param_tks.size());

        for (const token& param_tk : signature.type_param_tks) {
            result.push_back(deduction_of(param_tk.text(), signature));
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
            receiver.trace.add_to(e);
            throw;
        }
    }

    // e.g. 'tokenizer.to<name>' of 'func tokenizer.to<T type>()', 'T' names
    // 'name' while the definition is parsed again
    static auto
    parse_func_instance(toc& tc, const generic_func_info& generic,
                        const std::string& name,
                        const std::span<const type* const> type_args,
                        const generic_trace& call_trace) -> void {

        generic_reparse reparse{tc, generic.definition, generic.start_tk};

        // e.g. 'text' names 'str' for 'func text.append<T type>(s T)'
        if (generic.receiver) {
            reparse.scope().add_receiver(*generic.receiver);
        }

        // e.g. 'T' names 'name'
        std::vector<generic_arg> args;

        for (const auto [param_tk, type_arg] :
             std::views::zip(generic.signature.type_param_tks, type_args)) {

            args.push_back({
                .param_tk{param_tk},
                .arg_tk{},
                .value{type_arg},
            });
        }

        reparse.scope().add_args(args);

        tokenizer& tz{reparse.tz()};

        const func_head head{func_head::read(tc, tz)};

        func_instance instance{
            .name{name},
            .generic_method{},
            .constants{},
            .trace{call_trace},
        };

        // e.g. 'str.append<i8>' keeps 'text.append' and the constants of
        // 'text<8>', like the instance of a method without type parameters
        if (generic.receiver) {
            // note: the head of an instance names the receiver type
            assert(head.method_name_tk);

            instance.generic_method =
                generic.receiver->method_name(head.method_name_tk->text());

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

        generic_reparse reparse{tc, method.definition, method.start_tk};
        reparse.scope().add_receiver(receiver);

        tokenizer& tz{reparse.tz()};

        // note: 'text' names 'str' now, so the head is the method 'str.print'
        const func_head head{func_head::read(tc, tz)};

        assert(not head.is_generic_type_method());
        assert(head.method_name_tk);

        if (not method.signature.type_param_tks.empty()) {
            define_func(tc, method.func_tk, head, method.signature, receiver,
                        method.definition);

            return;
        }

        func_instance instance{
            .name{head.name},
            .generic_method{receiver.method_name(head.method_name_tk->text())},
            .constants{receiver.constants()},
            .trace{receiver.trace},
        };

        tc.add_func_instance(std::make_shared<stmt_def_func>(
            tc, method.func_tk, head, tz, std::move(instance)));
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

        // note: the signature is checked before the definition, a method
        //       definition makes the instances of the types already defined,
        //       e.g. 'str.print'

        // e.g. 'func f<T type> {'
        if (not head.open_paren_tk) {
            throw compile_error{tz, "expected '(' after function name"};
        }

        const generic_signature signature{read_signature(tc, head, tz)};

        if (head.is_generic_type_method()) {
            func_instantiator::define_method(tc, tk, head, signature);
        } else {
            func_instantiator::define_func(
                tc, tk, head, signature, std::nullopt, tc.current_definition());
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

    // the rules of a generic definition, checked once whether or not an
    // instance is made: the type parameters hide nothing and are named apart
    // from the signature, a constructor has a receiver and no type parameters
    static auto assert_generic_signature(const toc& tc, const func_head& head,
                                         const generic_signature& signature)
        -> void {

        // e.g. 'to' of 'func text.to<T type>()' and of 'func
        // tokenizer.to<T type>()', 'f' of 'func f<T type>()'
        const token& name_tk{
            head.method_name_tk ? *head.method_name_tk : head.name_tk,
        };

        for (const token& param_tk : signature.type_param_tks) {
            // e.g. 'n' of 'func box.f<n type>()' of 'type box<n>'
            if (head.is_generic_type_method()) {
                tc.assert_method_type_param_free(param_tk, head.name_tk.text());
            }

            tc.assert_generic_param_free(param_tk);

            tc.assert_generic_param_apart(
                param_tk, name_tk, head.method_name_tk ? "method" : "function");

            for (const generic_signature_param& param : signature.params) {
                tc.assert_generic_param_apart(param_tk, param.name_tk,
                                              "parameter");
            }

            if (signature.result_name_tk) {
                tc.assert_generic_param_apart(
                    param_tk, *signature.result_name_tk, "result");
            }
        }

        if (not signature.is_constructor()) {
            return;
        }

        assert(signature.result_name_tk);

        head.assert_receiver_for_self(*signature.result_name_tk);

        // a constructor is called as 'point.at(...)', there is no place for
        // type arguments
        if (signature.type_param_tks.empty()) {
            return;
        }

        throw compile_error{
            *signature.result_name_tk,
            std::format("constructor {} cannot have type parameters",
                        in_quotes(head.name))};
    }

    // the signature after the head, the '(' and the body must follow, else
    // the skip of the body would take the next definition, e.g. 'func g'
    // after 'func f<T type>(x T)'; the types are resolved when an instance is
    // parsed; 'tz' is after the '(' of the head
    [[nodiscard]] static auto
    read_signature(const toc& tc, const func_head& head, tokenizer& tz)
        -> generic_signature {

        generic_signature signature{
            .type_param_tks{head.type_param_tks},
            .params{},
            .result_name_tk{std::nullopt},
            .result_type_tk{std::nullopt},
        };

        const stmt_def_func_param::syntax_list list{
            stmt_def_func_param::syntax_list::read(tz),
        };

        for (const stmt_def_func_param::syntax& param : list.params) {
            signature.params.push_back({
                .name_tk{param.name_tk},
                .type_name{
                    param.type_tk and not param.is_array()
                        ? param.type_tk->text()
                        : "",
                },
            });
        }

        if (const std::optional<func_result> result{
                func_result::read(tc, tz),
            }) {

            signature.result_name_tk = result->name_tk;
            signature.result_type_tk = result->type_tk;
        }

        assert_generic_signature(tc, head, signature);

        const token open_brace_tk{
            tz.expect_char_token('{', "expected '{' to begin block"),
        };

        tz.put_back_token(open_brace_tk);

        return signature;
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
