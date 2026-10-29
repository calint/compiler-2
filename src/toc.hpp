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
//   records         'func_info', 'noinline_body', 'body_uses' (what a body
//                   reaches that its callers name too), 'param_binding' (what
//                   a parameter of an inlined call stands for), 'const_info'
//   'scope'         a block, a loop or a function being compiled: its
//                   constants, variables and parameter bindings; "frame" in
//                   'scope' and 'storage_layout' is memory, e.g. the frame
//                   of a noinline call, while the register trace
//                   ('scope_entry', 'register_trace') and 'call_frame' of an
//                   error use it for a call level
//   tables          'type_table', 'noinline_worklist' (the noinline bodies to
//                   emit), 'function_table'
//   'data_table'    the dats ('dat_extent' as declared, 'placed_dat' as
//                   placed) and the read-only constants
//   options         'check_options', 'usage_statistics'
//   'scope_stack'   the scopes from the globals to the innermost scope and
//                   'lookup', the one rule of what a name means
//                   ('name_meaning'), the target of 'break' ('loop_target')
//   'storage_layout' where variables are placed and how much storage is used
//   'builtin_roles' the types the front end needs by role
//   'ident_resolver' resolves one name by 'lookup'
//   'definition_checks' the errors of a name defined twice
//   'variable_comments' the comment of where a variable is stored
//   'toc'           the facade the statements use, owns all of the above; a
//                   method of 'toc' changes state or combines tables, a query
//                   of one table goes through its const accessor, e.g.
//                   'tc.scopes().current_func_scope()', except a query asked
//                   at every step, e.g. 'is_reachable'
//   generics        'resolve_generic_args' and 'generic_scope', the names of
//                   the parameters of a generic instance
//

class stmt_def_dat;
class stmt_def_func;

struct func_info {
    token src_loc_tk;           // the definition
    const stmt_def_func* def{}; // never null
    // e.g. 'text.size' for the instance 'str.size', empty if not an instance
    // of a method of a generic type
    std::string generic_method;
    // see 'definition_order', 0 for a method, a constructor or an instance
    size_t definition{};
};

// a noinline body compiled for the array lengths of its calls
struct noinline_body {
    const stmt_def_func* func{};
    // one per array parameter, in parameter order
    std::vector<size_t> array_lengths;
    // the first call that asked for the body, where an error in the body
    // reports it was called from, e.g. 'third(b)'
    token call_tk;
    std::string call_source;
};

// whether a use of a variable reads or writes it, see 'toc::record_use'
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
    // in memory, see 'inline_call::compile'
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

struct const_info {
    token src_loc_tk; // the definition
    // typed when the initializer of a 'let' is or its type is not the
    // default type, e.g. 'let v = int(5)' or 'let b = true'
    folded_value constant{};
};

// what a variable is declared by: 'var', 'dat' or the signature of a function,
// a parameter or the result
enum class var_kind : uint8_t { var, dat, param, result };

