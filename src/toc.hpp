#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "access_path.hpp"
#include "compile_error.hpp"
#include "constant_parser.hpp"
#include "generics.hpp"
#include "ident_info.hpp"
#include "lut.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "token.hpp"
#include "type.hpp"
#include "ub_assigned.hpp"

//
// the table of contents: what the names of the program are while it is parsed
// and compiled, see 'etc/docs/README.md'. the classes below, in reading
// order:
//
//   records         'func_info', 'body_uses' (what a body reaches that its
//                   callers name too), 'param_binding' (what a parameter of
//                   an inlined call stands for), 'const_info', 'loop_reach'
//                   and 'func_reach' (which code the compile reaches,
//                   README 1.5.1)
//   'scope'         a block, a loop or a function being parsed or compiled:
//                   its constants, variables and parameter bindings; "frame"
//                   in 'scope' and 'storage_layout' is memory, e.g. the frame
//                   of a noinline call, while the register trace
//                   ('compile_toc::scope_entry', 'register_trace') and
//                   'call_frame' of an error use it for a call level
//   tables          'type_table', 'function_table'
//   'data_table'    the dats ('placed_dat') and the read-only constants
//   'scope_stack'   the scopes from the globals to the innermost scope and
//                   'lookup', the one rule of what a name means
//                   ('name_meaning'), the target of 'break'
//                   ('innermost_loop')
//   'storage_layout' where variables are placed and how much storage is used
//   'builtin_roles' the types the front end needs by role
//   name errors     'throw_invalid_name', 'throw_unparsed_number' and
//                   'throw_if_ends_with_semicolon' before them
//   'ident_resolver' resolves one name by 'lookup'
//   'definition_checks' the errors of a name defined twice
//   'toc_tables'    owns all of the above and 'generic_registry',
//                   'definition_order' ('generics.hpp') and
//                   'source_locations' ('source_files.hpp'), held by
//                   'program'
//   'toc'           the context every node receives, over the tables: what
//                   both phases do; a method of a context changes state or
//                   combines tables, a query of one table goes through its
//                   const accessor, e.g. 'tc.scopes().current_func_scope()';
//                   a table without an accessor is asked through the
//                   context: 'where', 'source_of' (the source locations),
//                   'current_definition' (the definition order),
//                   'resolve_declared', 'storage_of' (the resolver)
//   'parse_toc'     the context of the parse: declarations without storage,
//                   definitions, generic instances
//   records         'noinline_body', 'noinline_body_list' (the noinline
//                   bodies to emit or only to check), 'usage_statistics'
//   'compile_toc'   the context of the compile: placed declarations, the
//                   listing indent, reachability, dry runs, versions,
//                   noinline bodies
//   generics        'resolve_generic_args' and 'parse_toc::generic_scope',
//                   the names of the parameters of a generic instance;
//                   'generic_reparse', the definition parsed again for an
//                   instance
//

class stmt_def_dat;
class stmt_def_func;

// a function of the function table: its definition, the generic method it
// is an instance of and its place in the definition order
struct func_info {
    token src_loc_tk;           // the definition
    const stmt_def_func* def{}; // never null
    // e.g. 'text.size' for the instance 'str.size', empty if not an instance
    // of a method of a generic type
    std::string generic_method;
    // see 'definition_order', 0 for a method, a constructor or an instance
    size_t definition{};
};

// whether a use of a variable reads or writes it, see 'parse_toc::record_use'
enum class use_kind : uint8_t { read, write };

// what a function body reaches that its callers name too: global variables
// and data, its parameters and its result, each with the path below it and
// whether it is written, also through the functions it calls; a call compares
// what the callee writes through a parameter or the result with the globals
// it uses, and what it writes to a global with what it reaches through a
// parameter or the result, see 'stmt_call::assert_no_shared_storage'
class body_uses final {
  public:
    // e.g. 'entities' and '.array[?].messages' of 'entities.array[e].messages'
    struct global_use {
        storage_path storage{};
        bool is_written{};
    };

    // e.g. parameter 0 and '.messages.len' of 'self.messages.len = 0'
    struct binding_use {
        // empty for the result
        std::optional<size_t> param_index;
        access_path path{};
        bool is_written{};
    };

  private:
    // in the order of first use, one per path
    std::vector<global_use> globals_;
    std::vector<binding_use> bindings_;
    // the result is passed to a 'mut' parameter, so an inlined call keeps it
    // in memory, see 'inline_call::compile_in_block'
    bool is_result_passed_as_mut_{};

  public:
    auto add_binding(const std::optional<size_t> param_index,
                     const access_path& path, const bool is_written) -> void {

        for (binding_use& u : bindings_) {
            if (u.param_index == param_index and u.path.is_same(path)) {
                u.is_written = u.is_written or is_written;
                return;
            }
        }

        bindings_.push_back({
            .param_index{param_index},
            .path{path},
            .is_written{is_written},
        });
    }

    auto add_global(const storage_path& storage, const bool is_written)
        -> void {

        for (global_use& u : globals_) {
            if (u.storage.root.is_same(storage.root) and
                u.storage.path.is_same(storage.path)) {

                u.is_written = u.is_written or is_written;
                return;
            }
        }

        globals_.push_back({
            .storage{storage},
            .is_written{is_written},
        });
    }

    [[nodiscard]] auto bindings() const -> std::span<const binding_use> {
        return bindings_;
    }

    [[nodiscard]] auto globals() const -> std::span<const global_use> {
        return globals_;
    }

    [[nodiscard]] auto is_result_passed_as_mut() const -> bool {
        return is_result_passed_as_mut_;
    }

    auto mark_result_passed_as_mut() -> void {
        is_result_passed_as_mut_ = true;
    }
};

// what the name of a parameter or of the result of an inlined call stands
// for in its body: a constant, e.g. 'x' of 'f(3)', which decides the body at
// compile time; a register, e.g. 'x' of 'f(a + 1)'; or a place of the caller,
// e.g. 'p' of 'f(world.rooms[i])' at the address the call computed
struct param_binding {
    std::string from;
    ident_info info;
    // what the parameter stands for, as the comments show it
    std::string text;

    //
    // statics
    //

    [[nodiscard]] static auto make_constant(const std::string_view name,
                                            const type& param_type,
                                            const int64_t value)
        -> param_binding {

        // note: only a built-in parameter takes a constant
        assert(param_type.is_builtin());

        return {
            .from{std::string{name}},
            .info{
                ident_info::make_const(param_type,
                                       {
                                           .value{value},
                                           .own_type{&param_type},
                                       }),
            },
            .text{std::format("{}", value)},
        };
    }

    [[nodiscard]] static auto make_name(const std::string_view name,
                                        const ident_info& info,
                                        const std::string_view text)
        -> param_binding {

        return {
            .from{std::string{name}},
            .info{info},
            .text{std::string{text}},
        };
    }

    // 'text' is how the comments show it, the register or the name that
    // held it
    [[nodiscard]] static auto make_register(const std::string_view name,
                                            const operand& reg,
                                            const std::string_view text)
        -> param_binding {

        return {
            .from{std::string{name}},
            .info{ident_info::make_register(reg)},
            .text{std::string{text}},
        };
    }
};

// a constant of a scope: a 'let' that folds, the 'n' of a 'foo' or a
// constant parameter of a generic instance
struct const_info {
    token src_loc_tk; // the definition
    // typed when the initializer of a 'let' is or its type is not the
    // default type, e.g. 'let v = int(5)' or 'let b = true'
    folded_value constant{};
};

// what a variable is declared by: 'var', 'dat' or the signature of a function,
// a parameter or the result
enum class var_kind : uint8_t { var, dat, param, result };

// whether the end of a loop and its counter step are reached by a jump, a
// 'break' or a 'continue' of it was compiled; read by 'compile_toc::join_loop'
struct loop_reach {
    bool is_left_by_break{};
    bool is_continued{};
};

// while a function body compiles: the code after the current statement runs on
// some path ('compile_toc::is_reachable'), and a 'return' of it was compiled,
// which jumps to the end of an inlined call ('compile_toc::pop_scope')
struct func_reach {
    bool is_reachable{true};
    bool is_left_by_return{};

    // the end of the body of an inlined call is reached, by its last
    // statement or by a 'return'
    [[nodiscard]] auto is_end_reached() const -> bool {
        return is_reachable or is_left_by_return;
    }
};

// a block, a loop or a function being parsed or compiled; the static
// functions make each kind, e.g. 'scope::block()'
class scope final {
    // a function scope is made by 'main_func', 'inlined_func' or
    // 'noinline_func'
    enum class func_kind : uint8_t { main, inlined, noinline };

    // what only a function scope has
    struct function_part {
        func_kind kind{};

        // a unique path of source locations of the inlined call
        std::string call_path;

        // the label to jump to when exiting an inlined function
        std::string ret_label;

        // the parameters and the result of an inlined call, see
        // 'param_binding'
        lut<param_binding> bindings;

        func_reach reach;

        // compiled without a call, so what it folds is no constant of a call:
        // 'main', a noinline body without array parameters and the body check
        // of 'stmt_def_func::check_body', see 'compile_toc::check_skipped'
        bool is_without_call{};
        // of such a body or of a checked body with a length unknown: a call
        // would give it what the check does not have, e.g. the length of an
        // unsized array, so the constants that end a recursion are unknown,
        // see 'stmt_def_func::check_body' and 'inline_call::check_recursion'
        bool takes_from_call{};

        // the name of the result, empty when there is none
        std::string result_name;

        // the parameters, in order
        std::vector<std::string> param_names;

        // while the body is parsed: what its callers name too
        body_uses uses;
    };

    // what only a loop scope has
    struct loop_part {
        // how its end is reached and where a 'continue' jumps, e.g. the head
        // of a 'loop' or the counter step of a 'foo'
        loop_end end{loop_end::at_break};
        std::string continue_label;
        loop_reach reach;
    };

    // the function name or the loop label, empty for a block
    std::string_view name_;

    // the bytes of variable storage this scope uses
    size_t allocated_storage_size_bytes_{};

    lut<const_info> consts_;

    // variables declared in this scope
    lut<var_info> vars_;

    std::optional<loop_part> loop_;
    std::optional<function_part> func_;

    // the block of a variable while its initializer is parsed, see
    // 'scope::initializer'
    bool is_initializer_{};

  public:
    auto add_binding(const param_binding& binding) -> void {
        func_part().bindings.put(std::string{binding.from}, binding);
    }

    auto add_const(const std::string_view name, const const_info& ci) -> void {
        consts_.put(std::string{name}, ci);
    }

    // the bytes that 'storage_layout::allocate_var' placed in this scope
    auto add_storage_bytes(const size_t size_bytes) -> void {
        allocated_storage_size_bytes_ =
            sum_storage_size(allocated_storage_size_bytes_, size_bytes);
    }

    // returns the stored copy of 'var'
    [[nodiscard]] auto add_var(const var_info& var, const var_kind kind)
        -> var_info& {

        vars_.put(var.name, var);

        if (kind == var_kind::result) {
            func_part().result_name = var.name;
        }

        if (kind == var_kind::param) {
            func_part().param_names.emplace_back(var.name);
        }

        return vars_.get_ref(var.name);
    }

    [[nodiscard]] auto allocated_storage_size_bytes() const -> size_t {
        return allocated_storage_size_bytes_;
    }

    [[nodiscard]] auto body_uses_of_func() const -> const body_uses& {
        return func_part().uses;
    }

    [[nodiscard]] auto body_uses_of_func() -> body_uses& {
        return func_part().uses;
    }

    [[nodiscard]] auto call_path() const -> std::string_view {
        return func_part().call_path;
    }

    [[nodiscard]] auto continue_label() const -> std::string_view {
        return loop().continue_label;
    }

    [[nodiscard]] auto func_ret_label() const -> std::string_view {
        return func_part().ret_label;
    }

    [[nodiscard]] auto get_binding(const std::string_view name) const
        -> const param_binding& {

        return func_part().bindings.get_const_ref(name);
    }

    [[nodiscard]] auto get_const(const std::string_view name) const
        -> const const_info& {

        return consts_.get_const_ref(name);
    }

    [[nodiscard]] auto get_var(const std::string_view name) const
        -> const var_info& {

        return vars_.get_const_ref(name);
    }

    [[nodiscard]] auto has_binding(const std::string_view name) const -> bool {
        return func_ and func_->bindings.has(name);
    }

    [[nodiscard]] auto has_const(const std::string_view name) const -> bool {
        return consts_.has(name);
    }

    [[nodiscard]] auto has_frame() const -> bool {
        return func_ and func_->kind == func_kind::noinline;
    }

    [[nodiscard]] auto has_var(const std::string_view name) const -> bool {
        return vars_.has(name);
    }

    [[nodiscard]] auto is_block() const -> bool {
        return not func_ and not loop_;
    }

    [[nodiscard]] auto is_func() const -> bool { return func_.has_value(); }

    [[nodiscard]] auto is_initializer() const -> bool {
        return is_initializer_;
    }

    // only a call inlined; 'main' is compiled in place too but is not one
    [[nodiscard]] auto is_inlined_func() const -> bool {
        return func_ and func_->kind == func_kind::inlined;
    }

    [[nodiscard]] auto is_loop() const -> bool { return loop_.has_value(); }

    [[nodiscard]] auto is_main() const -> bool {
        return func_ and func_->kind == func_kind::main;
    }

    [[nodiscard]] auto is_name(const std::string_view name) const -> bool {
        return name_ == name;
    }

    [[nodiscard]] auto is_result(const std::string_view name) const -> bool {
        return func_part().result_name == name;
    }

    [[nodiscard]] auto is_without_call() const -> bool {
        return func_part().is_without_call;
    }

    [[nodiscard]] auto loop_end_kind() const -> loop_end { return loop().end; }

    auto make_var_read_only(const std::string_view name,
                            const read_only_cause cause) -> void {

        vars_.get_ref(name).read_only_why = cause;
    }

    auto mark_taking_from_call() -> void { func_part().takes_from_call = true; }

    auto mark_without_call() -> void { func_part().is_without_call = true; }

    [[nodiscard]] auto name() const -> std::string_view { return name_; }

    // the index of the parameter 'name'
    [[nodiscard]] auto param_index(const std::string_view name) const
        -> size_t {

        const std::vector<std::string>& names{func_part().param_names};

        for (size_t i{}; i < names.size(); ++i) {
            if (names.at(i) == name) {
                return i;
            }
        }

        std::unreachable();
    }

    [[nodiscard]] auto reach_of_func() -> func_reach& {
        return func_part().reach;
    }

    [[nodiscard]] auto reach_of_func() const -> const func_reach& {
        return func_part().reach;
    }

