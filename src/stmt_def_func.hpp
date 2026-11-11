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
//   'frame_slot'             a slot of the frame of a noinline call
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
    // the '.' and the name of a method, e.g. '.add' of 'func list.add(x)',
    // and the type 'name_tk' names before it
    struct method_part {
        token dot_tk;
        token name_tk;
        // null for a generic type, e.g. 'text' of 'func text.print()', each
        // instance of 'text' reads the head again from 'start_tk', where
        // 'text' names a type
        const type* receiver_tp{};
    };

  public:
    // where the definition continues after 'func', an instance starts here
    token start_tk;
    std::optional<token> noinline_tk;
    std::optional<token> mut_tk;
    // the name, or the receiver type of a method, e.g. 'list' of 'list.add'
    token name_tk;
    // none for a function
    std::optional<method_part> method;

    // the name with the receiver of a method, named as the type it is bound
    // to, e.g. 'str.print' for 'text.print' when 'text' names 'str'
    std::string name;
    // e.g. 'T' of '<T type>'
    std::vector<token> type_param_tks;
    // none until it is read, e.g. after 'list.add' of 'func list.add('
    std::optional<token> open_paren_tk;

    // a constructor builds an instance of its receiver type, e.g. 'func
    // point.at(x, y) self'; one spelling, the type is named before the '.'
    auto assert_constructor_result(const token& self_tk,
                                   const std::optional<token>& type_tk) const
        -> void {

        if (not is_type_member()) {
            throw compile_error{
                self_tk, "result 'self' requires a constructor 'type.name'"};
        }

        if (type_tk) {
            throw compile_error{
                *type_tk, "result 'self' has the type of the constructor"};
        }
    }

    // e.g. 'func mut point.move(dx)', a constructor writes its 'self' anyway
    auto assert_mut_has_receiver(const bool is_constructor) const -> void {
        if (not mut_tk or (is_type_member() and not is_constructor)) {
            return;
        }

        if (is_constructor) {
            throw compile_error{
                *mut_tk, "a constructor builds 'self', 'mut' is not needed"};
        }

        throw compile_error{*mut_tk, "'mut' requires a method 'type.name'"};
    }

    // e.g. 'func f<T type> {' has no '(' after its name; reported at the
    // name, the next token can be on a later line
    auto assert_open_paren() const -> void {
        if (open_paren_tk) {
            return;
        }

        if (method) {
            throw compile_error{method->name_tk,
                                "expected '(' after method name"};
        }

        throw compile_error{name_tk, "expected '(' after function name"};
    }

    // kept as text, its instances are the functions
    [[nodiscard]] auto is_generic() const -> bool {
        return is_generic_type_method() or not type_param_tks.empty();
    }

    // e.g. 'func text.print()' of the generic type 'text'
    [[nodiscard]] auto is_generic_type_method() const -> bool {
        return method and method->receiver_tp == nullptr;
    }

    // reached through its type wherever it is defined, e.g. 'list.add' or
    // the constructor 'point.at'
    [[nodiscard]] auto is_type_member() const -> bool {
        return method.has_value();
    }

    // the type of 'name_tk' of a method of a type that is not generic
    [[nodiscard]] auto receiver_type() const -> const type& {
        assert(method and method->receiver_tp != nullptr);

        return *method->receiver_tp;
    }

    //
    // statics
    //

    // 'tz' is after 'func'
    //
    // the '(' is looked for after each word: a word followed by '(', '.' or
    // '<' is the name, e.g. 'func noinline()' and 'func mut.m()' read the
    // keyword as the name, which is then rejected
    [[nodiscard]] static auto read(const toc& tc, tokenizer& tz) -> func_head {
        func_head head{};
        head.start_tk = tz.cur_position_token();
        head.read_name(tz);
        head.noinline_tk = head.read_modifier(tz, "noinline");
        head.mut_tk = head.read_modifier(tz, "mut");
        head.name = head.name_tk.text();

        // a receiver names a type, e.g. 'point' of 'func point.at()', which
        // 'read_method' checks
        if (not head.open_paren_tk and tz.peek_char_after_whitespace() == '.') {
            toc::assert_valid_name(head.name_tk);
            head.read_method(tc, tz);
        } else {
            tc.assert_valid_unqualified_name(head.name_tk);
        }

        // e.g. 'func tokenizer.to<T type>()' or 'func text.to<T type>()'
        if (not head.open_paren_tk and tz.peek_char_after_whitespace() == '<') {
            head.type_param_tks = read_type_params(tc, tz);
            head.open_paren_tk = tz.accept_char_token('(');
        }

        return head;
    }

  private:
    // 'name_tk' is a modifier, a word follows it, not the '(', '.' or '<'
    // after a name
    [[nodiscard]] auto is_name_next(const tokenizer& tz) const -> bool {
        const char next{tz.peek_char_after_whitespace()};
        return not open_paren_tk and next != '.' and next != '<';
    }

    // e.g. '.add(' of 'func list.add(x)', the '(' if it follows; 'name_tk'
    // is the receiver: a type, whose method gets an implicit first parameter
    // 'self' and whose constructor builds a 'self', or a generic type, whose
    // instances get the method; the name of a method of a type is that of the
    // type it names, e.g. 'str.print' where 'text' names 'str'
    auto read_method(const toc& tc, tokenizer& tz) -> void {
        const token dot_tk{tz.seen_char_token('.')};

        // see 'method_part::receiver_tp'
        const type* receiver_tp{};

        if (not tc.generics().has_type(name_tk.text()) or
            tc.types().has(name_tk.text())) {

            receiver_tp = &tc.get_type_or_throw(name_tk);

            if (receiver_tp->is_builtin()) {
                throw compile_error{
                    name_tk, std::format("methods on built-in type {} are not "
                                         "supported",
                                         in_quotes(receiver_tp->name()))};
            }
        }

        const token method_name_tk{read_method_name_token(tz)};

        method = method_part{
            .dot_tk{dot_tk},
            .name_tk{method_name_tk},
            .receiver_tp{receiver_tp},
        };

        const std::string_view type_name{
            receiver_tp != nullptr ? receiver_tp->name() : name_tk.text(),
        };

        name = method_name_of(type_name, method_name_tk.text());
        open_paren_tk = tz.accept_char_token('(');
    }

    // 'keyword' before the name, e.g. 'noinline', then the next word is read
    // as the name; none when 'name_tk' is not it or is the name
    auto read_modifier(tokenizer& tz, const std::string_view keyword)
        -> std::optional<token> {

        if (not name_tk.is_text(keyword) or not is_name_next(tz)) {
            return std::nullopt;
        }

        const token modifier_tk{name_tk};
        read_name(tz);

        return modifier_tk;
    }

    // a word, the name or a modifier before it, and the '(' if it follows
    auto read_name(tokenizer& tz) -> void {
        name_tk = tz.next_token();
        open_paren_tk = tz.accept_char_token('(');
    }

    //
    // statics
    //

    // the name after the '.' of a method, e.g. 'print' of 'func text.print()'
    [[nodiscard]] static auto read_method_name_token(tokenizer& tz) -> token {
        const token name_tk{tz.next_token()};

        if (not name_tk.is_word()) {
            throw compile_error{name_tk, "expected a method name after '.'"};
        }

        toc::assert_valid_name(name_tk);

        return name_tk;
    }

    // e.g. '<T type, U type>', a generic function has type parameters only
    [[nodiscard]] static auto read_type_params(const toc& tc, tokenizer& tz)
        -> std::vector<token> {

        std::vector<token> param_tks;

        for (const generic_param& param : generic_param::parse(tz)) {
            // the name first, the kind is suggested with it
            toc::assert_valid_name(param.name_tk);

            if (not param.is_type) {
                // note: a name no parameter can have first, e.g. 'i8' of
                //       'func f<i8>()'; an instance has only type parameters

                tc.assert_valid_unqualified_name(param.name_tk);

                throw compile_error{
                    param.name_tk,
                    std::format("a generic function has only type "
                                "parameters, write '{} type'",
                                param.name_tk.text())};
            }

            param_tks.emplace_back(param.name_tk);
        }

        return param_tks;
    }
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

        // 'self' is the result of a constructor
        const bool is_keyword{
            not name_tk.is_text(reserved_names::self) and
                std::ranges::contains(reserved_names::keywords, name_tk.text()),
        };

        // a keyword is not a result name, e.g. the 'func' of 'func g<T
        // type>()' after a function that lacks its body
        if (not is_keyword) {
            assert_plain_type(tc, tz, type_tk);
        }

        // a word or a string before the '{' of the body is meant as the
        // type, else a missing '{' is reported by the body, e.g. 'func f()
        // res number {'
        // note: the '{' also tells the type of a generic definition, where
        //       'T' of 'res T {' is not a type yet
        const bool is_type_before_body{
            (type_tk.is_word() or type_tk.is_string()) and
                tz.peek_char_after_whitespace() == '{',
        };

        // a keyword is read as a result name only before a '{' or a known
        // type; before another name it starts what follows a function that
        // lacks its body, e.g. the 'type' of 'type p {x}'
        const bool has_type{
            tc.types().has(type_tk.text()) or
                (is_type_before_body and not is_keyword),
        };

        if (has_type) {
            statement::assert_not_literal_type(type_tk);
        } else {
            tz.put_back_token(type_tk);

            // a keyword without a '{' after it starts what follows a function
            // that lacks its body, e.g. the 'func' of the next definition;
            // before a '{' it is reported as a keyword
            if (is_keyword and tz.peek_char_after_whitespace() != '{') {
                tz.put_back_token(name_tk);
                return std::nullopt;
            }
        }

        // where it is read, also in a generic definition that no instance
        // parses, e.g. the '5' of 'func f<T type>() 5 {'; without the '{'
        // the body reports it missing first
        if (not name_tk.is_text(reserved_names::self) and
            tz.peek_char_after_whitespace() == '{') {

            tc.assert_valid_unqualified_name(name_tk);
        }

        return func_result{
            .name_tk{name_tk},
            .type_tk{has_type ? std::optional{type_tk} : std::nullopt},
        };
    }

  private:
    //
    // statics
    //

    // what follows the result name has no type arguments, 'mut' or '['
    static auto assert_plain_type(const toc& tc, tokenizer& tz,
                                  const token& type_tk) -> void {

        if (type_tk.is_word()) {
            statement::assert_no_type_arguments(tc, tz, type_tk);
        }

        // e.g. 'res mut', a parameter's modifier
        if (type_tk.is_text("mut")) {
            throw compile_error{type_tk,
                                "a result takes no 'mut', its function writes "
                                "it"};
        }

        // e.g. the '[' of 'res i8[4]' or 'res[4]'
        if (tz.peek_char_after_whitespace() == '[') {
            throw compile_error{tz, "a result cannot be an array, pass the "
                                    "array as a 'mut' parameter"};
        }
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
// emits nothing, the body is compiled by its calls, that of 'main' where the
// program starts; the ways a body is compiled:
//
//   when                  scope              the parameters
//   'parse_body'          'inlined_func' or  variables, named only
//                         'noinline_func'
//   an inlined call,      'inlined_func'     bindings to the arguments
//   'compile_inlined'
//   a noinline body,      'noinline_func'    variables reached through the
//   'emit_noinline_body',                    addresses in the frame
//   'check_body' and                         ('compile_in_frame')
//   'check_noinline_body'
//   'main',               'main_func'        none
//   'compile_main'
//
// 'check_body' compiles a function without its calls like a noinline
// body
class stmt_def_func final : public statement {
    func_head head_{};
    std::vector<stmt_def_func_param> params_;
    std::vector<token> param_delimiter_tks_;
    token close_paren_tk_;
    std::optional<func_result> result_;
    stmt_block code_;
    // the definition parsed again with the parameters naming the arguments
    std::optional<func_instance> instance_;
    // what the body reaches that its callers name too, see 'body_uses'
    body_uses body_uses_;
    // false while the body is parsed, its record is then incomplete
    bool is_body_parsed_{};

    // how the body reaches the result and the parameters: a parsed body only
    // names them, a compiled noinline body has their addresses in its frame
    // and the receiver's in 'receiver_register' when it is passed in one
    struct names_only {};

    struct through_frame {
        std::optional<operand> receiver_register;
    };

    using var_access = std::variant<names_only, through_frame>;

  public:
    // a function of the user code, or an instance of a generic definition,
    // e.g. 'tokenizer.to<name>' or 'str.print', see 'func_instantiator';
    // 'tz' is after 'head'
    stmt_def_func(toc& tc, const token& src_loc_tk, const func_head& head,
                  tokenizer& tz, std::optional<func_instance> instance)
        : statement{src_loc_tk}, head_{head}, instance_{std::move(instance)} {

        // note: a generic definition is a 'stmt_def_func_generic', a head
        //       with type parameters is read here only for an instance

        assert(not head.is_generic_type_method());
        assert(instance_ or head.type_param_tks.empty());

        parse_signature(tc, tz);

        parse_body(tc, tz);
    }

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

    // the body is compiled by its calls: in place by 'compile_inlined', else
    // once per array lengths by 'emit_noinline_body'
    auto compile([[maybe_unused]] toc& tc,
                 [[maybe_unused]] const destination& dst) const
        -> void override {}

    //
    // class methods
    //

    // 'func' is a keyword so no user name or internal label starts with 'func.'
    // e.g. 'func.sum..len.4.8' for array lengths 4 and 8
    [[nodiscard]] auto body_label(
        const std::span<const std::optional<size_t>> array_lengths = {}) const
        -> std::string {

        return std::format("{}{}", machine::body_label_prefix,
                           name_with_lengths(array_lengths));
    }

    // for the errors of a function without its calls, also those in code a
    // call folds away, compiled like a noinline body and discarded, with its
    // registers and variable storage; an unsized array parameter has no
    // length ('ident_info::is_length_unknown'); an inlined recursion of a
    // body that takes from a call ends at a limit ('compile_error::is_limit',
    // 'inline_call::check_recursion'); an inlined result is the destination
    // of the call, checked here through an address
    auto check_body(toc& tc) const -> void {
        try {
            dry_run_in_frame(tc, {});
        } catch (compile_error& e) {
            // the frames of what made the instance, see 'func_instance::trace'
            if (instance_) {
                instance_->trace.add_to(e);
            }

            throw;
        }
    }

    // the body of a noinline function for array lengths that only a dry run
    // asked for, compiled as 'emit_noinline_body' does, nothing kept
    auto check_noinline_body(
        toc& tc,
        const std::span<const std::optional<size_t>> array_lengths) const
        -> void {

        assert(not is_inlined());
        assert(array_lengths.size() == array_param_count());

        dry_run_in_frame(tc, array_lengths);
    }

    // the body in place at the call 'call_tk', under labels named by the
    // call, its result and parameters named by the bindings to the call
    auto compile_inlined(toc& tc, const token& call_tk,
                         const std::optional<param_binding>& result,
                         const std::span<const param_binding> arguments) const
        -> void {

        assert(is_inlined());
        assert(result.has_value() == has_result());
        assert(arguments.size() == params_.size());

        const std::string call_path{tc.call_path_at(call_tk)};

        const std::string call_label{
            std::format("{}..{}", body_label(), call_path),
        };

        const std::string ret_jmp_label{toc::end_label(call_label)};

        machine& x{tc.machine()};

        source_def_comment_to(x, tc.indent());

        x.label(tc.indent(), call_label);

        const toc::scope_entry body{
            tc,
            scope::inlined_func(name(), call_path, ret_jmp_label),
            call_tk,
        };

        add_constants(tc);

        const auto add_binding{
            [&](const param_binding& binding) -> void {
                const ident_info& bound{binding.info};

                x.comment_binding(
                    {call_tk, tc.indent() + 1}, binding.from, binding.text,
                    bound.is_address_by_call() ? std::optional{bound.operand()}
                                               : std::nullopt);

                tc.add_binding(binding);
            },
        };

        if (result) {
            add_binding(*result);
        }

        for (const param_binding& binding : arguments) {
            add_binding(binding);
        }

        // the result reaches the destination through the binding of 'res',
        // so the statements of the body have no destination
        code_.compile(tc, destination{});

        // provide the exit label for 'return' to jump to
        x.label(tc.indent(), ret_jmp_label);
    }

    // where the program starts, 'main' takes nothing from a call
    auto compile_main(toc& tc) const -> void {
        assert(name() == reserved_names::main);

        const toc::scope_entry main{tc, scope::main_func()};

        machine& x{tc.machine()};

        x.comment({token{}, 0}, "");

        x.label(0, reserved_names::main);

        code_.compile(tc, destination{});

        // code after an 'exit' or 'return' on every path would never run;
        // the compile knows the conditions it folded, e.g. with constant
        // arguments of an inlined call
        if (tc.is_reachable()) {
            x.exit({token{}, 1}, operand::imm(0, tc.builtins().default_type()));
        }
    }

    // the body of a noinline function for one set of array lengths, after
    // 'main'; the lengths are those of the array arguments, one per array
    // parameter
    auto emit_noinline_body(
        toc& tc,
        const std::span<const std::optional<size_t>> array_lengths) const
        -> void {

        assert(not is_inlined());
        assert(array_lengths.size() == array_param_count());

        const toc::scope_entry body{tc, frame_scope(array_lengths)};

        machine& x{tc.machine()};

        x.comment({token{}, 0}, "");
        source_def_comment_to(x, 0);

        if (not array_lengths.empty()) {
            // note: an emitted body has every length, see 'add_noinline_body'
            const auto known{
                [](const std::optional<size_t>& length) -> size_t {
                    return length.value();
                },
            };

            x.comment({token{}, 0}, "array parameter lengths: {:n}",
                      std::views::transform(array_lengths, known));
        }

        x.begin_noinline_body(std::string{name()}, body_label(array_lengths));

        const size_t frame_size_bytes{compile_in_frame(tc, array_lengths)};

        x.end_noinline_body();

        x.define_constant(frame_size_label(array_lengths), frame_size_bytes);
    }

    // 'size.' is a prefix, a suffix could collide with a method body, e.g.
    // 'func.list.size' of function 'list' and method 'list.size'
    [[nodiscard]] auto frame_size_label(
        const std::span<const std::optional<size_t>> array_lengths) const
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
                .is_in_register{i == 0 and has_self_param()},
            });
        }

        return slots;
    }

    [[nodiscard]] auto has_result() const -> bool {
        return result_.has_value();
    }

    // e.g. 'tokenizer.to<name>', empty for a function written in the source
    [[nodiscard]] auto instance_name() const -> std::string_view {
        if (not instance_) {
            return {};
        }

        return instance_->name;
    }

    [[nodiscard]] auto is_body_parsed() const -> bool {
        return is_body_parsed_;
    }

    // e.g. 'func point.at(x, y) self'
    [[nodiscard]] auto is_constructor() const -> bool {
        return result_ and result_->is_self();
    }

    [[nodiscard]] auto is_inlined() const -> bool {
        return not head_.noinline_tk;
    }

    [[nodiscard]] auto name() const -> std::string_view {
        return instance_ ? instance_->name : head_.name;
    }

    [[nodiscard]] auto params() const -> std::span<const stmt_def_func_param> {
        return params_;
    }

    [[nodiscard]] auto result() const -> const std::optional<func_result>& {
        return result_;
    }

    // the type of the result, 'void' without one; the node's type
    [[nodiscard]] auto result_type() const -> const type& { return get_type(); }

    [[nodiscard]] auto uses() const -> const body_uses& { return body_uses_; }

  private:
    // the constant arguments of the generic type of a method, in the scope of
    // the body
    auto add_constants(toc& tc) const -> void {
        if (not instance_) {
            return;
        }

        // listed like the statements of the body
        const toc::indent_scope names{tc, tc.indent() + 1};

        for (const generic_arg& constant : instance_->constants) {
            tc.add_const(constant.param_tk, constant.param_tk.text(),
                         folded_value::untyped(constant.constant()));
        }
    }

    auto add_result_var(toc& tc, const var_placement& placement) const -> void {
        assert(result_);

        tc.add_var(
            {
                .name{result_->name_tk.text()},
                .type_ptr{&result_type()},
                .src_loc_tk{result_->name_tk},
                .shape{},
                .read_only_why{},
                .placement{placement},
                .foo_array{},
            },
            var_kind::result);
    }

    // located at the method name for diagnostics
    auto add_self_param() -> void {
        // note: a method has its name after the '.'
        assert(head_.method);

        const token self_tk{
            token::synthetic(head_.method->name_tk.file_index(),
                             reserved_names::self,
                             head_.method->name_tk.start_index(),
                             head_.method->name_tk.at_line()),
        };

        params_.insert(
            params_.begin(),
            stmt_def_func_param{self_tk, head_.receiver_type(), head_.mut_tk});
    }

    // the result and the parameters in the order of 'frame_slots', placed by
    // 'placement_of'; the array lengths are those of the body instance, empty
    // where no call gives them: in the parse and in 'check_body'; one is
    // unknown where 'check_body' passes an unsized array parameter on
    auto add_signature_vars(
        toc& tc, const var_access& access,
        const std::span<const std::optional<size_t>> array_lengths) const
        -> void {

        assert(array_lengths.empty() or
               array_lengths.size() == array_param_count());

        // listed like the statements of the body
        const toc::indent_scope names{tc, tc.indent() + 1};

        size_t next_array_index{};

        for (const frame_slot& slot : frame_slots()) {
            const var_placement placement{
                placement_of(access, slot),
            };

            // note: a slot without a parameter is the result
            if (not slot.param_index) {
                add_result_var(tc, placement);
                continue;
            }

            const stmt_def_func_param& param{params_.at(*slot.param_index)};

            const std::optional<size_t> array_len{
                param.is_array() and not array_lengths.empty()
                    ? array_lengths.at(next_array_index++)
                    : std::nullopt,
            };

            add_param_var(tc, param, array_len, placement);
        }
    }

    [[nodiscard]] auto array_param_count() const -> size_t {
        return static_cast<size_t>(
            std::ranges::count_if(params_, &stmt_def_func_param::is_array));
    }

    // checked after the body, a result name may be the first word of a body
    // without its '{', e.g. the 'exit' of 'func main() exit(0)'
    auto assert_main_has_no_result() const -> void {
        if (name() == reserved_names::main and result_) {
            throw compile_error{result_->name_tk,
                                "'main' cannot have a result"};
        }
    }

    // no caller gives 'main' arguments or calls it out of line, the result
    // is 'assert_main_has_no_result'
    auto assert_main_signature() const -> void {
        if (name() != reserved_names::main) {
            return;
        }

        if (head_.noinline_tk) {
            throw compile_error{*head_.noinline_tk,
                                "'main' cannot be declared 'noinline'"};
        }

        if (not params_.empty()) {
            throw compile_error{params_.front().tok(),
                                "'main' cannot have parameters"};
        }
    }

    // every statement is reached and every path through the body sets the
    // result, also the end, which returns like a 'return' statement
    // note: without a result the type is 'void', of no bytes, which every
    //       path sets
    auto assert_sets_result() const -> void {
        assignment_flow flow{
            result_ ? result_->name_tk : tok(),
            result_type().size_bytes(),
        };

        code_.trace_assignment(flow);
        flow.assert_set_at_return();
    }

    // keeps the labels of the body unique: the instances of one generic
    // definition have the same source locations, and a noinline body is
    // compiled once per array lengths
    [[nodiscard]] auto body_call_path(
        const std::span<const std::optional<size_t>> array_lengths) const
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
    [[nodiscard]] auto compile_in_frame(
        toc& tc,
        const std::span<const std::optional<size_t>> array_lengths) const
        -> size_t {

        // a body is listed from the left also where a call checks it
        const toc::indent_scope body_indent{tc, 0};

        // the register is held while the body runs
        std::optional<operand> receiver_register;

        machine& x{tc.machine()};

        if (has_self_param()) {
            receiver_register = x.alloc_named_register(
                {tok(), 1}, x.receiver_register(), tc.builtins().address());
        }

        add_constants(tc);

        add_signature_vars(tc,
                           through_frame{
                               .receiver_register{receiver_register},
                           },
                           array_lengths);

        code_.compile(tc, destination{});
        x.return_function(1);

        const size_t frame_size_bytes{tc.layout().max_frame_size_bytes()};

        if (receiver_register) {
            x.free_named_register({tok(), 1}, *receiver_register);
        }

        return frame_size_bytes;
    }

    // the body compiled in a frame of its own for 'array_lengths', nothing
    // kept
    auto dry_run_in_frame(
        toc& tc,
        const std::span<const std::optional<size_t>> array_lengths) const
        -> void {

        tc.dry_run(
            [&] -> void {
                const toc::scope_entry body{tc, frame_scope(array_lengths)};

                std::ignore = compile_in_frame(tc, array_lengths);
            },
            machine::discarded_code::body);
    }

    // the scope of the body in a frame of its own, entered before the frame
    // base and the receiver register are taken; whether the body takes from
    // a call ('scope::takes_from_call'):
    //   1. without array lengths it is compiled without a call: a noinline
    //      body without array parameters takes nothing, it is checked as it
    //      is emitted, as 'main' is; 'check_body' takes what a call gives,
    //      the array lengths or, inlined, the bindings of the parameters and
    //      the result
    //   2. with array lengths it takes from a call when one is unknown, as
    //      'check_body' does
    [[nodiscard]] auto frame_scope(
        const std::span<const std::optional<size_t>> array_lengths) const
        -> scope {

        scope body_scope{
            scope::noinline_func(name(), body_call_path(array_lengths)),
        };

        if (array_lengths.empty()) {
            const bool takes_from_call{
                array_param_count() != 0 or
                    (is_inlined() and (not params_.empty() or has_result())),
            };

            body_scope.mark_without_call(takes_from_call);

            return body_scope;
        }

        // note: a checked body with a length unknown does not check its
        //       skipped code, as an inlined call does not; 'check_body' does

        const bool has_unknown_length{
            std::ranges::any_of(array_lengths,
                                [](const std::optional<size_t>& length)
                                    -> bool { return not length; }),
        };

        if (has_unknown_length) {
            body_scope.mark_taking_from_call();
        }

        return body_scope;
    }

    // a method of a type, a constructor builds its 'self' instead; the
    // address of 'self' is in the receiver register of the backend, not in a
    // frame slot
    [[nodiscard]] auto has_self_param() const -> bool {
        return head_.method and not is_constructor();
    }

    // e.g. 'sum..len.4.8' for array lengths 4 and 8, 'sum' without lengths
    [[nodiscard]] auto name_with_lengths(
        const std::span<const std::optional<size_t>> array_lengths) const
        -> std::string {

        if (array_lengths.empty()) {
            return label_name(name());
        }

        return std::format("{}..{}", label_name(name()),
                           array_lengths_path(array_lengths));
    }

    // the function scope is established before the body is parsed, its
    // variables are registered without emitting output
    auto parse_body(toc& tc, tokenizer& tz) -> void {
        const toc::scope_entry body{
            tc,
            is_inlined() ? scope::inlined_func(name(), {}, {})
                         : scope::noinline_func(name(), {}),
        };

        add_constants(tc);

        // note: 'func_result::read' only keeps a result with a name
        assert(not result_ or result_->name_tk.is_word());

        add_signature_vars(tc, names_only{}, {});

        code_ = {tc, tz, block_braces::required};

        assert_main_has_no_result();

        // checked here so that the error of an instance gets its frames
        assert_sets_result();

        body_uses_ = tc.scopes().current_func_scope().body_uses_of_func();
        is_body_parsed_ = true;
    }

    // the parameters read into 'list', e.g. 'a i32, b i32', with their types
    auto parse_params(toc& tc, const stmt_def_func_param::syntax_list& list)
        -> void {

        for (const stmt_def_func_param::syntax& param : list.params) {
            params_.emplace_back(tc, param);
        }

        param_delimiter_tks_ = list.delimiter_tks;
        close_paren_tk_ = list.close_paren_tk;
    }

    // the result 'name [type]', without one the type is 'void'
    auto parse_result(toc& tc, tokenizer& tz) -> void {
        result_ = func_result::read(tc, tz);

        if (not result_) {
            set_type(tc.builtins().void_type());
            return;
        }

        // 'self' names the value a constructor builds, e.g. 'func
        // point.at(x, y) self' builds a 'point'
        if (result_->is_self()) {
            head_.assert_constructor_result(result_->name_tk, result_->type_tk);
            set_type(head_.receiver_type());
            return;
        }

        set_type(result_->type_tk ? tc.get_type_or_throw(*result_->type_tk)
                                  : tc.builtins().default_type());
    }

    // '(parameters) [result]', then the calls after it know the function
    auto parse_signature(toc& tc, tokenizer& tz) -> void {
        const stmt_def_func_param::syntax_list list{
            stmt_def_func_param::syntax_list::read(tc, tz),
        };

        parse_params(tc, list);

        parse_result(tc, tz);

        if (result_) {
            list.assert_result_not_named_like_param(result_->name_tk);
        }

        head_.assert_mut_has_receiver(is_constructor());

        // before the body, e.g. 'func main(x) { x = 1 }' is rejected for its
        // parameter, not for the write to it
        assert_main_signature();

        // known only after the result: a constructor builds 'self' instead
        if (has_self_param()) {
            add_self_param();
        }

        // see 'definition_order'
        const bool is_ordered{not instance_ and not head_.is_type_member()};

        tc.add_func(
            std::string{name()},
            {
                .src_loc_tk{head_.name_tk},
                .def{this},
                .generic_method{
                    instance_ ? instance_->generic_method : std::string{},
                },
                .definition{is_ordered ? tc.current_definition() : 0},
            });
    }

    auto source_def_comment_to(machine& x, const size_t indent) const -> void {
        std::stringstream ss;
        source_signature_to(ss);

        const std::string signature{one_line_source::of(ss.view())};

        // the signature of a generic definition names its parameters, the
        // instance names the arguments, e.g. 'pt.put<i16>'
        if (instance_) {
            x.comment({head_.name_tk, indent}, "{}, instance {}", signature,
                      in_quotes(instance_->name));

            return;
        }

        x.comment({head_.name_tk, indent}, "{}", signature);
    }

    // the definition after its first token, without the body
    auto source_signature_to(std::ostream& os) const -> void {
        token::source_to(os, head_.noinline_tk);
        token::source_to(os, head_.mut_tk);
        head_.name_tk.source_to(os);

        if (head_.method) {
            head_.method->dot_tk.source_to(os);
            head_.method->name_tk.source_to(os);
        }

        token::source_to(os, head_.open_paren_tk);

        // the implicit 'self' has no source and no delimiter after it
        const size_t first_param{has_self_param() ? size_t{1} : size_t{0}};

        source_delimited_to(os, params_ | std::views::drop(first_param),
                            param_delimiter_tks_);

        close_paren_tk_.source_to(os);

        if (result_) {
            result_->name_tk.source_to(os);
            token::source_to(os, result_->type_tk);
        }
    }

    //
    // statics
    //

    // 'array_len' is that of an array parameter, empty where no call gives it
    static auto add_param_var(toc& tc, const stmt_def_func_param& param,
                              const std::optional<size_t> array_len,
                              const var_placement& placement) -> void {

        tc.add_var(
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

    // e.g. 'len.4.8', 'len.u.8' with the first length unknown, unlike the
    // 'L.C' pairs of inlined calls
    [[nodiscard]] static auto array_lengths_path(
        const std::span<const std::optional<size_t>> array_lengths)
        -> std::string {

        std::string path{"len"};

        for (const std::optional<size_t> length : array_lengths) {
            if (not length) {
                path += ".u";
                continue;
            }

            path += std::format(".{}", *length);
        }

        return path;
    }

    // a label has no '<', '>' or ',', e.g. 'tokenizer.to<name>' is
    // 'tokenizer.to..name'; a name has no '..', so 'f<a,b>' ('f..a.b') and
    // the method instance 'f.a<b>' ('f.a..b') differ; a path follows a name
    // after '..' too, which a generic name and a plain name of the same
    // function or method cannot both take, so the function 'h' for array
    // lengths ('h..len.4') and the method 'h.len' inlined at 4:1
    // ('h.len..4.1') differ
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

    // a parsed body names its result and parameters in place, a compiled
    // noinline body reaches each through the address in its frame slot, the
    // receiver through the address in the receiver register
    [[nodiscard]] static auto placement_of(const var_access& access,
                                           const frame_slot& slot)
        -> var_placement {

        const through_frame* const frame{std::get_if<through_frame>(&access)};

        if (frame == nullptr) {
            return in_storage{
                .offset{},
                .base_register{},
                .holds_address{},
            };
        }

        if (slot.is_in_register) {
            // note: 'compile_in_frame' holds the receiver register for a
            //       receiver passed in a register

            assert(frame->receiver_register);

            return at_address_register{*frame->receiver_register};
        }

        return in_storage{
            .offset{},
            .base_register{},
            .holds_address{true},
        };
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
//   1. a definition: 'define' records a generic function with its
//      'generic_signature', which tells a call how to deduce its types, or a
//      method, which it makes for the instances of the type made so far
//   2. an alias: 'instantiate_methods' makes each method for the new instance
//      and its bases in the order they are defined ('instantiate_method',
//      'parse_method_instance'), a method with type parameters of its own
//      is recorded as a generic function of the instance
//   3. a call: 'instantiate_func' makes the instance for the type arguments
//      once ('parse_func_instance')
class func_instantiator final {
  public:
    //
    // statics
    //

    // records a generic function for its calls, or a method of a generic
    // type, e.g. 'func text.print()', for every instance of the type, also
    // those made after this definition
    static auto define(toc& tc, const token& func_tk, const func_head& head,
                       const generic_signature& signature) -> void {

        // note: the definition checked that the '(' follows
        assert(head.open_paren_tk);

        const generic_func_info generic{
            .src_loc_tk{head.name_tk},
            .func_tk{func_tk},
            .start_tk{head.start_tk},
            .report_name{head.name},
            .signature{signature},
            .receiver{std::nullopt},
            .definition{tc.current_definition()},
            .is_type_member{head.is_type_member()},
        };

        if (not head.is_generic_type_method()) {
            // note: the definition checked the type parameters and that it is
            //       no constructor, see 'assert_generic_signature'

            assert(not signature.is_constructor());

            tc.add_generic_func(head.name, generic);

            return;
        }

        tc.add_generic_method(generic);

        for (const generic_type_instance& instance :
             tc.generics().get_type_instances(head.name_tk.text())) {

            instantiate_method(tc, generic, instance);
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

    // the methods defined so far for a new instance of a generic type and its
    // bases ('levels'), e.g. 'str.print' of 'func text.print()' for 'type str
    // = text<8>'; made in the order they are defined, as a method defined
    // after the alias is, so a body sees the methods a hand-written type
    // would
    static auto instantiate_methods(
        toc& tc, const std::vector<generic_type_instance>& levels) -> void {

        for (const generic_func_info& method : tc.generics().methods()) {
            for (const generic_type_instance& level : levels) {
                if (level.generic_name == method.src_loc_tk.text()) {
                    instantiate_method(tc, method, level);
                }
            }
        }
    }

  private:
    //
    // statics
    //

    // an error in the method is in the generic code, the frames name the bases
    // and the alias
    static auto instantiate_method(toc& tc, const generic_func_info& method,
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
    // 'name' while the definition is parsed again; a method of a generic type
    // keeps the method it instantiates and the constants of the receiver,
    // e.g. 'str.append<i8>' keeps 'text.append' and those of 'text<8>'
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
                .value{type_arg},
            });
        }

        reparse.scope().add_args(args);

        tokenizer& tz{reparse.tz()};

        const func_head head{func_head::read(tc, tz)};

        func_instance instance{
            .name{name},
            .generic_method{generic.generic_method()},
            .constants{},
            .trace{call_trace},
        };

        if (generic.receiver) {
            instance.constants = generic.receiver->constants();
        }

        tc.add_func_instance(std::make_shared<stmt_def_func>(
            tc, generic.func_tk, head, tz, std::move(instance)));
    }

    // e.g. 'str.print' of 'func text.print()', 'text' names 'str' while the
    // definition is parsed again; a method with type parameters of its own is
    // a generic function of the instance, e.g. 'str.append' of 'func
    // text.append<T type>(s T)', which keeps 'text<8>'
    static auto parse_method_instance(toc& tc, const generic_func_info& method,
                                      const generic_type_instance& receiver)
        -> void {

        generic_reparse reparse{tc, method.definition, method.start_tk};
        reparse.scope().add_receiver(receiver);

        tokenizer& tz{reparse.tz()};

        // note: 'text' names 'str' now, so the head is the method 'str.print'
        const func_head head{func_head::read(tc, tz)};

        assert(not head.is_generic_type_method());
        assert(head.method);

        if (not method.signature.type_param_tks.empty()) {
            generic_func_info generic{method};
            generic.receiver = receiver;

            tc.add_generic_func(head.name, std::move(generic));

            return;
        }

        func_instance instance{
            .name{head.name},
            .generic_method{method.report_name},
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
    stmt_def_func_generic(toc& tc, const token& src_loc_tk,
                          const func_head& head, tokenizer& tz)
        : statement{src_loc_tk} {

        // note: the signature is checked before the definition, a method
        //       definition makes the instances of the types already defined,
        //       e.g. 'str.print'

        const generic_signature signature{read_signature(tc, head, tz)};

        func_instantiator::define(tc, src_loc_tk, head, signature);

        generic_text_ = tz.skip_braced_block_after(src_loc_tk);
    }

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        std::print(os, "{}", generic_text_);
    }

    // only its instances are functions
    auto compile([[maybe_unused]] toc& tc,
                 [[maybe_unused]] const destination& dst) const
        -> void override {}

  private:
    //
    // statics
    //

    // the body of a method of a generic type sees the parameters of the type,
    // e.g. 'T' of 'type box<T type>' in 'func box.f(T)' would name two
    // things; a parameter or a result may hide a constant parameter, which
    // 'is_type_only' leaves out; 'what' names the kind of 'name_tk'
    static auto assert_apart_from_receiver_params(
        const toc& tc, const func_head& head, const token& name_tk,
        const std::string_view what, const bool is_type_only) -> void {

        const generic_type_info& receiver{
            tc.generics().get_type(head.name_tk.text()),
        };

        for (const generic_param& param : receiver.params) {
            if (is_type_only and not param.is_type) {
                continue;
            }

            if (param.name_tk.text() == name_tk.text()) {
                throw compile_error{
                    name_tk,
                    std::format("{} {} is also the name of the generic "
                                "parameter of {} at {}, use another name",
                                what, in_quotes(name_tk.text()),
                                in_quotes(head.name_tk.text()),
                                tc.where(param.name_tk))};
            }
        }
    }

    // a constructor has a receiver and no type parameters
    static auto assert_constructor_signature(const func_head& head,
                                             const generic_signature& signature)
        -> void {

        assert(signature.result_name_tk);

        head.assert_constructor_result(*signature.result_name_tk,
                                       signature.result_type_tk);

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

    // the rules of a generic definition, checked once whether or not an
    // instance is made: the type parameters name no built-in and are named
    // apart from the signature, so are those of the generic type of a method,
    // a constructor has a receiver and no type parameters
    static auto assert_generic_signature(const toc& tc, const func_head& head,
                                         const generic_signature& signature)
        -> void {

        for (const token& param_tk : signature.type_param_tks) {
            assert_type_param_apart(tc, head, signature, param_tk);
        }

        // e.g. 'T' of 'func box.f(T)' of 'type box<T type>'
        if (head.is_generic_type_method()) {
            assert_names_apart_from_receiver_params(tc, head, signature);
        }

        if (not signature.is_constructor()) {
            return;
        }

        assert_constructor_signature(head, signature);
    }

    // the parameters and the result of a method of a generic type are named
    // apart from the type parameters of the type
    static auto assert_names_apart_from_receiver_params(
        const toc& tc, const func_head& head,
        const generic_signature& signature) -> void {

        for (const generic_signature_param& param : signature.params) {
            assert_apart_from_receiver_params(tc, head, param.name_tk,
                                              "parameter", true);
        }

        if (signature.result_name_tk) {
            assert_apart_from_receiver_params(
                tc, head, *signature.result_name_tk, "result", true);
        }
    }

    // the type parameter 'param_tk' is a valid name apart from the type
    // parameters of the generic type of a method, the function, the receiver
    // type, the parameters and the result
    static auto assert_type_param_apart(const toc& tc, const func_head& head,
                                        const generic_signature& signature,
                                        const token& param_tk) -> void {

        // e.g. 'to' of 'func text.to<T type>()' and of 'func
        // tokenizer.to<T type>()', 'f' of 'func f<T type>()'
        const token& name_tk{
            head.method ? head.method->name_tk : head.name_tk,
        };

        // e.g. 'n' of 'func box.f<n type>()' of 'type box<n>'
        if (head.is_generic_type_method()) {
            assert_apart_from_receiver_params(tc, head, param_tk,
                                              "generic parameter", false);
        }

        tc.assert_valid_unqualified_name(param_tk);

        tc.definitions().assert_generic_param_apart(
            param_tk, name_tk, head.method ? "method" : "function");

        // e.g. 'box' of 'func box.f<box type>()', the receiver type
        if (head.method) {
            tc.definitions().assert_generic_param_apart(param_tk, head.name_tk,
                                                        "type");
        }

        for (const generic_signature_param& param : signature.params) {
            tc.definitions().assert_generic_param_apart(param_tk, param.name_tk,
                                                        "parameter");
        }

        if (signature.result_name_tk) {
            tc.definitions().assert_generic_param_apart(
                param_tk, *signature.result_name_tk, "result");
        }
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
            stmt_def_func_param::syntax_list::read(tc, tz),
        };

        for (const stmt_def_func_param::syntax& param : list.params) {
            signature.params.push_back({
                .name_tk{param.name_tk},
                .type_name{
                    param.type_tk and not param.is_array()
                        ? param.type_tk->text()
                        : "",
                },
                .is_array{param.is_array()},
            });
        }

        if (const std::optional<func_result> result{
                func_result::read(tc, tz),
            }) {

            list.assert_result_not_named_like_param(result->name_tk);

            signature.result_name_tk = result->name_tk;
            signature.result_type_tk = result->type_tk;
        }

        assert_generic_signature(tc, head, signature);
        head.assert_mut_has_receiver(signature.is_constructor());

        // the body reads the '{'
        if (tz.peek_char_after_whitespace() != '{') {
            throw compile_error{tz, "expected '{' to begin block"};
        }

        return signature;
    }
};

// reads the head and makes the statement of the kind that follows it
inline auto make_stmt_def_func(toc& tc, const token& src_loc_tk, tokenizer& tz)
    -> std::unique_ptr<statement> {

    const func_head head{func_head::read(tc, tz)};

    // no call gives 'main' type arguments, the rest of its signature is
    // checked by 'stmt_def_func::assert_main_signature'
    if (head.name == reserved_names::main and not head.type_param_tks.empty()) {
        throw compile_error{head.type_param_tks.front(),
                            "'main' cannot have type parameters"};
    }

    head.assert_open_paren();

    // e.g. 'func text.print()' or 'func tokenizer.to<T type>()'
    if (head.is_generic()) {
        return std::make_unique<stmt_def_func_generic>(tc, src_loc_tk, head,
                                                       tz);
    }

    // e.g. 'func f(x)' or 'func list.add(x)'
    return std::make_unique<stmt_def_func>(tc, src_loc_tk, head, tz,
                                           std::nullopt);
}