// a block, a loop or a function being compiled; the static functions make
// each kind, e.g. 'scope::block()'
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

        // while the body compiles: the code after the current statement runs
        // on some path, see 'toc::is_reachable'
        bool is_reachable{true};

        // a 'return' of the body was compiled, it jumps to the end of an
        // inlined call, see 'toc::pop_scope'
        bool is_left_by_return{};

        // compiled without a call, so what it folds is no constant of a call:
        // 'main' and the body check of 'stmt_def_func::check_body', see
        // 'toc::check_skipped'
        bool is_without_call{};
        // such a body that a call gives something, its parameters or an
        // unsized array, so an error that depends on a call is no error of
        // the body, see 'compile_error::depends_on_call'
        bool takes_from_call{};

        // the name of the result, empty when there is none
        std::string result_name;

        // the parameters, in order
        std::vector<std::string> param_names;

        // while the body is parsed: what its callers name too
        body_uses uses;

        // a noinline body: the register its variables are placed from and the
        // most bytes they took
        std::string_view frame_base_register;
        size_t peak_frame_size_bytes{};
    };

    // what only a loop scope has
    struct loop_part {
        // how its end is reached and where a 'continue' jumps, e.g. the head
        // of a 'loop' or the counter step of a 'foo'
        loop_end end{loop_end::at_break};
        std::string continue_label;
        // a 'break' of it was compiled, so its end is reached; read by
        // 'toc::join_loop' and 'scope_stack::reach_state_now'
        bool is_left_by_break{};
        // a 'continue' of it was compiled, so its counter step is reached;
        // read by 'toc::join_loop' and 'scope_stack::reach_state_now'
        bool is_continued{};
    };

    // optional name
    std::string_view name_;

    // the bytes of variable storage this scope uses
    size_t allocated_storage_size_bytes_{};

    lut<const_info> consts_;

    // variables declared in this scope
    lut<var_info> vars_;

    std::optional<loop_part> loop_;
    std::optional<function_part> func_;

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

    auto add_var(const var_info& var, const var_kind kind) -> void {
        vars_.put(var.name, var);

        if (kind == var_kind::result) {
            func_part().result_name = var.name;
        }

        if (kind == var_kind::param) {
            func_part().param_names.emplace_back(var.name);
        }
    }

    [[nodiscard]] auto allocated_storage_size_bytes() const -> size_t {
        return allocated_storage_size_bytes_;
    }

    [[nodiscard]] auto body_uses_of_func() const -> const body_uses& {
        return func_part().uses;
    }

    [[nodiscard]] auto body_uses_ref() -> body_uses& {
        return func_part().uses;
    }

    [[nodiscard]] auto call_path() const -> std::string_view {
        return func_part().call_path;
    }

    [[nodiscard]] auto continue_label() const -> std::string_view {
        return loop().continue_label;
    }

    [[nodiscard]] auto frame_base_register() const -> std::string_view {
        assert(has_frame());

        return func_part().frame_base_register;
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

    [[nodiscard]] auto get_var_const_ref(const std::string_view name) const
        -> const var_info& {

        return vars_.get_const_ref(name);
    }

    [[nodiscard]] auto has_binding(const std::string_view name) const -> bool {
        return func_part().bindings.has(name);
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

    [[nodiscard]] auto is_compiled_in_place() const -> bool {
        return func_part().kind != func_kind::noinline;
    }

    [[nodiscard]] auto is_continued() const -> bool {
        return loop().is_continued;
    }

    // the end of the body of an inlined call is reached, by its last
    // statement or by a 'return'
    [[nodiscard]] auto is_end_reached() const -> bool {
        return func_part().is_reachable or func_part().is_left_by_return;
    }

    [[nodiscard]] auto is_func() const -> bool { return func_.has_value(); }

    // only a call inlined; 'main' is compiled in place too but is not one
    // ('is_compiled_in_place')
    [[nodiscard]] auto is_inlined_func() const -> bool {
        return func_ and func_->kind == func_kind::inlined;
    }

    [[nodiscard]] auto is_left_by_break() const -> bool {
        return loop().is_left_by_break;
    }

    [[nodiscard]] auto is_left_by_return() const -> bool {
        return func_part().is_left_by_return;
    }

    [[nodiscard]] auto is_loop() const -> bool { return loop_.has_value(); }

    [[nodiscard]] auto is_main() const -> bool {
        return func_ and func_->kind == func_kind::main;
    }

    [[nodiscard]] auto is_name(const std::string_view name) const -> bool {
        return name_ == name;
    }

    [[nodiscard]] auto is_reachable() const -> bool {
        return func_part().is_reachable;
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

    auto mark_without_call(const bool takes_from_call_in) -> void {
        func_part().is_without_call = true;
        func_part().takes_from_call = takes_from_call_in;
    }

    [[nodiscard]] auto name() const -> std::string_view { return name_; }

    // the index of the parameter 'name', none for another name
    [[nodiscard]] auto param_index(const std::string_view name) const
        -> std::optional<size_t> {

        const std::vector<std::string>& names{func_part().param_names};

        for (size_t i{}; i < names.size(); ++i) {
            if (names.at(i) == name) {
                return i;
            }
        }

        return std::nullopt;
    }

    [[nodiscard]] auto peak_frame_size_bytes() const -> size_t {
        assert(has_frame());

        return func_part().peak_frame_size_bytes;
    }

    auto record_frame_size_bytes(const size_t size_bytes) -> void {
        assert(has_frame());

        function_part& func{func_part()};

        func.peak_frame_size_bytes =
            std::max(func.peak_frame_size_bytes, size_bytes);
    }

    auto restore_peak_frame_size_bytes(const size_t size_bytes) -> void {
        assert(has_frame());

        func_part().peak_frame_size_bytes = size_bytes;
    }

    auto set_continued(const bool is_continued_in) -> void {
        loop().is_continued = is_continued_in;
    }

    auto set_left_by_break(const bool is_left) -> void {
        loop().is_left_by_break = is_left;
    }

    auto set_left_by_return(const bool is_left) -> void {
        func_part().is_left_by_return = is_left;
    }

    auto set_reachable(const bool is_reachable_in) -> void {
        func_part().is_reachable = is_reachable_in;
    }

    [[nodiscard]] auto takes_from_call() const -> bool {
        return func_part().takes_from_call;
    }

    //
    // statics
    //

    [[nodiscard]] static auto block() -> scope { return scope{""}; }

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
            .is_left_by_break{},
            .is_continued{},
        };

        return scp;
    }

    // 'main' is compiled in place, 'return' exits the program
    [[nodiscard]] static auto main_func() -> scope {
        scope scp{
            function(reserved_names::main, func_kind::main, {}, {}),
        };

        scp.mark_without_call(false);

        return scp;
    }

    // a function with a body of its own, its variables are placed in its
    // frame from 'frame_base_register'
    [[nodiscard]] static auto
    noinline_func(const std::string_view name, std::string call_path,
                  const std::string_view frame_base_register) -> scope {

        assert(not frame_base_register.empty());

        scope scp{
            function(name, func_kind::noinline, std::move(call_path), {}),
        };

        scp.func_part().frame_base_register = frame_base_register;

        return scp;
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
            .is_reachable{true},
            .is_left_by_return{},
            .is_without_call{},
            .takes_from_call{},
            .result_name{},
            .param_names{},
            .uses{},
            .frame_base_register{},
            .peak_frame_size_bytes{},
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
        // e.g. 'i8' or 'int', which have no place in the source
        bool is_builtin{};
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
                         .is_builtin{},
                     });
    }

    // a built-in type under 'name', e.g. 'i8', or 'int' for the default type
    auto add_builtin(const std::string_view name, const type& tpe) -> void {
        entries_.put(std::string{name}, {
                                            .src_loc_tk{},
                                            .type_ptr{&tpe},
                                            .definition{},
                                            .is_builtin{true},
                                        });
    }

    // e.g. 'T' names 'i8' in an instance of 'func f<T type>()'
    auto add_param(const token& src_loc_tk, const std::string_view name,
                   const type& tpe) -> void {

        assert(not exists(name));

        params_.back().put(std::string{name}, {
                                                  .src_loc_tk{src_loc_tk},
                                                  .type_ptr{&tpe},
                                                  .definition{},
                                                  .is_builtin{},
                                              });
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

    // a built-in type name, also one a type parameter does not hide
    [[nodiscard]] auto is_builtin(const std::string_view name) const -> bool {
        return entries_.has(name) and entries_.get_const_ref(name).is_builtin;
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

    [[nodiscard]] auto is_param(const std::string_view name) const -> bool {
        return not params_.empty() and params_.back().has(name);
    }
};

// what a compile made: the bodies of the noinline functions to emit, a
// worklist that grows while they compile, and the bodies that only a dry run
// asks for, checked and not emitted
class noinline_worklist final {
    std::vector<noinline_body> noinline_bodies_;
    std::vector<noinline_body> checked_bodies_;

  public:
    // a body asked for by a call in a dry run is checked once per array
    // lengths, unless it is emitted
    auto add_checked_body(const stmt_def_func& func,
                          std::vector<size_t> array_lengths,
                          const token& call_tk, std::string call_source)
        -> void {

        add_once(checked_bodies_, func, std::move(array_lengths), call_tk,
                 std::move(call_source));
    }

    // a body is emitted once per array lengths
    auto add_noinline_body(const stmt_def_func& func,
                           std::vector<size_t> array_lengths,
                           const token& call_tk, std::string call_source)
        -> void {

        add_once(noinline_bodies_, func, std::move(array_lengths), call_tk,
                 std::move(call_source));
    }

    // a copy, the list grows while the bodies are checked
    [[nodiscard]] auto checked_body_at(const size_t index) const
        -> noinline_body {

        return checked_bodies_.at(index);
    }

    [[nodiscard]] auto checked_body_count() const -> size_t {
        return checked_bodies_.size();
    }

    [[nodiscard]] auto is_emitted(const noinline_body& body) const -> bool {
        return contains(noinline_bodies_, *body.func, body.array_lengths);
    }

    [[nodiscard]] auto is_empty() const -> bool {
        return noinline_bodies_.empty() and checked_bodies_.empty();
    }

    // a copy, the list grows while the bodies compile
    [[nodiscard]] auto noinline_body_at(const size_t index) const
        -> noinline_body {

        return noinline_bodies_.at(index);
    }

    [[nodiscard]] auto noinline_body_count() const -> size_t {
        return noinline_bodies_.size();
    }

  private:
    //
    // statics
    //

    static auto add_once(std::vector<noinline_body>& bodies,
                         const stmt_def_func& func,
                         std::vector<size_t> array_lengths,
                         const token& call_tk, std::string call_source)
        -> void {

        if (contains(bodies, func, array_lengths)) {
            return;
        }

        bodies.push_back({
            .func{&func},
            .array_lengths{std::move(array_lengths)},
            .call_tk{call_tk},
            .call_source{std::move(call_source)},
        });
    }

    [[nodiscard]] static auto contains(const std::vector<noinline_body>& bodies,
                                       const stmt_def_func& func,
                                       const std::vector<size_t>& array_lengths)
        -> bool {

        return std::ranges::any_of(
            bodies, [&](const noinline_body& known) -> bool {
                return known.func == &func and
                       known.array_lengths == array_lengths;
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
    auto add(const token& src_loc_tk, std::string name,
             const stmt_def_func& func_def, std::string generic_method,
             const size_t definition) -> void {

        funcs_.put(std::move(name),
                   {
                       .src_loc_tk{src_loc_tk},
                       .def{&func_def},
                       .generic_method{std::move(generic_method)},
                       .definition{definition},
                   });
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

// what the data of a 'dat' takes, from its declaration
struct dat_extent {
    token name_tk;
    size_t alignment{};
    size_t size_bytes{};
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
    std::vector<machine::string_constant> constants_;
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

        for (const machine::string_constant& constant : constants_) {
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
    auto add_dat(const stmt_def_dat& dat, const dat_extent& extent) -> void {
        const size_t offset_bytes{
            align_storage_size(total_size_bytes_, extent.alignment),
        };

        const size_t padding_bytes{offset_bytes - total_size_bytes_};

        total_size_bytes_ =
            add_storage_size(extent.name_tk, offset_bytes, extent.size_bytes);

        dats_.push_back({
            .def{&dat},
            .name{extent.name_tk.text()},
            .padding_bytes{padding_bytes},
            .offset_bytes{offset_bytes},
        });
    }

    [[nodiscard]] auto constant_count() const -> size_t {
        return constants_.size();
    }

    [[nodiscard]] auto constants() const
        -> std::span<const machine::string_constant> {

        return constants_;
    }

    // the padding after the dats that aligns the variables
    [[nodiscard]] auto dat_var_padding_bytes() const -> size_t {
        return align_storage_size(total_size_bytes_, data_alignment_) -
               total_size_bytes_;
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

// what the compiler checks while it compiles and what it makes the program
// check at run time
struct check_options {
    bool bounds_upper{};
    bool bounds_lower{};
    // read by 'main' only, the machine has it as 'target_options::reports_line'
    bool reports_line{};
    bool frame{};
    bool division{};
    bool shift{};
    bool overlap{};
    bool stack{};
    bool overflow{};

    [[nodiscard]] auto bounds_options() const -> machine::bounds_check_options {
        return {
            .upper{bounds_upper},
            .lower{bounds_lower},
        };
    }

    // either bound
    [[nodiscard]] auto is_bounds() const -> bool {
        return bounds_upper or bounds_lower;
    }

    [[nodiscard]] auto overflow_options() const -> machine::runtime_check {
        return runtime(overflow);
    }

    [[nodiscard]] auto shift_options() const -> machine::runtime_check {
        return runtime(shift);
    }

  private:
    //
    // statics
    //

    [[nodiscard]] static auto runtime(const bool enabled)
        -> machine::runtime_check {

        return {
            .enabled{enabled},
        };
    }
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

// a loop named by its label and where its 'continue' jumps
struct loop_target {
    std::string_view label;
    std::string_view continue_label;
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

// the scopes being compiled, from the root scope that holds the globals to the
// innermost one
class scope_stack final {
    std::vector<scope> scopes_;
    size_t max_scope_count_{};
    std::reference_wrapper<const definition_order> order_;
    // the definition of each global variable, data and constant, see
    // 'definition_order'
    std::map<std::string, size_t, std::less<>> global_definitions_;

  public:
    // whether the code after the current statement is reached, saved around
    // code compiled only for its checks, see 'toc::check_skipped': a 'break'
    // or a 'return' there must not end the paths of the code that runs
    struct reach_state {
        // the innermost loop, the one a 'break' or a 'continue' of skipped
        // code marks
        std::optional<size_t> loop_index;
        bool is_left_by_break{};
        bool is_continued{};
        bool is_reachable{};
        bool is_left_by_return{};
    };

    explicit scope_stack(const definition_order& order) : order_{order} {}

    [[nodiscard]] auto all() const -> const std::vector<scope>& {
        return scopes_;
    }

    // checked at the parse, which reaches the branches that are not compiled,
    // e.g. the 'break' of 'if 1 == 2 { break }'
    auto assert_in_loop(const token& src_loc_tk) const -> void {
        if (not innermost_loop()) {
            throw compile_error{src_loc_tk,
                                std::format("{} is not in a loop",
                                            in_quotes(src_loc_tk.text()))};
        }
    }

    [[nodiscard]] auto at(const size_t index) -> scope& {
        return scopes_.at(index);
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
    // nearest function with a body of its own
    [[nodiscard]] auto inlined_nesting(const std::string_view name) const
        -> size_t {

        size_t count{};
        for (const scope& scp : scopes_ | std::views::reverse) {
            if (not scp.is_func()) {
                continue;
            }

            if (not scp.is_compiled_in_place()) {
                break;
            }

            if (scp.is_name(name)) {
                ++count;
            }
        }

        return count;
    }

    // the innermost 'loop' or 'foo' of the function, the target of a 'break'
    // or a 'continue'
    [[nodiscard]] auto innermost_loop() const -> std::optional<loop_target> {
        const std::optional<size_t> index{innermost_loop_index()};

        if (not index) {
            return std::nullopt;
        }

        const scope& loop{scopes_.at(*index)};

        return loop_target{
            .label{loop.name()},
            .continue_label{loop.continue_label()},
        };
    }

    // only the scope of the globals is open
    [[nodiscard]] auto is_at_root() const -> bool {
        return scopes_.size() == 1;
    }

    [[nodiscard]] auto is_empty() const -> bool { return scopes_.empty(); }

    // a check of a body that takes from a call is in the stack, see
    // 'toc::is_checking_without_call'
    [[nodiscard]] auto is_in_body_taking_from_call() const -> bool {
        return std::ranges::any_of(scopes_, [](const scope& scp) -> bool {
            return scp.is_func() and scp.is_without_call() and
                   scp.takes_from_call();
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

            const scope& scp{scopes_.at(index)};

            if (scp.has_var(name)) {
                return {
                    .what{name_meaning::kind::variable},
                    .scope_index{index},
                };
            }

            if (scp.is_func() and scp.has_binding(name)) {
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

            if (scp.is_func()) {
                break;
            }
        }

        const scope& root{scopes_.front()};

        // e.g. a global defined after the generic whose instance is parsed
        if (not is_global_visible(name)) {
            return {};
        }

        if (root.has_var(name)) {
            return {
                .what{name_meaning::kind::variable},
                .scope_index{},
            };
        }

        if (root.has_const(name)) {
            return {
                .what{name_meaning::kind::constant},
                .scope_index{},
            };
        }

        return {};
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
        track_scope_count();
    }

    [[nodiscard]] auto reach_state_now() const -> reach_state {
        // see 'toc::is_reachable'
        if (not is_in_func()) {
            return {
                .loop_index{},
                .is_left_by_break{},
                .is_continued{},
                .is_reachable{true},
                .is_left_by_return{},
            };
        }

        const std::optional<size_t> loop_index{innermost_loop_index()};

        return {
            .loop_index{loop_index},
            .is_left_by_break{
                loop_index and scopes_.at(*loop_index).is_left_by_break(),
            },
            .is_continued{
                loop_index and scopes_.at(*loop_index).is_continued(),
            },
            .is_reachable{current_func_scope().is_reachable()},
            .is_left_by_return{current_func_scope().is_left_by_return()},
        };
    }

    // a 'break' of the innermost 'loop' or 'foo' is compiled, see
    // 'scope::is_left_by_break'
    auto record_break() -> void {
        innermost_loop_ref().set_left_by_break(true);
    }

    // a 'continue' of the innermost 'loop' or 'foo' is compiled, see
    // 'scope::is_continued'
    auto record_continue() -> void { innermost_loop_ref().set_continued(true); }

    // a variable, data or constant of the root scope
    auto record_global(const std::string_view name) -> void {
        global_definitions_.try_emplace(std::string{name},
                                        order_.get().current());
    }

    // the body being parsed passes its result to a 'mut' parameter
    auto record_result_passed_as_mut() -> void {
        func_scope().body_uses_ref().mark_result_passed_as_mut();
    }

    // a 'return' of the innermost function is compiled, see
    // 'scope::is_end_reached'
    auto record_return() -> void { func_scope().set_left_by_return(true); }

    // records 'storage' when its root is a global, a parameter or the result
    // of the body being parsed, see 'body_uses'; a local of the body is not
    // named by the callers
    auto record_storage_use(const storage_path& storage, const bool is_written)
        -> void {

        scope& func{func_scope()};

        if (storage.root.is_global()) {
            func.body_uses_ref().add_global(storage, is_written);
            return;
        }

        if (storage.root.scope_index != func_scope_index()) {
            return;
        }

        if (func.is_result(storage.root.name)) {
            func.body_uses_ref().add_binding(std::nullopt, storage.path,
                                             is_written);

            return;
        }

        if (const std::optional<size_t> index{
                func.param_index(storage.root.name),
            }) {

            func.body_uses_ref().add_binding(index, storage.path, is_written);
        }
    }

    auto restore_reach_state(const reach_state& state) -> void {
        if (not is_in_func()) {
            return;
        }

        if (state.loop_index) {
            scope& loop{scopes_.at(*state.loop_index)};

            loop.set_left_by_break(state.is_left_by_break);
            loop.set_continued(state.is_continued);
        }

        func_scope().set_reachable(state.is_reachable);
        func_scope().set_left_by_return(state.is_left_by_return);
    }

    auto set_max_scope_count(const size_t count) -> void {
        max_scope_count_ = count;
    }

    // see 'toc::is_reachable'
    auto set_reachable(const bool is_reachable) -> void {
        if (not is_in_func()) {
            return;
        }

        func_scope().set_reachable(is_reachable);
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

    auto track_scope_count() -> void {
        max_scope_count_ = std::max(scopes_.size(), max_scope_count_);
    }
};

// where the variables are placed, after the dats that 'data_table' placed: the
// scopes that have a frame of their own, the capacity of the section and the
// target limits; the size in use is the sum of what the scopes hold
class storage_layout final {
    // where the next variable is placed: 'next_offset_bytes' counts from the
    // base of the frame of the scope 'scope_index' or, when it is empty, from
    // 'dat'
    struct storage_location {
        std::optional<size_t> scope_index;
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

  public:
    // the peaks that a dry run or a version of 'toc::emit_most_efficient'
    // raises and 'restore_peaks' puts back
    struct peaks {
        size_t max_body_vars_size_bytes{};
        // the index of the scope, scopes added meanwhile may move the scopes
        std::optional<size_t> frame_scope_index;
        size_t peak_frame_size_bytes{};
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
    auto allocate_var(const token& src_loc_tk, var_info& var,
                      const bool checks_capacity) -> void {

        const size_t var_size_bytes{var_storage_size_bytes(src_loc_tk, var)};
        const size_t var_alignment{var_storage_alignment(var)};

        // note: a frame base and 'dat' are both aligned
        const storage_location location{find_storage_location()};
        const size_t next_offset_bytes{location.next_offset_bytes};

        const size_t padding_bytes{
            align_storage_size(next_offset_bytes, var_alignment) -
                next_offset_bytes,
        };

        const size_t allocated_size_bytes{
            add_storage_size(src_loc_tk, padding_bytes, var_size_bytes),
        };

        if (checks_capacity) {
            assert_vars_capacity(src_loc_tk, var, allocated_size_bytes);
        }

        assert_total_fits_target(
            src_loc_tk, add_storage_size(src_loc_tk, dats_and_vars_size_bytes(),
                                         allocated_size_bytes));

        in_storage& stored{std::get<in_storage>(var.placement)};

        const size_t offset_bytes{next_offset_bytes + padding_bytes};

        if (location.scope_index) {
            scope& frame_scope{
                scopes_.get().at(*location.scope_index),
            };

            stored.offset = address_offset(offset_bytes);
            stored.base_register = frame_scope.frame_base_register();

            // note: the frame is within the data and variables checked above

            frame_scope.record_frame_size_bytes(
                sum_storage_size(next_offset_bytes, allocated_size_bytes));
        } else {
            stored.offset = offset_from_variables_base(offset_bytes);
        }

        scopes_.get().back().add_storage_bytes(allocated_size_bytes);

        // note: a dry run puts the peak back ('restore_peaks')
        max_body_vars_size_bytes_ =
            std::max(used_vars_size_bytes(), max_body_vars_size_bytes_);
    }

    auto assert_released() const -> void {
        assert(used_vars_size_bytes() == 0);
    }

    // the declaration that makes the data and variables reach what the target
    // addresses is the error, not the end of the build; checked before the
    // scope holds the bytes
    auto assert_total_fits_target(const token& src_loc_tk,
                                  const size_t total_bytes) const -> void {

        const size_t max_bytes{machine_.get().max_storage_bytes()};

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

    [[nodiscard]] auto max_body_vars_size_bytes() const -> size_t {
        return max_body_vars_size_bytes_;
    }

    // a callee frame starts aligned for any variable it holds
    [[nodiscard]] auto next_frame_address(const type& address_type) const
        -> operand {

        const size_t frame_alignment{machine_.get().address_size_bytes()};

        const storage_location location{find_storage_location()};

        const size_t aligned_offset_bytes{
            align_storage_size(location.next_offset_bytes, frame_alignment),
        };

        if (location.scope_index) {
            return operand::mem(
                scope_at(*location.scope_index).frame_base_register(), {}, 1,
                address_offset(aligned_offset_bytes), address_type);
        }

        return operand::mem(machine_.get().variables_base_register(), {}, 1,
                            offset_from_variables_base(aligned_offset_bytes),
                            address_type);
    }

    [[nodiscard]] auto peak_frame_size_bytes() const -> size_t {
        const std::optional<size_t> scope_index{
            find_storage_location().scope_index,
        };

        // note: asked inside a noinline body, which has a frame
        assert(scope_index);

        return scope_at(*scope_index).peak_frame_size_bytes();
    }

    [[nodiscard]] auto peaks_now() const -> peaks {
        const std::optional<size_t> scope_index{
            find_storage_location().scope_index,
        };

        return {
            .max_body_vars_size_bytes{max_body_vars_size_bytes_},
            .frame_scope_index{scope_index},
            .peak_frame_size_bytes{
                scope_index ? scope_at(*scope_index).peak_frame_size_bytes()
                            : 0,
            },
        };
    }

    // the storage of the dat 'var' is where the data table placed it, before
    // the variables; its offset is set, the scopes hold none of its bytes
    auto place_dat(const token& src_loc_tk, var_info& var) const -> void {
        const placed_dat& dat{data_.get().get_dat(var.name)};

        // note: a 'dat' comes before any 'var', so no variable is placed yet
        assert(used_vars_size_bytes() == 0);

        // note: 'toc::add_dat' checked the dats against the target
        assert(dat.offset_bytes + var_storage_size_bytes(src_loc_tk, var) <
               machine_.get().max_storage_bytes());

        std::get<in_storage>(var.placement).offset =
            offset_from_variables_base(dat.offset_bytes);
    }

    auto restore_peaks(const peaks& saved) -> void {
        max_body_vars_size_bytes_ = saved.max_body_vars_size_bytes;

        if (saved.frame_scope_index) {
            scopes_.get()
                .at(*saved.frame_scope_index)
                .restore_peak_frame_size_bytes(saved.peak_frame_size_bytes);
        }
    }

    [[nodiscard]] auto var_storage_alignment(const var_info& var) const
        -> size_t {

        if (holds_address(var.placement)) {
            return machine_.get().address_size_bytes();
        }

        return var.type_ptr->alignment();
    }

    [[nodiscard]] auto var_storage_size_bytes(const token& src_loc_tk,
                                              const var_info& var) const
        -> size_t {

        if (holds_address(var.placement)) {
            return machine_.get().address_size_bytes();
        }

        return var.shape.value_size_bytes(src_loc_tk, *var.type_ptr);
    }

    [[nodiscard]] auto vars_capacity_bytes() const -> size_t {
        return vars_capacity_bytes_;
    }

  private:
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
    // frame and of the scopes inside it, other variables start at the
    // variables base
    [[nodiscard]] auto find_storage_location() const -> storage_location {
        const std::vector<scope>& scopes{scopes_.get().all()};
        size_t local_size_bytes{};

        for (size_t i{scopes.size()}; i != 0; --i) {
            const scope& scp{scopes.at(i - 1)};

            local_size_bytes = sum_storage_size(
                local_size_bytes, scp.allocated_storage_size_bytes());

            if (scp.has_frame()) {
                return {
                    .scope_index{i - 1},
                    .next_offset_bytes{local_size_bytes},
                };
            }
        }

        return {
            .scope_index{},
            .next_offset_bytes{dats_and_vars_size_bytes()},
        };
    }

    // the operand offset of storage 'offset_from_dat' bytes past 'dat': the
    // variables base is 'dat' unless the target puts it past 'vars', which
    // makes the dats negative and the variables start below the base
    [[nodiscard]] auto
    offset_from_variables_base(const size_t offset_from_dat) const -> int64_t {

        const int64_t offset{address_offset(offset_from_dat)};

        const std::optional<size_t> past_vars_bytes{
            machine_.get().variables_base_past_vars_bytes(),
        };

        if (not past_vars_bytes) {
            return offset;
        }

        const int64_t shift{
            address_offset(
                sum_storage_size(vars_start_bytes(), *past_vars_bytes)),
        };

        return add_address_offset(offset, -shift);
    }

    [[nodiscard]] auto scope_at(const size_t index) const -> const scope& {
        return std::as_const(scopes_.get()).at(index);
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

        std::string message{
            std::format("variable {} does not fit in the variable storage, "
                        "see '--vars'",
                        in_quotes(var.name)),
        };

        // the name of a temporary is not in the source, the error is at its
        // call
        if (var.is_temporary()) {
            message = "a temporary of the call does not fit in the variable "
                      "storage, see '--vars'";
        }

        compile_error error{src_loc_tk, std::move(message)};

        error.kind = compile_error::error_kind::out_of_storage;

        return error;
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
        return machine_.get().default_type();
    }

    [[nodiscard]] auto boolean() const -> const type& {
        return types_.get().boolean;
    }

    [[nodiscard]] auto default_type() const -> const type& {
        return machine_.get().default_type();
    }

    [[nodiscard]] auto i64() const -> const type& { return types_.get().i64; }

    [[nodiscard]] auto i8() const -> const type& { return types_.get().i8; }

    [[nodiscard]] auto void_type() const -> const type& {
        return types_.get().void_type;
    }
};

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

        const ident_info id_info{resolve_or_empty(src_loc_tk, ident)};

        if (not id_info.is_empty()) {
            return id_info;
        }

        if (generics_.get().has_type(ident)) {
            throw compile_error{
                src_loc_tk,
                std::format("generic type {} is not a value, {}",
                            in_quotes(ident), instance_hint(ident))};
        }

        if (generics_.get().has_func(ident)) {
            throw_generic_function_without_arguments(src_loc_tk, ident, ident);
        }

        // note: a function used as a value is rejected where it is parsed
        //       ('parse_call_or_identifier')

        assert(not funcs_.get().has(ident));

        // a broken number, e.g. '0x' or '12_000'
        if (starts_number(ident)) {
            throw compile_error{
                src_loc_tk,
                std::format("cannot parse constant {}", in_quotes(ident))};
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
            scopes_.get().at(meaning.scope_index).get_var_const_ref(name),
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
                                        const std::string_view ident) const
        -> ident_info {

        assert(not ident.empty());

        const name_meaning meaning{scopes_.get().lookup(ident)};

        if (not meaning.is_value()) {
            return resolve_literal_or_empty(src_loc_tk, ident);
        }

        const scope& scp{scopes_.get().at(meaning.scope_index)};

        if (meaning.is_variable()) {
            return resolve_var(scp.get_var_const_ref(ident));
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

    [[nodiscard]] auto resolve_var(const var_info& var) const -> ident_info {
        if (const in_value_register* const in_register{
                std::get_if<in_value_register>(&var.placement),
            }) {

            return ident_info::make_register(in_register->reg);
        }

        return ident_info::make_var(
            *var.type_ptr,
            {
                .operand{
                    memory_at(var.placement,
                              machine_.get().variables_base_register(),
                              var.type_ptr->first_builtin()),
                },
                .is_address_by_call{},
                .shape{var.shape},
                .holds_address{holds_address(var.placement)},
                .read_only_why{var.read_only_why},
            });
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

            return operand::mem(pointed->reg.register_name(), "", 1, 0,
                                value_type);
        }

        // note: a variable in a value register has no memory, 'resolve_var'
        //       makes its register operand

        const in_storage& stored{std::get<in_storage>(placement)};

        const std::string_view memory_base{
            stored.base_register.empty() ? base_register : stored.base_register,
        };

        return operand::mem(memory_base, "", 1, stored.offset, value_type);
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
                                   fn.src_loc_tk,
                                   fn.receiver ? fn.report_name : "");
        }
    }

    auto
    assert_generic_method_not_defined(const generic_method_info& method) const
        -> void {

        for (const generic_method_info& known :
             generics_.get().get_methods(method.type_tk.text())) {

            if (known.method_name_tk.text() == method.method_name_tk.text()) {
                throw compile_error{
                    method.method_name_tk,
                    std::format(
                        "generic method {} already defined at {}",
                        in_quotes(method_name_of(method.type_tk.text(),
                                                 method.method_name_tk.text())),
                        where(known.method_name_tk))};
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

    // a generic parameter of a definition cannot be the name of a generic
    // type; an instance names the generic type itself, e.g. 'text' names 'str'
    auto assert_generic_param_not_generic_type(
        const token& src_loc_tk, const std::string_view name) const -> void {

        if (not generics_.get().exists_type(name)) {
            return;
        }

        throw compile_error{
            src_loc_tk,
            std::format("generic parameter {} hides the generic type {} "
                        "defined at {}, use another name",
                        in_quotes(name), in_quotes(name),
                        where(generics_.get().get_type(name).src_loc_tk))};
    }

    // the body of a method of a generic type sees the parameters of the
    // type too, e.g. 'n' of 'type box<n>' in 'func box.f<n type>()' would
    // name two things
    auto assert_generic_param_not_receiver_param(
        const token& src_loc_tk, const std::string_view name,
        const std::string_view generic_type) const -> void {

        const generic_type_info& generic{
            generics_.get().get_type(generic_type),
        };

        for (const generic_param& param : generic.params) {
            if (param.name_tk.text() == name) {
                throw compile_error{
                    src_loc_tk,
                    std::format("generic parameter {} is already declared "
                                "by {} at {}",
                                in_quotes(name), in_quotes(generic_type),
                                where(param.name_tk))};
            }
        }
    }

    // a generic parameter names its argument, so it cannot be the name of a
    // type
    auto assert_generic_param_not_type(const token& src_loc_tk,
                                       const std::string_view name) const
        -> void {

        if (not types_.get().exists(name)) {
            return;
        }

        // note: a built-in type name is rejected by
        //       'toc::assert_valid_unqualified_name' at the definition

        assert(not types_.get().is_builtin(name));

        const token& type_tk{types_.get().src_loc_tk_of(name)};

        throw compile_error{
            src_loc_tk,
            std::format("generic parameter {} hides the type {} defined "
                        "at {}, use another name",
                        in_quotes(name), in_quotes(name), where(type_tk))};
    }

    // a generic parameter names its argument, a value of that name would hide
    // it only in some places, e.g. 'U(5)' still converts after 'let U = 3'
    auto assert_generic_param_not_value(const token& src_loc_tk,
                                        const std::string_view name) const
        -> void {

        const name_meaning meaning{scopes_.get().lookup(name)};

        if (not meaning.is_value()) {
            return;
        }

        // note: a definition is parsed, bindings exist only while compiling
        assert(not meaning.is_binding());

        const scope& scp{scopes_.get().at(meaning.scope_index)};

        if (meaning.is_constant()) {
            throw compile_error{
                src_loc_tk,
                std::format("generic parameter {} hides the constant {} "
                            "defined at {}, use another name",
                            in_quotes(name), in_quotes(name),
                            where(scp.get_const(name).src_loc_tk))};
        }

        throw compile_error{
            src_loc_tk,
            std::format(
                "generic parameter {} hides the variable {} defined at {}, "
                "use another name",
                in_quotes(name), in_quotes(name),
                where(scp.get_var_const_ref(name).src_loc_tk))};
    }

    // a scope declares a variable or a constant of a name, not both
    auto assert_name_free(const token& src_loc_tk,
                          const std::string_view name) const -> void {

        assert_not_declared_in_scope(src_loc_tk, name);

        if (not scopes_.get().back().has_const(name)) {
            return;
        }

        const const_info& existing{scopes_.get().back().get_const(name)};

        throw compile_error{src_loc_tk,
                            std::format("constant {} already defined at {}",
                                        in_quotes(name),
                                        where(existing.src_loc_tk))};
    }

    // a parameter may hide a constant of the generic instance in the same
    // function scope
    auto assert_not_declared_in_scope(const token& src_loc_tk,
                                      const std::string_view name) const
        -> void {

        if (not scopes_.get().back().has_var(name)) {
            return;
        }

        const var_info& decl_var{scopes_.get().back().get_var_const_ref(name)};

        throw compile_error{src_loc_tk,
                            std::format("variable {} already defined at {}",
                                        in_quotes(name),
                                        where(decl_var.src_loc_tk))};
    }

    // the body of a function is the block above its scope, a name declared
    // there cannot repeat a parameter or the result; a deeper block may
    auto assert_not_hiding_signature(const token& src_loc_tk,
                                     const std::string_view what,
                                     const std::string_view name) const
        -> void {

        const std::vector<scope>& scopes{scopes_.get().all()};

        if (scopes.size() < 2 or not scopes.back().is_block()) {
            return;
        }

        const scope& func_scp{scopes.at(scopes.size() - 2)};

        if (not func_scp.is_func() or not func_scp.has_var(name)) {
            return;
        }

        const var_info& decl_var{func_scp.get_var_const_ref(name)};

        const std::string_view role{
            func_scp.is_result(name) ? "result" : "parameter",
        };

        throw compile_error{
            src_loc_tk,
            std::format("{} {} hides the {} {} declared at {}", what,
                        in_quotes(name), role, in_quotes(name),
                        where(decl_var.src_loc_tk)),
        };
    }

    // a generic type and a type share the namespace of types
    auto assert_type_not_defined(const token& src_loc_tk,
                                 const std::string_view name) const -> void {

        if (types_.get().exists(name)) {
            // note: a built-in type name is rejected by
            //       'toc::assert_valid_unqualified_name'

            assert(not types_.get().is_builtin(name));

            const token& type_tk{types_.get().src_loc_tk_of(name)};

            throw compile_error{src_loc_tk,
                                std::format("type {} already defined at {}",
                                            in_quotes(name), where(type_tk))};
        }

        if (generics_.get().exists_type(name)) {
            throw compile_error{
                src_loc_tk,
                std::format("type {} already defined as a generic type at {}",
                            in_quotes(name),
                            where(generics_.get().get_type(name).src_loc_tk))};
        }
    }

  private:
    // the message tells a built-in, an instance of a generic method and a
    // function written in the source apart
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

        throw compile_error{src_loc_tk,
                            std::format("function {} already defined at {}",
                                        in_quotes(name), where(existing_tk))};
    }

    [[nodiscard]] auto where(const token& src_loc_tk) const -> std::string {
        return locations_.get().for_message(src_loc_tk);
    }
};

// the comment in the output that tells where a variable is stored
class variable_comments final {
    std::reference_wrapper<::machine> machine_;
    std::reference_wrapper<const ident_resolver> resolver_;

  public:
    variable_comments(::machine& backend, const ident_resolver& resolver)
        : machine_{backend}, resolver_{resolver} {}

    // the resolved name shows where the variable is stored
    auto comment(const token& src_loc_tk, const size_t indent,
                 const var_info& var) const -> void {

        const ident_info name_info{
            resolver_.get().resolve(src_loc_tk, var.name),
        };

        std::string text{
            std::format("{}: {}", var.name, name_info.type_ref().name()),
        };

        if (const std::optional<size_t> length{var.shape.length()}) {
            text += std::format("[{}]", *length);
        }

        // the iterator 'e' is memory at its register, the counter 'i' is the
        // register
        const std::optional<operand> reg{register_of(var.placement)};

        ::machine& x{machine_.get()};

        if (reg) {
            x.comment(src_loc_tk, indent, "{} ({})", text,
                      reg->register_name());

            return;
        }

        if (holds_address(var.placement)) {
            x.comment_held_address(src_loc_tk, indent, text,
                                   name_info.operand());

            return;
        }

        x.comment_variable(src_loc_tk, indent, text,
                           name_info.value_size_bytes(src_loc_tk),
                           name_info.operand());
    }
};

// the context of every node: the names, their storage, the machine and the
// phase; the program is parsed first ('is_compiling_' false: the names are
// declared without storage and without comments), then compiled
// ('begin_compile'), after which every scope and its storage is released
// ('assert_released'); the formatter sorts the methods by name, so by phase:
//
//   parse          'add_type', 'add_type_param', 'add_func',
//                  'add_func_instance', 'add_generic_func',
//                  'add_generic_method', 'add_generic_type',
//                  'add_generic_type_instance', 'add_dat',
//                  'mark_generic_func_instantiated', 'enter_type_params',
//                  'exit_type_params', 'assert_generic_param_apart',
//                  'assert_generic_param_free', 'assert_generic_params_free',
//                  'assert_method_type_param_free',
//                  'assert_valid_unqualified_name', 'begin_definition',
//                  'current_definition', 'instance_visibility',
//                  'constant_value_of', 'record_use',
//                  'record_storage_use', 'record_result_passed_as_mut',
//                  'parse_nesting_scope'
//   compile        'add_binding', 'add_bytes_constant', 'add_string_constant',
//                  'add_noinline_body', 'dry_run', 'check_skipped',
//                  'is_checking_without_call', 'emit_most_efficient',
//                  'create_cmp_label', 'next_frame_address', the reachability
//                  ('is_reachable', 'end_path', 'record_break',
//                  'record_continue', 'record_return', 'join_loop',
//                  'path_merge')
//   both           'add_var' and 'add_const' (placed and commented only while
//                  compiling), 'make_var_read_only', 'scope_entry',
//                  'resolve_declared', 'get_func_or_throw',
//                  'get_main_or_throw', 'get_type_or_throw', 'is_func',
//                  'use_of', 'use_at', 'overflow_options', 'checks_overflow',
//                  'source_of', 'storage_of', 'call_path_at', 'where' and
//                  'create_unique_label' (a loop scope of the parse has a
//                  label)
//   tables         'builtins', 'checks', 'data',
//                  'funcs', 'generics', 'layout', 'machine',
//                  'noinline_worklist', 'scopes', 'types', 'usage'
//
// a statement may be compiled more than once (each inlined call, both
// versions of 'emit_most_efficient', dry runs, 'check_body'), so what a
// compile adds here is the same each time: a constant is kept once by its
// bytes ('add_read_only_constant'), a noinline body once
// ('add_noinline_body'), the scopes and their variables are popped
// ('scope_entry'); the peaks ('compile_peaks') are put back by a dry run
// with the constant count ('dry_run_scope') and set to those of the kept
// version by 'emit_most_efficient'
class toc final {
    // the most levels of blocks and expressions inside each other, the parsers
    // recurse for each of them so the limit keeps the source from exhausting
    // the stack
    static constexpr size_t max_parse_nesting_{128};

    std::reference_wrapper<::machine> machine_;
    source_locations locations_;
    // before the tables, which refer to it
    definition_order order_;
    scope_stack scopes_;
    data_table data_;
    function_table funcs_;
    ::noinline_worklist noinline_worklist_;
    generic_registry generics_;
    type_table types_;
    definition_checks definitions_;
    builtin_roles builtins_;
    ident_resolver resolver_;
    variable_comments var_comments_;
    storage_layout storage_;
    check_options checks_{};
    // above zero inside 'dry_run': no noinline body is added and the storage
    // capacity is not checked there
    size_t dry_run_depth_{};
    // false while the program is parsed: the names are declared without
    // storage and without comments
    bool is_compiling_{};
    // the first global that is not a 'dat', a 'dat' after it is rejected
    std::optional<token> first_global_var_tk_;
    // the parsers recurse on the one stack, e.g. a generic instance is parsed
    // inside the parse that needs it, so the levels are counted once for all
    // of them, see 'parse_nesting_scope'
    size_t parse_nesting_{};

    // the peaks that compiling raises, put back by a dry run and set to those
    // of the kept version by 'emit_most_efficient'
    struct compile_peaks {
        size_t max_scope_count{};
        storage_layout::peaks storage{};
    };

    // what a dry run and a version that is not kept put back: the peaks and
    // whether the code after is reached, which an error unwinding an inlined
    // body ends ('pop_scope')
    struct compile_state {
        compile_peaks peaks{};
        scope_stack::reach_state reach{};
    };

    // what a dry run changes in 'toc' is put back when it ends, also when an
    // error unwinds: the 'compile_state' and the string constants; while it
    // lives a noinline body that a call asks for is only checked, see
    // 'add_noinline_body'
    class dry_run_scope final {
        toc& tc_;
        compile_state kept_state_{};
        size_t kept_string_constant_count_{};

      public:
        explicit dry_run_scope(toc& tc)
            : tc_{tc}, kept_state_{tc.compile_state_now()},
              kept_string_constant_count_{tc.data_.constant_count()} {

            ++tc_.dry_run_depth_;
        }

        dry_run_scope(const dry_run_scope&) = delete;
        dry_run_scope(dry_run_scope&&) = delete;
        auto operator=(const dry_run_scope&) -> dry_run_scope& = delete;
        auto operator=(dry_run_scope&&) -> dry_run_scope& = delete;

        ~dry_run_scope() {
            --tc_.dry_run_depth_;
            tc_.restore_compile_state(kept_state_);
            tc_.data_.resize_constants(kept_string_constant_count_);
        }
    };

  public:
    // the paths of an 'if' start where the condition before them is false,
    // the first at the 'if', of an 'and'/'or' list at an element that may end
    // it; the end is reached when one path that runs reaches it; set when the
    // merge ends, also when an error unwinds, like
    // 'assignment_flow::path_merge'
    class path_merge final {
        toc& tc_;
        bool is_reachable_{};

      public:
        explicit path_merge(toc& tc) : tc_{tc} {}

        path_merge(const path_merge&) = delete;
        path_merge(path_merge&&) = delete;
        auto operator=(const path_merge&) -> path_merge& = delete;
        auto operator=(path_merge&&) -> path_merge& = delete;

        ~path_merge() { tc_.scopes_.set_reachable(is_reachable_); }

        auto begin_path(const bool is_start_reachable) -> void {
            tc_.scopes_.set_reachable(is_start_reachable);
        }

        // the path that ran ends here
        auto join_path() -> void {
            is_reachable_ = is_reachable_ or tc_.is_reachable();
        }
    };

    // a scope of the scope stack while it lives, also when an error unwinds,
    // so a failed version or dry run leaves the scopes of its caller, e.g.
    // 'const toc::scope_entry block{tc, scope::block()};'; a function scope
    // compiled is also a frame of the register trace, named by the function
    // and called at 'call_site_tk', so the two stacks agree
    class scope_entry final {
        toc& tc_;
        // the scopes below this one, the scopes are left in stack order
        size_t outer_scope_count_{};
        std::optional<::machine::register_trace_scope> trace_frame_;

      public:
        scope_entry(toc& tc, scope scp, const token& call_site_tk = {})
            : tc_{tc}, outer_scope_count_{tc.scopes_.count()} {

            if (tc.is_compiling_ and scp.is_func()) {
                trace_frame_.emplace(
                    tc.machine(), call_site_tk, std::string{scp.name()},
                    scp.is_compiled_in_place()
                        ? ::machine::frame_kind::inlined
                        : ::machine::frame_kind::noinline_body);
            }

            tc_.scopes_.push(std::move(scp));
        }

        scope_entry(const scope_entry&) = delete;
        scope_entry(scope_entry&&) = delete;
        auto operator=(const scope_entry&) -> scope_entry& = delete;
        auto operator=(scope_entry&&) -> scope_entry& = delete;

        ~scope_entry() {
            assert(tc_.scopes_.count() == outer_scope_count_ + 1);

            tc_.pop_scope();
        }
    };

    // one level of nesting while it lives
    class parse_nesting_scope final {
        toc& tc_;

      public:
        parse_nesting_scope(toc& tc, const tokenizer& tz) : tc_{tc} {
            if (tc_.parse_nesting_ >= max_parse_nesting_) {
                throw compile_error{
                    tz, std::format("nesting is too deep, the limit is {} "
                                    "levels",
                                    max_parse_nesting_)};
            }

            ++tc_.parse_nesting_;
        }

        parse_nesting_scope(const parse_nesting_scope&) = delete;
        parse_nesting_scope(parse_nesting_scope&&) = delete;
        auto operator=(const parse_nesting_scope&)
            -> parse_nesting_scope& = delete;
        auto operator=(parse_nesting_scope&&) -> parse_nesting_scope& = delete;

        ~parse_nesting_scope() { --tc_.parse_nesting_; }
    };

    // 'builtins' are the types of the language, shared with the machine
    toc(::machine& backend, const builtin_type_set& builtins,
        const source_files& files, const size_t vars_capacity_bytes,
        const check_options& checks)
        : machine_{backend}, locations_{files}, scopes_{order_},
          data_{backend.data_alignment()}, funcs_{order_}, generics_{order_},
          types_{order_},
          definitions_{scopes_, funcs_, types_, generics_, locations_},
          builtins_{backend, builtins},
          resolver_{scopes_, backend, funcs_, generics_, builtins_},
          var_comments_{backend, resolver_},
          storage_{backend, scopes_, data_, vars_capacity_bytes},
          checks_{checks} {

        // the built-in types are looked up by name like the defined ones
        for (const type* const tp : builtins.all()) {
            types_.add_builtin(tp->name(), *tp);
        }

        // the default type has a name of its own so that it is portable
        types_.add_builtin(reserved_names::default_type_name,
                           builtins_.default_type());
    }

    auto add_binding(const param_binding& binding) -> void {
        assert(is_compiling_);

        scopes_.back().add_binding(binding);
    }

    // e.g. the packed elements of a constant '{...}' initializer
    [[nodiscard]] auto add_bytes_constant(const token& src_loc_tk,
                                          const std::string_view bytes)
        -> std::string {

        assert(is_compiling_);

        return add_read_only_constant("init", src_loc_tk, std::string{bytes});
    }

    auto add_const(const token& src_loc_tk, const size_t indent,
                   const std::string_view name, const folded_value& constant)
        -> void {

        definitions_.assert_name_free(src_loc_tk, name);
        definitions_.assert_not_hiding_signature(src_loc_tk, "constant", name);

        if (is_compiling_) {
            ::machine& x{machine()};

            x.comment(src_loc_tk, indent, "const {} = {}", name,
                      constant.value);
        }

        scopes_.back().add_const(name, {
                                           .src_loc_tk{src_loc_tk},
                                           .constant{constant},
                                       });

        record_if_global(name);
    }

    // the parse places the data of 'dat' after the dats before it and
    // declares the variable 'var' that names it, the compile declares it again
    // with 'add_var'; 'dat_tk' locates the errors of where it is declared
    auto add_dat(const stmt_def_dat& dat, const token& dat_tk,
                 const var_info& var) -> void {

        assert(not is_compiling_);

        // note: a name declared twice is reported before where it is declared
        definitions_.assert_name_free(var.src_loc_tk, var.name);

        // note: a 'dat' in a block is rejected when it is parsed
        assert(scopes_.is_at_root());

        if (first_global_var_tk_) {
            throw compile_error{
                dat_tk,
                std::format("'dat' must be before the first "
                            "variable, {} at {}",
                            in_quotes(first_global_var_tk_->text()),
                            locations_.for_message(*first_global_var_tk_))};
        }

        const size_t size_bytes{
            storage_.var_storage_size_bytes(var.src_loc_tk, var),
        };

        data_.add_dat(dat, {
                               .name_tk{var.src_loc_tk},
                               .alignment{storage_.var_storage_alignment(var)},
                               .size_bytes{size_bytes},
                           });

        // before it is placed, whose offsets assume a size the target
        // addresses
        storage_.assert_total_fits_target(var.src_loc_tk,
                                          data_.total_size_bytes());

        add_var(var.src_loc_tk, 0, var, var_kind::dat);
    }

    // see 'definition_order'
    auto add_func(const token& src_loc_tk, std::string name,
                  const stmt_def_func& func_def, std::string generic_method,
                  const bool is_ordered) -> void {

        assert(not is_compiling_);

        definitions_.assert_function_not_defined(src_loc_tk, name,
                                                 generic_method);

        funcs_.add(src_loc_tk, std::move(name), func_def,
                   std::move(generic_method),
                   is_ordered ? order_.current() : 0);
    }

    // keeps the instance alive, 'funcs_' refers to it
    auto add_func_instance(std::shared_ptr<const stmt_def_func> instance)
        -> void {

        assert(not is_compiling_);

        funcs_.add_instance(std::move(instance));
    }

    auto add_generic_func(std::string name, generic_func_info info) -> void {
        assert(not is_compiling_);

        definitions_.assert_function_not_defined(
            info.src_loc_tk, name, info.receiver ? info.report_name : "");

        generics_.add_func(std::move(name), std::move(info));
    }

    // a method is defined once for its generic type, also when no alias
    // makes an instance of the type
    auto add_generic_method(const generic_method_info& method) -> void {
        assert(not is_compiling_);

        definitions_.assert_generic_method_not_defined(method);

        generics_.add_method(method);
    }

    auto add_generic_type(const std::string_view name, generic_type_info info)
        -> void {

        assert(not is_compiling_);

        definitions_.assert_type_not_defined(info.src_loc_tk, name);

        generics_.add_type(name, std::move(info));
    }

    auto add_generic_type_instance(generic_type_instance instance) -> void {
        assert(not is_compiling_);

        generics_.add_type_instance(std::move(instance));
    }

    // a body is emitted once per array lengths, for a call that is
    // compiled for real; a call in a dry run asks for a body to check
    auto add_noinline_body(const stmt_def_func& func,
                           std::vector<size_t> array_lengths,
                           const token& call_tk, std::string call_source)
        -> void {

        assert(is_compiling_);

        if (dry_run_depth_ != 0) {
            noinline_worklist_.add_checked_body(func, std::move(array_lengths),
                                                call_tk,
                                                std::move(call_source));

            return;
        }

        noinline_worklist_.add_noinline_body(func, std::move(array_lengths),
                                             call_tk, std::move(call_source));
    }

    // identical strings share the label of the first one compiled
    [[nodiscard]] auto add_string_constant(const token& string_tk)
        -> std::string {

        assert(is_compiling_);

        return add_read_only_constant("str", string_tk,
                                      string_tk.string_bytes());
    }

    auto add_type(const token& src_loc_tk, const type& tpe) -> void {
        assert(not is_compiling_);

        definitions_.assert_type_not_defined(src_loc_tk, tpe.name());
        assert_type_fits_target(src_loc_tk, tpe);

        types_.add(src_loc_tk, tpe);
    }

    // e.g. 'T' names 'i8' in an instance of 'func f<T type>()', until
    // 'exit_type_params'
    auto add_type_param(const token& src_loc_tk, const std::string_view name,
                        const type& tpe) -> void {

        assert(not is_compiling_);

        definitions_.assert_generic_param_not_type(src_loc_tk, name);
        types_.add_param(src_loc_tk, name, tpe);
    }

    auto add_var(const token& src_loc_tk, const size_t indent, var_info var,
                 const var_kind kind) -> void {

        if (kind == var_kind::param or kind == var_kind::result) {
            definitions_.assert_not_declared_in_scope(src_loc_tk, var.name);
        } else {
            definitions_.assert_name_free(src_loc_tk, var.name);

            definitions_.assert_not_hiding_signature(src_loc_tk, "variable",
                                                     var.name);
        }

        // a variable in a register or at one has no storage; the parse only
        // declares the names, the compile places them
        const bool is_in_storage{
            std::holds_alternative<in_storage>(var.placement),
        };

        // note: a 'dat' here is the compile half of 'add_dat'
        if (is_compiling_ and kind == var_kind::dat) {
            storage_.place_dat(src_loc_tk, var);
        } else if (is_compiling_ and is_in_storage) {
            storage_.allocate_var(src_loc_tk, var, checks_storage_capacity());
        }

        scopes_.back().add_var(var, kind);

        record_if_global(var.name);

        if (scopes_.is_at_root() and kind != var_kind::dat and
            not first_global_var_tk_) {

            first_global_var_tk_ = src_loc_tk;
        }

        if (is_compiling_) {
            var_comments_.comment(src_loc_tk, indent, var);
        }
    }

    // see 'definition_checks::assert_generic_param_apart'
    auto assert_generic_param_apart(const token& param_tk, const token& name_tk,
                                    const std::string_view what) const -> void {

        assert(not is_compiling_);

        definitions_.assert_generic_param_apart(param_tk, name_tk, what);
    }

    // a generic parameter is a name like the others, e.g. not a keyword
    auto assert_generic_param_free(const token& name_tk) const -> void {
        assert(not is_compiling_);

        assert_valid_unqualified_name(name_tk);
        definitions_.assert_generic_param_not_type(name_tk, name_tk.text());
        definitions_.assert_generic_param_not_value(name_tk, name_tk.text());

        definitions_.assert_generic_param_not_generic_type(name_tk,
                                                           name_tk.text());
    }

    // the parse is over: a type or a global value defined after a generic
    // definition is checked against its parameters too, so the order of the
    // definitions does not matter
    auto assert_generic_params_free() const -> void {
        assert(not is_compiling_);

        for (const token& param_tk : generics_.param_tokens()) {
            assert_generic_param_free(param_tk);
        }
    }

    // a type parameter of a method of the generic type 'generic_type'
    auto
    assert_method_type_param_free(const token& name_tk,
                                  const std::string_view generic_type) const
        -> void {

        assert(not is_compiling_);

        definitions_.assert_generic_param_not_receiver_param(
            name_tk, name_tk.text(), generic_type);
    }

    auto assert_released() const -> void {
        assert(scopes_.is_empty());

        storage_.assert_released();
    }

    // a name of a type, function, variable, constant, data or parameter, which
    // is used without a qualifier
    auto assert_valid_unqualified_name(const token& name_tk) const -> void {
        assert(not is_compiling_);

        assert_valid_name(name_tk);

        if (reserved_names::find_builtin_function(name_tk.text()) != nullptr) {
            throw compile_error{name_tk,
                                std::format("{} is a built-in function",
                                            in_quotes(name_tk.text()))};
        }

        // a value would hide the type only in some places, e.g. 'i8(300)'
        // still converts after 'var i8 = 3'
        if (types_.is_builtin(name_tk.text())) {
            throw compile_error{name_tk,
                                std::format("{} is a built-in type",
                                            in_quotes(name_tk.text()))};
        }

        // e.g. 'U' bound to 'i8'
        if (const type* const builtin{builtin_scalar_named(name_tk)}) {
            throw compile_error{name_tk,
                                std::format("{} names the built-in type {}",
                                            in_quotes(name_tk.text()),
                                            in_quotes(builtin->name()))};
        }
    }

    // the parse is over: its scopes are popped, it declared the names
    // without storage; from here on the declarations are placed, comment
    // where they are stored and the storage capacity is checked, e.g. a
    // parameter 'self' of a type larger than the vars section holds an
    // address once the body is compiled
    auto begin_compile() -> void {
        assert(not is_compiling_);
        assert(scopes_.is_empty());

        // note: the parse declared the names without storage
        storage_.assert_released();
        scopes_.set_max_scope_count(0);

        // note: the parse compiles no call
        assert(noinline_worklist_.is_empty());

        is_compiling_ = true;
    }

    // a top-level definition starts, see 'definition_order'
    auto begin_definition() -> void {
        assert(not is_compiling_);

        order_.begin_definition();
    }

    // the tables are read through these const accessors, a change goes
    // through a method of 'toc', which may combine tables (README 7.6)
    [[nodiscard]] auto builtins() const -> const builtin_roles& {
        return builtins_;
    }

    // the location of 'src_loc_tk' and the call path of the inlined bodies
    // around it, which keeps labels unique per call, e.g. '12.5.30.9' for
    // 12:5 in a body inlined by the call at 30:9; the labels of a listing:
    //
    //   if.L.C         an 'if' at line L, column C; '.N' its branch N,
    //                  '.code', '.else', '.end' ('stmt_if::branch_label')
    //   cmp.L.C        a condition ('create_cmp_label')
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
    //                  '.Lbaz_equal.0' (x86_64), '.Lbaz_jump.0' (rv32i)
    //
    // e.g. 'if.7.5.36.13' is the 'if' at 7:5 inlined by the call at 36:13;
    // inside a noinline body for array lengths every label ends in them, e.g.
    // 'loop.14.5.len.3' ('stmt_def_func::body_call_path'); a location in an
    // included file is 'fN.L.C' ('source_locations::for_label')
    [[nodiscard]] auto call_path_at(const token& src_loc_tk) const
        -> std::string {

        const std::string_view call_path{scopes_.call_path()};

        if (call_path.empty()) {
            return locations_.for_label(src_loc_tk);
        }

        return std::format("{}.{}", locations_.for_label(src_loc_tk),
                           call_path);
    }

    // code that the compile skips, e.g. a branch that folds to false or the
    // code after a statement that leaves on every path, is compiled for its
    // checks where what it folds is no constant of a call: in 'main' and in
    // the body check of a function, and in the initializer of a global, which
    // takes nothing from a call either; there every compile rule holds in all
    // code but the capacity of '--vars', its locals take no storage; in a body
    // that takes from a call an error that depends on a call is dropped as in
    // that check, any other rejects the program
    auto check_skipped(const std::function_ref<void()> compile) -> void {
        assert(is_compiling_);

        if (scopes_.is_in_func() and
            not scopes_.current_func_scope().is_without_call()) {

            return;
        }

        const bool takes_from_call{
            scopes_.is_in_func() and
                scopes_.current_func_scope().takes_from_call(),
        };

        // the dry run puts back the reach of the code after it
        std::optional<compile_error> error{
            dry_run(
                [&] -> void {
                    scopes_.set_reachable(true);
                    compile();
                },
                machine::discarded_code::here),
        };

        if (error and not(takes_from_call and error->depends_on_call())) {
            throw_dry_run_error(std::move(*error));
        }
    }

    // what '--checks' turned on
    [[nodiscard]] auto checks() const -> const check_options& {
        return checks_;
    }

    // whether the arithmetic of a value that 'dst' receives is checked for
    // overflow
    [[nodiscard]] auto checks_overflow(const destination& dst) const -> bool {
        return overflow_options(dst).enabled;
    }

    // a number, untyped, or a constant with its type
    [[nodiscard]] auto constant_value_of(const token& tk) const
        -> std::optional<folded_value> {

        assert(not is_compiling_);

        if (const std::optional<int64_t> value{
                constant_parser::parse_constant(tk, tk.text()),
            }) {

            return folded_value::untyped(*value);
        }

        return scopes_.const_value(tk.text());
    }

    // the label of the start of a comparison or of a list of comparisons
    [[nodiscard]] auto create_cmp_label(const token& src_loc_tk) const
        -> std::string {

        assert(is_compiling_);

        return create_unique_label(src_loc_tk, "cmp");
    }

    [[nodiscard]] auto create_unique_label(const token& src_loc_tk,
                                           const std::string_view prefix) const
        -> std::string {

        return std::format("{}.{}", prefix, call_path_at(src_loc_tk));
    }

    [[nodiscard]] auto current_definition() const -> size_t {
        assert(not is_compiling_);

        return order_.current();
    }

    [[nodiscard]] auto data() const -> const data_table& { return data_; }

    // runs 'compile' for its errors only: its output, string constants and
    // use of storage leave no trace; the error it ends at, if any, is handed
    // back, see 'throw_dry_run_error'; 'code' says which registers it starts
    // with ('machine::emit_discarded')
    [[nodiscard]] auto dry_run(const std::function_ref<void()> compile,
                               const machine::discarded_code code)
        -> std::optional<compile_error> {

        assert(is_compiling_);

        const dry_run_scope scope{*this};

        ::machine& x{machine()};

        try {
            x.emit_discarded(compile, code);
        } catch (compile_error& e) {
            return std::move(e);
        }

        return std::nullopt;
    }

    // 'machine::emit_most_efficient' with each version compiled from the
    // state before it ('compile_state'), so the state that remains is that of
    // the kept version, e.g. the frame size of a noinline body; a string
    // constant that the other version added stays, unused, so the labels stay
    // unique
    auto emit_most_efficient(const token& src_loc_tk, const size_t indent,
                             const std::function_ref<void()> emit_first,
                             const std::function_ref<void()> emit_second)
        -> void {

        assert(is_compiling_);

        const compile_state start{compile_state_now()};

        compile_state first_end{start};
        compile_state second_end{start};

        const auto first = [&] -> void {
            restore_compile_state(start);
            emit_first();
            first_end = compile_state_now();
        };

        const auto second = [&] -> void {
            restore_compile_state(start);
            emit_second();
            second_end = compile_state_now();
        };

        ::machine& x{machine()};

        const bool keeps_first{
            x.emit_most_efficient(src_loc_tk, indent, first, second),
        };

        restore_compile_state(keeps_first ? first_end : second_end);
    }

    // e.g. 'exit' leaves the code after it unreached
    auto end_path() -> void {
        assert(is_compiling_);

        scopes_.set_reachable(false);
    }

    // the type parameters of an enclosing instance are hidden until
    // 'exit_type_params'
    auto enter_type_params() -> void {
        assert(not is_compiling_);

        types_.enter_params();
    }

    auto exit_type_params() -> void {
        assert(not is_compiling_);

        types_.exit_params();
    }

    [[nodiscard]] auto funcs() const -> const function_table& { return funcs_; }

    [[nodiscard]] auto generics() const -> const generic_registry& {
        return generics_;
    }

    [[nodiscard]] auto get_func_or_throw(const token& src_loc_tk,
                                         const std::string_view name) const
        -> const stmt_def_func& {

        if (funcs_.has(name)) {
            return *funcs_.get(name).def;
        }

        // e.g. 'x(1)' after 'var x = 1', a call still calls a hidden function
        if (scopes_.lookup(name).is_value()) {
            throw compile_error{
                src_loc_tk,
                std::format("{} is a value, not a function", in_quotes(name))};
        }

        throw compile_error{
            src_loc_tk, std::format("function {} not found", in_quotes(name))};
    }

    // no token names a missing 'main'
    [[nodiscard]] auto get_main_or_throw() const -> const stmt_def_func& {
        if (not funcs_.has(reserved_names::main)) {
            throw compile_error::file_level("function 'main' not found");
        }

        return *funcs_.get(reserved_names::main).def;
    }

    [[nodiscard]] auto get_type_or_throw(const token& type_tk) const
        -> const type& {

        // e.g. the '"i8"' of 'func f() res "i8" {'
        if (type_tk.is_string()) {
            throw compile_error{type_tk, "expected a type name, got a string"};
        }

        const std::string_view name{type_tk.text()};

        if (not types_.has(name)) {
            if (generics_.has_type(name)) {
                throw compile_error{
                    type_tk, std::format("generic type {} is not a type, {}",
                                         in_quotes(name), instance_hint(name))};
            }

            throw compile_error{
                type_tk, std::format("type {} not found", in_quotes(name))};
        }

        const type& tp{types_.get(name)};

        // note: 'void' is the type of what has no value, it has no size
        if (tp.is_same(builtins_.void_type())) {
            throw compile_error{
                type_tk,
                std::format("{} is not a type of a value", in_quotes(name))};
        }

        // the default type is the widest integer the target computes with,
        // e.g. 'i32' on rv32i
        const type& widest{builtins_.default_type()};

        if (tp.is_builtin() and tp.size_bytes() > widest.size_bytes()) {
            throw compile_error{
                type_tk,
                std::format("{} is not available on this target, use {} "
                            "or 'int'",
                            in_quotes(name), in_quotes(widest.name()))};
        }

        return tp;
    }

    // an instance parse sees the definitions up to 'definition' while the
    // result lives
    [[nodiscard]] auto instance_visibility(const size_t definition)
        -> definition_order::instance_scope {

        assert(not is_compiling_);

        return {order_, definition};
    }

    // inside the check of a body that takes from a call, see
    // 'stmt_def_func::check_body': a recursive inlined call is not expanded
    // there ('stmt_call::check_arguments_alone')
    [[nodiscard]] auto is_checking_without_call() const -> bool {
        assert(is_compiling_);

        return scopes_.is_in_body_taking_from_call();
    }

    [[nodiscard]] auto is_func(const std::string_view name) const -> bool {
        return funcs_.has(name) or generics_.has_func(name);
    }

    // the code after the current statement of the function being compiled
    // runs on some path, the words of 'assignment_flow': 'end_path',
    // 'record_break', 'path_merge', and 'join_loop' for its 'trace_loop'; a
    // block stops compiling at an unreached statement, e.g. after the
    // 'return' of 'if n == 0 return' in an inlined call with the constant
    // argument 0; outside a function, in the initializer of a global, every
    // path continues, it makes no call
    [[nodiscard]] auto is_reachable() const -> bool {
        assert(is_compiling_);

        if (not scopes_.is_in_func()) {
            return true;
        }

        return scopes_.current_func_scope().is_reachable();
    }

    // the end of a loop is reached by a compiled 'break' or as its
    // 'loop_end' says; called as the last step inside the loop's scope, where
    // the end of its body is the current statement
    auto join_loop() -> void {
        assert(is_compiling_);

        const scope& loop{scopes_.back()};

        assert(loop.is_loop());

        const loop_end end{loop.loop_end_kind()};

        const bool is_step_reached{is_reachable() or loop.is_continued()};

        const bool runs_out{
            end == loop_end::also_when_done or
                (end == loop_end::when_stepped and is_step_reached),
        };

        scopes_.set_reachable(runs_out or loop.is_left_by_break());
    }

    [[nodiscard]] auto layout() const -> const storage_layout& {
        return storage_;
    }

    [[nodiscard]] auto machine() -> ::machine& { return machine_.get(); }

    // the variable declared last in this scope, e.g. a 'let' variable once
    // its initializer is parsed
    auto make_var_read_only(const std::string_view name,
                            const read_only_cause cause) -> void {

        scopes_.back().make_var_read_only(name, cause);
    }

    auto mark_generic_func_instantiated(const std::string_view name) -> void {
        assert(not is_compiling_);

        generics_.mark_func_instantiated(name);
    }

    // a callee frame starts aligned for any variable it holds
    [[nodiscard]] auto next_frame_address() const -> operand {
        assert(is_compiling_);

        return storage_.next_frame_address(builtins_.address());
    }

    [[nodiscard]] auto noinline_worklist() const -> const ::noinline_worklist& {
        return noinline_worklist_;
    }

    [[nodiscard]] auto overflow_options(const destination& dst) const
        -> machine::runtime_check {

        if (dst.wraps()) {
            return {};
        }

        return checks_.overflow_options();
    }

    // a 'break' leaves the innermost 'loop' or 'foo', whose end is then
    // reached, and the code after it unreached
    auto record_break() -> void {
        assert(is_compiling_);

        scopes_.record_break();
        end_path();
    }

    // a 'continue' jumps to the head of the innermost 'loop' or the counter
    // step of a 'foo' and leaves the code after it unreached, see
    // 'loop_end::when_stepped'
    auto record_continue() -> void {
        assert(is_compiling_);

        scopes_.record_continue();
        end_path();
    }

    // see 'body_uses::is_result_passed_as_mut'
    auto record_result_passed_as_mut() -> void {
        assert(not is_compiling_);

        scopes_.record_result_passed_as_mut();
    }

    // a 'return' leaves the code after it unreached and reaches the end of an
    // inlined call
    auto record_return() -> void {
        assert(is_compiling_);

        scopes_.record_return();
        end_path();
    }

    // a resolved 'storage' the body being parsed uses, e.g. a global that a
    // function it calls uses, so a local of the same name does not shadow it
    auto record_storage_use(const storage_path& storage, const use_kind kind)
        -> void {

        assert(not is_compiling_);

        scopes_.record_storage_use(storage, kind == use_kind::write);
    }

    // a path in a function body whose root is 'name', see 'body_uses';
    // outside a function nothing is recorded
    auto record_use(const std::string_view name, const access_path& path,
                    const use_kind kind) -> void {

        assert(not is_compiling_);

        if (not scopes_.is_in_func()) {
            return;
        }

        // note: the parse records the uses, no name is bound to an argument

        if (const std::optional<storage_path> storage{storage_of(name, path)}) {
            record_storage_use(*storage, kind);
        }
    }

    // the declared name 'ident', e.g. the whole array of 'ps[1]'; the free
    // 'make_ident_info' of a path gives what the path names, its element
    [[nodiscard]] auto resolve_declared(const token& src_loc_tk,
                                        const std::string_view ident) const
        -> ident_info {

        return resolver_.resolve(src_loc_tk, ident);
    }

    [[nodiscard]] auto scopes() const -> const scope_stack& { return scopes_; }

    // the text of the file of the token
    [[nodiscard]] auto source_of(const token& src_loc_tk) const
        -> std::string_view {

        return locations_.source_of(src_loc_tk);
    }

    // see 'ident_resolver::resolve_storage'
    [[nodiscard]] auto storage_of(const std::string_view name,
                                  const access_path& path) const
        -> std::optional<storage_path> {

        return resolver_.resolve_storage(name, path);
    }

    [[nodiscard]] auto types() const -> const type_table& { return types_; }

    [[nodiscard]] auto usage() const -> usage_statistics {
        return {
            .max_scope_count{scopes_.max_scope_count()},
            .max_body_vars_size_bytes{storage_.max_body_vars_size_bytes()},
            .dat_size_bytes{data_.total_size_bytes()},
            .dat_var_padding_bytes{data_.dat_var_padding_bytes()},
            .uninstantiated_generics{generics_.uninstantiated_func_names()},
        };
    }

    // a value used without a destination, e.g. a comparison side or an
    // argument, is checked as '--checks' asks
    [[nodiscard]] auto use_at(const type& width) const -> value_use {
        return {
            .width{&width},
            .checks_overflow{checks_.overflow},
        };
    }

    [[nodiscard]] auto use_of(const destination& dst) const -> value_use {
        return {
            .width{&dst.type_ref()},
            .checks_overflow{checks_overflow(dst)},
        };
    }

    // where 'src_loc_tk' is, for a message, e.g. '4:9'
    [[nodiscard]] auto where(const token& src_loc_tk) const -> std::string {
        return locations_.for_message(src_loc_tk);
    }

    //
    // statics
    //

    // a name is an identifier that is not a keyword
    static auto assert_valid_name(const token& name_tk) -> void {
        if (not name_tk.is_word()) {
            throw compile_error{name_tk, "expected a name"};
        }

        if (not is_identifier(name_tk)) {
            throw compile_error{
                name_tk,
                std::format("{} is not a valid name, a name starts with a "
                            "letter or '_' and has only letters, digits and "
                            "'_'",
                            in_quotes(name_tk.text()))};
        }

        if (std::ranges::contains(reserved_names::keywords, name_tk.text())) {
            throw compile_error{
                name_tk,
                std::format("{} is a keyword", in_quotes(name_tk.text()))};
        }
    }

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

    // an error that ends a dry run is reported as an error of the source: the
    // check of a body starts from the registers of a noinline body, and
    // skipped code is never inside a version, an inlined body skips no code,
    // see 'check_skipped'
    [[noreturn]] static auto throw_dry_run_error(compile_error error) -> void {
        error.kind = compile_error::error_kind::source;
        throw error;
    }

  private:
    // the same bytes share a label, see 'data_table::add_constant'
    [[nodiscard]] auto add_read_only_constant(const std::string_view kind,
                                              const token& src_loc_tk,
                                              std::string bytes)
        -> std::string {

        // note: the bytes of a literal may differ per call, e.g. '{k, 2}'
        return data_.add_constant(create_unique_label(src_loc_tk, kind),
                                  std::move(bytes));
    }

    // a value of the type, e.g. a parameter, is addressed by the target even
    // when no storage of it is ever allocated; a value is smaller than the
    // address space, so its size and element counts fit an address
    auto assert_type_fits_target(const token& src_loc_tk, const type& tpe) const
        -> void {

        const ::machine& x{machine_.get()};

        const size_t max_bytes{x.max_storage_bytes()};

        if (tpe.size_bytes() < max_bytes) {
            return;
        }

        throw compile_error{
            src_loc_tk,
            std::format("type {} of {} B is too large, the target addresses "
                        "{} B",
                        in_quotes(tpe.name()), tpe.size_bytes(), max_bytes)};
    }

    // the built-in integer or 'bool' type that 'tk' names, also through a
    // type parameter, e.g. 'U' bound to 'i8'; null for any other token
    [[nodiscard]] auto builtin_scalar_named(const token& tk) const
        -> const type* {

        if (not tk.is_word() or not types_.has(tk.text())) {
            return nullptr;
        }

        const type& tp{types_.get(tk.text())};

        if (not tp.is_bool() and not tp.is_integer()) {
            return nullptr;
        }

        return &tp;
    }

    // a dry run emits nothing, its locals take no variable storage
    [[nodiscard]] auto checks_storage_capacity() const -> bool {
        return dry_run_depth_ == 0;
    }

    [[nodiscard]] auto compile_state_now() const -> compile_state {
        return {
            .peaks{peaks_now()},
            .reach{scopes_.reach_state_now()},
        };
    }

    [[nodiscard]] auto peaks_now() const -> compile_peaks {
        return {
            .max_scope_count{scopes_.max_scope_count()},
            .storage{storage_.peaks_now()},
        };
    }

    // a scope is pushed by 'scope_entry' and popped here with its storage; an
    // inlined call whose body does not reach its end, e.g. a 'loop' whose
    // 'break' folds away for a constant argument, leaves the code after the
    // call unreached
    auto pop_scope() -> void {
        const scope& popped{scopes_.back()};

        const bool ends_caller_path{
            popped.is_inlined_func() and not popped.is_end_reached(),
        };

        scopes_.pop();

        if (ends_caller_path) {
            end_path();
        }
    }

    // a global keeps the definition that declared it at the parse, the
    // compile declares it again
    auto record_if_global(const std::string_view name) -> void {
        if (scopes_.is_at_root()) {
            scopes_.record_global(name);
        }
    }

    auto restore_compile_state(const compile_state& state) -> void {
        restore_peaks(state.peaks);
        scopes_.restore_reach_state(state.reach);
    }

    auto restore_peaks(const compile_peaks& peaks) -> void {
        scopes_.set_max_scope_count(peaks.max_scope_count);
        storage_.restore_peaks(peaks.storage);
    }

    //
    // statics
    //

    // letters, digits and '_', not starting with a digit, written without
    // quotes
    [[nodiscard]] static auto is_identifier(const token& name_tk) -> bool {
        const std::string_view text{name_tk.text()};

        const auto is_letter = [](const char ch) -> bool {
            return (ch >= 'a' and ch <= 'z') or (ch >= 'A' and ch <= 'Z') or
                   ch == '_';
        };

        return not text.empty() and is_letter(text.front()) and
               std::ranges::all_of(text, [&](const char ch) -> bool {
                   return is_letter(ch) or (ch >= '0' and ch <= '9');
               });
    }
};

//
// generics: an instance is made in two steps, see 'generics.hpp'
//   1. 'resolve_generic_args' reads the argument tokens of an alias or a base
//      as types and constants, e.g. '<i8, 8>' of 'list<T type, capacity>'
//      gives 'T' 'i8' and 'capacity' 8; a call reads its type arguments in
//      'stmt_call'
//   2. a 'generic_scope' makes them names while the text of the definition is
//      parsed again, 'type_table' holds the type names of the innermost scope
//

// a generic argument of the wrong kind, e.g. a constant for a type
// parameter; 'got' is what was given, e.g. "a string"
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
[[nodiscard]] inline auto resolve_type_arg(const toc& tc,
                                           const std::string_view generic_name,
                                           const std::string_view param_name,
                                           const token& arg_tk) -> const type& {

    if (arg_tk.is_string()) {
        throw_generic_arg_mismatch(arg_tk, generic_name, param_name, "type",
                                   "a string");
    }

    if (tc.constant_value_of(arg_tk)) {
        throw_generic_arg_mismatch(
            arg_tk, generic_name, param_name, "type",
            std::format("the constant {}", in_quotes(arg_tk.text())));
    }

    return tc.get_type_or_throw(arg_tk);
}

// e.g. '8' of 'list<i8, 8>' for 'capacity'
[[nodiscard]] inline auto resolve_const_arg(const toc& tc,
                                            const std::string_view generic_name,
                                            const std::string_view param_name,
                                            const token& arg_tk) -> int64_t {

    if (arg_tk.is_string()) {
        throw_generic_arg_mismatch(arg_tk, generic_name, param_name, "constant",
                                   "a string");
    }

    const std::optional<folded_value> constant{tc.constant_value_of(arg_tk)};

    if (not constant) {
        throw_generic_arg_mismatch(
            arg_tk, generic_name, param_name, "constant",
            std::format("{}{}",
                        tc.types().has(arg_tk.text()) ? "the type " : "",
                        in_quotes(arg_tk.text())));
    }

    // e.g. 'b' of 'let b = true', no number like 'true'
    if (constant->is_typed() and constant->own_type->is_bool()) {
        throw_generic_arg_mismatch(
            arg_tk, generic_name, param_name, "number",
            std::format("the 'bool' constant {}", in_quotes(arg_tk.text())));
    }

    return constant->value;
}

// a type parameter needs a type, a constant parameter a constant, e.g. 'T' is
// 'i8' and 'capacity' is 8 for '<i8, 8>' of 'list<T type, capacity>'
[[nodiscard]] inline auto
resolve_generic_args(const toc& tc, const std::string_view generic_name,
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
                .arg_tk{arg_tk},
                .value{&resolve_type_arg(tc, generic_name, name, arg_tk)},
            });
        } else {
            args.push_back({
                .param_tk{param.name_tk},
                .arg_tk{arg_tk},
                .value{resolve_const_arg(tc, generic_name, name, arg_tk)},
            });
        }
    }

    return args;
}

// while an instance of a generic definition is parsed, each parameter names
// its argument, e.g. 'T' names 'i8' and 'capacity' is the constant 8; the
// parameters of an enclosing instance are hidden and only the definitions up
// to the generic one are seen, all of it ends with the scope
class generic_scope final {
    toc& tc_;
    definition_order::instance_scope visible_;
    // the constants are in a block of their own
    std::optional<toc::scope_entry> constants_block_;

  public:
    generic_scope(toc& tc, const size_t definition)
        : tc_{tc}, visible_{tc.instance_visibility(definition)} {

        tc_.enter_type_params();
    }

    generic_scope(const generic_scope&) = delete;
    generic_scope(generic_scope&&) = delete;
    auto operator=(const generic_scope&) -> generic_scope& = delete;
    auto operator=(generic_scope&&) -> generic_scope& = delete;

    ~generic_scope() {
        constants_block_.reset();
        tc_.exit_type_params();
    }

    // a type is located at its parameter, a constant at its argument
    auto add_args(const std::span<const generic_arg> args) -> void {
        for (const generic_arg& arg : args) {
            if (arg.is_type()) {
                add_type(arg.param_tk, arg.param_tk.text(), arg.type_ref());
            } else {
                // note: a constant argument is always written
                assert(arg.arg_tk);

                add_const(*arg.arg_tk, arg.param_tk.text(), arg.constant());
            }
        }
    }

    // a method of an instance of a generic type sees the generic type as the
    // instance and its type arguments, e.g. 'list' names 'str' and 'T' names
    // 'i8'; its constants are in the scope of the body
    auto add_receiver(const generic_type_instance& instance) -> void {
        const generic_type_info& generic{
            tc_.generics().get_type(instance.generic_name),
        };

        add_type(generic.src_loc_tk, instance.generic_name, *instance.type_ptr);
        add_args(instance.type_args());
    }

  private:
    auto add_const(const token& src_loc_tk, const std::string_view name,
                   const int64_t value) -> void {

        if (not constants_block_) {
            constants_block_.emplace(tc_, scope::block());
        }

        tc_.add_const(src_loc_tk, 0, name, folded_value::untyped(value));
    }

    auto add_type(const token& src_loc_tk, const std::string_view name,
                  const type& tpe) -> void {

        tc_.add_type_param(src_loc_tk, name, tpe);
    }
};

// a generic definition parsed again for an instance while it lives: its
// parameters name the arguments the caller adds to 'scope()', 'tz()' reads
// from 'start_tk', and the parse counts toward the nesting of the parse
// that needs the instance
class generic_reparse final {
    generic_scope scope_;
    tokenizer tz_;
    toc::parse_nesting_scope nesting_;

  public:
    generic_reparse(toc& tc, const size_t definition, const token& start_tk)
        : scope_{tc, definition}, tz_{tc.source_of(start_tk), start_tk},
          nesting_{tc, tz_} {}

    generic_reparse(const generic_reparse&) = delete;
    generic_reparse(generic_reparse&&) = delete;
    auto operator=(const generic_reparse&) -> generic_reparse& = delete;
    auto operator=(generic_reparse&&) -> generic_reparse& = delete;
    ~generic_reparse() = default;

    [[nodiscard]] auto scope() -> generic_scope& { return scope_; }

    [[nodiscard]] auto tz() -> tokenizer& { return tz_; }
};