    [[nodiscard]] auto reach_of_loop() -> loop_reach& { return loop().reach; }

    [[nodiscard]] auto reach_of_loop() const -> const loop_reach& {
        return loop().reach;
    }

    [[nodiscard]] auto takes_from_call() const -> bool {
        return func_part().takes_from_call;
    }

    // what the name 'name' of this scope is in a message: a function scope
    // declares only the parameters, the result and the constants of the
    // generic type of a method, a loop scope only the 'e', 'i' and 'n' of a
    // 'foo'
    [[nodiscard]] auto var_role(const std::string_view name) const
        -> std::string_view {

        if (is_loop()) {
            if (name == reserved_names::foo_counter) {
                return "index";
            }

            return name == reserved_names::foo_length ? "length" : "element";
        }

        if (not is_func()) {
            return "variable";
        }

        if (has_const(name)) {
            return "generic parameter";
        }

        return is_result(name) ? "result" : "parameter";
    }

    //
    // statics
    //

    [[nodiscard]] static auto block() -> scope { return scope{""}; }

    // a block that declares a variable while its initializer is parsed, so a
    // name in it resolves to the variable, e.g. the 'x' of 'var x = x + 1',
    // which is not set yet
    [[nodiscard]] static auto initializer() -> scope {
        scope scp{""};

        scp.is_initializer_ = true;

        return scp;
    }

    // a function compiled in place, 'call_path' keeps its labels unique and
    // 'return' jumps to 'return_label'
    [[nodiscard]] static auto inlined_func(const std::string_view name,
                                           std::string call_path,
                                           std::string return_label) -> scope {

        return function(name, func_kind::inlined, std::move(call_path),
                        std::move(return_label));
    }

    // the body of a loop named by its label, a 'continue' jumps to
    // 'continue_label'
    [[nodiscard]] static auto loop(const std::string_view name,
                                   const loop_end end,
                                   std::string continue_label) -> scope {

        scope scp{name};

        scp.loop_ = loop_part{
            .end{end},
            .continue_label{std::move(continue_label)},
            .reach{loop_reach{}},
        };

        return scp;
    }

    // 'main' is compiled in place, 'return' exits the program
    [[nodiscard]] static auto main_func() -> scope {
        scope scp{
            function(reserved_names::main, func_kind::main, {}, {}),
        };

        scp.mark_without_call();

        return scp;
    }

    // a function with a body of its own, its variables are placed in its
    // frame, see 'machine::frame_base_register'
    [[nodiscard]] static auto noinline_func(const std::string_view name,
                                            std::string call_path) -> scope {

        return function(name, func_kind::noinline, std::move(call_path), {});
    }

  private:
    explicit scope(const std::string_view name) noexcept : name_{name} {}

    [[nodiscard]] auto func_part() -> function_part& {
        assert(func_);

        return *func_;
    }

    [[nodiscard]] auto func_part() const -> const function_part& {
        assert(func_);

        return *func_;
    }

    [[nodiscard]] auto loop() -> loop_part& {
        assert(loop_);

        return *loop_;
    }

    [[nodiscard]] auto loop() const -> const loop_part& {
        assert(loop_);

        return *loop_;
    }

    //
    // statics
    //

    [[nodiscard]] static auto function(const std::string_view name,
                                       const func_kind kind,
                                       std::string call_path,
                                       std::string return_label) -> scope {

        scope scp{name};

        scp.func_ = function_part{
            .kind{kind},
            .call_path{std::move(call_path)},
            .ret_label{std::move(return_label)},
            .bindings{},
            .reach{func_reach{}},
            .is_without_call{},
            .takes_from_call{},
            .result_name{},
            .param_names{},
            .uses{},
        };

        return scp;
    }
};

// the types by name. while an instance of a generic definition is parsed, its
// type parameters are names of types too, e.g. 'T' names 'i8'; only those of
// the innermost instance are visible
class type_table final {
    struct entry {
        token src_loc_tk;
        const type* type_ptr{};
        // see 'definition_order'
        size_t definition{};
        // the generic type of a method naming its instance, see
        // 'is_type_param'
        bool is_receiver{};
    };

    lut<entry> entries_;
    // the type parameters of the instances being parsed, the innermost last
    std::vector<lut<entry>> params_;
    std::reference_wrapper<const definition_order> order_;

  public:
    explicit type_table(const definition_order& order) : order_{order} {}

    auto add(const token& src_loc_tk, const type& tpe) -> void {
        entries_.put(std::string{tpe.name()},
                     {
                         .src_loc_tk{src_loc_tk},
                         .type_ptr{&tpe},
                         .definition{order_.get().current()},
                         .is_receiver{},
                     });
    }

    // a built-in type under 'name', e.g. 'i8', or 'int' for the default type
    auto add_builtin(const std::string_view name, const type& tpe) -> void {
        entries_.put(std::string{name}, {
                                            .src_loc_tk{},
                                            .type_ptr{&tpe},
                                            .definition{},
                                            .is_receiver{},
                                        });
    }

    // e.g. 'T' names 'i8' in an instance of 'func f<T type>()', 'name_tk'
    // is the parameter
    auto add_param(const token& name_tk, const type& tpe) -> void {
        put_param(name_tk, tpe, false);
    }

    // e.g. 'list' names 'str' in a method of 'str', 'type str = list<...>',
    // 'name_tk' is the name of the generic type
    auto add_receiver(const token& name_tk, const type& tpe) -> void {
        put_param(name_tk, tpe, true);
    }

    // the parameters of an enclosing instance are hidden until 'exit_params'
    auto enter_params() -> void { params_.emplace_back(); }

    // also a type that the current instance parse does not see, for the
    // checks of a name defined twice
    [[nodiscard]] auto exists(const std::string_view name) const -> bool {
        return is_param(name) or entries_.has(name);
    }

    auto exit_params() -> void { params_.pop_back(); }

    [[nodiscard]] auto get(const std::string_view name) const -> const type& {
        return *find(name).type_ptr;
    }

    // a type the current instance parse sees, see 'definition_order'
    [[nodiscard]] auto has(const std::string_view name) const -> bool {
        return is_param(name) or (entries_.has(name) and
                                  order_.get().is_visible(
                                      entries_.get_const_ref(name).definition));
    }

    // a built-in type name, no type parameter is named like one
    [[nodiscard]] auto is_builtin(const std::string_view name) const -> bool {
        return entries_.has(name) and
               entries_.get_const_ref(name).type_ptr->is_builtin();
    }

    // a type parameter of the innermost instance being parsed, not the
    // generic type of a method, which names its instance like a type name does
    [[nodiscard]] auto is_type_param(const std::string_view name) const
        -> bool {

        return is_param(name) and
               not params_.back().get_const_ref(name).is_receiver;
    }

    [[nodiscard]] auto src_loc_tk_of(const std::string_view name) const
        -> const token& {

        return find(name).src_loc_tk;
    }

  private:
    [[nodiscard]] auto find(const std::string_view name) const -> const entry& {
        if (is_param(name)) {
            return params_.back().get_const_ref(name);
        }

        return entries_.get_const_ref(name);
    }

    // a type parameter of the innermost instance being parsed
    [[nodiscard]] auto is_param(const std::string_view name) const -> bool {
        return not params_.empty() and params_.back().has(name);
    }

    auto put_param(const token& name_tk, const type& tpe,
                   const bool is_receiver) -> void {

        // note: it hides a type of that name, see 'find'

        assert(not is_param(name_tk.text()));

        params_.back().put(std::string{name_tk.text()},
                           {
                               .src_loc_tk{name_tk},
                               .type_ptr{&tpe},
                               .definition{},
                               .is_receiver{is_receiver},
                           });
    }
};

// the functions of the program: the defined ones and the instances of
// generic ones
class function_table final {
    lut<func_info> funcs_;
    std::vector<std::shared_ptr<const stmt_def_func>> instances_;
    std::reference_wrapper<const definition_order> order_;

  public:
    explicit function_table(const definition_order& order) : order_{order} {}

    // the name has been checked
    auto add(std::string name, func_info info) -> void {
        funcs_.put(std::move(name), std::move(info));
    }

    // keeps the instance alive, 'funcs_' refers to it
    auto add_instance(std::shared_ptr<const stmt_def_func> instance) -> void {
        instances_.emplace_back(std::move(instance));
    }

    // in the order they were added
    [[nodiscard]] auto defs() const -> auto {
        return funcs_.values() | std::views::transform(&func_info::def);
    }

    // also a function that the current instance parse does not see, for the
    // checks of a name defined twice
    [[nodiscard]] auto exists(const std::string_view name) const -> bool {
        return funcs_.has(name);
    }

    // the name is known
    [[nodiscard]] auto get(const std::string_view name) const
        -> const func_info& {

        return funcs_.get_const_ref(name);
    }

    // a function the current instance parse sees, see 'definition_order'
    [[nodiscard]] auto has(const std::string_view name) const -> bool {
        return funcs_.has(name) and
               order_.get().is_visible(funcs_.get_const_ref(name).definition);
    }
};

// a 'dat' at its place in the data section: 'padding_bytes' of zeros after the
// dat before it align it at 'offset_bytes' from the start of the section
struct placed_dat {
    const stmt_def_dat* def{};
    std::string name;
    size_t padding_bytes{};
    size_t offset_bytes{};
};

// the 'dat' declarations and the read-only constants of the program; the dats
// are placed here once, in declaration order, the storage and the data section
// read their offsets
class data_table final {
    std::vector<placed_dat> dats_;
    std::vector<machine::read_only_constant> constants_;
    size_t total_size_bytes_{};
    // the alignment of the data section of the machine and of the variables
    // after the dats
    size_t data_alignment_{};

  public:
    explicit data_table(const size_t data_alignment)
        : data_alignment_{data_alignment} {}

    // the same bytes share the label of the first constant added, e.g. the
    // strings '"A"' and '"\x41"'
    [[nodiscard]] auto add_constant(std::string label, std::string bytes)
        -> std::string {

        for (const machine::read_only_constant& constant : constants_) {
            if (constant.bytes == bytes) {
                return constant.label;
            }
        }

        constants_.push_back({
            .label{std::move(label)},
            .bytes{std::move(bytes)},
        });

        return constants_.back().label;
    }

    // the size errors are located at the name of the dat
    auto add_dat(const stmt_def_dat& dat, const token& name_tk,
                 const size_t alignment, const size_t size_bytes) -> void {

        const size_t padding_bytes{
            alignment_padding_bytes(total_size_bytes_, alignment),
        };

        const size_t offset_bytes{
            sum_storage_size(total_size_bytes_, padding_bytes),
        };

        total_size_bytes_ = add_storage_size(name_tk, offset_bytes, size_bytes);

        dats_.push_back({
            .def{&dat},
            .name{name_tk.text()},
            .padding_bytes{padding_bytes},
            .offset_bytes{offset_bytes},
        });
    }

    [[nodiscard]] auto constant_count() const -> size_t {
        return constants_.size();
    }

    [[nodiscard]] auto constants() const
        -> std::span<const machine::read_only_constant> {

        return constants_;
    }

    // the padding after the dats that aligns the variables
    [[nodiscard]] auto dat_var_padding_bytes() const -> size_t {
        return alignment_padding_bytes(total_size_bytes_, data_alignment_);
    }

    [[nodiscard]] auto dats() const -> std::span<const placed_dat> {
        return dats_;
    }

    // the name is a dat
    [[nodiscard]] auto get_dat(const std::string_view name) const
        -> const placed_dat& {

        for (const placed_dat& dat : dats_) {
            if (dat.name == name) {
                return dat;
            }
        }

        std::unreachable();
    }

    // drops the constants added after the first 'count'
    auto resize_constants(const size_t count) -> void {
        constants_.resize(count);
    }

    [[nodiscard]] auto total_size_bytes() const -> size_t {
        return total_size_bytes_;
    }
};

// what a name means where it is used, see 'scope_stack::lookup'
struct name_meaning {
    enum class kind : uint8_t { none, variable, binding, constant };

    kind what{kind::none};
    // the scope that declares the name
    size_t scope_index{};

    [[nodiscard]] auto is_binding() const -> bool {
        return what == kind::binding;
    }

    [[nodiscard]] auto is_constant() const -> bool {
        return what == kind::constant;
    }

    // a name that has a value hides a type or a generic function of that name,
    // a call 'f()' still calls the function
    [[nodiscard]] auto is_value() const -> bool { return what != kind::none; }

    [[nodiscard]] auto is_variable() const -> bool {
        return what == kind::variable;
    }
};

// the scopes being parsed or compiled, from the root scope that holds the
// globals to the innermost one
class scope_stack final {
    std::vector<scope> scopes_;
    size_t max_scope_count_{};
    std::reference_wrapper<const definition_order> order_;
    // the definition of each global variable, data and constant, see
    // 'definition_order'
    std::map<std::string, size_t, std::less<>> global_definitions_;

  public:
    // whether the code after the current statement is reached, saved around
    // code compiled only for its checks, see 'compile_toc::check_skipped': a
    // 'break' or a 'return' there must not end the paths of the code that runs
    struct reach_state {
        // the innermost loop, the one a 'break' or a 'continue' of skipped
        // code marks
        std::optional<size_t> loop_index;
        loop_reach loop;
        func_reach func;
    };

    explicit scope_stack(const definition_order& order) : order_{order} {}

    [[nodiscard]] auto all() const -> const std::vector<scope>& {
        return scopes_;
    }

    // checked at the parse, which reaches the branches that are not compiled,
    // e.g. the 'break' of 'if 1 == 2 { break }'
    auto assert_in_loop(const token& src_loc_tk) const -> void {
        if (innermost_loop() == nullptr) {
            throw compile_error{src_loc_tk,
                                std::format("{} is not in a loop",
                                            in_quotes(src_loc_tk.text()))};
        }
    }

    [[nodiscard]] auto at(const size_t index) const -> const scope& {
        return scopes_.at(index);
    }

    [[nodiscard]] auto back() -> scope& { return scopes_.back(); }

    [[nodiscard]] auto back() const -> const scope& { return scopes_.back(); }

    // empty outside of a function, e.g. in the initializer of a global variable
    [[nodiscard]] auto call_path() const -> std::string_view {
        if (not is_in_func()) {
            return {};
        }

        return current_func_scope().call_path();
    }

    // the value of 'name' when it means a constant
    [[nodiscard]] auto const_value(const std::string_view name) const
        -> std::optional<folded_value> {

        const name_meaning meaning{lookup(name)};

        if (not meaning.is_constant()) {
            return std::nullopt;
        }

        return scopes_.at(meaning.scope_index).get_const(name).constant;
    }

