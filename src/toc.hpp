#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "compile_error.hpp"
#include "constant_parser.hpp"
#include "generics.hpp"
#include "ident_info.hpp"
#include "lut.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "token.hpp"
#include "type.hpp"

//
// the table of contents: what the names of the program are while it is parsed
// and compiled, see 'etc/docs/README.md'. the classes below, in reading
// order:
//
//   records         'func_info', 'noinline_body', 'storage_target',
//                   'alias_info' (what a parameter of an inlined call stands
//                   for), 'const_info'
//   'frame'         one scope: its constants, variables and aliases
//   'ident_path'    a name as its path of elements, e.g. 'p.x'
//   'ident_builder' the 'ident_info' of a variable and of a path below a known
//                   address
//   tables          'type_table', 'function_table', 'data_table'
//   options         'check_options', 'usage_statistics'
//   'scope_stack'   the frames from the globals to the innermost scope and
//                   'lookup', the one rule of what a name means
//   'storage_layout' where variables are placed and how much storage is used
//   'builtin_types' the types the front end needs by role
//   'ident_resolver' resolves a name by walking the frames, following aliases
//   'definition_checks' the errors of a name defined twice
//   'variable_comments' the comment of where a variable is stored
//   'toc'           the facade the statements use, owns all of the above; a
//                   method of 'toc' changes state or combines tables, a query
//                   of one table goes through its const accessor, e.g.
//                   'tc.scopes().is_alias(name)'
//   generics        'resolve_generic_args' and 'generic_scope', the names of
//                   the parameters of a generic instance
//

class stmt_def_dat;
class stmt_def_func;

struct func_info {
    token src_loc_tk;           // the definition, empty for a built-in
    const stmt_def_func* def{}; // null if built-in function
    const type* type_ptr{};     // return type or void
    // e.g. 'text.size' for the instance 'str.size', empty if not an instance
    // of a method of a generic type
    std::string generic_method;
};

// a noinline body compiled for the array lengths of one kind of call
struct noinline_body {
    const stmt_def_func* func{};
    // one per array parameter, in parameter order
    std::vector<size_t> array_lengths;
};

// storage under the name of a root variable: the root variable and the path
// to what is accessed, empty when it is not known
struct storage_target {
    // 'acc@2' is the variable 'acc' declared in the frame 2, a callee's local
    // of the name of a variable of the caller is another variable
    static constexpr char frame_mark{'@'};

    std::string root;
    std::optional<access_path> path;

    // the root variable as written, without the frame that tells apart
    // variables of the same name
    [[nodiscard]] auto display_root() const -> std::string_view {
        return std::string_view{root}.substr(0, root.find(frame_mark));
    }

    // a variable of the root frame, e.g. 'g@0'
    [[nodiscard]] auto is_global() const -> bool {
        return root.ends_with(std::format("{}0", frame_mark));
    }

    // true when both may name the same bytes, a read of other bytes or of
    // another variable is not
    [[nodiscard]] auto may_overlap(const storage_target& other) const -> bool {
        if (root != other.root) {
            return false;
        }

        // note: an unknown path is the whole variable
        if (not path or not other.path) {
            return true;
        }

        return path->overlaps(*other.path);
    }
};

// a parameter that is a constant, e.g. 'x' of 'f(3)' or of 'f(2 * n)' when
// 'n' is a constant, the body is decided at compile time
struct bound_constant {
    int64_t value{};
};

// a parameter whose value is in a register, e.g. 'x' of 'f(a + 1)'
struct bound_register {
    operand reg;
    // how the comments show it, the register or the name that held it
    std::string name;
};

// a parameter that names storage of the caller, e.g. 'p' of
// 'f(world.rooms[i])', or the result that names the destination
struct bound_storage {
    // e.g. 'world.rooms', resolved in the frames of the caller
    std::string path;
    // the address computed at the call, empty when the path finds it, e.g.
    // '[rbp + r15 * 8 + 16]' for a run-time index
    operand lea;
    // e.g. 'i' -> 'arr[ix]' names one element, not the array 'arr'
    bool is_element{};
    // the root variable and the path that the alias names, found where
    // the call is made because the callee does not see the caller's names,
    // e.g. the element 'e' of a 'foo'; empty for a temporary
    storage_target bound;
};

// what the name of a parameter or of the result of an inlined call stands
// for in its body
struct alias_info {
    std::string from;
    // the type of the parameter or the result
    const type* type_ptr{};
    std::variant<bound_constant, bound_register, bound_storage> binding;

    // what the alias stands for, as the comments show it
    [[nodiscard]] auto target_text() const -> std::string {
        if (const bound_constant* const constant{
                std::get_if<bound_constant>(&binding),
            }) {

            return std::format("{}", constant->value);
        }

        if (const bound_register* const reg{
                std::get_if<bound_register>(&binding),
            }) {

            return reg->name;
        }

        return std::get<bound_storage>(binding).path;
    }

    //
    // statics
    //

    [[nodiscard]] static auto make_constant(const std::string_view name,
                                            const type& alias_type,
                                            const int64_t value) -> alias_info {

        // note: only a built-in parameter takes a constant
        assert(alias_type.is_builtin());

        return {
            .from{std::string{name}},
            .type_ptr{&alias_type},
            .binding{
                bound_constant{
                    .value{value},
                },
            },
        };
    }

    [[nodiscard]] static auto
    make_register(const std::string_view name, const type& alias_type,
                  const operand& reg, const std::string_view reg_name)
        -> alias_info {

        assert(reg.is_register());

        return {
            .from{std::string{name}},
            .type_ptr{&alias_type},
            .binding{
                bound_register{
                    .reg{reg},
                    .name{std::string{reg_name}},
                },
            },
        };
    }

    [[nodiscard]] static auto make_register(const std::string_view name,
                                            const type& alias_type,
                                            const operand& reg) -> alias_info {

        return make_register(name, alias_type, reg, reg.base_register());
    }

    [[nodiscard]] static auto make_storage(const std::string_view name,
                                           const type& alias_type,
                                           bound_storage storage)
        -> alias_info {

        return {
            .from{std::string{name}},
            .type_ptr{&alias_type},
            .binding{std::move(storage)},
        };
    }
};

struct const_info {
    token src_loc_tk; // the definition
    int64_t value;
};

// what a variable is declared by: 'var', 'dat' or the signature of a function,
// a parameter or the result
enum class var_kind : uint8_t { var, dat, param, result };

// one scope; the static functions make each kind, e.g. 'frame::block()'
class frame final {
    enum class frame_type : uint8_t { func, block, loop, foo };

    // optional name
    std::string_view name_;

    // a unique path of source locations of the inlined call
    std::string call_path_;

    // number of bytes used on the stack by this frame
    size_t allocated_stack_size_bytes_{};

    // the root frame: the padding between the dats and its variables
    size_t dat_var_padding_bytes_{};

    lut<const_info> consts_;

    // variables declared in this frame
    lut<var_info> vars_;

    // the parameters and the result of an inlined call, see 'alias_info'
    lut<alias_info> aliases_;

    // the label to jump to when exiting an inlined function
    std::string func_ret_label_;

    // the name of the result of a function frame, empty when it has none
    std::string result_name_;

    // true if var that is not dat has been added
    bool non_dat_var_has_been_added_{};

    frame_type type_{frame_type::func};
    bool is_inlined_{true};
    // see 'noinline_dry_run'
    bool is_noinline_dry_run_{};
    std::string_view storage_base_register_;
    size_t peak_storage_size_bytes_{};

  public:
    auto add_alias(const alias_info& ai) -> void {
        aliases_.put(std::string{ai.from}, ai);
    }

    auto add_const(const std::string_view name, const const_info& ci) -> void {
        consts_.put(std::string{name}, ci);
    }

    auto add_var(const var_info& var, const size_t allocated_size_bytes,
                 const var_kind kind) -> void {

        allocated_stack_size_bytes_ =
            sum_storage_size(allocated_stack_size_bytes_, allocated_size_bytes);

        vars_.put(var.name, var);

        if (kind == var_kind::result) {
            result_name_ = var.name;
        }

        if (kind != var_kind::dat) {
            non_dat_var_has_been_added_ = true;
        }
    }

    [[nodiscard]] auto allocated_stack_size_bytes() const -> size_t {
        return sum_storage_size(allocated_stack_size_bytes_,
                                dat_var_padding_bytes_);
    }

    [[nodiscard]] auto call_path() const -> std::string_view {
        return call_path_;
    }

    [[nodiscard]] auto func_ret_label() const -> std::string_view {
        return func_ret_label_;
    }

    [[nodiscard]] auto get_alias(const std::string_view name) const
        -> const alias_info& {

        return aliases_.get_const_ref(name);
    }

    [[nodiscard]] auto get_const(const std::string_view name) const
        -> const const_info& {

        return consts_.get_const_ref(name);
    }

    [[nodiscard]] auto get_var_const_ref(const std::string_view name) const
        -> const var_info& {

        return vars_.get_const_ref(name);
    }

    [[nodiscard]] auto has_alias(const std::string_view name) const -> bool {
        return aliases_.has(name);
    }

    [[nodiscard]] auto has_const(const std::string_view name) const -> bool {
        return consts_.has(name);
    }

    [[nodiscard]] auto has_non_dat_var_been_added() const -> bool {
        return non_dat_var_has_been_added_;
    }

    [[nodiscard]] auto has_var(const std::string_view name) const -> bool {
        return vars_.has(name);
    }

    [[nodiscard]] auto is_block() const -> bool {
        return type_ == frame_type::block;
    }

    [[nodiscard]] auto is_foo() const -> bool {
        return type_ == frame_type::foo;
    }

    [[nodiscard]] auto is_func() const -> bool {
        return type_ == frame_type::func;
    }

    [[nodiscard]] auto is_inlined_func() const -> bool {
        assert(is_func());

        return is_inlined_;
    }

    [[nodiscard]] auto is_loop() const -> bool {
        return type_ == frame_type::loop;
    }

    [[nodiscard]] auto is_name(const std::string_view name) const -> bool {
        return name_ == name;
    }

    [[nodiscard]] auto is_noinline_dry_run() const -> bool {
        return is_noinline_dry_run_;
    }

    [[nodiscard]] auto is_result(const std::string_view name) const -> bool {
        return result_name_ == name;
    }

    auto make_var_read_only(const std::string_view name,
                            const read_only_cause cause) -> void {

        vars_.get_ref(name).read_only_why = cause;
    }

    [[nodiscard]] auto name() const -> std::string_view { return name_; }

    [[nodiscard]] auto peak_storage_size_bytes() const -> size_t {
        assert(not storage_base_register_.empty());

        return peak_storage_size_bytes_;
    }

    auto record_storage_size_bytes(const size_t size_bytes) -> void {
        assert(not storage_base_register_.empty());

        peak_storage_size_bytes_ =
            std::max(peak_storage_size_bytes_, size_bytes);
    }