    [[nodiscard]] auto count() const -> size_t { return scopes_.size(); }

    // blocks and loops belong to the function scope below them
    [[nodiscard]] auto current_func_scope() const -> const scope& {
        return scopes_.at(func_scope_index());
    }

    // how many times the inlined function 'name' is being compiled in place
    // by the current function body, inlined bodies are searched down to the
    // nearest function with a body of its own; that is the inlined function
    // itself in the check of its body without its calls, where a call of it
    // is a recursion ('stmt_def_func::check_body')
    [[nodiscard]] auto inlined_nesting(const std::string_view name) const
        -> size_t {

        size_t count{};

        for (const scope& scp : scopes_ | std::views::reverse) {
            if (not scp.is_func()) {
                continue;
            }

            if (scp.is_name(name)) {
                ++count;
            }

            if (scp.has_frame()) {
                break;
            }
        }

        return count;
    }

    // the innermost 'loop' or 'foo' of the function, the target of a 'break'
    // or a 'continue'; null outside a loop
    [[nodiscard]] auto innermost_loop() const -> const scope* {
        const std::optional<size_t> index{innermost_loop_index()};

        if (not index) {
            return nullptr;
        }

        return &scopes_.at(*index);
    }

    // only the scope of the globals is open
    [[nodiscard]] auto is_at_root() const -> bool {
        return scopes_.size() == 1;
    }

    [[nodiscard]] auto is_empty() const -> bool { return scopes_.empty(); }

    // a check of a body that takes from a call is in the stack, see
    // 'stmt_def_func::check_body': a recursive inlined call is expanded in a
    // dry run or not at all ('inline_call::check_recursion')
    [[nodiscard]] auto is_in_body_taking_from_call() const -> bool {
        return std::ranges::any_of(scopes_, [](const scope& scp) -> bool {
            return scp.is_func() and scp.takes_from_call();
        });
    }

    // the initializer of a global variable is outside of any function
    [[nodiscard]] auto is_in_func() const -> bool {
        return std::ranges::any_of(
            scopes_, [](const scope& scp) -> bool { return scp.is_func(); });
    }

    // what 'name' means in the innermost scope, the one rule of what a name
    // means: the scopes of the innermost function from the innermost out,
    // then the root scope. a scope declares a variable or a constant of a
    // name, not both; in a function scope a parameter hides a constant of the
    // generic instance, which is the context of the definition
    [[nodiscard]] auto lookup(const std::string_view name) const
        -> name_meaning {

        // the root is checked after the loop, where the order of the
        // definitions applies
        for (const size_t index : std::views::iota(size_t{1}, scopes_.size()) |
                                      std::views::reverse) {

            const name_meaning meaning{meaning_in(index, name)};

            if (meaning.is_value()) {
                return meaning;
            }

            if (scopes_.at(index).is_func()) {
                break;
            }
        }

        // e.g. a global defined after the generic whose instance is parsed
        if (not is_global_visible(name)) {
            return {};
        }

        return meaning_in(0, name);
    }

    [[nodiscard]] auto max_scope_count() const -> size_t {
        return max_scope_count_;
    }

    // 'name' means the result of the innermost function, not a name that
    // hides it, e.g. the 'e' of a 'foo' over the result
    [[nodiscard]] auto names_result(const std::string_view name) const -> bool {
        if (not is_in_func()) {
            return false;
        }

        const name_meaning meaning{lookup(name)};

        return meaning.is_variable() and
               meaning.scope_index == func_scope_index() and
               current_func_scope().is_result(name);
    }

    auto pop() -> void { scopes_.pop_back(); }

    auto push(scope scp) -> void {
        scopes_.push_back(std::move(scp));
        max_scope_count_ = std::max(scopes_.size(), max_scope_count_);
    }

    [[nodiscard]] auto reach_state_now() const -> reach_state {
        // see 'compile_toc::is_reachable'
        if (not is_in_func()) {
            return {};
        }

        const std::optional<size_t> loop_index{innermost_loop_index()};

        loop_reach loop{};

        if (loop_index) {
            loop = scopes_.at(*loop_index).reach_of_loop();
        }

        return {
            .loop_index{loop_index},
            .loop{loop},
            .func{current_func_scope().reach_of_func()},
        };
    }

    // a 'break' of the innermost 'loop' or 'foo' is compiled, see
    // 'loop_reach'
    auto record_break() -> void {
        innermost_loop_ref().reach_of_loop().is_left_by_break = true;
    }

    // a 'continue' of the innermost 'loop' or 'foo' is compiled, see
    // 'loop_reach'
    auto record_continue() -> void {
        innermost_loop_ref().reach_of_loop().is_continued = true;
    }

    // a variable, data or constant declared in the root scope keeps the
    // definition that declared it at the parse, the compile declares it again
    auto record_if_global(const std::string_view name) -> void {
        if (not is_at_root()) {
            return;
        }

        global_definitions_.try_emplace(std::string{name},
                                        order_.get().current());
    }

    // the body being parsed passes its result to a 'mut' parameter
    auto record_result_passed_as_mut() -> void {
        func_scope().body_uses_of_func().mark_result_passed_as_mut();
    }

    // a 'return' of the innermost function is compiled, see
    // 'func_reach'
    auto record_return() -> void {
        func_scope().reach_of_func().is_left_by_return = true;
    }

    // records 'storage' when its root is a global, a parameter or the result
    // of the body being parsed, see 'body_uses'; a local of the body is not
    // named by the callers
    auto record_storage_use(const storage_path& storage, const bool is_written)
        -> void {

        scope& func{func_scope()};

        if (storage.root.is_global()) {
            func.body_uses_of_func().add_global(storage, is_written);
            return;
        }

        if (storage.root.scope_index != func_scope_index()) {
            return;
        }

        if (func.is_result(storage.root.name)) {
            func.body_uses_of_func().add_binding(std::nullopt, storage.path,
                                                 is_written);

            return;
        }

        // note: at the parse a function scope declares only its parameters
        //       and its result ('add_signature_vars')

        func.body_uses_of_func().add_binding(
            func.param_index(storage.root.name), storage.path, is_written);
    }

    auto restore_reach_state(const reach_state& state) -> void {
        if (not is_in_func()) {
            return;
        }

        if (state.loop_index) {
            scopes_.at(*state.loop_index).reach_of_loop() = state.loop;
        }

        func_scope().reach_of_func() = state.func;
    }

    auto set_max_scope_count(const size_t count) -> void {
        max_scope_count_ = count;
    }

    // see 'compile_toc::is_reachable'
    auto set_reachable(const bool is_reachable) -> void {
        if (not is_in_func()) {
            return;
        }

        func_scope().reach_of_func().is_reachable = is_reachable;
    }

  private:
    [[nodiscard]] auto func_scope() -> scope& {
        return scopes_.at(func_scope_index());
    }

    [[nodiscard]] auto func_scope_index() const -> size_t {
        for (size_t i{scopes_.size()}; i > 0; --i) {
            if (scopes_.at(i - 1).is_func()) {
                return i - 1;
            }
        }

        std::unreachable();
    }

    // the index of the innermost 'loop' or 'foo' of the function
    [[nodiscard]] auto innermost_loop_index() const -> std::optional<size_t> {
        for (size_t i{scopes_.size()}; i > 0; --i) {
            const scope& scp{scopes_.at(i - 1)};

            if (scp.is_loop()) {
                return i - 1;
            }

            if (scp.is_func()) {
                return std::nullopt;
            }
        }

        std::unreachable();
    }

    [[nodiscard]] auto innermost_loop_ref() -> scope& {
        const std::optional<size_t> index{innermost_loop_index()};

        // note: the parse rejected a 'break' or a 'continue' outside a loop
        assert(index);

        return scopes_.at(*index);
    }

    [[nodiscard]] auto is_global_visible(const std::string_view name) const
        -> bool {

        const auto it{global_definitions_.find(name)};

        return it == global_definitions_.end() or
               order_.get().is_visible(it->second);
    }

    // what 'name' means in the scope 'index' alone, none when it does not
    // declare it
    [[nodiscard]] auto meaning_in(const size_t index,
                                  const std::string_view name) const
        -> name_meaning {

        const scope& scp{scopes_.at(index)};

        if (scp.has_var(name)) {
            return {
                .what{name_meaning::kind::variable},
                .scope_index{index},
            };
        }

        if (scp.has_binding(name)) {
            return {
                .what{name_meaning::kind::binding},
                .scope_index{index},
            };
        }

        if (scp.has_const(name)) {
            return {
                .what{name_meaning::kind::constant},
                .scope_index{index},
            };
        }

        return {};
    }
};

// where the variables are placed, after the dats that 'data_table' placed: in
// the frame of the nearest scope that has one, else after the variables in
// use, within the '--vars' capacity and what the target addresses; the size
// in use is the sum of what the scopes hold
class storage_layout final {
    // where the next variable is placed: 'next_offset_bytes' counts from the
    // base of the frame of the nearest scope with one or, outside a frame,
    // from the label 'dat', the start of the data section
    struct storage_location {
        bool is_in_frame{};
        size_t next_offset_bytes{};
    };

    std::reference_wrapper<const ::machine> machine_;
    std::reference_wrapper<scope_stack> scopes_;
    std::reference_wrapper<const data_table> data_;
    size_t vars_capacity_bytes_{};
    // the most bytes of variables in use while one body is compiled: the
    // globals with 'main' or with one noinline body, its inlined calls
    // included; the frames of noinline calls stack at run time, which
    // '--checks=frame' checks
    size_t max_body_vars_size_bytes_{};
    // the most bytes of variables in the frame being compiled, see
    // 'begin_frame'
    size_t max_frame_size_bytes_{};

  public:
    // the peaks that a dry run or a version of
    // 'compile_toc::emit_most_efficient' raises and 'restore_peaks' puts back
    struct peaks {
        size_t max_body_vars_size_bytes{};
        size_t max_frame_size_bytes{};
    };

    storage_layout(const ::machine& backend, scope_stack& scopes,
                   const data_table& data, const size_t vars_capacity_bytes)
        : machine_{backend}, scopes_{scopes}, data_{data},
          vars_capacity_bytes_{vars_capacity_bytes} {

        // the option is wrong, not the source; the variables stay below the
        // end of the address space like the data and variables in all
        if (vars_capacity_bytes >= backend.max_storage_bytes()) {
            throw option_error{std::format(
                "invalid --vars: {} B reaches the {} B that the target "
                "addresses",
                vars_capacity_bytes, backend.max_storage_bytes())};
        }
    }

    // places 'var' after the storage in use of the nearest scope with a
    // frame, else after the variables; its storage is set and the innermost
    // scope holds the bytes it takes with the alignment padding
    auto allocate_var(const token& src_loc_tk, var_info& var) -> void {
        const size_t var_size_bytes{var_storage_size_bytes(src_loc_tk, var)};
        const size_t var_alignment{var_storage_alignment(var)};

        // note: a frame base and 'dat' are both aligned
        const storage_location location{find_storage_location()};
        const size_t next_offset_bytes{location.next_offset_bytes};

        const size_t padding_bytes{
            alignment_padding_bytes(next_offset_bytes, var_alignment),
        };

        const size_t allocated_size_bytes{
            add_storage_size(src_loc_tk, padding_bytes, var_size_bytes),
        };

        assert_vars_capacity(src_loc_tk, var, allocated_size_bytes);

        const size_t total_bytes{
            add_storage_size(src_loc_tk, dats_and_vars_size_bytes(),
                             allocated_size_bytes),
        };

        assert_total_fits_target(src_loc_tk, total_bytes);

        in_storage& stored{std::get<in_storage>(var.placement)};

        const size_t offset_bytes{next_offset_bytes + padding_bytes};

        if (location.is_in_frame) {
            stored.offset = address_offset(offset_bytes);

            const ::machine& x{machine_.get()};

            stored.base_register = x.frame_base_register();

            // note: the frame is within the data and variables checked above

            max_frame_size_bytes_ = std::max(
                sum_storage_size(next_offset_bytes, allocated_size_bytes),
                max_frame_size_bytes_);
        } else {
            stored.offset = offset_from_variables_base(offset_bytes);
        }

        scopes_.get().back().add_storage_bytes(allocated_size_bytes);

        // note: a dry run puts the peak back ('restore_peaks')
        max_body_vars_size_bytes_ =
            std::max(used_vars_size_bytes(), max_body_vars_size_bytes_);
    }

    // the dats, then with the padding after them the whole '--vars' capacity,
    // which the image reserves, reach no further than the target addresses
    auto assert_dats_and_vars_fit_target(const token& src_loc_tk) const
        -> void {

        assert_total_fits_target(src_loc_tk, data_.get().total_size_bytes());

        assert_total_fits_target(
            src_loc_tk,
            sum_storage_size(vars_start_bytes(), vars_capacity_bytes_));
    }

    // a scope with a frame is compiled; the frames do not nest, a noinline
    // body is compiled after the body that calls it
    auto begin_frame() -> void {
        assert(not find_storage_location().is_in_frame);

        max_frame_size_bytes_ = 0;
    }

    [[nodiscard]] auto max_body_vars_size_bytes() const -> size_t {
        return max_body_vars_size_bytes_;
    }

    // asked inside a noinline body, which has a frame
    [[nodiscard]] auto max_frame_size_bytes() const -> size_t {
        assert(find_storage_location().is_in_frame);

        return max_frame_size_bytes_;
    }

    // a callee frame starts aligned for any variable it holds
    [[nodiscard]] auto next_frame_address() const -> operand {
        const ::machine& x{machine_.get()};

        const size_t frame_alignment{x.address_size_bytes()};

        const storage_location location{find_storage_location()};

        const size_t aligned_offset_bytes{
            align_storage_size(location.next_offset_bytes, frame_alignment),
        };

        if (location.is_in_frame) {
            return operand::mem(x.frame_base_register(), {}, 1,
                                address_offset(aligned_offset_bytes),
                                x.default_type());
        }

        return operand::mem(x.variables_base_register(), {}, 1,
                            offset_from_variables_base(aligned_offset_bytes),
                            x.default_type());
    }

    [[nodiscard]] auto peaks_now() const -> peaks {
        return {
            .max_body_vars_size_bytes{max_body_vars_size_bytes_},
            .max_frame_size_bytes{max_frame_size_bytes_},
        };
    }

    // the storage of the dat 'var' is where the data table placed it, before
    // the variables; its offset is set, the scopes hold none of its bytes
    auto place_dat(const token& src_loc_tk, var_info& var) const -> void {
        const placed_dat& dat{data_.get().get_dat(var.name)};

        const ::machine& x{machine_.get()};

        // note: 'parse_toc::add_dat' checked the dats against the target
        assert(dat.offset_bytes + var_storage_size_bytes(src_loc_tk, var) <
               x.max_storage_bytes());

        std::get<in_storage>(var.placement).offset =
            offset_from_variables_base(dat.offset_bytes);
    }

    auto restore_peaks(const peaks& saved) -> void {
        max_body_vars_size_bytes_ = saved.max_body_vars_size_bytes;
        max_frame_size_bytes_ = saved.max_frame_size_bytes;
    }

    [[nodiscard]] auto var_storage_alignment(const var_info& var) const
        -> size_t {

        if (holds_address(var.placement)) {
            const ::machine& x{machine_.get()};
            return x.address_size_bytes();
        }

        return var.type_ptr->alignment();
    }

    [[nodiscard]] auto var_storage_size_bytes(const token& src_loc_tk,
                                              const var_info& var) const
        -> size_t {

        if (holds_address(var.placement)) {
            const ::machine& x{machine_.get()};
            return x.address_size_bytes();
        }

        return var.shape.value_size_bytes(src_loc_tk, *var.type_ptr);
    }

    [[nodiscard]] auto vars_capacity_bytes() const -> size_t {
        return vars_capacity_bytes_;
    }

  private:
    // the declaration that makes the data and variables reach what the target
    // addresses is the error, not the end of the build; checked before the
    // scope holds the bytes
    auto assert_total_fits_target(const token& src_loc_tk,
                                  const size_t total_bytes) const -> void {

        const ::machine& x{machine_.get()};

        const size_t max_bytes{x.max_storage_bytes()};

        // note: below, the code needs room too and a type stays below the
        //       end of the address space

        if (total_bytes < max_bytes) {
            return;
        }

        throw compile_error{
            src_loc_tk,
            std::format("data and variables of {} B reach the {} B that the "
                        "target addresses",
                        total_bytes, max_bytes)};
    }

    auto assert_vars_capacity(const token& src_loc_tk, const var_info& var,
                              const size_t allocated_size_bytes) const -> void {

        if (allocated_size_bytes <=
            vars_capacity_bytes_ - used_vars_size_bytes()) {

            return;
        }

        throw vars_capacity_error(src_loc_tk, var);
    }

    // the dats, the padding after them and the variables of every scope
    [[nodiscard]] auto dats_and_vars_size_bytes() const -> size_t {
        return sum_storage_size(vars_start_bytes(), used_vars_size_bytes());
    }

    // a local starts after the storage in use of the nearest scope with a
    // frame and of the scopes inside it, any other variable after the dats
    // and the variables in use
    [[nodiscard]] auto find_storage_location() const -> storage_location {
        const std::vector<scope>& scopes{scopes_.get().all()};
        size_t local_size_bytes{};

        for (size_t i{scopes.size()}; i != 0; --i) {
            const scope& scp{scopes.at(i - 1)};

            local_size_bytes = sum_storage_size(
                local_size_bytes, scp.allocated_storage_size_bytes());

            if (scp.has_frame()) {
                return {
                    .is_in_frame{true},
                    .next_offset_bytes{local_size_bytes},
                };
            }
        }

        return {
            .is_in_frame{},
            .next_offset_bytes{dats_and_vars_size_bytes()},
        };
    }

    // the operand offset of storage 'offset_from_dat' bytes past the label
    // 'dat': the variables base is 'dat' unless the target puts it past the
    // label 'vars', which makes the dats negative and the variables start
    // below the base
    [[nodiscard]] auto
    offset_from_variables_base(const size_t offset_from_dat) const -> int64_t {

        const int64_t offset{address_offset(offset_from_dat)};

        const ::machine& x{machine_.get()};

        const std::optional<size_t> past_vars_bytes{
            x.variables_base_past_vars_bytes(),
        };

        if (not past_vars_bytes) {
            return offset;
        }

        const size_t shift_bytes{
            sum_storage_size(vars_start_bytes(), *past_vars_bytes),
        };

        const int64_t shift{address_offset(shift_bytes)};

        return add_address_offset(offset, -shift);
    }

    // bytes of variables, without the dats and the padding after them
    [[nodiscard]] auto used_vars_size_bytes() const -> size_t {
        size_t total_bytes{};

        for (const scope& scp : scopes_.get().all()) {
            total_bytes = sum_storage_size(total_bytes,
                                           scp.allocated_storage_size_bytes());
        }

        return total_bytes;
    }

    // the variables start after the dats and the padding that aligns them;
    // every dat is placed by the parse, so this is fixed while compiling
    [[nodiscard]] auto vars_start_bytes() const -> size_t {
        return sum_storage_size(data_.get().total_size_bytes(),
                                data_.get().dat_var_padding_bytes());
    }

    //
    // statics
    //

    // of the kind that lets another version of the code be kept
    [[nodiscard]] static auto vars_capacity_error(const token& src_loc_tk,
                                                  const var_info& var)
        -> compile_error {

        // the name of a temporary is not in the source, the error is at its
        // call
        const std::string what{
            var.is_temporary()
                ? "a temporary of the call"
                : std::format("variable {}", in_quotes(var.name)),
        };

        return {
            src_loc_tk,
            std::format("{} does not fit in the variable storage, see '--vars'",
                        what),
            compile_error::error_kind::out_of_storage,
        };
    }
};

// the types the front end needs by role: the builtin ones and the ones the
// machine decides
class builtin_roles final {
    std::reference_wrapper<const ::machine> machine_;
    std::reference_wrapper<const builtin_type_set> types_;

  public:
    builtin_roles(const ::machine& backend, const builtin_type_set& types)
        : machine_{backend}, types_{types} {}

    // note: the same type as 'default_type', the word of the target
    [[nodiscard]] auto address() const -> const type& {
        const ::machine& x{machine_.get()};
        return x.default_type();
    }

    [[nodiscard]] auto boolean() const -> const type& {
        return types_.get().boolean;
    }

    [[nodiscard]] auto default_type() const -> const type& {
        const ::machine& x{machine_.get()};
        return x.default_type();
    }

    [[nodiscard]] auto i32() const -> const type& { return types_.get().i32; }

    [[nodiscard]] auto i64() const -> const type& { return types_.get().i64; }

    [[nodiscard]] auto i8() const -> const type& { return types_.get().i8; }

    [[nodiscard]] auto void_type() const -> const type& {
        return types_.get().void_type;
    }
};

// e.g. the ';' of 'var x = 3;', which the tokenizer reads as part of the word
// before it; the error points at the first ';' of 'var x = 3;;;'
inline auto throw_if_ends_with_semicolon(const token& src_loc_tk) -> void {
    const std::string_view word{src_loc_tk.text()};

    if (not word.ends_with(';')) {
        return;
    }

    // note: 'npos + 1' is 0 when the word is only ';'
    const size_t run_start{word.find_last_not_of(';') + 1};

    const token semicolon_tk{
        token::position(src_loc_tk.file_index(),
                        src_loc_tk.start_index() + run_start,
                        src_loc_tk.at_line()),
    };

    throw compile_error{semicolon_tk,
                        "';' is not used, statements need no separator"};
}

// a word that cannot be a name, e.g. '$x'
[[noreturn]] inline auto throw_invalid_name(const token& src_loc_tk,
                                            const std::string_view name)
    -> void {

    throw_if_ends_with_semicolon(src_loc_tk);

    throw compile_error{
        src_loc_tk,
        std::format("{} is not a valid name, a name starts with a letter "
                    "or '_' and has only letters, digits and '_'",
                    in_quotes(name))};
}

// a word that starts like a number but is none, e.g. '0x' or '12_000'
[[noreturn]] inline auto throw_unparsed_number(const token& src_loc_tk,
                                               const std::string_view text)
    -> void {

    throw_if_ends_with_semicolon(src_loc_tk);

    throw compile_error{
        src_loc_tk, std::format("cannot parse constant {}", in_quotes(text))};
}

// resolves one name by 'scope_stack::lookup' to what it means, a variable, a
// register or a constant ('resolve'), and to the storage it reaches
// ('resolve_storage'); a binding means what its argument resolved to in the
// caller, the fields and the indexes of a path are stepped by
// 'stmt_identifier'
class ident_resolver final {
    std::reference_wrapper<const scope_stack> scopes_;
    std::reference_wrapper<const ::machine> machine_;
    std::reference_wrapper<const function_table> funcs_;
    std::reference_wrapper<const generic_registry> generics_;
    std::reference_wrapper<const builtin_roles> builtins_;

  public:
    ident_resolver(const scope_stack& scopes, const ::machine& backend,
                   const function_table& funcs,
                   const generic_registry& generics,
                   const builtin_roles& builtins)
        : scopes_{scopes}, machine_{backend}, funcs_{funcs},
          generics_{generics}, builtins_{builtins} {}

    // the diagnostic names what an unresolved identifier is instead
    [[nodiscard]] auto resolve(const token& src_loc_tk,
                               const std::string_view ident) const
        -> ident_info {

        const name_meaning meaning{scopes_.get().lookup(ident)};

        // e.g. the 'a' of 'var a = a[0]', before its type, which is not known
        // yet, tells another mistake
        if (meaning.is_variable() and
            scopes_.get().at(meaning.scope_index).is_initializer()) {

            throw compile_error{src_loc_tk,
                                std::format("{} is read in its own initializer",
                                            in_quotes(ident))};
        }

        const ident_info id_info{resolve_or_empty(src_loc_tk, ident, meaning)};

        if (not id_info.is_empty()) {
            return id_info;
        }

        if (generics_.get().has_type(ident)) {
            throw_generic_type_not(src_loc_tk, ident, "a value");
        }

        // note: a function used as a value is rejected where it is parsed
        //       ('parse_call_or_identifier')

        assert(not funcs_.get().has(ident) and
               not generics_.get().has_func(ident));

        if (starts_number(ident)) {
            throw_unparsed_number(src_loc_tk, ident);
        }

        // e.g. the '$' of 'var x = $', which no definition can name
        if (not is_identifier(ident)) {
            throw_invalid_name(src_loc_tk, ident);
        }

        throw compile_error{src_loc_tk,
                            std::format("{} not found", in_quotes(ident))};
    }

    // what 'name' names with 'path' inside it, as a root variable and the
    // path inside it, empty when it holds no storage, e.g. a constant. the
    // element 'e' of a 'foo' is any element of the array it iterates
    [[nodiscard]] auto resolve_storage(const std::string_view name,
                                       const access_path& path) const
        -> std::optional<storage_path> {

        const name_meaning meaning{scopes_.get().lookup(name)};

        // the storage when the name has its own
        storage_path named{
            .root{
                .name{std::string{name}},
                .scope_index{meaning.scope_index},
            },
            .path{path},
        };

        // note: a binding is a root of its own, storage it shares with the
        //       caller is rejected at the call, see
        //       'stmt_call::assert_no_shared_storage'

        if (meaning.is_binding()) {
            const ident_info& bound{
                scopes_.get().at(meaning.scope_index).get_binding(name).info,
            };

            if (not bound.is_var()) {
                return std::nullopt;
            }

            return named;
        }

        if (not meaning.is_variable()) {
            return std::nullopt;
        }

        const var_info& var{
            scopes_.get().at(meaning.scope_index).get_var(name),
        };

        // note: the array was resolved to its root variable when the 'foo'
        //       was made, the callee of a call in the 'foo' does not see 'e'

        if (var.foo_array) {
            return var.foo_array->any_element(path);
        }

        // a value in a register has no storage, e.g. the counter 'i' of a 'foo'
        if (std::holds_alternative<in_value_register>(var.placement)) {
            return std::nullopt;
        }

        return named;
    }

    // the storage of a variable as it is placed
    [[nodiscard]] auto resolve_var(const var_info& var) const -> ident_info {
        if (const in_value_register* const in_register{
                std::get_if<in_value_register>(&var.placement),
            }) {

            return ident_info::make_register(in_register->reg);
        }

        const var_address address{
            holds_address(var.placement) ? var_address::held
                                         : var_address::storage,
        };

        const ::machine& x{machine_.get()};

        return ident_info::make_var({
            .operand{
                memory_at(var.placement, x.variables_base_register(),
                          *var.type_ptr),
            },
            .address{address},
            .shape{var.shape},
            .read_only_why{var.read_only_why},
        });
    }

  private:
    // a number, 'true' or 'false', else empty: the caller reports an unknown
    // name with its own wording
    [[nodiscard]] auto
    resolve_literal_or_empty(const token& src_loc_tk,
                             const std::string_view ident) const -> ident_info {

        // e.g. '0x10' or 'true'
        const std::optional<constant_parser::literal> literal{
            constant_parser::literal_value(src_loc_tk, ident),
        };

        if (not literal) {
            return ident_info::make_empty();
        }

        const type& literal_type{
            literal->is_bool ? builtins_.get().boolean()
                             : builtins_.get().default_type(),
        };

        return ident_info::make_const(literal_type,
                                      folded_value::untyped(literal->value));
    }

    [[nodiscard]] auto resolve_or_empty(const token& src_loc_tk,
                                        const std::string_view ident,
                                        const name_meaning& meaning) const
        -> ident_info {

        assert(not ident.empty());

        if (not meaning.is_value()) {
            return resolve_literal_or_empty(src_loc_tk, ident);
        }

        const scope& scp{scopes_.get().at(meaning.scope_index)};

        if (meaning.is_variable()) {
            return resolve_var(scp.get_var(ident));
        }

        if (meaning.is_constant()) {
            const folded_value& constant{scp.get_const(ident).constant};

            // an untyped constant reads at the default type
            const type& constant_type{
                constant.is_typed() ? *constant.own_type
                                    : builtins_.get().default_type(),
            };

            return ident_info::make_const(constant_type, constant);
        }

        // the argument was resolved where the call is made
        return scp.get_binding(ident).info;
    }

    //
    // statics
    //

    // the memory of a variable placed at 'placement', in storage from its own
    // base or else from 'base_register'
    [[nodiscard]] static auto memory_at(const var_placement& placement,
                                        const std::string_view base_register,
                                        const type& value_type) -> operand {

        if (const at_address_register* const pointed{
                std::get_if<at_address_register>(&placement),
            }) {

            return operand::mem(pointed->reg, value_type);
        }

        // note: a variable in a value register has no memory, 'resolve_var'
        //       makes its register operand

        const in_storage& stored{std::get<in_storage>(placement)};

        const std::string_view memory_base{
            stored.base_register.empty() ? base_register : stored.base_register,
        };

        return operand::mem(memory_base, {}, 1, stored.offset, value_type);
    }
};