    auto restore_peak_storage_size_bytes(const size_t size_bytes) -> void {
        assert(not storage_base_register_.empty());

        peak_storage_size_bytes_ = size_bytes;
    }

    auto set_dat_var_padding_bytes(const size_t size_bytes) -> void {
        assert(dat_var_padding_bytes_ == 0);

        dat_var_padding_bytes_ = size_bytes;
    }

    [[nodiscard]] auto storage_base_register() const -> std::string_view {
        return storage_base_register_;
    }

    //
    // statics
    //

    [[nodiscard]] static auto block() -> frame {
        return {"", frame_type::block, {}, {}, true, {}};
    }

    // the body of a 'foo' named by its loop label
    [[nodiscard]] static auto foo(const std::string_view name) -> frame {
        return {name, frame_type::foo, {}, {}, true, {}};
    }

    // a function compiled in place, 'call_path' keeps its labels unique and
    // 'return' jumps to 'return_label'
    [[nodiscard]] static auto inlined_func(const std::string_view name,
                                           std::string call_path,
                                           std::string return_label) -> frame {

        return {name,
                frame_type::func,
                std::move(call_path),
                std::move(return_label),
                true,
                {}};
    }

    // the body of a loop named by its label
    [[nodiscard]] static auto loop(const std::string_view name) -> frame {
        return {name, frame_type::loop, {}, {}, true, {}};
    }

    // a noinline function compiled in place, the dry run that checks the
    // aliasing of a call to it, see 'noinline_call::check_aliasing'
    [[nodiscard]] static auto noinline_dry_run(const std::string_view name,
                                               std::string call_path,
                                               std::string return_label)
        -> frame {

        frame result{
            inlined_func(name, std::move(call_path), std::move(return_label)),
        };

        result.is_noinline_dry_run_ = true;

        return result;
    }

    // a function with a body of its own, its variables are placed from
    // 'storage_base_register' when not empty
    [[nodiscard]] static auto
    noinline_func(const std::string_view name, std::string call_path,
                  const std::string_view storage_base_register) -> frame {

        return {name,  frame_type::func,     std::move(call_path), {},
                false, storage_base_register};
    }

  private:
    frame(const std::string_view name, const frame_type frm_type,
          std::string call_path, std::string func_ret_label,
          const bool is_inlined,
          const std::string_view storage_base_register) noexcept
        : name_{name}, call_path_{std::move(call_path)},
          func_ret_label_{std::move(func_ret_label)}, type_{frm_type},
          is_inlined_{is_inlined},
          storage_base_register_{storage_base_register} {}
};

class ident_path final {
    std::string id_;
    std::vector<std::string> path_;

  public:
    explicit ident_path(std::string id) : id_{std::move(id)} {
        assert(not id_.empty());

        refresh_path();

        assert(not path_.empty());
    }

    auto append(const std::string_view path_elem) -> void {
        id_ = std::format("{}.{}", id_, path_elem);
        refresh_path();
    }

    [[nodiscard]] auto base() const -> std::string_view {
        assert(not path_.empty());

        return path_.at(0);
    }

    [[nodiscard]] auto path() const -> const std::vector<std::string>& {
        return path_;
    }

    [[nodiscard]] auto str() const -> const std::string& { return id_; }

  private:
    auto refresh_path() -> void {
        path_.clear();
        for (const auto part : id_ | std::views::split('.')) {
            path_.emplace_back(part.begin(), part.end());
        }
    }
};

// builds the identifier info of a path: the elements of an indexed array, the
// address held below a run-time index and the base of an alias
class ident_builder final {
  public:
    //
    // statics
    //

    [[nodiscard]] static auto as_element_if(const bool is_element,
                                            ident_info info) -> ident_info {

        if (not is_element or not info.is_var()) {
            return info;
        }

        ident_info::storage_part& storage{info.storage()};
        storage.is_array = false;
        storage.array_len = std::nullopt;

        return info;
    }

    // the identifier 'ident' of the variable 'var', its path walks the fields
    // of the type of the variable, 'base_register' is where its storage starts
    // unless the variable has a base of its own
    [[nodiscard]] static auto
    make_var_ident_info(const token& src_loc_tk, const std::string_view ident,
                        const std::vector<std::string>& path,
                        const var_info& var,
                        const std::string_view base_register) -> ident_info {

        const type* tp{var.type_ptr};

        std::vector<const type*> type_path;
        type_path.emplace_back(tp);

        size_t offset{};
        bool is_array{var.is_array};
        std::optional<size_t> array_count{var.array_len};

        for (const std::string& field_name : path | std::views::drop(1)) {
            // note: drop 1 because the first element is retrieved outside the
            //       loop

            const type_field& tf{tp->field(src_loc_tk, field_name)};
            offset = sum_storage_size(offset, tf.offset);
            tp = tf.type_ptr;
            is_array = tf.is_array();
            array_count = tf.array_count;
            type_path.emplace_back(tp);
        }

        const int64_t idx{
            var.pointer_register.is_empty()
                ? add_address_offset(var.offset, address_offset(offset))
                : address_offset(offset),
        };

        const std::string_view storage_base{
            var.base_register.empty() ? base_register : var.base_register,
        };

        // a memory operand needs a built-in type, the first field of an
        // instance starts at its address
        while (not tp->is_builtin()) {
            tp = tp->fields().front().type_ptr;
        }

        const operand op{
            operand::mem(var.pointer_register.is_empty()
                             ? storage_base
                             : var.pointer_register.base_register(),
                         "", 1, idx, *tp),
        };

        return ident_info::make_var(std::string{ident}, path,
                                    std::move(type_path),
                                    {
                                        .operand{op},
                                        .known{},
                                        .array_len{array_count},
                                        .is_array{is_array},
                                        .is_pointer{var.is_pointer},
                                        .read_only_why{var.read_only_why},
                                    });
    }

    // a built-in identifier below a run-time indexed element is addressed from
    // the last address held, e.g. 'wld.rooms[ix].description.data'
    static auto place_operand_from_lea(const token& src_loc_tk,
                                       ident_info& info) -> void {

        const std::optional<known_address>& known{info.storage().known};

        if (not known) {
            return;
        }

        const operand& lea{known->address};

        const size_t lea_index{
            info.elem_path().size() - 1 - known->trailing_count,
        };

        // example of resulting data structure:
        //
        // type string { len i8, data i8[127] }
        // type room { name string, description string, note string }
        // type world { rooms room[128] }
        //
        // id path     |  type  |  lea          |
        // ------------|--------|---------------|
        // wld         | world  | -             |
        // rooms[i]    | room   | r15           |
        // description | string | -             |
        // data        | i8     | r15 + 129     |
        //
        // the index into 'rooms' is known only at run time, so the address of
        // 'rooms[i]' is in a register and the fields after it are addressed
        // from that register

        // the fields after the known address are at fixed offsets from it
        const std::span<const std::string> elem_path_from_lea{
            std::span{info.elem_path()}.subspan(lea_index),
        };

        const size_t offset{
            info.type_path().at(lea_index)->field_offset(src_loc_tk,
                                                         elem_path_from_lea),
        };

        ::operand& element{info.storage().operand};
        element = operand::mem(lea, info.type_ref());

        if (offset != 0) {
            element.increment_offset(address_offset(offset));
        }
    }

    // 'id' with the base replaced by what the alias refers to, e.g.
    //   res -> pt.x becomes pt.x
    //   pt.x -> p becomes p.x
    //   lnk.count -> world.room.link becomes world.room.link.count
    [[nodiscard]] static auto replace_alias_base(const bound_storage& alias,
                                                 const ident_path& id)
        -> ident_path {

        ident_path target{alias.path};

        for (const std::string& s : id.path() | std::views::drop(1)) {
            target.append(s);
        }

        return target;
    }
};

// the types by name. while an instance of a generic definition is parsed, its
// type parameters are names of types too, e.g. 'T' names 'i8'; only those of
// the innermost instance are visible
class type_table final {
    struct entry {
        token src_loc_tk;
        const type* type_ptr;
    };

    lut<entry> entries_;
    // the type parameters of the instances being parsed, the innermost last
    std::vector<lut<entry>> params_;

  public:
    auto add(const token& src_loc_tk, const type& tpe) -> void {
        entries_.put(std::string{tpe.name()}, {
                                                  .src_loc_tk{src_loc_tk},
                                                  .type_ptr{&tpe},
                                              });
    }

    // another name for a type, e.g. 'int' for the default type
    auto add_alias(const token& src_loc_tk, const std::string_view name,
                   const type& tpe) -> void {

        entries_.put(std::string{name}, {
                                            .src_loc_tk{src_loc_tk},
                                            .type_ptr{&tpe},
                                        });
    }

    // e.g. 'T' names 'i8' in an instance of 'func f<T type>()'
    auto add_param(const token& src_loc_tk, const std::string_view name,
                   const type& tpe) -> void {

        assert(not has(name));

        params_.back().put(std::string{name}, {
                                                  .src_loc_tk{src_loc_tk},
                                                  .type_ptr{&tpe},
                                              });
    }

    // the parameters of an enclosing instance are hidden until 'exit_params'
    auto enter_params() -> void { params_.emplace_back(); }

    auto exit_params() -> void { params_.pop_back(); }

    [[nodiscard]] auto get(const std::string_view name) const -> const type& {
        return *find(name).type_ptr;
    }