// the errors of a name that is defined twice or that hides another
class definition_checks final {
    std::reference_wrapper<const scope_stack> scopes_;
    std::reference_wrapper<const function_table> funcs_;
    std::reference_wrapper<const type_table> types_;
    std::reference_wrapper<const generic_registry> generics_;
    std::reference_wrapper<const source_locations> locations_;

  public:
    definition_checks(const scope_stack& scopes, const function_table& funcs,
                      const type_table& types, const generic_registry& generics,
                      const source_locations& locations)
        : scopes_{scopes}, funcs_{funcs}, types_{types}, generics_{generics},
          locations_{locations} {}

    // 'generic_method' is the generic method that the function is an instance
    // of, empty if it is not
    auto assert_function_not_defined(
        const token& src_loc_tk, const std::string_view name,
        const std::string_view generic_method) const -> void {

        if (funcs_.get().exists(name)) {
            const func_info& fn{funcs_.get().get(name)};

            throw_function_defined(src_loc_tk, name, generic_method,
                                   fn.src_loc_tk, fn.generic_method);
        }

        // e.g. 'str.app' of 'func text.app<T type>()', a method with type
        // parameters of its own is a generic function of the instance
        if (generics_.get().exists_func(name)) {
            const generic_func_info& fn{generics_.get().get_func(name)};

            throw_function_defined(src_loc_tk, name, generic_method,
                                   fn.src_loc_tk, fn.generic_method());
        }
    }

    auto
    assert_generic_method_not_defined(const generic_func_info& method) const
        -> void {

        for (const generic_func_info& known : generics_.get().methods()) {
            if (known.report_name == method.report_name) {
                throw_already_defined(method.src_loc_tk, "generic method",
                                      method.report_name,
                                      where(known.src_loc_tk));
            }
        }
    }

    // a type parameter names its argument in the signature and the body, a
    // name of the signature would name two things, e.g. 'x' of 'func
    // f<x type>(x x)'; 'what' names the kind of 'name_tk'
    auto assert_generic_param_apart(const token& param_tk, const token& name_tk,
                                    const std::string_view what) const -> void {

        if (param_tk.text() != name_tk.text()) {
            return;
        }

        throw compile_error{
            param_tk,
            std::format("generic parameter {} is also the name of the {} at "
                        "{}, use another name",
                        in_quotes(param_tk.text()), what, where(name_tk))};
    }

    // a scope declares a variable or a constant of a name, not both, and the
    // body of a function repeats no parameter or the result; 'what' names
    // the kind of the declaration, e.g. "constant"
    auto assert_name_free(const token& src_loc_tk, const std::string_view what,
                          const std::string_view name) const -> void {

        assert_not_declared_in_scope(src_loc_tk, name);

        const scope& scp{scopes_.get().back()};

        if (scp.has_const(name)) {
            const const_info& existing{scp.get_const(name)};

            throw_already_defined(src_loc_tk, "constant", name,
                                  where(existing.src_loc_tk));
        }

        assert_not_hiding_signature(src_loc_tk, what, name);
    }

    // a parameter may hide a constant of the generic instance in the same
    // function scope
    auto assert_not_declared_in_scope(const token& src_loc_tk,
                                      const std::string_view name) const
        -> void {

        const scope& scp{scopes_.get().back()};

        if (not scp.has_var(name)) {
            return;
        }

        const var_info& decl_var{scp.get_var(name)};

        throw_already_defined(src_loc_tk, scp.var_role(name), name,
                              where(decl_var.src_loc_tk));
    }

    // a generic type and a type share the namespace of types
    auto assert_type_not_defined(const token& src_loc_tk,
                                 const std::string_view name) const -> void {

        if (types_.get().exists(name)) {
            // note: a built-in type name is rejected by
            //       'parse_toc::assert_valid_unqualified_name'

            assert(not types_.get().is_builtin(name));

            const token& type_tk{types_.get().src_loc_tk_of(name)};

            throw_already_defined(src_loc_tk, "type", name, where(type_tk));
        }

        if (generics_.get().exists_type(name)) {
            const token& generic_tk{generics_.get().get_type(name).src_loc_tk};

            throw compile_error{
                src_loc_tk,
                std::format("type {} already defined as a generic type at {}",
                            in_quotes(name), where(generic_tk))};
        }
    }

  private:
    // the body of a function or of a 'foo' is the block above its scope, a
    // name declared there cannot repeat a parameter, the result or the 'e',
    // 'i' and 'n' of the 'foo'; a deeper block may
    auto assert_not_hiding_signature(const token& src_loc_tk,
                                     const std::string_view what,
                                     const std::string_view name) const
        -> void {

        const std::vector<scope>& scopes{scopes_.get().all()};

        if (scopes.size() < 2 or not scopes.back().is_block()) {
            return;
        }

        const scope& outer_scp{scopes.at(scopes.size() - 2)};

        if (not outer_scp.is_func() and not outer_scp.is_loop()) {
            return;
        }

        // note: a known 'n' of a 'foo' is a constant
        const bool is_const{outer_scp.has_const(name)};

        if (not is_const and not outer_scp.has_var(name)) {
            return;
        }

        const token& decl_tk{
            is_const ? outer_scp.get_const(name).src_loc_tk
                     : outer_scp.get_var(name).src_loc_tk,
        };

        throw compile_error{
            src_loc_tk,
            std::format("{} {} hides the {} {} declared at {}", what,
                        in_quotes(name), outer_scp.var_role(name),
                        in_quotes(name), where(decl_tk)),
        };
    }

    // the message tells an instance of a generic method and a function
    // written in the source apart
    [[noreturn]] auto throw_function_defined(
        const token& src_loc_tk, const std::string_view name,
        const std::string_view generic_method, const token& existing_tk,
        const std::string_view existing_generic_method) const -> void {

        const bool is_generic{not generic_method.empty()};
        const bool is_existing_generic{not existing_generic_method.empty()};

        // note: a second definition of the same generic method is rejected
        //       where it is parsed ('assert_generic_method_not_defined')

        assert(not(is_generic and is_existing_generic) or
               generic_method != existing_generic_method);

        // e.g. 'text.print' and 'list.print' for
        // 'type text<size> = list<i8, size>'
        if (is_generic and is_existing_generic) {
            throw compile_error{
                src_loc_tk,
                std::format("generic method {} clashes with the generic "
                            "method {} defined at {}",
                            in_quotes(generic_method),
                            in_quotes(existing_generic_method),
                            where(existing_tk))};
        }

        if (is_generic) {
            throw compile_error{
                src_loc_tk,
                std::format("generic method {} clashes with the method "
                            "{} defined at {}",
                            in_quotes(generic_method), in_quotes(name),
                            where(existing_tk))};
        }

        if (is_existing_generic) {
            throw compile_error{
                src_loc_tk,
                std::format("method {} clashes with the generic method "
                            "{} defined at {}",
                            in_quotes(name), in_quotes(existing_generic_method),
                            where(existing_tk))};
        }

        // note: a method is named with its type, e.g. 't.x'
        const std::string_view kind{name.contains('.') ? "method" : "function"};

        throw_already_defined(src_loc_tk, kind, name, where(existing_tk));
    }

    [[nodiscard]] auto where(const token& src_loc_tk) const -> std::string {
        return locations_.get().for_message(src_loc_tk);
    }
};

// the tables of the program, owned by 'program' and shared by its two
// phases: the program is parsed with a 'parse_toc', then compiled with a
// 'compile_toc', each a 'toc' over these tables; each phase starts and ends
// without scopes, the root scope is entered again by the compile
struct toc_tables final {
    // 'builtin_types' are the types of the language, shared with the machine
    toc_tables(::machine& backend_in, const builtin_type_set& builtin_types,
               const source_files& files, const size_t vars_capacity_bytes)
        : backend{backend_in}, locations{files}, scopes{order},
          data{backend_in.data_alignment()}, funcs{order}, generics{order},
          types{order}, definitions{scopes, funcs, types, generics, locations},
          builtins{backend_in, builtin_types},
          resolver{scopes, backend_in, funcs, generics, builtins},
          storage{backend_in, scopes, data, vars_capacity_bytes} {

        // the built-in types are looked up by name like the defined ones
        for (const type* const tp : builtin_types.all()) {
            types.add_builtin(tp->name(), *tp);
        }

        // the default type has a name of its own so that it is portable
        types.add_builtin(reserved_names::default_type_name,
                          builtins.default_type());
    }

    toc_tables(const toc_tables&) = delete;
    toc_tables(toc_tables&&) = delete;
    auto operator=(const toc_tables&) -> toc_tables& = delete;
    auto operator=(toc_tables&&) -> toc_tables& = delete;
    ~toc_tables() = default;

    std::reference_wrapper<::machine> backend;
    source_locations locations;
    // before the tables, which refer to it
    definition_order order;
    scope_stack scopes;
    data_table data;
    function_table funcs;
    generic_registry generics;
    type_table types;
    definition_checks definitions;
    builtin_roles builtins;
    ident_resolver resolver;
    storage_layout storage;
};

// the context every node receives over the tables, the table of contents:
// the names, their storage and the machine. the phase is the type of the
// context: a parse-time constructor takes a 'parse_toc', a 'compile' a
// 'compile_toc' and what both phases run, e.g. 'fold' or the declarations of
// a function body, this base, which holds what both phases do
//
//   both           'add_var' and 'add_const' (placed and commented only by
//                  'compile_toc'), 'resolve_declared', 'storage_of',
//                  'use_at', 'overflow_check' without a destination,
//                  'get_main_or_throw'
//   tables         'builtins', 'checks', 'funcs', 'generics', 'machine',
//                  'scopes', 'types'
//
// the tables are read through these const accessors, a change goes through a
// method of a context, which may combine tables (README 7.6)
class toc {
    std::reference_wrapper<toc_tables> tables_;

  public:
    toc(const toc&) = delete;
    toc(toc&&) = delete;
    auto operator=(const toc&) -> toc& = delete;
    auto operator=(toc&&) -> toc& = delete;

    virtual ~toc() = default;

    //
    // virtual methods
    //

    // a constant of the innermost scope, e.g. a 'let' that folds or a
    // constant argument of a generic instance
    virtual auto add_const(const token& src_loc_tk, std::string_view name,
                           const folded_value& constant) -> void = 0;

    // a variable of the innermost scope; the parse only declares the name
    virtual auto add_var(const var_info& var, var_kind kind) -> void = 0;

    //
    // class methods
    //

    [[nodiscard]] auto builtins() const -> const builtin_roles& {
        return tables().builtins;
    }

    // what '--checks' turned on that the target honors, held by the machine
    [[nodiscard]] auto checks() const -> const check_options& {
        const ::machine& x{machine()};
        return x.checks();
    }

    [[nodiscard]] auto funcs() const -> const function_table& {
        return tables().funcs;
    }

    [[nodiscard]] auto generics() const -> const generic_registry& {
        return tables().generics;
    }

    // no token names a missing 'main'
    [[nodiscard]] auto get_main_or_throw() const -> const stmt_def_func& {
        const function_table& table{funcs()};

        if (not table.has(reserved_names::main)) {
            throw compile_error::file_level("function 'main' not found");
        }

        return *table.get(reserved_names::main).def;
    }

    [[nodiscard]] auto machine() const -> const ::machine& {
        return tables().backend.get();
    }

    // the overflow check of the arithmetic of a value without a destination,
    // e.g. a comparison side or an argument, as '--checks' asks
    [[nodiscard]] auto overflow_check() const -> machine::runtime_check {
        return {
            .enabled{checks().overflow},
        };
    }

    // the declared name 'ident', e.g. the whole array of 'ps[1]'; the free
    // 'make_ident_info' of a path gives what the path names, its element
    [[nodiscard]] auto resolve_declared(const token& src_loc_tk,
                                        const std::string_view ident) const
        -> ident_info {

        return tables().resolver.resolve(src_loc_tk, ident);
    }

    [[nodiscard]] auto scopes() const -> const scope_stack& {
        return tables().scopes;
    }

    // see 'ident_resolver::resolve_storage'
    [[nodiscard]] auto storage_of(const std::string_view name,
                                  const access_path& path) const
        -> std::optional<storage_path> {

        return tables().resolver.resolve_storage(name, path);
    }

    [[nodiscard]] auto types() const -> const type_table& {
        return tables().types;
    }

    // a value used without a destination at 'width'
    [[nodiscard]] auto use_at(const type& width) const -> value_use {
        return {
            .width{&width},
            .checks_overflow{overflow_check().enabled},
        };
    }

  protected:
    explicit toc(toc_tables& tables_in) : tables_{tables_in} {}

    // the declaration that both phases make, the declared constant is in the
    // innermost scope
    auto declare_const(const token& src_loc_tk, const std::string_view name,
                       const folded_value& constant) -> void {

        toc_tables& t{tables()};

        t.definitions.assert_name_free(src_loc_tk, "constant", name);

        t.scopes.back().add_const(name, {
                                            .src_loc_tk{src_loc_tk},
                                            .constant{constant},
                                        });

        t.scopes.record_if_global(name);
    }

    // the declaration that both phases make, the declared variable is in the
    // innermost scope
    auto declare_var(const var_info& var, const var_kind kind) -> var_info& {
        toc_tables& t{tables()};

        if (kind == var_kind::param or kind == var_kind::result) {
            t.definitions.assert_not_declared_in_scope(var.src_loc_tk,
                                                       var.name);
        } else {
            t.definitions.assert_name_free(var.src_loc_tk, "variable",
                                           var.name);
        }

        var_info& declared{t.scopes.back().add_var(var, kind)};

        t.scopes.record_if_global(var.name);

        return declared;
    }

    [[nodiscard]] auto tables() -> toc_tables& { return tables_.get(); }

    [[nodiscard]] auto tables() const -> const toc_tables& {
        return tables_.get();
    }
};

// the context of the parse: the names are declared without storage and the
// definitions are added to the tables; a generic instance is parsed inside
// the parse that needs it, in a 'generic_scope'
//
//   program        'begin_definition'
//   scopes         'scope_entry'
//   definitions    'definitions', 'add_type', 'add_func',
//                  'add_func_instance', 'add_generic_func',
//                  'add_generic_method', 'add_generic_type',
//                  'add_generic_type_instance', 'add_dat',
//                  'mark_generic_func_instantiated',
//                  'generic_scope', 'current_definition'
//   names          'assert_loop_names_free', 'assert_valid_unqualified_name',
//                  'assert_valid_name', 'constant_value_of',
//                  'get_func_or_throw', 'get_type_or_throw', 'is_func',
//                  'type_param_named', 'make_var_read_only'
//   body uses      'record_use', 'record_result_passed_as_mut'
//   source         'parse_nesting_scope', 'source_of', 'where'
class parse_toc final : public toc {
    // the most levels of blocks and expressions inside each other, the parsers
    // recurse for each of them so the limit keeps the source from exhausting
    // the stack
    static constexpr size_t max_parse_nesting_{128};