    [[nodiscard]] auto has(const std::string_view name) const -> bool {
        return is_param(name) or entries_.has(name);
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

// the functions of the program: the built-in ones, the defined ones and the
// instances of generic ones, the bodies of noinline functions to compile and
// the definitions a compile reached
class function_table final {
    lut<func_info> funcs_;
    std::vector<const stmt_def_func*> defs_;
    std::vector<std::shared_ptr<const stmt_def_func>> instances_;
    std::vector<noinline_body> noinline_bodies_;
    std::set<std::string> checked_calls_;
    // the definitions whose body a compile reached
    std::set<const stmt_def_func*> reached_;

  public:
    // the name has been checked, a built-in function has no definition
    auto add(const token& src_loc_tk, std::string name, const type& return_type,
             const stmt_def_func* const func_def, std::string generic_method)
        -> void {

        funcs_.put(std::move(name),
                   {
                       .src_loc_tk{src_loc_tk},
                       .def{func_def},
                       .type_ptr{&return_type},
                       .generic_method{std::move(generic_method)},
                   });

        if (func_def) {
            defs_.emplace_back(func_def);
        }
    }

    // false when the signature was already added, so a call is checked once
    [[nodiscard]] auto add_checked_call(std::string signature) -> bool {
        return checked_calls_.insert(std::move(signature)).second;
    }

    // keeps the instance alive, 'funcs_' refers to it
    auto add_instance(std::shared_ptr<const stmt_def_func> instance) -> void {
        instances_.emplace_back(std::move(instance));
    }

    // a body is emitted once per kind of call
    auto add_noinline_body(const stmt_def_func& func,
                           std::vector<size_t> array_lengths) -> void {

        const auto is_same_body = [&](const noinline_body& known) -> bool {
            return known.func == &func and known.array_lengths == array_lengths;
        };

        if (std::ranges::any_of(noinline_bodies_, is_same_body)) {
            return;
        }

        noinline_bodies_.push_back({
            .func{&func},
            .array_lengths{std::move(array_lengths)},
        });
    }

    [[nodiscard]] auto defs() const -> std::span<const stmt_def_func* const> {
        return defs_;
    }

    // the name is known
    [[nodiscard]] auto get(const std::string_view name) const
        -> const func_info& {

        return funcs_.get_const_ref(name);
    }

    [[nodiscard]] auto has(const std::string_view name) const -> bool {
        return funcs_.has(name);
    }

    // a noinline body to emit or a function reached by a call
    [[nodiscard]] auto has_compile_records() const -> bool {
        return not noinline_bodies_.empty() or not reached_.empty();
    }

    // declared without a definition, e.g. 'exit'
    [[nodiscard]] auto is_builtin(const std::string_view name) const -> bool {
        return get(name).def == nullptr;
    }

    [[nodiscard]] auto is_reached(const stmt_def_func& func) const -> bool {
        return reached_.contains(&func);
    }

    auto mark_reached(const stmt_def_func& func) -> void {
        reached_.insert(&func);
    }

    // a copy, the list grows while the bodies compile
    [[nodiscard]] auto noinline_body_at(const size_t index) const
        -> noinline_body {

        return noinline_bodies_.at(index);
    }

    [[nodiscard]] auto noinline_body_count() const -> size_t {
        return noinline_bodies_.size();
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
    size_t data_alignment_;

  public:
    explicit data_table(const size_t data_alignment)
        : data_alignment_{data_alignment} {}

    // identical text shares the label of the first constant added
    [[nodiscard]] auto add_constant(std::string label, std::string text)
        -> std::string {

        for (const machine::string_constant& constant : constants_) {
            if (constant.text == text) {
                return constant.label;
            }
        }

        constants_.push_back({
            .label{std::move(label)},
            .text{std::move(text)},
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
    bool with_line{};
    bool frame{};
    // on unless '--checks=-alias'
    bool alias{true};
    bool division{};
    bool shift{};
    bool overlap{};
    bool stack{};
    bool overflow{};

    [[nodiscard]] auto bounds_options() const -> machine::bounds_check_options {
        return {
            .upper{bounds_upper},
            .lower{bounds_lower},
            .with_line{with_line},
        };
    }

    [[nodiscard]] auto division_options() const -> machine::runtime_check {
        return runtime(division);
    }

    // either bound
    [[nodiscard]] auto is_bounds() const -> bool {
        return bounds_upper or bounds_lower;
    }

    [[nodiscard]] auto overflow_options() const -> machine::runtime_check {
        return runtime(overflow);
    }

    [[nodiscard]] auto overlap_options() const -> machine::runtime_check {
        return runtime(overlap);
    }

    [[nodiscard]] auto shift_options() const -> machine::runtime_check {
        return runtime(shift);
    }

  private:
    // every run-time check reports the line when '--checks=line' is given
    [[nodiscard]] auto runtime(const bool enabled) const
        -> machine::runtime_check {

        return {
            .enabled{enabled},
            .with_line{with_line},
        };
    }
};

// what the compile used, for the report
struct usage_statistics {
    size_t max_frame_count{};
    size_t max_vars_size_bytes{};
    size_t dat_size_bytes{};
    size_t dat_var_padding_bytes{};
    std::vector<std::string> uninstantiated_generics;
};

// a 'loop' or a 'foo' named by its label
struct loop_target {
    std::string_view label;
    bool is_foo{};
};

// what a name means where it is used, see 'scope_stack::lookup'
struct name_meaning {
    enum class kind : uint8_t { none, variable, alias, constant };

    kind what{kind::none};
    // the frame that declares the name
    size_t frame_index{};

    [[nodiscard]] auto is_alias() const -> bool { return what == kind::alias; }

    [[nodiscard]] auto is_constant() const -> bool {
        return what == kind::constant;
    }

    // a name that has a value hides a type or a function of that name
    [[nodiscard]] auto is_value() const -> bool { return what != kind::none; }

    [[nodiscard]] auto is_var_or_alias() const -> bool {
        return is_variable() or is_alias();
    }

    [[nodiscard]] auto is_variable() const -> bool {
        return what == kind::variable;
    }
};

// the frames of the scopes being compiled, from the root frame that holds the
// globals to the innermost one
class scope_stack final {
    std::vector<frame> frames_;
    size_t max_depth_{};

  public:
    [[nodiscard]] auto back() -> frame& { return frames_.back(); }

    [[nodiscard]] auto back() const -> const frame& { return frames_.back(); }

    // the root variable that the parameter 'name' names, empty when it is not
    // a parameter or the variable is not known
    [[nodiscard]] auto bound_root(const std::string_view name) const
        -> std::string_view {

        const alias_info* const alias{find_alias(name)};

        if (alias == nullptr) {
            return {};
        }

        const bound_storage* const storage{
            std::get_if<bound_storage>(&alias->binding),
        };

        // note: a constant or a register names no variable
        if (storage == nullptr) {
            return {};
        }

        return storage->bound.root;
    }

    // the value of 'name' when it means a constant
    [[nodiscard]] auto const_value(const std::string_view name) const
        -> std::optional<int64_t> {

        const name_meaning meaning{lookup(name)};

        if (not meaning.is_constant()) {
            return std::nullopt;
        }

        return frames_.at(meaning.frame_index).get_const(name).value;
    }

    // blocks and loops belong to the function frame below them
    [[nodiscard]] auto current_func_frame() const -> const frame& {
        for (const frame& frm : frames_ | std::views::reverse) {
            if (frm.is_func()) {
                return frm;
            }
        }

        std::unreachable();
    }

    auto enter_block() -> void {
        frames_.push_back(frame::block());
        track_depth();
    }

    auto enter_foo(const std::string_view name) -> void {
        frames_.push_back(frame::foo(name));
        track_depth();
    }

    // a function compiled in place, 'return' jumps to 'return_jmp_label'
    auto enter_func(const std::string_view name,
                    const std::string_view call_path,
                    const std::string_view return_jmp_label) -> void {

        frames_.push_back(frame::inlined_func(name, std::string{call_path},
                                              std::string{return_jmp_label}));

        track_depth();
    }

    auto enter_loop(const std::string_view name) -> void {
        frames_.push_back(frame::loop(name));
        track_depth();
    }

    // see 'frame::noinline_dry_run'
    auto enter_noinline_dry_run(const std::string_view name,
                                const std::string_view call_path,
                                const std::string_view return_jmp_label)
        -> void {

        frames_.push_back(frame::noinline_dry_run(
            name, std::string{call_path}, std::string{return_jmp_label}));

        track_depth();
    }

    // a function with a body of its own, its variables are placed from
    // 'storage_base_register' when not empty
    auto enter_noinline_func(const std::string_view name,
                             const std::string_view call_path,
                             const std::string_view storage_base_register)
        -> void {

        frames_.push_back(frame::noinline_func(name, std::string{call_path},
                                               storage_base_register));

        track_depth();
    }

    // the array that variable 'name' is an element of, empty when it is not the
    // element of a 'foo'
    [[nodiscard]] auto foo_array(const std::string_view name) const
        -> foo_array_info {

        const name_meaning meaning{lookup(name)};

        if (not meaning.is_variable()) {
            return {};
        }

        return frames_.at(meaning.frame_index)
            .get_var_const_ref(name)
            .foo_array;
    }

    [[nodiscard]] auto frames() -> std::vector<frame>& { return frames_; }

    [[nodiscard]] auto frames() const -> const std::vector<frame>& {
        return frames_;
    }

    [[nodiscard]] auto front() -> frame& { return frames_.front(); }

    [[nodiscard]] auto front() const -> const frame& { return frames_.front(); }

    // how many times the inlined function 'name' is being compiled in place
    // by the current function body, inlined bodies are searched down to the
    // nearest function with a body of its own
    [[nodiscard]] auto inlined_nesting(const std::string_view name) const
        -> size_t {

        size_t count{};
        for (const frame& frm : frames_ | std::views::reverse) {
            if (not frm.is_func()) {
                continue;
            }

            if (not frm.is_inlined_func()) {
                break;
            }

            if (frm.is_name(name)) {
                ++count;
            }
        }

        return count;
    }

    // the innermost 'loop' or 'foo' of the function, the target of a 'break'
    // or a 'continue'
    [[nodiscard]] auto innermost_loop_or_throw(const token& src_loc_tk) const
        -> loop_target {

        for (const frame& frm : frames_ | std::views::reverse) {
            if (frm.is_loop() or frm.is_foo()) {
                return {
                    .label{frm.name()},
                    .is_foo{frm.is_foo()},
                };
            }

            if (frm.is_func()) {
                throw compile_error{src_loc_tk, "not in a loop"};
            }
        }

        std::unreachable();
    }

    // true when 'name' is a parameter that names its argument
    [[nodiscard]] auto is_alias(const std::string_view name) const -> bool {
        return find_alias(name) != nullptr;
    }

    [[nodiscard]] auto is_empty() const -> bool { return frames_.empty(); }

    // the initializer of a global variable is outside of any function
    [[nodiscard]] auto is_in_func() const -> bool {
        return std::ranges::any_of(
            frames_, [](const frame& frm) -> bool { return frm.is_func(); });
    }

    // what 'name' means in the innermost scope
    [[nodiscard]] auto lookup(const std::string_view name) const
        -> name_meaning {

        return lookup_below(name, frames_.size());
    }

    // what 'name' means to the frames below the index 'end', the one rule of
    // what a name means: the frames of the innermost function from the
    // innermost out, then the root frame. a frame declares a variable or a
    // constant of a name, not both; in a function frame a parameter hides a
    // constant of the generic instance, which is the context of the definition
    [[nodiscard]] auto lookup_below(const std::string_view name,
                                    const size_t end) const -> name_meaning {

        for (const size_t index :
             std::views::iota(size_t{}, end) | std::views::reverse) {

            const frame& frm{frames_.at(index)};

            if (frm.has_var(name)) {
                return {
                    .what{name_meaning::kind::variable},
                    .frame_index{index},
                };
            }

            if (frm.is_func() and frm.has_alias(name)) {
                return {
                    .what{name_meaning::kind::alias},
                    .frame_index{index},
                };
            }

            if (frm.has_const(name)) {
                return {
                    .what{name_meaning::kind::constant},
                    .frame_index{index},
                };
            }

            if (frm.is_func()) {
                break;
            }
        }

        const frame& root{frames_.front()};

        if (root.has_var(name)) {
            return {
                .what{name_meaning::kind::variable},
                .frame_index{},
            };
        }

        if (root.has_const(name)) {
            return {
                .what{name_meaning::kind::constant},
                .frame_index{},
            };
        }

        return {};
    }

    [[nodiscard]] auto max_depth() const -> size_t { return max_depth_; }

    auto pop() -> void { frames_.pop_back(); }

    // 'path' of the variable or parameter 'name' as bytes of the root
    // variable it names, empty when that is not known
    [[nodiscard]] auto root_path(const std::string_view name,
                                 const std::optional<access_path>& path) const
        -> std::optional<access_path> {

        const alias_info* const alias{find_alias(name)};

        if (alias == nullptr) {
            return path;
        }

        const bound_storage* const storage{
            std::get_if<bound_storage>(&alias->binding),
        };

        if (storage == nullptr) {
            return std::nullopt;
        }

        const std::optional<access_path>& bound{storage->bound.path};

        if (not bound or not path) {
            return std::nullopt;
        }

        return bound->extended_by(*path);
    }

    auto set_max_depth(const size_t depth) -> void { max_depth_ = depth; }

  private:
    // the alias 'name' of the innermost function, null when 'name' means
    // something else
    [[nodiscard]] auto find_alias(const std::string_view name) const
        -> const alias_info* {

        const name_meaning meaning{lookup(name)};

        if (not meaning.is_alias()) {
            return nullptr;
        }

        return &frames_.at(meaning.frame_index).get_alias(name);
    }

    auto track_depth() -> void {
        max_depth_ = std::max(frames_.size(), max_depth_);
    }
};

// where the variables are placed, after the dats that 'data_table' placed: the
// running size of the variables section, the frames that have a storage base
// of their own, the capacity of the section and the target limits
class storage_layout final {
    // where a variable is placed, 'frame_index' is empty at the variables base
    struct storage_location {
        std::optional<size_t> frame_index;
        size_t base_offset{};
    };

    std::reference_wrapper<const ::machine> machine_;
    std::reference_wrapper<scope_stack> scopes_;
    std::reference_wrapper<const data_table> data_;
    size_t vars_size_bytes_{};
    size_t vars_capacity_bytes_;
    size_t max_vars_size_bytes_{};
    bool dat_var_padding_applied_{};

    // the parse declares names without checking the capacity, the compile
    // checks it where the declaration is compiled; the locals of a dry run
    // live in the callee's own frame
    bool capacity_unchecked_{true};

  public:
    // what a dry run changes and 'end_dry_run' puts back
    struct dry_run_state {
        bool capacity_unchecked;
        size_t max_vars_size_bytes;
        // the index of the frame, frames added meanwhile may move the frames
        std::optional<size_t> storage_frame_index;
        size_t peak_storage_size_bytes;
    };

    storage_layout(const ::machine& backend, scope_stack& scopes,
                   const data_table& data, const size_t vars_capacity_bytes)
        : machine_{backend}, scopes_{scopes}, data_{data},
          vars_capacity_bytes_{vars_capacity_bytes} {}

    // the storage of the dat 'var' is where the data table placed it, after
    // the dats before it; returns the bytes it takes with the padding before
    // it, 'var.offset' is set
    [[nodiscard]] auto allocate_dat(const token& src_loc_tk, var_info& var)
        -> size_t {

        const placed_dat& dat{data_.get().get_dat(var.name)};

        // note: the compile pass places the dats in order and before the
        //       variables; the parse also places the variables of the
        //       function bodies it reads, whose offsets are not used
        assert(capacity_unchecked_ or
               sum_storage_size(vars_size_bytes_, dat.padding_bytes) ==
                   dat.offset_bytes);

        const size_t allocated_size_bytes{
            sum_storage_size(dat.padding_bytes,
                             var_storage_size_bytes(src_loc_tk, var)),
        };

        var.offset = add_address_offset(address_offset(dat.offset_bytes),
                                        -variables_base_shift_bytes());

        add_allocated_size_bytes(src_loc_tk, allocated_size_bytes);

        return allocated_size_bytes;
    }

    // places 'var' in the innermost storage that has room and returns the
    // bytes it takes with the alignment padding, 'var.offset' is set
    [[nodiscard]] auto allocate_var(const token& src_loc_tk, var_info& var)
        -> size_t {

        const size_t var_size_bytes{var_storage_size_bytes(src_loc_tk, var)};
        const size_t var_alignment{var_storage_alignment(var)};

        apply_dat_var_padding(src_loc_tk);

        // offsets are relative to the variables base or to the nearest frame
        // with its own storage base, both are aligned
        const storage_location location{find_storage_location()};
        const size_t base_offset{location.base_offset};

        const size_t padding_bytes{
            align_storage_size(base_offset, var_alignment) - base_offset,
        };

        const size_t allocated_size_bytes{
            add_storage_size(src_loc_tk, padding_bytes, var_size_bytes),
        };

        assert_vars_capacity(src_loc_tk, var, allocated_size_bytes);

        var.offset = address_offset(base_offset + padding_bytes);

        if (location.frame_index) {
            frame& storage_frame{
                scopes_.get().frames().at(*location.frame_index),
            };

            var.base_register = storage_frame.storage_base_register();

            storage_frame.record_storage_size_bytes(add_storage_size(
                src_loc_tk, base_offset, allocated_size_bytes));
        } else {
            var.offset =
                add_address_offset(var.offset, -variables_base_shift_bytes());
        }

        add_allocated_size_bytes(src_loc_tk, allocated_size_bytes);

        max_vars_size_bytes_ =
            std::max(used_vars_size_bytes(), max_vars_size_bytes_);

        return allocated_size_bytes;
    }

    auto assert_released() const -> void { assert(vars_size_bytes_ == 0); }

    // the declarations from here on are checked against the capacity
    auto begin_compile() -> void { capacity_unchecked_ = false; }

    auto begin_dry_run() -> dry_run_state {
        const std::optional<size_t> frame_index{
            find_storage_location().frame_index,
        };

        const dry_run_state saved{
            .capacity_unchecked{capacity_unchecked_},
            .max_vars_size_bytes{max_vars_size_bytes_},
            .storage_frame_index{frame_index},
            .peak_storage_size_bytes{
                frame_index ? frame_at(*frame_index).peak_storage_size_bytes()
                            : 0,
            },
        };

        capacity_unchecked_ = true;

        return saved;
    }

    auto end_dry_run(const dry_run_state& saved) -> void {
        capacity_unchecked_ = saved.capacity_unchecked;
        max_vars_size_bytes_ = saved.max_vars_size_bytes;

        if (saved.storage_frame_index) {
            scopes_.get()
                .frames()
                .at(*saved.storage_frame_index)
                .restore_peak_storage_size_bytes(saved.peak_storage_size_bytes);
        }
    }

    [[nodiscard]] auto max_vars_size_bytes() const -> size_t {
        return max_vars_size_bytes_;
    }

    // a callee frame starts aligned for any variable it holds
    [[nodiscard]] auto next_frame_address(const type& address_type) const
        -> operand {

        const size_t frame_alignment{machine_.get().address_size_bytes()};

        const storage_location location{find_storage_location()};

        if (location.frame_index) {
            return operand::mem(
                frame_at(*location.frame_index).storage_base_register(), {}, 1,
                address_offset(
                    align_storage_size(location.base_offset, frame_alignment)),
                address_type);
        }

        const size_t root_size_bytes{
            sum_storage_size(location.base_offset,
                             dat_var_padding_applied_
                                 ? 0
                                 : data_.get().dat_var_padding_bytes()),
        };

        const size_t aligned_size_bytes{
            align_storage_size(root_size_bytes, frame_alignment),
        };

        const int64_t offset_from_dat{address_offset(aligned_size_bytes)};

        const int64_t offset_from_base{
            add_address_offset(offset_from_dat, -variables_base_shift_bytes()),
        };

        return operand::mem(machine_.get().variables_base_register(), {}, 1,
                            offset_from_base, address_type);
    }

    [[nodiscard]] auto peak_frame_size_bytes() const -> size_t {
        const std::optional<size_t> frame_index{
            find_storage_location().frame_index,
        };

        // note: asked inside a noinline body, which has a storage base
        assert(frame_index);

        return frame_at(*frame_index).peak_storage_size_bytes();
    }

    // the root frame applies the dat var padding again when it is entered anew
    auto pop_scope() -> void {
        vars_size_bytes_ -= scopes_.get().back().allocated_stack_size_bytes();
        scopes_.get().pop();

        if (not scopes_.get().is_empty()) {
            return;
        }

        assert(vars_size_bytes_ == 0);

        dat_var_padding_applied_ = false;
    }

    // the frames of the parse are popped, its use of storage is forgotten
    auto reset_usage() -> void {
        assert(vars_size_bytes_ == 0);

        max_vars_size_bytes_ = 0;
    }

    [[nodiscard]] auto var_storage_alignment(const var_info& var) const
        -> size_t {

        if (var.is_pointer) {
            return machine_.get().address_size_bytes();
        }

        return var.type_ptr->alignment();
    }

    [[nodiscard]] auto var_storage_size_bytes(const token& src_loc_tk,
                                              const var_info& var) const
        -> size_t {

        if (var.is_pointer) {
            return machine_.get().address_size_bytes();
        }

        return multiply_storage_size(
            src_loc_tk, var.type_ptr->size_bytes(),
            var.is_array ? var.array_len.value_or(discarded_array_len) : 1);
    }

    [[nodiscard]] auto vars_capacity_bytes() const -> size_t {
        return vars_capacity_bytes_;
    }

  private:
    // the total accepts the size before a frame sums it
    auto add_allocated_size_bytes(const token& src_loc_tk,
                                  const size_t allocated_size_bytes) -> void {

        vars_size_bytes_ = add_storage_size(src_loc_tk, vars_size_bytes_,
                                            allocated_size_bytes);

        assert_storage_fits_target(src_loc_tk);
    }

    // the first variable after the dats starts past the padding that aligns it
    auto apply_dat_var_padding(const token& src_loc_tk) -> void {
        if (dat_var_padding_applied_) {
            return;
        }

        const size_t padding_bytes{data_.get().dat_var_padding_bytes()};

        scopes_.get().front().set_dat_var_padding_bytes(padding_bytes);

        vars_size_bytes_ =
            add_storage_size(src_loc_tk, vars_size_bytes_, padding_bytes);

        dat_var_padding_applied_ = true;
    }

    // the declaration that takes the data and variables past what the target
    // addresses is the error, not the end of the build
    auto assert_storage_fits_target(const token& src_loc_tk) const -> void {
        const size_t max_bytes{machine_.get().max_storage_bytes()};

        if (vars_size_bytes_ <= max_bytes) {
            return;
        }

        throw compile_error{
            src_loc_tk,
            std::format("data and variables of {} B exceed the {} B that the "
                        "target addresses",
                        vars_size_bytes_, max_bytes)};
    }

    auto assert_vars_capacity(const token& src_loc_tk, const var_info& var,
                              const size_t allocated_size_bytes) const -> void {

        if (capacity_unchecked_ or
            allocated_size_bytes <=
                vars_capacity_bytes_ - used_vars_size_bytes()) {

            return;
        }

        // the name of a temporary is not in the source, the error is at its
        // call
        if (var.is_temporary) {
            throw compile_error{src_loc_tk,
                                "a temporary of the call does not fit in the "
                                "variable storage, see '--vars'"};
        }

        throw compile_error{
            src_loc_tk,
            std::format("variable '{}' does not fit in the variable storage, "
                        "see '--vars'",
                        var.name)};
    }

    // a local starts after the storage in use of the nearest frame with its own
    // storage base and of the frames inside it, other variables start at the
    // variables base
    [[nodiscard]] auto find_storage_location() const -> storage_location {
        const std::vector<frame>& frames{std::as_const(scopes_.get()).frames()};
        size_t local_size_bytes{};

        for (size_t i{frames.size()}; i != 0; --i) {
            const frame& frm{frames.at(i - 1)};

            local_size_bytes = sum_storage_size(
                local_size_bytes, frm.allocated_stack_size_bytes());

            if (not frm.storage_base_register().empty()) {
                return {
                    .frame_index{i - 1},
                    .base_offset{local_size_bytes},
                };
            }
        }

        return {
            .frame_index{},
            .base_offset{vars_size_bytes_},
        };
    }

    [[nodiscard]] auto frame_at(const size_t index) const -> const frame& {
        return std::as_const(scopes_.get()).frames().at(index);
    }

    // bytes of variables, without the dats and the padding after them
    [[nodiscard]] auto used_vars_size_bytes() const -> size_t {
        return vars_size_bytes_ - data_.get().total_size_bytes() -
               data_.get().dat_var_padding_bytes();
    }

    // offsets count from 'dat', a base register past 'vars' makes the dats
    // negative and the variables start below the base
    [[nodiscard]] auto variables_base_shift_bytes() const -> int64_t {
        const std::optional<size_t> past_vars_bytes{
            machine_.get().variables_base_past_vars_bytes(),
        };

        if (not past_vars_bytes) {
            return 0;
        }

        const size_t dats_bytes{
            sum_storage_size(data_.get().total_size_bytes(),
                             data_.get().dat_var_padding_bytes()),
        };

        return address_offset(sum_storage_size(dats_bytes, *past_vars_bytes));
    }
};

// the types the front end needs by role: the builtin ones given by the
// program and the ones the machine decides
class builtin_types final {
    std::reference_wrapper<const ::machine> machine_;
    builtin_type_set types_;

  public:
    builtin_types(const ::machine& backend, const builtin_type_set& types)
        : machine_{backend}, types_{types} {}

    [[nodiscard]] auto address() const -> const type& {
        return machine_.get().address_size_bytes() == sizeof(int32_t)
                   ? *types_.i32
                   : *types_.i64;
    }

    [[nodiscard]] auto boolean() const -> const type& {
        return *types_.boolean;
    }

    [[nodiscard]] auto default_type() const -> const type& {
        return machine_.get().default_type();
    }

    [[nodiscard]] auto i8() const -> const type& { return *types_.i8; }

    // 'int' and the names of the builtin integer types
    [[nodiscard]] auto is_integer_name(const std::string_view name) const
        -> bool {

        return name == "int" or name == types_.i8->name() or
               name == types_.i16->name() or name == types_.i32->name() or
               name == types_.i64->name();
    }

    [[nodiscard]] auto void_type() const -> const type& {
        return *types_.void_type;
    }
};

// resolves a name to a variable, a register or a constant by
// 'scope_stack::lookup_below', an alias is followed into the frames of the
// caller
class ident_resolver final {
    std::reference_wrapper<const scope_stack> scopes_;
    std::reference_wrapper<const ::machine> machine_;
    std::reference_wrapper<const generic_registry> generics_;
    std::reference_wrapper<const builtin_types> builtins_;

    // where the walk through the frames has got to
    struct walk {
        explicit walk(ident_path identifier) : id{std::move(identifier)} {}

        // the identifier, an alias rewrites it to what the alias stands for
        ident_path id;

        // the first address met is the deepest, outer aliases only rewrite
        // the base of the path
        std::optional<known_address> known;

        // an alias of an element names the element, not the array holding it
        bool is_element{};
    };

  public:
    ident_resolver(const scope_stack& scopes, const ::machine& backend,
                   const generic_registry& generics,
                   const builtin_types& builtins)
        : scopes_{scopes}, machine_{backend}, generics_{generics},
          builtins_{builtins} {}

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
                std::format("generic type '{}' is not a value, name an "
                            "instance first, e.g. 'type str = {}<...>'",
                            ident, ident)};
        }

        if (generics_.get().has_func(ident)) {
            throw compile_error{
                src_loc_tk,
                std::format("generic function '{}' needs type arguments, "
                            "e.g. '{}<...>'",
                            ident, ident)};
        }

        // a name starts with a letter or '_', so a digit starts a broken
        // number, e.g. '0x' or '12_000'
        if (ident.front() >= '0' and ident.front() <= '9') {
            throw compile_error{
                src_loc_tk, std::format("cannot parse constant '{}'", ident)};
        }

        throw compile_error{
            src_loc_tk, std::format("cannot resolve identifier '{}'", ident)};
    }

  private:
    // a number, 'true' or 'false', else empty: the caller reports an unknown
    // name with its own wording
    [[nodiscard]] auto resolve_literal_or_empty(const token& src_loc_tk,
                                                const std::string_view ident,
                                                const ident_path& id) const
        -> ident_info {

        // e.g. '0x10'
        if (const std::optional<int64_t> value{
                constant_parser::parse_constant(src_loc_tk, id.str()),
            };
            value) {

            return ident_info::make_const(
                ident, id.str(), builtins_.get().default_type(), *value);
        }

        if (id.base() == reserved_names::true_value) {
            return ident_info::make_const(ident, id.str(),
                                          builtins_.get().boolean(), 1);
        }

        if (id.base() == reserved_names::false_value) {
            return ident_info::make_const(ident, id.str(),
                                          builtins_.get().boolean(), 0);
        }

        return ident_info::make_empty();
    }

    // the root name of the identifier is looked up in the frames below 'end',
    // an alias rewrites the identifier and the lookup goes on in the frames of
    // the caller
    [[nodiscard]] auto resolve_or_empty(const token& src_loc_tk,
                                        const std::string_view ident) const
        -> ident_info {

        assert(not ident.empty());

        // e.g. 'lnks' of 'lnks[1].pos.y'
        walk step{ident_path{std::string{ident}}};
        size_t end{scopes_.get().frames().size()};

        while (true) {
            const name_meaning meaning{
                scopes_.get().lookup_below(step.id.base(), end),
            };

            if (not meaning.is_value()) {
                return resolve_literal_or_empty(src_loc_tk, ident, step.id);
            }

            const frame& frm{scopes_.get().frames().at(meaning.frame_index)};

            if (meaning.is_variable()) {
                ident_info info{
                    resolve_var(src_loc_tk, ident, step.id,
                                frm.get_var_const_ref(step.id.base()),
                                step.known),
                };

                return ident_builder::as_element_if(step.is_element,
                                                    std::move(info));
            }

            // note: a constant has no fields and no elements
            if (meaning.is_constant()) {
                if (step.id.path().size() != 1) {
                    return ident_info::make_empty();
                }

                return ident_info::make_const(
                    ident, step.id.str(), builtins_.get().default_type(),
                    frm.get_const(step.id.base()).value);
            }

            if (std::optional<ident_info> found{
                    follow_alias(frm.get_alias(step.id.base()), ident, step),
                };
                found) {

                return *std::move(found);
            }

            end = meaning.frame_index;
        }
    }

    [[nodiscard]] auto
    resolve_var(const token& src_loc_tk, const std::string_view ident,
                const ident_path& id, const var_info& var,
                const std::optional<known_address>& known) const -> ident_info {

        if (var.value_register.is_register() and id.path().size() == 1) {
            return ident_info::make_register(ident, var.value_register);
        }

        ident_info info{
            ident_builder::make_var_ident_info(
                src_loc_tk, ident, id.path(), var,
                machine_.get().variables_base_register()),
        };

        info.storage().known = known;

        if (not info.type_ref().is_builtin()) {
            return info;
        }

        ident_builder::place_operand_from_lea(src_loc_tk, info);

        return info;
    }

    //
    // statics
    //

    // the constant or the register an alias stands for, else the walk goes on
    // with the identifier rewritten to the variable that the alias refers to
    [[nodiscard]] static auto follow_alias(const alias_info& alias,
                                           const std::string_view ident,
                                           walk& step)
        -> std::optional<ident_info> {

        const bool is_single{step.id.path().size() == 1};

        // the value of an argument keeps the type of its parameter
        if (const bound_constant* const constant{
                std::get_if<bound_constant>(&alias.binding),
            }) {

            // note: a built-in has no fields and no elements
            assert(is_single);

            return ident_info::make_const(ident, alias.target_text(),
                                          *alias.type_ptr, constant->value,
                                          true);
        }

        if (const bound_register* const reg{
                std::get_if<bound_register>(&alias.binding),
            }) {

            // note: a built-in has no fields and no elements
            assert(is_single);

            return ident_info::make_register(ident, reg->reg);
        }

        const bound_storage& storage{std::get<bound_storage>(alias.binding)};

        // a field path such as 'p.x' gets its array-ness from the field
        if (storage.is_element and is_single) {
            step.is_element = true;
        }

        if (not step.known and not storage.lea.is_empty()) {
            step.known = known_address{
                .address{storage.lea},
                .trailing_count{step.id.path().size() - 1},
            };
            // note: -1 because the alias names the first element of the path
        }

        step.id = ident_builder::replace_alias_base(storage, step.id);

        return std::nullopt;
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

        if (funcs_.get().has(name)) {
            throw_function_defined(src_loc_tk, name, generic_method,
                                   funcs_.get().get(name));
        }

        if (generics_.get().has_func(name)) {
            const generic_func_info& fn{generics_.get().get_func(name)};

            throw compile_error{
                src_loc_tk, std::format("function '{}' already defined at {}",
                                        name, where(fn.src_loc_tk))};
        }
    }

    // a generic parameter names its argument, so it cannot be the name of a
    // type
    auto assert_generic_param_free(const token& src_loc_tk,
                                   const std::string_view name) const -> void {

        if (not types_.get().has(name)) {
            return;
        }

        throw compile_error{
            src_loc_tk,
            std::format("generic parameter '{}' hides the type '{}' defined "
                        "at {}, use another name",
                        name, name, where(types_.get().src_loc_tk_of(name)))};
    }

    // a frame declares a variable or a constant of a name, not both
    auto assert_name_free(const token& src_loc_tk,
                          const std::string_view name) const -> void {

        assert_not_declared_in_scope(src_loc_tk, name);

        if (not scopes_.get().back().has_const(name)) {
            return;
        }

        const const_info& c{scopes_.get().back().get_const(name)};

        throw compile_error{
            src_loc_tk,
            std::format("constant '{}' already defined in this block at {}",
                        name, where(c.src_loc_tk))};
    }

    // note: a parameter may hide a constant of the generic instance in the
    //       same function frame
    auto assert_not_declared_in_scope(const token& src_loc_tk,
                                      const std::string_view name) const
        -> void {

        if (not scopes_.get().back().has_var(name)) {
            return;
        }

        const var_info& decl_var{scopes_.get().back().get_var_const_ref(name)};

        throw compile_error{src_loc_tk,
                            std::format("variable '{}' already declared at {}",
                                        name, where(decl_var.src_loc_tk))};
    }

    // the body of a function is the block above its frame, a name declared
    // there cannot repeat a parameter or the result; a deeper block may
    auto assert_not_hiding_signature(const token& src_loc_tk,
                                     const std::string_view what,
                                     const std::string_view name) const
        -> void {

        const std::vector<frame>& frames{scopes_.get().frames()};

        if (frames.size() < 2 or not frames.back().is_block()) {
            return;
        }

        const frame& func_frm{frames.at(frames.size() - 2)};

        if (not func_frm.is_func() or not func_frm.has_var(name)) {
            return;
        }

        const var_info& decl_var{func_frm.get_var_const_ref(name)};

        const std::string_view role{
            func_frm.is_result(name) ? "result" : "parameter",
        };

        throw compile_error{
            src_loc_tk,
            std::format("{} '{}' hides the {} '{}' declared at {}", what, name,
                        role, name, where(decl_var.src_loc_tk)),
        };
    }

    // a generic type and a type share the namespace of types
    auto assert_type_not_defined(const token& src_loc_tk,
                                 const std::string_view name) const -> void {

        if (types_.get().has(name)) {
            throw compile_error{
                src_loc_tk,
                std::format("type '{}' already defined at {}", name,
                            where(types_.get().src_loc_tk_of(name)))};
        }

        if (generics_.get().has_type(name)) {
            throw compile_error{
                src_loc_tk,
                std::format("type '{}' already defined as a generic type at {}",
                            name,
                            where(generics_.get().get_type(name).src_loc_tk))};
        }
    }

  private:
    // the message tells a built-in, an instance of a generic method and a
    // function written in the source apart
    [[noreturn]] auto
    throw_function_defined(const token& src_loc_tk, const std::string_view name,
                           const std::string_view generic_method,
                           const func_info& fn) const -> void {

        // note: the name of a built-in function is rejected by
        //       'assert_valid_unqualified_name'
        assert(fn.src_loc_tk.at_line() != 0);

        const bool is_generic{not generic_method.empty()};
        const bool is_existing_generic{not fn.generic_method.empty()};

        // e.g. 'text.print' and 'list.print' for
        // 'type text<size> = list<i8, size>'
        if (is_generic and is_existing_generic and
            generic_method != fn.generic_method) {

            throw compile_error{
                src_loc_tk,
                std::format("generic method '{}' clashes with the generic "
                            "method '{}' defined at {}",
                            generic_method, fn.generic_method,
                            where(fn.src_loc_tk))};
        }

        if (is_generic and is_existing_generic) {
            throw compile_error{
                src_loc_tk,
                std::format("generic method '{}' already defined at {}",
                            generic_method, where(fn.src_loc_tk))};
        }

        if (is_generic) {
            throw compile_error{
                src_loc_tk,
                std::format("generic method '{}' clashes with the method "
                            "'{}' defined at {}",
                            generic_method, name, where(fn.src_loc_tk))};
        }

        if (is_existing_generic) {
            throw compile_error{
                src_loc_tk,
                std::format("method '{}' clashes with the generic method "
                            "'{}' defined at {}",
                            name, fn.generic_method, where(fn.src_loc_tk))};
        }

        throw compile_error{src_loc_tk,
                            std::format("function '{}' already defined at {}",
                                        name, where(fn.src_loc_tk))};
    }

    [[nodiscard]] auto where(const token& src_loc_tk) const -> std::string {
        return locations_.get().human_readable(src_loc_tk);
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

        ::machine& x{machine_.get()};

        std::string text{
            std::format("{}: {}", var.name, name_info.type_ref().name()),
        };

        if (var.array_len) {
            text += std::format("[{}]", *var.array_len);
        }

        // the iterator 'e' is memory at its register, the counter 'i' is the
        // register
        const operand& reg{
            var.value_register.is_empty() ? var.pointer_register
                                          : var.value_register,
        };

        if (not reg.is_empty()) {
            x.comment(src_loc_tk, indent, "{} ({})", text, reg.base_register());
            return;
        }

        x.comment_variable(
            src_loc_tk, indent, text,
            multiply_storage_size(src_loc_tk, name_info.type_ref().size_bytes(),
                                  name_info.element_count()),
            name_info.operand());
    }
};

class toc final {
    std::reference_wrapper<::machine> machine_;
    source_locations locations_;
    scope_stack scopes_;
    data_table data_;
    function_table funcs_;
    generic_registry generics_;
    type_table types_;
    definition_checks definitions_;
    builtin_types builtins_;
    ident_resolver resolver_;
    variable_comments var_comments_;
    storage_layout storage_;
    check_options checks_;
    // above zero while the code is compiled only to be checked
    size_t dry_run_depth_{};
    // false while the program is parsed: the names are declared without
    // comments and without the capacity check of the storage
    bool is_compiling_{};

    // what a dry run changes in 'toc' is put back when it ends, also when an
    // error unwinds: the deepest frame count, the string constants and the
    // use of storage; while it lives a noinline body that a call asks for is
    // not added, the call is compiled again for real
    class dry_run_scope final {
        toc& tc_;
        size_t kept_max_frame_count_;
        size_t kept_string_constant_count_;
        storage_layout::dry_run_state kept_storage_;

      public:
        explicit dry_run_scope(toc& tc)
            : tc_{tc}, kept_max_frame_count_{tc.scopes_.max_depth()},
              kept_string_constant_count_{tc.data_.constant_count()},
              kept_storage_{tc.storage_.begin_dry_run()} {

            ++tc_.dry_run_depth_;
        }

        dry_run_scope(const dry_run_scope&) = delete;
        dry_run_scope(dry_run_scope&&) = delete;
        auto operator=(const dry_run_scope&) -> dry_run_scope& = delete;
        auto operator=(dry_run_scope&&) -> dry_run_scope& = delete;

        ~dry_run_scope() {
            --tc_.dry_run_depth_;
            tc_.storage_.end_dry_run(kept_storage_);
            tc_.scopes_.set_max_depth(kept_max_frame_count_);
            tc_.data_.resize_constants(kept_string_constant_count_);
        }
    };

    // a version of code that fails leaves the frames it entered, so the other
    // version is compiled in the frames of the caller, see
    // 'emit_most_efficient'
    class frame_depth_scope final {
        toc& tc_;
        size_t depth_;

      public:
        explicit frame_depth_scope(toc& tc)
            : tc_{tc}, depth_{tc.scopes_.frames().size()} {}

        frame_depth_scope(const frame_depth_scope&) = delete;
        frame_depth_scope(frame_depth_scope&&) = delete;
        auto operator=(const frame_depth_scope&) -> frame_depth_scope& = delete;
        auto operator=(frame_depth_scope&&) -> frame_depth_scope& = delete;

        ~frame_depth_scope() {
            while (tc_.scopes_.frames().size() > depth_) {
                tc_.storage_.pop_scope();
            }
        }
    };

  public:
    // an explicit conversion wraps the arithmetic of its argument, the
    // overflow check is off while it is compiled
    class overflow_check_pause final {
        toc& tc_;
        bool was_enabled_;

      public:
        explicit overflow_check_pause(toc& tc)
            : tc_{tc}, was_enabled_{tc.checks_.overflow} {

            tc_.checks_.overflow = false;
        }

        overflow_check_pause(const overflow_check_pause&) = delete;
        overflow_check_pause(overflow_check_pause&&) = delete;
        auto operator=(const overflow_check_pause&)
            -> overflow_check_pause& = delete;
        auto operator=(overflow_check_pause&&)
            -> overflow_check_pause& = delete;

        ~overflow_check_pause() { tc_.checks_.overflow = was_enabled_; }
    };

    toc(::machine& backend, const source_files& files,
        const builtin_type_set& builtin_set, const size_t vars_capacity_bytes,
        const check_options& checks)
        : machine_{backend}, locations_{&files},
          data_{backend.data_alignment()},
          definitions_{scopes_, funcs_, types_, generics_, locations_},
          builtins_{backend, builtin_set},
          resolver_{scopes_, backend, generics_, builtins_},
          var_comments_{backend, resolver_},
          storage_{backend, scopes_, data_, vars_capacity_bytes},
          checks_{checks} {}

    auto add_alias(const alias_info& ai) -> void {
        scopes_.back().add_alias(ai);
    }

    // e.g. the packed elements of a constant '{...}' initializer
    [[nodiscard]] auto add_bytes_constant(const token& src_loc_tk,
                                          const std::string_view bytes)
        -> std::string {

        return add_read_only_constant("init", src_loc_tk,
                                      token::encode_string(bytes));
    }

    // false when the signature was already added, so a call is checked once
    [[nodiscard]] auto add_checked_noinline_call(std::string signature)
        -> bool {

        return funcs_.add_checked_call(std::move(signature));
    }

    auto add_const(const token& src_loc_tk, const size_t indent,
                   const std::string_view name, const int64_t value) -> void {

        definitions_.assert_name_free(src_loc_tk, name);
        definitions_.assert_not_hiding_signature(src_loc_tk, "constant", name);

        if (is_compiling_) {
            ::machine& x{machine()};

            x.comment(src_loc_tk, indent, "const {} = {}", name, value);
        }

        scopes_.back().add_const(name, {
                                           .src_loc_tk{src_loc_tk},
                                           .value{value},
                                       });
    }

    // the parse places the data of 'dat' after the dats before it and
    // declares the variable 'var' that names it, the compile declares it again
    // with 'add_var'; 'dat_tk' locates the errors of where it is declared
    auto add_dat(const stmt_def_dat& dat, const token& dat_tk,
                 const var_info& var) -> void {

        // note: a name declared twice is reported before where it is declared
        definitions_.assert_name_free(var.src_loc_tk, var.name);

        // note: a 'dat' in a block is rejected when it is parsed
        assert(scopes_.frames().size() == 1);

        if (scopes_.front().has_non_dat_var_been_added()) {
            throw compile_error{dat_tk,
                                "'dat' can only be declared before any 'var'"};
        }

        const size_t size_bytes{
            storage_.var_storage_size_bytes(var.src_loc_tk, var),
        };

        data_.add_dat(dat, {
                               .name_tk{var.src_loc_tk},
                               .alignment{storage_.var_storage_alignment(var)},
                               .size_bytes{size_bytes},
                           });

        add_var(var.src_loc_tk, 0, var, var_kind::dat);
    }

    auto add_func(const token& src_loc_tk, std::string name,
                  const type& return_type, const stmt_def_func* const func_def,
                  std::string generic_method) -> void {

        definitions_.assert_function_not_defined(src_loc_tk, name,
                                                 generic_method);

        funcs_.add(src_loc_tk, std::move(name), return_type, func_def,
                   std::move(generic_method));
    }

    // keeps the instance alive, 'funcs_' refers to it
    auto add_func_instance(std::shared_ptr<const stmt_def_func> instance)
        -> void {

        funcs_.add_instance(std::move(instance));
    }

    auto add_generic_func(std::string name, generic_func_info info) -> void {
        definitions_.assert_function_not_defined(info.src_loc_tk, name, {});

        generics_.add_func(std::move(name), std::move(info));
    }

    auto add_generic_method(const generic_method_info& method) -> void {
        generics_.add_method(method);
    }

    auto add_generic_type(const std::string_view name, generic_type_info info)
        -> void {

        definitions_.assert_type_not_defined(info.src_loc_tk, name);

        generics_.add_type(name, std::move(info));
    }

    auto add_generic_type_instance(generic_type_instance instance) -> void {
        generics_.add_type_instance(std::move(instance));
    }

    // a body is emitted once per kind of call, for a call that is
    // compiled for real: a dry run leaves no trace, the call is compiled again
    auto add_noinline_body(const stmt_def_func& func,
                           std::vector<size_t> array_lengths) -> void {

        if (dry_run_depth_ != 0) {
            return;
        }

        funcs_.add_noinline_body(func, std::move(array_lengths));
    }

    // identical strings share the label of the first one compiled
    [[nodiscard]] auto add_string_constant(const token& string_tk)
        -> std::string {

        return add_read_only_constant("str", string_tk,
                                      string_tk.string_text());
    }

    auto add_type(const token& src_loc_tk, const type& tpe) -> void {
        definitions_.assert_type_not_defined(src_loc_tk, tpe.name());

        types_.add(src_loc_tk, tpe);
    }

    auto add_type_alias(const token& src_loc_tk, const std::string_view name,
                        const type& tpe) -> void {

        definitions_.assert_type_not_defined(src_loc_tk, name);
        types_.add_alias(src_loc_tk, name, tpe);
    }

    // e.g. 'T' names 'i8' in an instance of 'func f<T type>()', until
    // 'exit_type_params'
    auto add_type_param(const token& src_loc_tk, const std::string_view name,
                        const type& tpe) -> void {

        definitions_.assert_generic_param_free(src_loc_tk, name);
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

        // the value lives in its register for the whole scope
        const bool is_value_register{not var.value_register.is_empty()};

        // the variable is where the register points, it has no storage
        const bool is_pointer_register{not var.pointer_register.is_empty()};

        size_t allocated_size_bytes{};

        if (kind == var_kind::dat) {
            allocated_size_bytes = storage_.allocate_dat(src_loc_tk, var);
        } else if (not is_value_register and not is_pointer_register) {
            allocated_size_bytes = storage_.allocate_var(src_loc_tk, var);
        }

        scopes_.back().add_var(var, allocated_size_bytes, kind);

        if (is_compiling_) {
            var_comments_.comment(src_loc_tk, indent, var);
        }
    }

    auto assert_generic_param_free(const token& src_loc_tk,
                                   const std::string_view name) const -> void {

        definitions_.assert_generic_param_free(src_loc_tk, name);
    }

    auto assert_released() const -> void {
        assert(scopes_.is_empty());

        storage_.assert_released();
    }

    // the parse is over: declarations comment where they are stored and the
    // storage capacity is checked, e.g. a parameter 'self' of a type larger
    // than the vars section is a pointer once the body is compiled
    auto begin_compile() -> void {
        assert(not is_compiling_);

        is_compiling_ = true;
        storage_.begin_compile();
    }

    // the tables are read through these accessors, a change goes through a
    // method of 'toc' because it keeps several of them consistent
    [[nodiscard]] auto builtins() const -> const builtin_types& {
        return builtins_;
    }

    // runs 'compile' for its errors only: its output, string constants and
    // use of storage leave no trace, also when 'compile' ends at an error it
    // ignores, see 'stmt_def_func::check_body'
    auto check_only(const std::function_ref<void()> compile) -> void {
        const dry_run_scope dry_run{*this};

        ::machine& x{machine()};

        x.emit_discarded([&] -> void {
            const frame_depth_scope frames{*this};
            compile();
        });
    }

    // what '--checks' turned on
    [[nodiscard]] auto checks() const -> const check_options& {
        return checks_;
    }

    // a number or a constant
    [[nodiscard]] auto constant_value_of(const token& tk) const
        -> std::optional<int64_t> {

        if (const std::optional<int64_t> value{
                constant_parser::parse_constant(tk, tk.text()),
            };
            value) {

            return value;
        }

        return scopes_.const_value(tk.text());
    }

    // the label of the start of a comparison or of a list of comparisons
    [[nodiscard]] auto create_cmp_label(const token& src_loc_tk) const
        -> std::string {

        return create_unique_label(src_loc_tk, "cmp");
    }

    [[nodiscard]] auto create_unique_label(const token& src_loc_tk,
                                           const std::string_view prefix) const
        -> std::string {

        const std::string_view call_path{get_call_path()};
        const std::string src_loc{source_location_for_use_in_label(src_loc_tk)};

        return std::format("{}.{}{}", prefix, src_loc,
                           call_path.empty() ? std::string{}
                                             : std::format(".{}", call_path));
    }

    [[nodiscard]] auto data() const -> const data_table& { return data_; }

    // 'machine::emit_most_efficient' of versions that compile statements
    auto emit_most_efficient(const token& src_loc_tk, const size_t indent,
                             const std::function_ref<void()> emit_first,
                             const std::function_ref<void()> emit_second)
        -> void {

        ::machine& x{machine()};

        x.emit_most_efficient(
            src_loc_tk, indent,
            [&] -> void {
                const frame_depth_scope frames{*this};
                emit_first();
            },
            [&] -> void {
                const frame_depth_scope frames{*this};
                emit_second();
            });
    }

    auto enter_block() -> void { scopes_.enter_block(); }

    auto enter_foo(const std::string_view name) -> void {
        scopes_.enter_foo(name);
    }

    // a function compiled in place, 'return' jumps to 'return_jmp_label'
    auto enter_func(const std::string_view name,
                    const std::string_view call_path = {},
                    const std::string_view return_jmp_label = {}) -> void {

        scopes_.enter_func(name, call_path, return_jmp_label);
    }

    auto enter_loop(const std::string_view name) -> void {
        scopes_.enter_loop(name);
    }

    // see 'frame::noinline_dry_run'
    auto enter_noinline_dry_run(const std::string_view name,
                                const std::string_view call_path,
                                const std::string_view return_jmp_label)
        -> void {

        scopes_.enter_noinline_dry_run(name, call_path, return_jmp_label);
    }

    // a function with a body of its own, its variables are placed from
    // 'storage_base_register' when not empty
    auto enter_noinline_func(const std::string_view name,
                             const std::string_view call_path,
                             const std::string_view storage_base_register)
        -> void {

        scopes_.enter_noinline_func(name, call_path, storage_base_register);
    }

    // the type parameters of an enclosing instance are hidden until
    // 'exit_type_params'
    auto enter_type_params() -> void { types_.enter_params(); }

    auto exit_block() -> void {
        assert(scopes_.back().is_block());

        storage_.pop_scope();
    }

    auto exit_foo([[maybe_unused]] const std::string_view name) -> void {
        assert(scopes_.back().is_foo() and scopes_.back().is_name(name));

        storage_.pop_scope();
    }

    auto exit_func([[maybe_unused]] const std::string_view name) -> void {
        assert(scopes_.back().is_func() and scopes_.back().is_name(name));

        storage_.pop_scope();
    }

    auto exit_loop([[maybe_unused]] const std::string_view name) -> void {
        assert(scopes_.back().is_loop() and scopes_.back().is_name(name));

        storage_.pop_scope();
    }

    auto exit_type_params() -> void { types_.exit_params(); }

    [[nodiscard]] auto funcs() const -> const function_table& { return funcs_; }

    [[nodiscard]] auto generics() const -> const generic_registry& {
        return generics_;
    }

    // empty outside of a function, e.g. in the initializer of a global variable
    [[nodiscard]] auto get_call_path() const -> std::string_view {
        if (not scopes_.is_in_func()) {
            return {};
        }

        return scopes_.current_func_frame().call_path();
    }

    [[nodiscard]] auto get_func_or_throw(const token& src_loc_tk,
                                         const std::string_view name) const
        -> const stmt_def_func& {

        return *get_func_info_or_throw(src_loc_tk, name).def;
    }

    [[nodiscard]] auto
    get_func_return_type_or_throw(const token& src_loc_tk,
                                  const std::string_view name) const
        -> const type& {

        return *get_func_info_or_throw(src_loc_tk, name).type_ptr;
    }

    // no token names a missing 'main'
    [[nodiscard]] auto get_main_or_throw() const -> const stmt_def_func& {
        if (not funcs_.has(reserved_names::main)) {
            throw compile_error::file_level("function 'main' not found");
        }

        return *funcs_.get(reserved_names::main).def;
    }

    [[nodiscard]] auto get_type_or_throw(const token& src_loc_tk,
                                         const std::string_view name) const
        -> const type& {

        if (not types_.has(name)) {
            if (generics_.has_type(name)) {
                throw compile_error{
                    src_loc_tk,
                    std::format("generic type '{}' is not a type, name an "
                                "instance first, e.g. 'type str = {}<...>'",
                                name, name)};
            }

            throw compile_error{src_loc_tk,
                                std::format("type '{}' not found", name)};
        }

        // note: 'void' is the type of what has no value, it has no size
        if (types_.get(name).is_same(builtins_.void_type())) {
            throw compile_error{
                src_loc_tk, std::format("'{}' is not a type of a value", name)};
        }

        return types_.get(name);
    }

    // whether the backend passes the address of the receiver of a method in a
    // register
    [[nodiscard]] auto has_slot_register() const -> bool {
        return not machine_.get().slot_register().empty();
    }

    [[nodiscard]] auto is_func(const std::string_view name) const -> bool {
        return funcs_.has(name) or generics_.has_func(name);
    }

    [[nodiscard]] auto machine() -> ::machine& { return machine_.get(); }

    [[nodiscard]] auto make_ident_info(const statement& st) const
        -> ident_info {

        // the name refers to the declared array, 'ps[1]' accesses one element
        const ident_info declared{
            resolver_.resolve(st.tok(), st.name_or_literal()),
        };

        const bool is_element{st.is_array_element()};
        ident_info info{ident_builder::as_element_if(is_element, declared)};

        info.set_src_loc_tk(st.name_token());

        return info;
    }

    [[nodiscard]] auto make_ident_info(const token& src_loc_tk,
                                       const std::string_view ident) const
        -> ident_info {

        return resolver_.resolve(src_loc_tk, ident);
    }

    // a whole array is not read as its first element
    [[nodiscard]] auto make_scalar_ident_info(const statement& st) const
        -> ident_info {

        ident_info info{make_ident_info(st)};
        assert_not_whole_array(st, info);
        return info;
    }

    // the variable declared last in this scope, e.g. a 'let' variable once
    // its initializer is parsed
    auto make_var_read_only(const std::string_view name,
                            const read_only_cause cause) -> void {

        scopes_.back().make_var_read_only(name, cause);
    }

    // also in a dry run, the errors of the body were checked
    auto mark_func_reached(const stmt_def_func& func) -> void {
        funcs_.mark_reached(func);
    }

    auto mark_generic_func_instantiated(const std::string_view name) -> void {
        generics_.mark_func_instantiated(name);
    }

    // a callee frame starts aligned for any variable it holds
    [[nodiscard]] auto next_frame_address() const -> operand {
        return storage_.next_frame_address(builtins_.address());
    }

    auto reset_usage() -> void {
        assert(scopes_.is_empty());

        storage_.reset_usage();
        scopes_.set_max_depth(0);

        // note: the parse compiles no call
        assert(not funcs_.has_compile_records());
    }

    [[nodiscard]] auto scopes() const -> const scope_stack& { return scopes_; }

    [[nodiscard]] auto
    source_location_for_use_in_label(const token& src_loc_tk) const
        -> std::string {

        return locations_.for_label(src_loc_tk);
    }

    // the text of the file of the token
    [[nodiscard]] auto source_of(const token& src_loc_tk) const
        -> std::string_view {

        return locations_.source_of(src_loc_tk);
    }

    [[nodiscard]] auto storage() const -> const storage_layout& {
        return storage_;
    }

    // what 'name' names with 'path' inside it, as a root variable and bytes
    // of it. a parameter is followed to its argument and the element 'e' of a
    // 'foo' is any element of the array it iterates
    [[nodiscard]] auto
    storage_target_of(const token& src_loc_tk, const std::string_view name,
                      const std::optional<access_path>& path) const
        -> storage_target {

        const foo_array_info array{scopes_.foo_array(name)};

        if (array.root.empty()) {
            return {
                .root{root_variable_of(src_loc_tk, name)},
                .path{scopes_.root_path(name, path)},
            };
        }

        // note: the array was resolved to its root variable when the 'foo'
        //       was made, the callee of a call in the 'foo' does not see 'e'
        std::optional<access_path> element_path{array.path};

        if (element_path) {
            element_path->add_run_time_index();

            if (path) {
                element_path = element_path->extended_by(*path);
            }
        }

        return {
            .root{array.root},
            .path{std::move(element_path)},
        };
    }

    [[nodiscard]] auto types() const -> const type_table& { return types_; }

    [[nodiscard]] auto usage() const -> usage_statistics {
        return {
            .max_frame_count{scopes_.max_depth()},
            .max_vars_size_bytes{storage_.max_vars_size_bytes()},
            .dat_size_bytes{data_.total_size_bytes()},
            .dat_var_padding_bytes{data_.dat_var_padding_bytes()},
            .uninstantiated_generics{generics_.uninstantiated_func_names()},
        };
    }

    //
    // statics
    //

    static auto assert_not_whole_array(const statement& st,
                                       const ident_info& info) -> void {

        if (not info.is_array()) {
            return;
        }

        throw compile_error{st.tok(), std::format("array '{}' must be indexed",
                                                  st.name_or_literal())};
    }

    // a name is an identifier that is not a keyword
    static auto assert_valid_name(const token& name_tk) -> void {
        if (name_tk.text().empty()) {
            throw compile_error{name_tk, "expected a name"};
        }

        if (not is_identifier(name_tk)) {
            throw compile_error{
                name_tk,
                std::format("'{}' is not a valid name, a name starts with a "
                            "letter or '_' and has only letters, digits and "
                            "'_'",
                            name_tk.text())};
        }

        if (std::ranges::contains(reserved_names::keywords, name_tk.text())) {
            throw compile_error{
                name_tk, std::format("'{}' is a keyword", name_tk.text())};
        }
    }

    // a name of a function, variable, constant, data or parameter, which is
    // used without a qualifier
    static auto assert_valid_unqualified_name(const token& name_tk) -> void {
        assert_valid_name(name_tk);

        if (std::ranges::contains(reserved_names::builtin_functions,
                                  name_tk.text())) {

            throw compile_error{
                name_tk,
                std::format("'{}' is a built-in function", name_tk.text())};
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

    // letters, digits and '_', not starting with a digit, written without
    // quotes
    [[nodiscard]] static auto is_identifier(const token& name_tk) -> bool {
        const std::string_view text{name_tk.text()};

        const auto is_letter = [](const char ch) -> bool {
            return (ch >= 'a' and ch <= 'z') or (ch >= 'A' and ch <= 'Z') or
                   ch == '_';
        };

        return not name_tk.is_string() and not text.empty() and
               is_letter(text.front()) and
               std::ranges::all_of(text, [&](const char ch) -> bool {
                   return is_letter(ch) or (ch >= '0' and ch <= '9');
               });
    }

    // the name of a variable the compiler adds, e.g. 'call-arg-12-0', it has a
    // '-' so no identifier of the source can have it; the offset of the token
    // tells the temporaries of calls in one block apart
    [[nodiscard]] static auto temporary_name(const token& src_loc_tk,
                                             const std::string_view role,
                                             const size_t index)
        -> std::string {

        return std::format("{}-{}-{}", role, src_loc_tk.start_index(), index);
    }

  private:
    [[nodiscard]] auto add_read_only_constant(const std::string_view kind,
                                              const token& src_loc_tk,
                                              std::string text) -> std::string {

        return data_.add_constant(
            std::format("{}.{}", kind,
                        source_location_for_use_in_label(src_loc_tk)),
            std::move(text));
    }

    [[nodiscard]] auto get_func_info_or_throw(const token& src_loc_tk,
                                              const std::string_view name) const
        -> const func_info& {

        if (not funcs_.has(name)) {
            throw compile_error{src_loc_tk,
                                std::format("function '{}' not found", name)};
        }

        return funcs_.get(name);
    }

    // the variable that 'name' names, a parameter is followed to its argument
    [[nodiscard]] auto root_variable_of(const token& src_loc_tk,
                                        const std::string_view name) const
        -> std::string {

        const std::string_view bound_root{scopes_.bound_root(name)};

        if (not bound_root.empty()) {
            return std::string{bound_root};
        }

        const ident_info info{make_ident_info(src_loc_tk, name)};

        // a value in a register is no variable, not even one that is named
        // like the register, e.g. 't1' of rv32i or 'r14' of x86_64
        if (info.is_register()) {
            return std::format("{}{}register", info.operand().base_register(),
                               storage_target::frame_mark);
        }

        const std::string_view root{info.elem_path().front()};

        const name_meaning meaning{scopes_.lookup(root)};

        // a constant is no variable, e.g. the '1' of 'f(1)'
        if (not meaning.is_variable()) {
            return std::format("{}{}constant", root,
                               storage_target::frame_mark);
        }

        return std::format("{}{}{}", root, storage_target::frame_mark,
                           meaning.frame_index);
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

// e.g. 'i8' of 'list<i8, 8>' for 'T'
[[nodiscard]] inline auto resolve_type_arg(const toc& tc,
                                           const std::string_view generic_name,
                                           const std::string_view param_name,
                                           const token& arg_tk) -> const type& {

    if (tc.constant_value_of(arg_tk)) {
        throw compile_error{
            arg_tk, std::format("generic parameter '{}' of '{}' needs a "
                                "type, got the constant '{}'",
                                param_name, generic_name, arg_tk.text())};
    }

    return tc.get_type_or_throw(arg_tk, arg_tk.text());
}

// e.g. '8' of 'list<i8, 8>' for 'capacity'
[[nodiscard]] inline auto resolve_const_arg(const toc& tc,
                                            const std::string_view generic_name,
                                            const std::string_view param_name,
                                            const token& arg_tk) -> int64_t {

    const std::optional<int64_t> value{tc.constant_value_of(arg_tk)};

    if (not value) {
        throw compile_error{
            arg_tk,
            std::format("generic parameter '{}' of '{}' needs a "
                        "constant, got {}'{}'",
                        param_name, generic_name,
                        tc.types().has(arg_tk.text()) ? "the type " : "",
                        arg_tk.text())};
    }

    return *value;
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
                .type_ptr{&resolve_type_arg(tc, generic_name, name, arg_tk)},
                .value{},
            });
        } else {
            args.push_back({
                .param_tk{param.name_tk},
                .arg_tk{arg_tk},
                .type_ptr{},
                .value{resolve_const_arg(tc, generic_name, name, arg_tk)},
            });
        }
    }

    return args;
}

// while an instance of a generic definition is parsed, each parameter names
// its argument, e.g. 'T' names 'i8' and 'capacity' is the constant 8; the
// parameters of an enclosing instance are hidden, all of it ends with the scope
class generic_scope final {
    toc& tc_;
    // the constants are in a block of their own
    bool is_block_entered_{};

  public:
    explicit generic_scope(toc& tc) : tc_{tc} { tc_.enter_type_params(); }

    generic_scope(const generic_scope&) = delete;
    generic_scope(generic_scope&&) = delete;
    auto operator=(const generic_scope&) -> generic_scope& = delete;
    auto operator=(generic_scope&&) -> generic_scope& = delete;

    ~generic_scope() {
        if (is_block_entered_) {
            tc_.exit_block();
        }

        tc_.exit_type_params();
    }

    // a type is located at its parameter, a constant at its argument
    auto add_args(const std::span<const generic_arg> args) -> void {
        for (const generic_arg& arg : args) {
            if (arg.type_ptr != nullptr) {
                add_type(arg.param_tk, arg.param_tk.text(), *arg.type_ptr);
            } else {
                add_const(arg.arg_tk, arg.param_tk.text(), arg.value);
            }
        }
    }

    // a method of an instance of a generic type sees the generic type as the
    // instance and its type arguments, e.g. 'list' names 'str' and 'T' names
    // 'i8'; its constants are in the frame of the body
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

        if (not is_block_entered_) {
            tc_.enter_block();
            is_block_entered_ = true;
        }

        tc_.add_const(src_loc_tk, 0, name, value);
    }

    auto add_type(const token& src_loc_tk, const std::string_view name,
                  const type& tpe) -> void {

        tc_.add_type_param(src_loc_tk, name, tpe);
    }
};