    // the parsers recurse on the one stack, e.g. a generic instance is parsed
    // inside the parse that needs it, so the levels are counted once for all
    // of them, see 'parse_nesting_scope'
    size_t parse_nesting_{};

  public:
    // a scope of the scope stack while it lives, also when an error unwinds,
    // e.g. 'const parse_toc::scope_entry block{tc, scope::block()};'
    class scope_entry final {
        parse_toc& tc_;
        // the scopes below this one, the scopes are left in stack order
        size_t outer_scope_count_{};

      public:
        scope_entry(parse_toc& tc, scope scp)
            : tc_{tc}, outer_scope_count_{tc.scopes().count()} {

            tc_.tables().scopes.push(std::move(scp));
        }

        scope_entry(const scope_entry&) = delete;
        scope_entry(scope_entry&&) = delete;
        auto operator=(const scope_entry&) -> scope_entry& = delete;
        auto operator=(scope_entry&&) -> scope_entry& = delete;

        ~scope_entry() {
            assert(tc_.scopes().count() == outer_scope_count_ + 1);

            tc_.tables().scopes.pop();
        }
    };

    // one level of nesting while it lives
    class parse_nesting_scope final {
        parse_toc& tc_;

      public:
        // the error is located at 'src_loc_tk', e.g. the '{' of a block
        parse_nesting_scope(parse_toc& tc, const token& src_loc_tk) : tc_{tc} {
            if (tc_.parse_nesting_ >= max_parse_nesting_) {
                throw compile_error{
                    src_loc_tk,
                    std::format("nesting is too deep, the limit is {} levels",
                                max_parse_nesting_)};
            }

            ++tc_.parse_nesting_;
        }

        parse_nesting_scope(parse_toc& tc, const tokenizer& tz)
            : parse_nesting_scope{tc, tz.peek_token_position()} {}

        parse_nesting_scope(const parse_nesting_scope&) = delete;
        parse_nesting_scope(parse_nesting_scope&&) = delete;
        auto operator=(const parse_nesting_scope&)
            -> parse_nesting_scope& = delete;
        auto operator=(parse_nesting_scope&&) -> parse_nesting_scope& = delete;

        ~parse_nesting_scope() { --tc_.parse_nesting_; }
    };

    // while an instance of a generic definition is parsed, each parameter names
    // its argument, e.g. 'T' names 'i8' and 'capacity' is the constant 8; the
    // parameters of an enclosing instance are hidden and only the definitions
    // up to the generic one are seen, all of it ends with the scope
    class generic_scope final {
        parse_toc& tc_;
        definition_order::instance_scope visible_;
        // the constants are in a block of their own
        std::optional<scope_entry> constants_block_;

      public:
        generic_scope(parse_toc& tc, const size_t definition)
            : tc_{tc}, visible_{tc.tables().order, definition} {

            tc_.tables().types.enter_params();
        }

        generic_scope(const generic_scope&) = delete;
        generic_scope(generic_scope&&) = delete;
        auto operator=(const generic_scope&) -> generic_scope& = delete;
        auto operator=(generic_scope&&) -> generic_scope& = delete;

        ~generic_scope() {
            constants_block_.reset();
            tc_.tables().types.exit_params();
        }

        // an argument is located at its parameter
        auto add_args(const std::span<const generic_arg> args) -> void {
            for (const generic_arg& arg : args) {
                if (arg.is_type()) {
                    tc_.tables().types.add_param(arg.param_tk, arg.type_ref());
                } else {
                    add_const(arg.param_tk, arg.constant());
                }
            }
        }

        // a method of an instance of a generic type sees the generic type as
        // the instance and its type arguments, e.g. 'list' names 'str' and 'T'
        // names 'i8'; its constants are not added here, the body adds them
        // where it is parsed and compiled ('stmt_def_func::add_constants')
        auto add_receiver(const generic_type_instance& instance) -> void {
            const generic_type_info& generic{
                tc_.generics().get_type(instance.generic_name),
            };

            // note: a generic type is registered under its 'src_loc_tk'
            tc_.tables().types.add_receiver(generic.src_loc_tk,
                                            *instance.type_ptr);

            for (const generic_arg& arg : instance.args) {
                if (arg.is_type()) {
                    tc_.tables().types.add_param(arg.param_tk, arg.type_ref());
                }
            }
        }

      private:
        auto add_const(const token& param_tk, const int64_t value) -> void {
            if (not constants_block_) {
                constants_block_.emplace(tc_, scope::block());
            }

            tc_.add_const(param_tk, param_tk.text(),
                          folded_value::untyped(value));
        }
    };

    explicit parse_toc(toc_tables& tables_in) : toc{tables_in} {}

    //
    // overridden methods
    //

    // declared, as at the compile, without a comment
    auto add_const(const token& src_loc_tk, const std::string_view name,
                   const folded_value& constant) -> void final {

        declare_const(src_loc_tk, name, constant);
    }

    // the name only; a variable in storage is placed by the compile
    auto add_var(const var_info& var, const var_kind kind) -> void final {
        std::ignore = declare_var(var, kind);
    }

    //
    // class methods
    //

    // the parse places the data of 'dat' after the dats before it and
    // declares the variable 'var' that names it, the compile declares it again
    // with 'compile_toc::add_var'
    auto add_dat(const stmt_def_dat& dat, const var_info& var) -> void {
        toc_tables& t{tables()};

        // note: a name declared twice is reported before where it is declared
        t.definitions.assert_name_free(var.src_loc_tk, "variable", var.name);

        // note: a 'dat' in a block is rejected when it is parsed
        assert(t.scopes.is_at_root());

        const size_t size_bytes{
            t.storage.var_storage_size_bytes(var.src_loc_tk, var),
        };

        t.data.add_dat(dat, var.src_loc_tk,
                       t.storage.var_storage_alignment(var), size_bytes);

        // the dats reach no further than the target addresses, 'place_dat'
        // asserts it at the compile
        t.storage.assert_dats_and_vars_fit_target(var.src_loc_tk);

        add_var(var, var_kind::dat);
    }

    auto add_func(std::string name, func_info info) -> void {
        toc_tables& t{tables()};

        t.definitions.assert_function_not_defined(info.src_loc_tk, name,
                                                  info.generic_method);

        t.funcs.add(std::move(name), std::move(info));
    }

    // keeps the instance alive, the function table refers to it
    auto add_func_instance(std::shared_ptr<const stmt_def_func> instance)
        -> void {

        tables().funcs.add_instance(std::move(instance));
    }

    auto add_generic_func(std::string name, generic_func_info info) -> void {
        toc_tables& t{tables()};

        t.definitions.assert_function_not_defined(info.src_loc_tk, name,
                                                  info.generic_method());

        t.generics.add_func(std::move(name), std::move(info));
    }

    // a method is defined once for its generic type, also when no alias
    // makes an instance of the type
    auto add_generic_method(const generic_func_info& method) -> void {
        toc_tables& t{tables()};

        t.definitions.assert_generic_method_not_defined(method);

        t.generics.add_method(method);
    }

    auto add_generic_type(generic_type_info info) -> void {
        // note: the name was checked by 'assert_type_not_defined'
        tables().generics.add_type(std::move(info));
    }

    auto add_generic_type_instance(generic_type_instance instance) -> void {
        tables().generics.add_type_instance(std::move(instance));
    }

    auto add_type(const token& src_loc_tk, const type& tpe) -> void {
        // note: the name was checked by 'assert_type_not_defined'
        assert_type_fits_target(src_loc_tk, tpe);

        tables().types.add(src_loc_tk, tpe);
    }

    // the names a 'foo' declares are no type parameter either, see
    // 'assert_valid_unqualified_name'
    auto assert_loop_names_free(const token& foo_tk) const -> void {
        for (const std::string_view name : reserved_names::foo_names) {
            if (const type* const bound{type_param_named(name)}) {
                throw compile_error{
                    foo_tk,
                    std::format("'foo' declares {}, which names the "
                                "type {}",
                                in_quotes(name), in_quotes(bound->name()))};
            }
        }
    }

    // a name of a type, function, variable, constant, data or parameter, which
    // is used without a qualifier
    auto assert_valid_unqualified_name(const token& name_tk) const -> void {
        assert_valid_name(name_tk);

        if (reserved_names::find_builtin_function(name_tk.text()) != nullptr) {
            throw compile_error{name_tk,
                                std::format("{} is a built-in function",
                                            in_quotes(name_tk.text()))};
        }

        // a value would hide the type only in some places, e.g. 'i8(300)'
        // still converts after 'var i8 = 3'
        if (types().is_builtin(name_tk.text())) {
            throw compile_error{name_tk,
                                std::format("{} is a built-in type",
                                            in_quotes(name_tk.text()))};
        }

        // a value would hide a type parameter only in some places, e.g.
        // 'U(v)' still converts; rejected whatever the argument, so that no
        // instance decides it
        if (const type* const bound{type_param_named(name_tk.text())}) {
            throw compile_error{
                name_tk,
                std::format("{} is a generic parameter, here the type {}, use "
                            "another name",
                            in_quotes(name_tk.text()),
                            in_quotes(bound->name()))};
        }
    }

    // a top-level definition starts, see 'definition_order'
    auto begin_definition() -> void { tables().order.begin_definition(); }

    // a number, untyped, or a constant with its type
    [[nodiscard]] auto constant_value_of(const token& tk) const
        -> std::optional<folded_value> {

        if (const std::optional<int64_t> value{
                constant_parser::parse_constant(tk, tk.text()),
            }) {

            return folded_value::untyped(*value);
        }

        // a broken number, as in a value
        if (tk.is_number()) {
            throw_unparsed_number(tk, tk.text());
        }

        return scopes().const_value(tk.text());
    }

    [[nodiscard]] auto current_definition() const -> size_t {
        return tables().order.current();
    }

    // a check of a name read at the parse, e.g. a type name is checked where
    // it is read, before the body, so the field type 'p' of a second
    // 'type p {x p}' names the first 'p'
    [[nodiscard]] auto definitions() const -> const definition_checks& {
        return tables().definitions;
    }

    [[nodiscard]] auto get_func_or_throw(const token& src_loc_tk,
                                         const std::string_view name) const
        -> const stmt_def_func& {

        if (funcs().has(name)) {
            return *funcs().get(name).def;
        }

        // e.g. 'x(1)' after 'var x = 1', a call still calls a hidden function
        if (scopes().lookup(name).is_value()) {
            throw_not_a_function(src_loc_tk, "value", name);
        }

        // e.g. 'box(1)' of 'type box<T type> {x T}'
        if (generics().has_type(name)) {
            throw_generic_type_not(src_loc_tk, name, "a function");
        }

        // e.g. the 'f'a'' of 'f'a'(1)', which no definition can name
        if (not is_identifier(name)) {
            throw_invalid_name(src_loc_tk, name);
        }

        throw compile_error{
            src_loc_tk, std::format("function {} not found", in_quotes(name))};
    }

    [[nodiscard]] auto get_type_or_throw(const token& type_tk) const
        -> const type& {

        // note: a declared type is read by 'statement::read_declared_type'
        //       and 'func_result::read', which reject a string

        assert(not type_tk.is_string());

        const std::string_view name{type_tk.text()};

        if (not types().has(name)) {
            if (generics().has_type(name)) {
                throw_generic_type_not(type_tk, name, "a type");
            }

            // e.g. the 'i8;' of 'type t { a i8; }'
            throw_if_ends_with_semicolon(type_tk);

            throw compile_error{
                type_tk, std::format("type {} not found", in_quotes(name))};
        }

        const type& tp{types().get(name)};

        // note: 'void' is the type of what has no value, it has no size
        if (tp.is_same(builtins().void_type())) {
            throw compile_error{
                type_tk,
                std::format("{} is not a type of a value", in_quotes(name))};
        }

        // the default type is the widest integer the target computes with,
        // e.g. 'i32' on rv32i
        const type& widest{builtins().default_type()};

        if (tp.is_builtin() and tp.size_bytes() > widest.size_bytes()) {
            throw compile_error{
                type_tk,
                std::format("{} is not available on this target, use 'i32' "
                            "or 'int'",
                            in_quotes(name))};
        }

        return tp;
    }

    // a plain or a generic function, unlike 'funcs().has'
    [[nodiscard]] auto is_func(const std::string_view name) const -> bool {
        return funcs().has(name) or generics().has_func(name);
    }

    // the variable declared last in this scope, e.g. a 'let' variable once
    // its initializer is parsed
    auto make_var_read_only(const std::string_view name,
                            const read_only_cause cause) -> void {

        tables().scopes.back().make_var_read_only(name, cause);
    }

    auto mark_generic_func_instantiated(const std::string_view name) -> void {
        tables().generics.mark_func_instantiated(name);
    }

    // see 'body_uses::is_result_passed_as_mut'
    auto record_result_passed_as_mut() -> void {
        tables().scopes.record_result_passed_as_mut();
    }

    // resolved 'storage' that the body being parsed uses, see 'body_uses',
    // e.g. a path in it or a global that a function it calls uses, so a
    // local of the same name does not shadow it; none outside a function or
    // without storage
    auto record_use(const std::optional<storage_path>& storage,
                    const use_kind kind) -> void {

        scope_stack& stack{tables().scopes};

        if (not storage or not stack.is_in_func()) {
            return;
        }

        stack.record_storage_use(*storage, kind == use_kind::write);
    }

    // the text of the file of the token
    [[nodiscard]] auto source_of(const token& src_loc_tk) const
        -> std::string_view {

        return tables().locations.source_of(src_loc_tk);
    }

    // the type a type parameter of the instance being parsed is bound to,
    // e.g. 'U' bound to 'i8'; null for any other name, also for the generic
    // type of a method, which names its instance like a type name does
    [[nodiscard]] auto type_param_named(const std::string_view name) const
        -> const type* {

        if (not types().is_type_param(name)) {
            return nullptr;
        }

        return &types().get(name);
    }

    // where 'src_loc_tk' is, for a message, e.g. '4:9'
    [[nodiscard]] auto where(const token& src_loc_tk) const -> std::string {
        return tables().locations.for_message(src_loc_tk);
    }

    //
    // statics
    //

    // a name is an identifier that is not a keyword
    static auto assert_valid_name(const token& name_tk) -> void {
        if (not name_tk.is_word()) {
            throw compile_error{name_tk, "expected a name"};
        }

        if (not is_identifier(name_tk.text())) {
            throw_invalid_name(name_tk, name_tk.text());
        }

        if (std::ranges::contains(reserved_names::keywords, name_tk.text())) {
            throw compile_error{
                name_tk,
                std::format("{} is a keyword", in_quotes(name_tk.text()))};
        }
    }

  private:
    // a value of the type, e.g. a parameter, is addressed by the target even
    // when no storage of it is ever allocated; a value is smaller than the
    // address space, so its size and element counts fit an address
    auto assert_type_fits_target(const token& src_loc_tk, const type& tpe) const
        -> void {

        const ::machine& x{machine()};

        const size_t max_bytes{x.max_storage_bytes()};

        if (tpe.size_bytes() < max_bytes) {
            return;
        }

        throw compile_error{
            src_loc_tk,
            std::format("type {} of {} B is too large, the target allows at "
                        "most {} B",
                        in_quotes(tpe.name()), tpe.size_bytes(),
                        max_bytes - 1)};
    }
};

// a noinline body compiled for the array lengths of its calls
struct noinline_body {
    const stmt_def_func* func{};
    // one per array parameter, in parameter order; a length is unknown only
    // in a body that is checked, not emitted, see
    // 'compile_toc::add_noinline_body'
    std::vector<std::optional<size_t>> array_lengths;
    // the first call that asked for the body, where an error in the body
    // reports it was called from, e.g. 'third(b)'
    token call_tk;
    std::string call_source;
};

// the noinline bodies of one kind, each once per function and array lengths:
// a worklist that grows while its bodies compile
class noinline_body_list final {
    std::vector<noinline_body> bodies_;

  public:
    auto add_once(noinline_body body) -> void {
        if (contains(*body.func, body.array_lengths)) {
            return;
        }

        bodies_.push_back(std::move(body));
    }

    // a copy, the list grows while the bodies compile
    [[nodiscard]] auto at(const size_t index) const -> noinline_body {
        return bodies_.at(index);
    }

    [[nodiscard]] auto
    contains(const stmt_def_func& func,
             const std::vector<std::optional<size_t>>& array_lengths) const
        -> bool {

        return std::ranges::any_of(
            bodies_, [&](const noinline_body& known) -> bool {
                return known.func == &func and
                       known.array_lengths == array_lengths;
            });
    }

    [[nodiscard]] auto count() const -> size_t { return bodies_.size(); }
};

// what the compile used, for the report
struct usage_statistics {
    size_t max_scope_count{};
    // see 'storage_layout::max_body_vars_size_bytes_'
    size_t max_body_vars_size_bytes{};
    size_t dat_size_bytes{};
    size_t dat_var_padding_bytes{};
    std::vector<std::string> uninstantiated_generics;
};

// the context of the compile, made after the parse: the declarations are
// placed, commented where they are stored and the storage capacity is checked,
// e.g. a parameter 'self' of a type larger than the vars section holds an
// address once the body is compiled; it holds what only the compile has:
//
//   program        'emitted_bodies', 'checked_bodies', 'usage',
//                  'assert_released'
//   tables         'data', 'layout'
//   scopes         'scope_entry', 'add_binding'
//   code           'add_read_only_constant', 'add_noinline_body',
//                  'dry_run', 'check_skipped',
//                  'emit_most_efficient', 'create_unique_label',
//                  'is_dry_run', 'call_path_at',
//                  'use_of', 'overflow_check' of a destination, the machine
//   listing        'indent', 'indent_scope', 'end_label', 'continue_label'
//   reachability   'is_reachable', 'end_path', 'record_break',
//                  'record_continue', 'record_return', 'join_loop',
//                  'path_merge'
//
// a statement may be compiled more than once (each inlined call, both
// versions of 'emit_most_efficient', dry runs, 'check_body'), so what a
// compile adds here is the same each time: a constant is kept once by its
// bytes ('add_read_only_constant'), a noinline body once
// ('add_noinline_body'), the scopes and their variables are popped
// ('scope_entry'); the peaks ('compile_state') are put back by a dry run
// with the constant count ('dry_run_scope') and set to those of the kept
// version by 'emit_most_efficient'
//
// the modes a node does not see, each set by one function and read elsewhere:
//
//   'is_dry_run_'             set by 'dry_run' ('dry_run_scope'); read by
//                             'add_noinline_body' (a body to check, not
//                             emit) and through 'is_dry_run' by
//                             'statement::comment_source'
//   'scope::is_without_call'  set by 'stmt_def_func::frame_scope' without
//                             array lengths and 'scope::main_func'; read for
//                             the innermost function by 'check_skipped'
//   'scope::takes_from_call'  set by 'stmt_def_func::frame_scope' for
//                             'stmt_def_func::check_body' and a checked
//                             body with a length unknown; read for
//                             any function by 'inline_call::compile' through
//                             'scope_stack::is_in_body_taking_from_call'
class compile_toc final : public toc {
    // the bodies to emit and those that only a dry run asks for, checked and
    // not emitted
    noinline_body_list emitted_bodies_;
    noinline_body_list checked_bodies_;
    // inside 'dry_run', also in a version inside it: the code compiled is
    // discarded, so a noinline call asks for a body to check, not to emit
    bool is_dry_run_{};
    // the indent of the listing lines the code being compiled emits, see
    // 'indent_scope'
    size_t indent_{};

    // what a dry run and a version that is not kept put back: the peaks that
    // compiling raises and whether the code after is reached, which an error
    // unwinding an inlined body ends ('pop_scope')
    struct compile_state {
        size_t max_scope_count{};
        storage_layout::peaks storage{};
        scope_stack::reach_state reach{};
    };

    // what a dry run changes in the context is put back when it ends, also
    // when an error unwinds: the 'compile_state', the read-only constants and
    // the mode ('is_dry_run')
    class dry_run_scope final {
        compile_toc& tc_;
        compile_state kept_state_{};
        size_t kept_constant_count_{};
        bool was_dry_run_{};

      public:
        explicit dry_run_scope(compile_toc& tc)
            : tc_{tc}, kept_state_{tc.compile_state_now()},
              kept_constant_count_{tc.data().constant_count()},
              was_dry_run_{tc.is_dry_run_} {

            tc.is_dry_run_ = true;
        }

        dry_run_scope(const dry_run_scope&) = delete;
        dry_run_scope(dry_run_scope&&) = delete;
        auto operator=(const dry_run_scope&) -> dry_run_scope& = delete;
        auto operator=(dry_run_scope&&) -> dry_run_scope& = delete;

        ~dry_run_scope() {
            tc_.restore_compile_state(kept_state_);
            tc_.tables().data.resize_constants(kept_constant_count_);
            tc_.is_dry_run_ = was_dry_run_;
        }
    };

  public:
    // the code compiled while it lives is written at 'indent_in', e.g. one
    // more for the statements of a block; put back also when an error unwinds
    class indent_scope final {
        compile_toc& tc_;
        size_t outer_indent_{};

      public:
        indent_scope(compile_toc& tc, const size_t indent_in)
            : tc_{tc}, outer_indent_{tc.indent_} {

            tc.indent_ = indent_in;
        }

        indent_scope(const indent_scope&) = delete;
        indent_scope(indent_scope&&) = delete;
        auto operator=(const indent_scope&) -> indent_scope& = delete;
        auto operator=(indent_scope&&) -> indent_scope& = delete;

        ~indent_scope() { tc_.indent_ = outer_indent_; }
    };

    // the paths of an 'if' start where the condition before them is false,
    // the first at the 'if', of an 'and'/'or' list at an element that may end
    // it; the end is reached when one path that runs reaches it; set when the
    // merge ends, also when an error unwinds, like
    // 'assignment_flow::path_merge'
    class path_merge final {
        compile_toc& tc_;
        bool is_reachable_{};

      public:
        explicit path_merge(compile_toc& tc) : tc_{tc} {}

        path_merge(const path_merge&) = delete;
        path_merge(path_merge&&) = delete;
        auto operator=(const path_merge&) -> path_merge& = delete;
        auto operator=(path_merge&&) -> path_merge& = delete;

        ~path_merge() { tc_.tables().scopes.set_reachable(is_reachable_); }

        auto begin_path(const bool is_start_reachable) -> void {
            tc_.tables().scopes.set_reachable(is_start_reachable);
        }

        // the path that ran ends here
        auto join_path() -> void {
            is_reachable_ = is_reachable_ or tc_.is_reachable();
        }
    };

    // a scope of the scope stack while it lives, also when an error unwinds,
    // so a failed version or dry run leaves the scopes of its caller, e.g.
    // 'const compile_toc::scope_entry block{tc, scope::block()};'; a function
    // scope is also a frame of the register trace, named by the function
    // and called at 'call_site_tk', so the two stacks agree
    class scope_entry final {
        compile_toc& tc_;
        // the scopes below this one, the scopes are left in stack order
        size_t outer_scope_count_{};
        std::optional<::machine::register_trace_scope> trace_frame_;

      public:
        scope_entry(compile_toc& tc, scope scp, const token& call_site_tk = {})
            : tc_{tc}, outer_scope_count_{tc.scopes().count()} {

            if (scp.is_func()) {
                trace_frame_.emplace(
                    tc.machine(), call_site_tk, std::string{scp.name()},
                    scp.has_frame() ? ::machine::frame_kind::noinline_body
                                    : ::machine::frame_kind::inlined);
            }

            if (scp.has_frame()) {
                tc_.tables().storage.begin_frame();
            }

            tc_.tables().scopes.push(std::move(scp));
        }

        scope_entry(const scope_entry&) = delete;
        scope_entry(scope_entry&&) = delete;
        auto operator=(const scope_entry&) -> scope_entry& = delete;
        auto operator=(scope_entry&&) -> scope_entry& = delete;

        ~scope_entry() {
            assert(tc_.scopes().count() == outer_scope_count_ + 1);

            tc_.pop_scope();
        }
    };

    // the parse is over: its scopes are popped, it declared the names
    // without storage
    explicit compile_toc(toc_tables& tables_in) : toc{tables_in} {
        assert(scopes().is_empty());

        tables().scopes.set_max_scope_count(0);
    }

    // the overloads of 'machine' and 'overflow_check' below would hide
    // those of the base
    using toc::machine;

    using toc::overflow_check;

    //
    // overridden methods
    //

    // commented where it is declared
    auto add_const(const token& src_loc_tk, const std::string_view name,
                   const folded_value& constant) -> void final {

        declare_const(src_loc_tk, name, constant);

        ::machine& x{machine()};

        x.comment({src_loc_tk, indent_}, "const {} = {}", name, constant.value);
    }

    // placed and commented where it is stored; a variable in a register or
    // at one has no storage
    auto add_var(const var_info& var, const var_kind kind) -> void final {
        const token src_loc_tk{var.src_loc_tk};

        const bool is_in_storage{
            std::holds_alternative<in_storage>(var.placement),
        };

        var_info& declared{declare_var(var, kind)};

        storage_layout& storage{tables().storage};

        // note: a 'dat' here is the compile half of 'parse_toc::add_dat'
        if (kind == var_kind::dat) {
            storage.place_dat(src_loc_tk, declared);
        } else if (is_in_storage) {
            storage.allocate_var(src_loc_tk, declared);
        }

        comment_var(src_loc_tk, declared);
    }

    //
    // class methods
    //

    auto add_binding(const param_binding& binding) -> void {
        tables().scopes.back().add_binding(binding);
    }

    // a body is emitted once per array lengths, for a call that is
    // compiled for real; a call in a dry run asks for a body to check, with
    // a length unknown where the check of a function without its calls
    // passes on an unsized array parameter
    auto add_noinline_body(const stmt_def_func& func,
                           std::vector<std::optional<size_t>> array_lengths,
                           const token& call_tk, std::string call_source)
        -> void {

        // note: a length is unknown only where 'stmt_def_func::check_body'
        //       passes an unsized array parameter on, in a dry run

        assert(is_dry_run_ or
               std::ranges::all_of(array_lengths,
                                   [](const std::optional<size_t>& length)
                                       -> bool { return length.has_value(); }));

        noinline_body body{
            .func{&func},
            .array_lengths{std::move(array_lengths)},
            .call_tk{call_tk},
            .call_source{std::move(call_source)},
        };

        if (is_dry_run_) {
            checked_bodies_.add_once(std::move(body));
            return;
        }

        emitted_bodies_.add_once(std::move(body));
    }

    // a read-only constant labeled 'kind' and the call path, e.g. "str" for
    // a string, kept once by its bytes
    [[nodiscard]] auto add_read_only_constant(const std::string_view kind,
                                              const token& src_loc_tk,
                                              std::string bytes)
        -> std::string {

        std::string label{create_unique_label(src_loc_tk, kind)};

        // note: the bytes of a literal may differ per call, e.g. '{k, 2}'
        return tables().data.add_constant(std::move(label), std::move(bytes));
    }

    // the scopes of the compile are popped
    auto assert_released() const -> void {
        // note: the scopes hold the storage in use, see 'storage_layout'
        assert(tables().scopes.is_empty());
    }

    // the location of 'src_loc_tk' and the call path of the inlined bodies
    // around it, which keeps labels unique per call, e.g. '12.5.30.9' for
    // 12:5 in a body inlined by the call at 30:9; the labels of a listing:
    //
    //   if.L.C         an 'if' at line L, column C; '.N' its branch N
    //                  ('stmt_if::branch_label'), '.code', '.else', '.end'
    //   cmp.L.C        a condition ('expr_bool')
    //   bool.L.C.end   the end of a condition stored as a value
    //   loop.L.C       a 'loop', '.end' ('end_label')
    //   foo.L.C        a 'foo', '.continue' ('continue_label'), '.end'
    //   init.L.C       the read-only bytes of a constant '{...}', 'str.L.C' of
    //                  a string ('add_read_only_constant')
    //   func.NAME..L.C a body inlined by the call at L:C, '.end'
    //   func.NAME      a noinline body, 'func.NAME..len.4' for array lengths,
    //                  'size.func.NAME' its frame size; 'f<i8>' is 'f..i8'
    //                  ('stmt_def_func::body_label', 'label_name')
    //   baz_NAME       a failure handler on both targets and a routine of
    //                  the x86_64 runtime, e.g. 'baz_bounds_panic',
    //                  'baz_report_line'
    //   .Lbaz_NAME     a routine of the rv32i runtime, e.g. '.Lbaz_divide';
    //                  numbered, a jump target inside one operation, e.g.
    //                  '.Lbaz_equal.0' (x86_64), on rv32i '.Lbaz_jump.0' the
    //                  skip of a branch grown to a far jump
    //
    // e.g. 'if.7.5.36.13' is the 'if' at 7:5 inlined by the call at 36:13;
    // inside a noinline body for array lengths the lengths follow the
    // location, e.g.
    // 'loop.14.5.len.3.end', and in the noinline body of a generic instance in
    // its name, e.g. 'if.2.3.f..i8' ('stmt_def_func::body_call_path'); a
    // location in an included file is 'fN.L.C' ('source_locations::for_label')
    [[nodiscard]] auto call_path_at(const token& src_loc_tk) const
        -> std::string {

        const std::string_view call_path{scopes().call_path()};

        const source_locations& locations{tables().locations};

        if (call_path.empty()) {
            return locations.for_label(src_loc_tk);
        }

        return std::format("{}.{}", locations.for_label(src_loc_tk), call_path);
    }

    // code that the compile skips, e.g. a branch that folds to false or the
    // code after a statement that leaves on every path, is compiled for its
    // checks where what it folds is no constant of a call: in 'main', in a
    // noinline body without array parameters, in the body check of a function
    // and in the initializer of a global, which takes nothing from a call
    // either; there every compile rule holds in all code
    auto check_skipped(const std::function_ref<void()> compile) -> void {
        if (scopes().is_in_func() and
            not scopes().current_func_scope().is_without_call()) {

            return;
        }

        // the dry run puts back the reach of the code after it
        dry_run(
            [&] -> void {
                tables().scopes.set_reachable(true);
                compile();
            },
            machine::discarded_code::here);
    }

    // the bodies that only a dry run asked for, see 'add_noinline_body'
    [[nodiscard]] auto checked_bodies() const -> const noinline_body_list& {
        return checked_bodies_;
    }

    // 'prefix' and the call path of 'src_loc_tk', e.g. 'if.7.5' for the
    // 'if' at 7:5, see 'call_path_at'
    [[nodiscard]] auto create_unique_label(const token& src_loc_tk,
                                           const std::string_view prefix) const
        -> std::string {

        return std::format("{}.{}", prefix, call_path_at(src_loc_tk));
    }

    [[nodiscard]] auto data() const -> const data_table& {
        return tables().data;
    }

    // runs 'compile' for its errors only: its output, read-only constants and
    // use of storage leave no trace; 'code' says which registers it starts
    // with ('machine::emit_discarded'); the error it ends at rejects the
    // program and is rethrown as an error of the source, so no reader of the
    // kind drops it: the check of a body starts from the registers of a
    // noinline body, and skipped code is never inside a version, an inlined
    // body skips no code, see 'check_skipped'; 'inline_call::check_recursion'
    // reads the kind inside the run, before it is rethrown
    auto dry_run(const std::function_ref<void()> compile,
                 const machine::discarded_code code) -> void {

        const dry_run_scope run{*this};

        ::machine& x{machine()};

        try {
            x.emit_discarded(compile, code);
        } catch (compile_error& e) {
            e.kind = compile_error::error_kind::source;

            throw;
        }
    }

    // 'machine::emit_most_efficient' with each version compiled from the
    // state before it ('compile_state'), so the state that remains is that of
    // the kept version, e.g. the frame size of a noinline body; a read-only
    // constant that the other version added stays, unused, so the labels stay
    // unique
    auto emit_most_efficient(const token& src_loc_tk,
                             const std::function_ref<void()> emit_first,
                             const std::function_ref<void()> emit_second)
        -> void {

        const compile_state start{compile_state_now()};

        compile_state first_end{start};
        compile_state second_end{start};

        const auto first{
            [&] -> void {
                restore_compile_state(start);
                emit_first();
                first_end = compile_state_now();
            },
        };

        const auto second{
            [&] -> void {
                restore_compile_state(start);
                emit_second();
                second_end = compile_state_now();
            },
        };

        ::machine& x{machine()};

        const bool keeps_first{
            x.emit_most_efficient({src_loc_tk, indent_}, first, second),
        };

        restore_compile_state(keeps_first ? first_end : second_end);
    }

    [[nodiscard]] auto emitted_bodies() const -> const noinline_body_list& {
        return emitted_bodies_;
    }

    // e.g. 'exit' leaves the code after it unreached
    auto end_path() -> void { tables().scopes.set_reachable(false); }

    // the indent of the listing lines the code being compiled emits
    [[nodiscard]] auto indent() const -> size_t { return indent_; }

    // see 'dry_run'
    [[nodiscard]] auto is_dry_run() const -> bool { return is_dry_run_; }

    // the code after the current statement of the function being compiled
    // runs on some path, the words of 'assignment_flow': 'end_path',
    // 'record_break', 'path_merge', and 'join_loop' for its 'trace_loop'; a
    // block stops compiling at an unreached statement, e.g. after the
    // 'return' of 'if n == 0 return' in an inlined call with the constant
    // argument 0; outside a function, in the initializer of a global, every
    // path continues, it makes no call
    [[nodiscard]] auto is_reachable() const -> bool {
        if (not scopes().is_in_func()) {
            return true;
        }

        return scopes().current_func_scope().reach_of_func().is_reachable;
    }

    // the end of a loop is reached by a compiled 'break' or as its
    // 'loop_end' says; called as the last step inside the loop's scope, where
    // the end of its body is the current statement
    auto join_loop() -> void {
        const scope& loop{scopes().back()};

        assert(loop.is_loop());

        const loop_end end{loop.loop_end_kind()};
        const loop_reach& reach{loop.reach_of_loop()};

        const bool is_step_reached{is_reachable() or reach.is_continued};

        const bool runs_out{
            end == loop_end::also_when_done or
                (end == loop_end::when_stepped and is_step_reached),
        };

        tables().scopes.set_reachable(runs_out or reach.is_left_by_break);
    }

    [[nodiscard]] auto layout() const -> const storage_layout& {
        return tables().storage;
    }

    [[nodiscard]] auto machine() -> ::machine& {
        return tables().backend.get();
    }

    // the overflow check of the arithmetic of a value that 'dst' receives,
    // off where 'dst' wraps
    [[nodiscard]] auto overflow_check(const destination& dst) const
        -> machine::runtime_check {

        if (dst.wraps()) {
            return {};
        }

        return overflow_check();
    }

    // a 'break' leaves the innermost 'loop' or 'foo', whose end is then
    // reached, and the code after it unreached
    auto record_break() -> void {
        tables().scopes.record_break();
        end_path();
    }

    // a 'continue' jumps to the head of the innermost 'loop' or the counter
    // step of a 'foo' and leaves the code after it unreached, see
    // 'loop_end::when_stepped'
    auto record_continue() -> void {
        tables().scopes.record_continue();
        end_path();
    }

    // a 'return' leaves the code after it unreached and reaches the end of an
    // inlined call
    auto record_return() -> void {
        tables().scopes.record_return();
        end_path();
    }

    [[nodiscard]] auto usage() const -> usage_statistics {
        const toc_tables& t{tables()};

        return {
            .max_scope_count{t.scopes.max_scope_count()},
            .max_body_vars_size_bytes{t.storage.max_body_vars_size_bytes()},
            .dat_size_bytes{t.data.total_size_bytes()},
            .dat_var_padding_bytes{t.data.dat_var_padding_bytes()},
            .uninstantiated_generics{t.generics.uninstantiated_func_names()},
        };
    }

    [[nodiscard]] auto use_of(const destination& dst) const -> value_use {
        return {
            .width{&dst.type_ref()},
            .checks_overflow{overflow_check(dst).enabled},
        };
    }

    //
    // statics
    //

    // the label of the next iteration of the 'foo' loop 'label' names
    [[nodiscard]] static auto continue_label(const std::string_view label)
        -> std::string {

        return std::format("{}.continue", label);
    }

    // the label after the code of the construct 'label' names, e.g. a loop
    [[nodiscard]] static auto end_label(const std::string_view label)
        -> std::string {

        return std::format("{}.end", label);
    }

  private:
    // the comment in the output that tells where a variable is stored, the
    // resolved name shows it
    auto comment_var(const token& src_loc_tk, const var_info& var) -> void {
        const ident_info name_info{tables().resolver.resolve_var(var)};

        std::string text{
            std::format("{}: {}", var.name, name_info.type_ref().name()),
        };

        if (const std::optional<size_t> length{var.shape.length()}) {
            text += std::format("[{}]", *length);
        }

        // the iterator 'e' is memory at its register, the counter 'i' is the
        // register
        const std::optional<operand> reg{register_of(var.placement)};

        ::machine& x{machine()};

        if (reg) {
            x.comment({src_loc_tk, indent_}, "{} ({})", text,
                      reg->register_name());

            return;
        }

        if (holds_address(var.placement)) {
            x.comment_held_address({src_loc_tk, indent_}, text,
                                   name_info.operand());

            return;
        }

        x.comment_variable({src_loc_tk, indent_}, text,
                           name_info.value_size_bytes(src_loc_tk),
                           name_info.operand());
    }

    [[nodiscard]] auto compile_state_now() const -> compile_state {
        return {
            .max_scope_count{scopes().max_scope_count()},
            .storage{layout().peaks_now()},
            .reach{scopes().reach_state_now()},
        };
    }

    // a scope is pushed by 'scope_entry' and popped here with its storage; an
    // inlined call whose body does not reach its end, e.g. a 'loop' whose
    // 'break' folds away for a constant argument, leaves the code after the
    // call unreached
    auto pop_scope() -> void {
        scope_stack& stack{tables().scopes};

        const scope& popped{stack.back()};

        const bool ends_caller_path{
            popped.is_inlined_func() and
                not popped.reach_of_func().is_end_reached(),
        };

        stack.pop();

        if (ends_caller_path) {
            end_path();
        }
    }

    auto restore_compile_state(const compile_state& state) -> void {
        toc_tables& t{tables()};

        t.scopes.set_max_scope_count(state.max_scope_count);
        t.storage.restore_peaks(state.storage);
        t.scopes.restore_reach_state(state.reach);
    }
};

//
// generics: an instance is made in two steps, see 'generics.hpp'
//   1. 'resolve_generic_args' reads the argument tokens of an alias or a base
//      as types and constants, e.g. '<i8, 8>' of 'list<T type, capacity>'
//      gives 'T' 'i8' and 'capacity' 8; a call reads its type arguments in
//      'stmt_call'
//   2. a 'parse_toc::generic_scope' makes them names while the text of the
//      definition is parsed again, 'type_table' holds the type names of the
//      innermost scope
//

// a generic argument of the wrong kind, e.g. a constant for a type
// parameter; 'got' is what was given
[[noreturn]] inline auto throw_generic_arg_mismatch(
    const token& arg_tk, const std::string_view generic_name,
    const std::string_view param_name, const std::string_view expected,
    const std::string_view got) -> void {

    throw compile_error{arg_tk,
                        std::format("generic parameter {} of {} needs a {}, "
                                    "got {}",
                                    in_quotes(param_name),
                                    in_quotes(generic_name), expected, got)};
}

// e.g. 'i8' of 'list<i8, 8>' for 'T'
[[nodiscard]] inline auto resolve_type_arg(const parse_toc& tc,
                                           const std::string_view generic_name,
                                           const std::string_view param_name,
                                           const token& arg_tk) -> const type& {

    if (arg_tk.is_string()) {
        throw_generic_arg_mismatch(arg_tk, generic_name, param_name, "type",
                                   "a string");
    }

    if (tc.constant_value_of(arg_tk) or
        constant_parser::is_bool_literal(arg_tk.text())) {

        throw_generic_arg_mismatch(
            arg_tk, generic_name, param_name, "type",
            std::format("the constant {}", in_quotes(arg_tk.text())));
    }

    return tc.get_type_or_throw(arg_tk);
}

// e.g. '8' of 'list<i8, 8>' for 'capacity'
[[nodiscard]] inline auto resolve_const_arg(const parse_toc& tc,
                                            const std::string_view generic_name,
                                            const std::string_view param_name,
                                            const token& arg_tk) -> int64_t {

    if (arg_tk.is_string()) {
        throw_generic_arg_mismatch(arg_tk, generic_name, param_name, "constant",
                                   "a string");
    }

    const std::optional<folded_value> constant{tc.constant_value_of(arg_tk)};

    // e.g. 'true' or 'b' of 'let b = true', no number
    if (constant_parser::is_bool_literal(arg_tk.text()) or
        (constant and constant->is_typed() and constant->own_type->is_bool())) {

        throw_generic_arg_mismatch(
            arg_tk, generic_name, param_name, "number",
            std::format("the 'bool' constant {}", in_quotes(arg_tk.text())));
    }

    if (not constant) {
        throw_generic_arg_mismatch(
            arg_tk, generic_name, param_name, "constant",
            std::format("{}{}",
                        tc.types().has(arg_tk.text()) ? "the type " : "",
                        in_quotes(arg_tk.text())));
    }

    return constant->value;
}

// a type parameter needs a type, a constant parameter a constant, e.g. 'T' is
// 'i8' and 'capacity' is 8 for '<i8, 8>' of 'list<T type, capacity>'
[[nodiscard]] inline auto
resolve_generic_args(const parse_toc& tc, const std::string_view generic_name,
                     const std::span<const generic_param> params,
                     const std::span<const token> arg_tks)
    -> std::vector<generic_arg> {

    assert(params.size() == arg_tks.size());

    std::vector<generic_arg> args;

    for (const auto [param, arg_tk] : std::views::zip(params, arg_tks)) {
        const std::string_view name{param.name_tk.text()};

        if (param.is_type) {
            args.push_back({
                .param_tk{param.name_tk},
                .value{&resolve_type_arg(tc, generic_name, name, arg_tk)},
            });
        } else {
            args.push_back({
                .param_tk{param.name_tk},
                .value{resolve_const_arg(tc, generic_name, name, arg_tk)},
            });
        }
    }

    return args;
}

// a generic definition parsed again for an instance while it lives: its
// parameters name the arguments the caller adds to 'scope()', 'tz()' reads
// from 'start_tk', and the parse counts toward the nesting of the parse
// that needs the instance
class generic_reparse final {
    parse_toc::generic_scope scope_;
    tokenizer tz_;
    parse_toc::parse_nesting_scope nesting_;

  public:
    generic_reparse(parse_toc& tc, const size_t definition,
                    const token& start_tk)
        : scope_{tc, definition}, tz_{tc.source_of(start_tk), start_tk},
          nesting_{tc, tz_} {}

    generic_reparse(const generic_reparse&) = delete;
    generic_reparse(generic_reparse&&) = delete;
    auto operator=(const generic_reparse&) -> generic_reparse& = delete;
    auto operator=(generic_reparse&&) -> generic_reparse& = delete;
    ~generic_reparse() = default;

    [[nodiscard]] auto scope() -> parse_toc::generic_scope& { return scope_; }

    [[nodiscard]] auto tz() -> tokenizer& { return tz_; }
};
