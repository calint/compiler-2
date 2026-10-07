#pragma once
// reviewed: 2025-09-29

#include <algorithm>
#include <cassert>
#include <charconv>
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
#include <system_error>
#include <utility>
#include <vector>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "generics.hpp"
#include "lut.hpp"
#include "machine.hpp"
#include "operand.hpp"
#include "statement.hpp"
#include "token.hpp"
#include "type.hpp"

class stmt_def_func;

struct func_info {
    token src_loc_tk;           // token for position in the source
    const stmt_def_func* def{}; // null if built-in function
    const type* type_ptr{};     // return type or void
    // e.g. 'text.size' for the instance 'str.size', empty if not an instance
    // of a method of a generic type
    std::string generic_method;
};

struct func_return_info {
    token type_tk;          // type token
    token ident_tk;         // identifier token
    const type* type_ptr{}; // type
};

// a non-inline body compiled for the array lengths of one kind of call
struct noninline_instance {
    const stmt_def_func* func{};
    // one per array parameter, in parameter order
    std::vector<size_t> array_lengths;
};

struct alias_info {
    std::string from;
    std::string to;
    operand lea;
    const type* type_ptr{};
    operand register_operand;
    // e.g. 'i' -> 'arr[ix]' names one element, not the array 'arr'
    bool is_element{};

    //
    // statics
    //

    [[nodiscard]] static auto make_register(const std::string_view name,
                                            const type& alias_type,
                                            const operand& reg) -> alias_info {

        assert(reg.is_register());

        return {
            .from{std::string{name}},
            .to{reg.base_register()},
            .lea{},
            .type_ptr{&alias_type},
            .register_operand{reg},
            .is_element{},
        };
    }
};

struct const_info {
    token src_loc_tk; // token for position in the source
    int64_t value;
};

// what a variable is declared by: 'var' or 'dat'
enum class var_kind : uint8_t { var, dat };

class frame final {
  public:
    enum class frame_type : uint8_t { func, block, loop, foo };

  private:
    // optional name
    std::string_view name_;

    // a unique path of source locations of the inlined call
    std::string call_path_;

    // number of bytes used on the stack by this frame
    size_t allocated_stack_size_bytes_{};

    // stack padding assigned to this frame
    size_t stack_padding_size_bytes_{};

    // constants
    lut<const_info> consts_;

    // variables declared in this frame
    lut<var_info> vars_;

    // aliases that refer to previous frame(s) alias or variable
    lut<alias_info> aliases_;

    // the label to jump to when exiting an inlined function
    std::string func_ret_label_;

    // true if var that is not dat has been added
    bool non_dat_var_has_been_added_{};

    frame_type type_{frame_type::func}; // frame type
    bool is_inlined_{true};
    std::string_view storage_base_register_;
    size_t peak_storage_size_bytes_{};

  public:
    frame(const std::string_view name, const frame_type frm_type,
          std::string call_path = "", std::string func_ret_label = "",
          const bool is_inlined = true,
          const std::string_view storage_base_register = {}) noexcept
        : name_{name}, call_path_{std::move(call_path)},
          func_ret_label_{std::move(func_ret_label)}, type_{frm_type},
          is_inlined_{is_inlined},
          storage_base_register_{storage_base_register} {}

    auto add_alias(const alias_info& ai) -> void {
        aliases_.put(std::string{ai.from}, ai);
    }

    auto add_const(const std::string_view name, const const_info& ci) -> void {
        consts_.put(std::string{name}, ci);
    }

    auto add_var(const var_info& var, const size_t allocated_size_bytes,
                 const var_kind kind = var_kind::var) -> void {

        allocated_stack_size_bytes_ =
            sum_storage_size(allocated_stack_size_bytes_, allocated_size_bytes);

        vars_.put(var.name, var);

        if (kind != var_kind::dat) {
            non_dat_var_has_been_added_ = true;
        }
    }

    [[nodiscard]] auto allocated_stack_size_bytes() const -> size_t {
        return sum_storage_size(allocated_stack_size_bytes_,
                                stack_padding_size_bytes_);
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

    auto set_padding_between_dats_and_vars(const size_t size_bytes) -> void {
        assert(stack_padding_size_bytes_ == 0);

        stack_padding_size_bytes_ = size_bytes;
    }

    [[nodiscard]] auto storage_base_register() const -> std::string_view {
        return storage_base_register_;
    }
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

    //
    // statics
    //

    // the variable name without the field path
    [[nodiscard]] static auto root_of(const std::string_view id)
        -> std::string_view {

        return id.substr(0, id.find('.'));
    }

  private:
    auto refresh_path() -> void {
        path_.clear();
        for (auto const part : id_ | std::views::split('.')) {
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

        if (not is_element) {
            return info;
        }

        info.is_array = false;
        info.array_len = 0;

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
        size_t array_count{var.array_len};

        for (const std::string& field_name : path | std::views::drop(1)) {
            // note: drop 1 because the first element is retrieved outside the
            //       loop

            const type_field& tf{tp->field(src_loc_tk, field_name)};
            offset = sum_storage_size(offset, tf.offset);
            tp = tf.type_ptr;
            is_array = tf.is_array;
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

        // find first field so operand gets a valid built-in
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
                                    std::move(type_path), op,
                                    {
                                        .offset{idx},
                                        .array_len{array_count},
                                        .is_array{is_array},
                                        .is_pointer{var.is_pointer},
                                    });
    }

    // makes room in 'lea_path' for the elements 'target_count' adds
    //
    // 'lea_path' has one entry per element of the identifier, an empty entry
    // where no address is known; the entries are built while walking the
    // frames outwards, so a target with several elements needs entries
    // between the alias address just added and the next base
    static auto pad_lea_path(std::vector<operand>& lea_path,
                             const size_t target_count) -> void {

        const size_t lea_count{lea_path.size()};

        if (target_count > lea_count and target_count - lea_count > 1) {
            // -2 because the last element is the alias address and the
            // first is added when its own frame is walked
            lea_path.resize(lea_count + target_count - 2);
        }
    }

    // a built-in identifier below a run-time indexed element is addressed from
    // the last address held, e.g. 'wld.rooms[ix].description.data'
    static auto place_operand_from_lea(const token& src_loc_tk,
                                       ident_info& info) -> void {

        // find the first element from the top that has a 'lea' and get
        // accessor relative to that
        operand lea;
        size_t lea_index{info.elem_path.size()};
        while (lea_index--) {
            if (not info.lea_path.at(lea_index).is_empty()) {
                lea = info.lea_path.at(lea_index);
                break;
            }
        }

        if (lea.is_empty()) {
            return;
        }

        // identifier has lea, construct operand

        // example of resulting data structure:
        //
        // type string { len : i8, data : i8[127] }
        // type room { name : string, description : string, note : string }
        // type world { rooms : room[128] }
        //
        // id path     |  type  |  lea          |
        // ------------|--------|---------------|
        // wld         | world  | -             |
        // rooms[2]    | room   | r15           |
        // description | string | -             |
        // data        | i8     | r15 + 129     |
        //
        // the indexing in 'rooms' is done at runtime thus the memory
        // location of 'rooms[2]' cannot be deduced statically, thus the
        // last lea encountered is the starting point when accessing
        // identifiers

        // start from the lea address and calculate offset to referred field
        const std::span<std::string> elem_path_from_lea{
            std::span{info.elem_path}.subspan(lea_index),
        };

        // navigate to referred element and get offset
        const size_t offset{
            info.type_path.at(lea_index)->field_offset(src_loc_tk,
                                                       elem_path_from_lea),
        };

        info.operand = operand::mem(lea, info.type_ref());

        if (offset != 0) {
            info.operand.increment_offset(address_offset(offset));
        }
    }

    // 'id' with the base replaced by what the alias refers to, e.g.
    //   res -> pt.x becomes pt.x
    //   pt.x -> p becomes p.x
    //   lnk.count -> world.room.link becomes world.room.link.count
    [[nodiscard]] static auto replace_alias_base(const alias_info& alias,
                                                 const ident_path& id,
                                                 std::vector<operand>& lea_path)
        -> ident_path {

        ident_path target{std::string{alias.to}};

        pad_lea_path(lea_path, target.path().size());

        for (const std::string& s : id.path() | std::views::drop(1)) {
            target.append(s);
        }

        return target;
    }
};

// the types by name. a generic instance also names the arguments of its type
// parameters while it is parsed, those names are aliases
class type_table final {
    struct entry {
        token src_loc_tk;
        const type* type_ptr;
    };

    using alias_list = std::vector<std::pair<std::string, entry>>;

    lut<entry> entries_;
    alias_list aliases_;
    // the aliases of the callers of the instance being parsed
    std::vector<alias_list> hidden_aliases_;

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

    // a type parameter names its argument until 'unbind', the name is free
    auto bind(const token& src_loc_tk, const std::string_view name,
              const type& tpe) -> void {

        assert(not has(name));

        entries_.put(std::string{name}, {
                                            .src_loc_tk{src_loc_tk},
                                            .type_ptr{&tpe},
                                        });

        aliases_.emplace_back(std::string{name}, entries_.get_const_ref(name));
    }

    [[nodiscard]] auto get(const std::string_view name) const -> const type& {
        return *entries_.get_const_ref(name).type_ptr;
    }

    [[nodiscard]] auto has(const std::string_view name) const -> bool {
        return entries_.has(name);
    }

    // an instance sees the types, not the type parameters of its callers
    auto hide_aliases() -> void {
        for (const auto& [name, entry] : aliases_) {
            entries_.erase(name);
        }

        hidden_aliases_.push_back(std::move(aliases_));
        aliases_.clear();
    }

    auto restore_aliases() -> void {
        assert(aliases_.empty());

        aliases_ = std::move(hidden_aliases_.back());
        hidden_aliases_.pop_back();

        for (const auto& [name, entry] : aliases_) {
            entries_.put(name, entry);
        }
    }

    [[nodiscard]] auto src_loc_tk_of(const std::string_view name) const
        -> const token& {

        return entries_.get_const_ref(name).src_loc_tk;
    }

    auto unbind(const std::string_view name) -> void {
        entries_.erase(name);

        std::erase_if(aliases_, [name](const auto& alias) -> bool {
            return alias.first == name;
        });
    }
};

// the value of a number or character literal as the tokenizer keeps its text
class constant_parser final {
  public:
    //
    // statics
    //

    // the tokenizer keeps the quotes in the text of a character literal
    [[nodiscard]] static auto is_character_literal(const std::string_view str)
        -> bool {

        return str.size() >= 2 and str.starts_with('\'') and
               str.ends_with('\'');
    }

    // the value is the byte, e.g. 'a' is 97 and '\\xff' is 255
    [[nodiscard]] static auto parse_character(const token& src_loc_tk,
                                              const std::string_view str)
        -> int64_t {

        assert(is_character_literal(str));

        const std::string_view body{str.substr(1, str.size() - 2)};
        // note: 1 and -2 because the literal is between the quotes

        if (body.size() == 1 and body.at(0) != '\\') {
            return static_cast<unsigned char>(body.at(0));
        }

        if (not body.starts_with('\\')) {
            throw compiler_exception{
                src_loc_tk,
                std::format("character literal {} must contain one character",
                            str)};
        }

        const std::optional<char> decoded{token::decode_escape(body.substr(1))};

        if (not decoded) {
            const size_t backslash_index{src_loc_tk.start_index() + 1};

            // note: +1 because the backslash follows the opening quote
            const token escape_tk{
                src_loc_tk.is_text(str)
                    ? token::position(backslash_index, src_loc_tk.at_line())
                    : src_loc_tk,
            };

            throw compiler_exception{
                escape_tk,
                std::format("unsupported escape in character literal {}", str)};
        }

        return static_cast<unsigned char>(*decoded);
    }

    // e.g. '12', '0x1f', '0b101' and ''a'' have a value, 'x' has none
    [[nodiscard]] static auto parse_constant(const token& src_loc_tk,
                                             const std::string_view str)
        -> std::optional<int64_t> {

        if (is_character_literal(str)) {
            return parse_character(src_loc_tk, str);
        }

        constexpr int base_decimal{10};
        constexpr int base_hex{16};
        constexpr int base_binary{2};

        int base{base_decimal};
        std::string_view digits{str};

        if (str.starts_with("0x") or str.starts_with("0X")) {
            base = base_hex;
            digits.remove_prefix(2);
            // note: 2 for the '0x' prefix
        } else if (str.starts_with("0b") or str.starts_with("0B")) {
            base = base_binary;
            digits.remove_prefix(2);
            // note: 2 for the '0b' prefix
        }

        int64_t value{};

        const char* const begin{std::to_address(digits.begin())};
        const char* const end{std::to_address(digits.end())};

        const std::from_chars_result result{
            std::from_chars(begin, end, value, base),
        };

        if (result.ec == std::errc::result_out_of_range) {
            throw compiler_exception{
                src_loc_tk, std::format("constant '{}' is out of range", str)};
        }

        if (result.ec == std::errc{} and result.ptr == end) {
            return value;
        }

        return std::nullopt;
    }
};

// the functions of the program: the built-in ones, the defined ones and the
// instances of generic ones, and the bodies of non-inline functions to compile
class function_table final {
    lut<func_info> funcs_;
    std::vector<const stmt_def_func*> defs_;
    std::vector<std::shared_ptr<const stmt_def_func>> instances_;
    std::vector<noninline_instance> noninline_instances_;
    std::set<std::string> checked_calls_;

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

    // a body is emitted once per distinct instance
    auto add_noninline_instance(const stmt_def_func& func,
                                std::vector<size_t> array_lengths) -> void {

        const auto is_same_instance =
            [&](const noninline_instance& known) -> bool {
            return known.func == &func and known.array_lengths == array_lengths;
        };

        if (std::ranges::any_of(noninline_instances_, is_same_instance)) {
            return;
        }

        noninline_instances_.push_back({
            .func{&func},
            .array_lengths{std::move(array_lengths)},
        });
    }

    auto clear_noninline_instances() -> void { noninline_instances_.clear(); }

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

    // a copy, the list grows while the instances compile
    [[nodiscard]] auto noninline_instance_at(const size_t index) const
        -> noninline_instance {

        return noninline_instances_.at(index);
    }

    [[nodiscard]] auto noninline_instance_count() const -> size_t {
        return noninline_instances_.size();
    }
};

// the 'dat' declarations and the read-only constants of the program
class data_table final {
    std::vector<const statement*> statements_;
    std::vector<machine::string_constant> constants_;
    size_t total_size_bytes_{};
    // padding after the dats so that the variables are aligned
    size_t entry_gap_{};

  public:
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

    // 'data_alignment' is the alignment of the data section of the machine
    auto add_dat(const statement* const stmt, const size_t data_alignment)
        -> void {

        statements_.emplace_back(stmt);

        // same padding as 'add_var' places before the dat
        total_size_bytes_ = add_storage_size(
            stmt->tok(),
            align_storage_size(total_size_bytes_, stmt->get_type().alignment()),
            stmt->dat_size_bytes());

        entry_gap_ = (data_alignment - (total_size_bytes_ % data_alignment)) %
                     data_alignment;
    }

    [[nodiscard]] auto constant_count() const -> size_t {
        return constants_.size();
    }

    [[nodiscard]] auto constants() const
        -> std::span<const machine::string_constant> {

        return constants_;
    }

    [[nodiscard]] auto entry_gap() const -> size_t { return entry_gap_; }

    // drops the constants added after the first 'count'
    auto resize_constants(const size_t count) -> void {
        constants_.resize(count);
    }

    [[nodiscard]] auto statements() const
        -> const std::vector<const statement*>& {

        return statements_;
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
    bool bounds_with_line{};
    bool frame{};
    bool alias{};
};

// what the compile used, for the report
struct usage_statistics {
    size_t max_frame_count{};
    size_t max_vars_size_bytes{};
    size_t dat_size_bytes{};
    size_t dat_var_padding_bytes{};
    std::vector<std::string> uninstantiated_generics;
};

// the frames of the scopes being compiled, from the root frame that holds the
// globals to the innermost one
class scope_stack final {
    std::vector<frame> frames_;
    size_t max_depth_{};

  public:
    [[nodiscard]] auto back() -> frame& { return frames_.back(); }

    [[nodiscard]] auto back() const -> const frame& { return frames_.back(); }

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
        frames_.emplace_back("", frame::frame_type::block);
        track_depth();
    }

    auto enter_foo(const std::string_view name) -> void {
        frames_.emplace_back(name, frame::frame_type::foo);
        track_depth();
    }

    // a function compiled in place, 'return' jumps to 'return_jmp_label'
    auto enter_func(const std::string_view name,
                    const std::string_view call_path,
                    const std::string_view return_jmp_label) -> void {

        frames_.emplace_back(name, frame::frame_type::func,
                             std::string{call_path},
                             std::string{return_jmp_label}, true);

        track_depth();
    }

    auto enter_loop(const std::string_view name) -> void {
        frames_.emplace_back(name, frame::frame_type::loop);
        track_depth();
    }

    // a function with a body of its own, its variables are placed from
    // 'storage_base_register' when not empty
    auto enter_noninline_func(const std::string_view name,
                              const std::string_view call_path,
                              const std::string_view storage_base_register)
        -> void {

        frames_.emplace_back(name, frame::frame_type::func,
                             std::string{call_path}, std::string{}, false,
                             storage_base_register);

        track_depth();
    }

    // constants of the current function, then the global ones, not those of
    // the calling functions
    [[nodiscard]] auto find_const(const std::string_view name) const
        -> const const_info* {

        for (const frame& f : frames_ | std::views::reverse) {
            if (f.has_const(name)) {
                return &f.get_const(name);
            }

            if (f.is_func()) {
                break;
            }
        }

        if (frames_.front().has_const(name)) {
            return &frames_.front().get_const(name);
        }

        return nullptr;
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
                count++;
            }
        }

        return count;
    }

    [[nodiscard]] auto is_empty() const -> bool { return frames_.empty(); }

    [[nodiscard]] auto is_in_loop_block() const -> bool {
        for (const frame& frm : frames_ | std::views::reverse) {
            if (frm.is_foo()) {
                return false;
            }

            if (frm.is_loop()) {
                return true;
            }
        }

        std::unreachable();
    }

    // same scoping as the identifier resolution, e.g. a variable 'limits'
    // hides the type 'limits'
    [[nodiscard]] auto is_var_or_alias(const std::string_view name) const
        -> bool {

        for (const frame& frm : frames_ | std::views::reverse) {
            if (frm.has_var(name)) {
                return true;
            }

            if (frm.is_func()) {
                return frm.has_alias(name) or frames_.front().has_var(name);
            }
        }

        return false;
    }

    [[nodiscard]] auto looping_label_or_throw(const token& src_loc_tk) const
        -> std::string_view {

        for (const frame& frm : frames_ | std::views::reverse) {
            if (frm.is_loop() or frm.is_foo()) {
                return frm.name();
            }

            if (frm.is_func()) {
                throw compiler_exception{src_loc_tk, "not in a loop"};
            }
        }

        std::unreachable();
    }

    [[nodiscard]] auto max_depth() const -> size_t { return max_depth_; }

    auto pop() -> void { frames_.pop_back(); }

    auto set_max_depth(const size_t depth) -> void { max_depth_ = depth; }

  private:
    auto track_depth() -> void {
        max_depth_ = std::max(frames_.size(), max_depth_);
    }
};

// where the variables and the dats are placed: the running size of the
// variables section, the frames that have a storage base of their own, the
// capacity of the section and the target limits
class storage_layout final {
    // where a variable is placed, 'storage_frame' is null at the variables base
    struct storage_location {
        frame* storage_frame;
        size_t base_offset;
    };

    std::reference_wrapper<::machine> machine_;
    std::reference_wrapper<scope_stack> scopes_;
    std::reference_wrapper<const data_table> data_;
    size_t vars_size_bytes_{};
    size_t vars_capacity_bytes_;
    size_t max_vars_size_bytes_{};
    bool entry_gap_applied_{};

    // the locals of a dry run live in the callee's own frame
    bool capacity_unchecked_{};

  public:
    // what a dry run changes and 'end_dry_run' puts back
    struct dry_run_state {
        bool capacity_unchecked;
        size_t max_vars_size_bytes;
        // the index of the frame, frames added meanwhile may move the frames
        std::optional<size_t> storage_frame_index;
        size_t peak_storage_size_bytes;
    };

    storage_layout(::machine& backend, scope_stack& scopes,
                   const data_table& data, const size_t vars_capacity_bytes)
        : machine_{backend}, scopes_{scopes}, data_{data},
          vars_capacity_bytes_{vars_capacity_bytes} {}

    // places 'var' in the innermost storage that has room and returns the
    // bytes it takes with the alignment padding, 'var.offset' is set
    [[nodiscard]] auto allocate(const token& src_loc_tk, var_info& var,
                                const var_kind kind) -> size_t {

        const size_t var_size_bytes{var_storage_size_bytes(src_loc_tk, var)};
        const size_t var_alignment{var_storage_alignment(var)};

        if (kind != var_kind::dat) {
            apply_entry_gap(src_loc_tk);
        }

        // offsets are relative to the variables base or to the nearest frame
        // with its own storage base, both are aligned
        const storage_location location{find_storage_location(kind)};

        frame* const storage_frame{location.storage_frame};
        const size_t base_offset{location.base_offset};

        const size_t padding_bytes{
            align_storage_size(base_offset, var_alignment) - base_offset,
        };

        const size_t allocated_size_bytes{
            add_storage_size(src_loc_tk, padding_bytes, var_size_bytes),
        };

        if (kind != var_kind::dat) {
            assert_vars_capacity(src_loc_tk, var.name, allocated_size_bytes);
        }

        var.offset = address_offset(base_offset + padding_bytes);

        if (storage_frame) {
            var.base_register = storage_frame->storage_base_register();

            storage_frame->record_storage_size_bytes(add_storage_size(
                src_loc_tk, base_offset, allocated_size_bytes));
        } else {
            var.offset =
                add_address_offset(var.offset, -variables_base_shift_bytes());
        }

        // the total accepts the size before a frame sums it
        vars_size_bytes_ = add_storage_size(src_loc_tk, vars_size_bytes_,
                                            allocated_size_bytes);

        assert_storage_fits_target(src_loc_tk);

        if (kind != var_kind::dat) {
            max_vars_size_bytes_ =
                std::max(used_vars_size_bytes(), max_vars_size_bytes_);
        }

        return allocated_size_bytes;
    }

    auto assert_released() const -> void { assert(vars_size_bytes_ == 0); }

    auto begin_dry_run() -> dry_run_state {
        frame* const storage_frame{
            find_storage_location(var_kind::var).storage_frame,
        };

        const dry_run_state saved{
            .capacity_unchecked{capacity_unchecked_},
            .max_vars_size_bytes{max_vars_size_bytes_},
            .storage_frame_index{
                storage_frame == nullptr
                    ? std::nullopt
                    : std::optional<size_t>{static_cast<size_t>(
                          storage_frame - scopes_.get().frames().data())},
            },
            .peak_storage_size_bytes{
                storage_frame ? storage_frame->peak_storage_size_bytes() : 0,
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

        size_t local_size_bytes{};
        for (const frame& frm : scopes_.get().frames() | std::views::reverse) {
            local_size_bytes = sum_storage_size(
                local_size_bytes, frm.allocated_stack_size_bytes());

            if (not frm.storage_base_register().empty()) {
                return operand::mem(frm.storage_base_register(), {}, 1,
                                    address_offset(align_storage_size(
                                        local_size_bytes, frame_alignment)),
                                    address_type);
            }
        }

        const size_t root_size_bytes{
            sum_storage_size(vars_size_bytes_,
                             entry_gap_applied_ ? 0 : data_.get().entry_gap()),
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
        for (const frame& frm : scopes_.get().frames() | std::views::reverse) {
            if (not frm.storage_base_register().empty()) {
                return frm.peak_storage_size_bytes();
            }
        }

        std::unreachable();
    }

    // the root frame applies the dat var gap again when it is entered anew
    auto pop_scope() -> void {
        vars_size_bytes_ -= scopes_.get().back().allocated_stack_size_bytes();
        scopes_.get().pop();

        if (not scopes_.get().is_empty()) {
            return;
        }

        assert(vars_size_bytes_ == 0);

        entry_gap_applied_ = false;
    }

    // all frames are popped and the usage of the last build is forgotten
    auto reset_usage() -> void {
        assert(vars_size_bytes_ == 0);

        max_vars_size_bytes_ = 0;
    }

  private:
    // the first variable after the dats starts past the entry gap
    auto apply_entry_gap(const token& src_loc_tk) -> void {
        if (entry_gap_applied_) {
            return;
        }

        scopes_.get().front().set_padding_between_dats_and_vars(
            data_.get().entry_gap());

        vars_size_bytes_ = add_storage_size(src_loc_tk, vars_size_bytes_,
                                            data_.get().entry_gap());

        entry_gap_applied_ = true;
    }

    // the declaration that takes the data and variables past what the target
    // addresses is the error, not the end of the build
    auto assert_storage_fits_target(const token& src_loc_tk) const -> void {
        const size_t max_bytes{machine_.get().max_storage_bytes()};

        if (vars_size_bytes_ <= max_bytes) {
            return;
        }

        throw compiler_exception{
            src_loc_tk,
            std::format("data and variables of {} B exceed the {} B that the "
                        "target addresses",
                        vars_size_bytes_, max_bytes)};
    }

    auto assert_vars_capacity(const token& src_loc_tk,
                              const std::string_view name,
                              const size_t allocated_size_bytes) const -> void {

        if (capacity_unchecked_ or
            allocated_size_bytes <=
                vars_capacity_bytes_ - used_vars_size_bytes()) {

            return;
        }

        throw compiler_exception{
            src_loc_tk,
            std::format("variable '{}' would overflow allocated vars section",
                        name)};
    }

    // a local starts after the storage in use of the nearest frame with its own
    // storage base and of the frames inside it, other variables and dats start
    // at the variables base
    [[nodiscard]] auto find_storage_location(const var_kind kind)
        -> storage_location {

        if (kind == var_kind::dat) {
            return {
                .storage_frame{},
                .base_offset{vars_size_bytes_},
            };
        }

        size_t local_size_bytes{};

        for (frame& frm : scopes_.get().frames() | std::views::reverse) {
            local_size_bytes = sum_storage_size(
                local_size_bytes, frm.allocated_stack_size_bytes());

            if (not frm.storage_base_register().empty()) {
                return {
                    .storage_frame{&frm},
                    .base_offset{local_size_bytes},
                };
            }
        }

        return {
            .storage_frame{},
            .base_offset{vars_size_bytes_},
        };
    }

    // bytes of variables, without the dats and the gap after them
    [[nodiscard]] auto used_vars_size_bytes() const -> size_t {
        return vars_size_bytes_ - data_.get().total_size_bytes() -
               data_.get().entry_gap();
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

        return multiply_storage_size(src_loc_tk, var.type_ptr->size_bytes(),
                                     var.is_array ? var.array_len : 1);
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
                             data_.get().entry_gap()),
        };

        return address_offset(sum_storage_size(dats_bytes, *past_vars_bytes));
    }
};

// the types the front end needs by role: the builtin ones given by the
// program and the ones the machine decides
class builtin_types final {
    std::reference_wrapper<const ::machine> machine_;
    const type* void_{};
    const type* bool_{};
    const type* i64_{};
    const type* i32_{};
    const type* i16_{};
    const type* i8_{};

  public:
    explicit builtin_types(const ::machine& backend) : machine_{backend} {}

    [[nodiscard]] auto address() const -> const type& {
        return machine_.get().address_size_bytes() == 4 ? *i32_ : *i64_;
    }

    [[nodiscard]] auto boolean() const -> const type& { return *bool_; }

    [[nodiscard]] auto default_type() const -> const type& {
        return machine_.get().default_type();
    }

    [[nodiscard]] auto i8() const -> const type& { return *i8_; }

    // 'int' and the names of the builtin integer types
    [[nodiscard]] auto is_integer_name(const std::string_view name) const
        -> bool {

        return name == "int" or name == i8_->name() or name == i16_->name() or
               name == i32_->name() or name == i64_->name();
    }

    auto set_boolean(const type& tpe) -> void { bool_ = &tpe; }

    auto set_integers(const type& t_i64, const type& t_i32, const type& t_i16,
                      const type& t_i8) -> void {

        i64_ = &t_i64;
        i32_ = &t_i32;
        i16_ = &t_i16;
        i8_ = &t_i8;
    }

    auto set_void(const type& tpe) -> void { void_ = &tpe; }

    [[nodiscard]] auto void_type() const -> const type& { return *void_; }
};

// resolves a name to a variable, a register or a constant by walking the
// frames from the innermost outwards and following aliases
class ident_resolver final {
    std::reference_wrapper<const scope_stack> scopes_;
    std::reference_wrapper<const ::machine> machine_;
    std::reference_wrapper<const generic_registry> generics_;
    std::reference_wrapper<const builtin_types> builtins_;

    // where the walk through the frames has got to
    struct walk {
        explicit walk(ident_path identifier)
            : id{std::move(identifier)},
              lea_path(id.path().size() - 1, operand{}) {

            assert(not id.path().empty());

            // ignore the elements after the first element:
            //  e.g.: lnks[1].pos.y
            //   ignore pos.y since those cannot have a lea
            //   add empty leas for those
            //   note: 'lea_path' will be reversed when complete so that
            //          'ident_path' elements have corresponding lea
            // note: -1 to exclude the first element
        }

        // the identifier, an alias rewrites it to what the alias stands for
        ident_path id;

        // note: 'lea' describes the effective address of an identifier's data
        //       'lea_path' associates address operands with identifier
        //       components; it is built while walking frames from the
        //       innermost outwards and reversed before use
        std::vector<operand> lea_path;

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
            throw compiler_exception{
                src_loc_tk,
                std::format("generic type '{}' is not a value, name an "
                            "instance first, e.g. 'type str = {}<...>'",
                            ident, ident)};
        }

        if (generics_.get().has_func(ident)) {
            throw compiler_exception{
                src_loc_tk,
                std::format("generic function '{}' needs type arguments, "
                            "e.g. '{}<...>'",
                            ident, ident)};
        }

        throw compiler_exception{
            src_loc_tk, std::format("cannot resolve identifier '{}'", ident)};
    }

  private:
    [[nodiscard]] auto resolve_constant_or_empty(const token& src_loc_tk,
                                                 const std::string_view ident,
                                                 const ident_path& id) const
        -> ident_info {

        // an integer constant
        if (const std::optional<int64_t> value{
                constant_parser::parse_constant(src_loc_tk, id.str()),
            };
            value) {

            return ident_info::make_const(
                ident, id.str(), builtins_.get().default_type(), *value);
        }

        // a boolean constant
        if (id.base() == "true") {
            return ident_info::make_const(ident, id.str(),
                                          builtins_.get().boolean(), 1);
        }

        if (id.base() == "false") {
            return ident_info::make_const(ident, id.str(),
                                          builtins_.get().boolean(), 0);
        }

        // is 'id' a constant?
        if (scopes_.get().find_const(id.str()) != nullptr) {
            return ident_info::make_const(
                ident, id.str(), builtins_.get().default_type(),
                scopes_.get().find_const(id.str())->value);
        }

        // not resolved, return empty info
        return ident_info::make_empty();
    }

    // what 'cur_frame' resolves the identifier to, nothing when the walk goes
    // on to the frame outside it
    [[nodiscard]] auto
    resolve_in(const frame& cur_frame, const token& src_loc_tk,
               const std::string_view ident, walk& step) const
        -> std::optional<ident_info> {

        // does this frame contain the variable?
        if (cur_frame.has_var(step.id.base())) {
            return resolve_var_in(cur_frame, src_loc_tk, ident, step);
        }

        // from the root frame of a function aliases are followed to the
        // actual variable referred to
        if (not cur_frame.is_func()) {
            return std::nullopt;
        }

        // this is an alias, continue resolving until it is a variable,
        // register or constant
        if (cur_frame.has_alias(step.id.base())) {
            return follow_alias(cur_frame.get_alias(step.id.base()), src_loc_tk,
                                ident, step);
        }

        // neither: a global, a constant or empty
        step.lea_path.emplace_back();

        return resolve_var_in(cur_frame, src_loc_tk, ident, step);
    }

    [[nodiscard]] auto
    resolve_in_frame(const frame& frm, const token& src_loc_tk,
                     const std::string_view ident, const ident_path& id,
                     std::vector<operand> lea_path) const -> ident_info {

        // try function scope
        if (frm.has_var(id.base())) {
            return resolve_var(src_loc_tk, ident, id,
                               frm.get_var_const_ref(id.base()),
                               std::move(lea_path));
        }

        // try global scope
        if (scopes_.get().front().has_var(id.base())) {
            return resolve_var(
                src_loc_tk, ident, id,
                scopes_.get().front().get_var_const_ref(id.base()), lea_path);
        }

        // try constant
        return resolve_constant_or_empty(src_loc_tk, ident, id);
    }

    // reviewed: 2026-09-09
    [[nodiscard]] auto resolve_or_empty(const token& src_loc_tk,
                                        const std::string_view ident) const
        -> ident_info {

        assert(not ident.empty());

        // get the base of the identifier: e.g. lnks[1].pos.y -> lnks
        // traverse the frames and resolve to a variable, register or constant

        walk step{ident_path{std::string{ident}}};

        for (const frame& cur_frame :
             scopes_.get().frames() | std::views::reverse) {

            if (std::optional<ident_info> found{
                    resolve_in(cur_frame, src_loc_tk, ident, step),
                };
                found) {

                return *std::move(found);
            }
        }

        return resolve_constant_or_empty(src_loc_tk, ident, step.id);
    }

    [[nodiscard]] auto resolve_var(const token& src_loc_tk,
                                   const std::string_view ident,
                                   const ident_path& id, const var_info& var,
                                   std::vector<operand> lea_path) const
        -> ident_info {

        if (var.value_register.is_register() and id.path().size() == 1) {
            return ident_info::make_register(ident, var.value_register);
        }

        ident_info info{
            ident_builder::make_var_ident_info(
                src_loc_tk, ident, id.path(), var,
                machine_.get().variables_base_register()),
        };

        info.read_only_why = var.read_only_why;

        lea_path.resize(id.path().size());
        // note: pad with empty for the remaining elements in the id path

        std::ranges::reverse(lea_path);
        // note: reverse it since it was constructed while traversing
        //       upwards in the frame stack but 'elem_path' and 'type_path'
        //       are ordered from the top down

        info.lea_path = std::move(lea_path);

        if (not info.type_ref().is_builtin()) {
            return info;
        }

        ident_builder::place_operand_from_lea(src_loc_tk, info);

        return info;
    }

    [[nodiscard]] auto resolve_var_in(const frame& cur_frame,
                                      const token& src_loc_tk,
                                      const std::string_view ident,
                                      walk& step) const -> ident_info {

        ident_info info{
            resolve_in_frame(cur_frame, src_loc_tk, ident, step.id,
                             std::move(step.lea_path)),
        };

        return ident_builder::as_element_if(step.is_element, std::move(info));
    }

    //
    // statics
    //

    // the register an alias stands for, else the walk goes on with the
    // identifier rewritten to the variable that the alias refers to
    [[nodiscard]] static auto
    follow_alias(const alias_info& alias, const token& src_loc_tk,
                 const std::string_view ident, walk& step)
        -> std::optional<ident_info> {

        const bool is_single{step.id.path().size() == 1};

        if (alias.register_operand.is_register() and is_single) {
            return ident_info::make_register(ident, alias.register_operand);
        }

        // the value of an argument keeps the type of its parameter
        if (is_single and alias.type_ptr != nullptr and
            alias.type_ptr->is_builtin()) {

            if (const std::optional<int64_t> value{
                    constant_parser::parse_constant(src_loc_tk, alias.to),
                };
                value) {

                return ident_info::make_const(ident, alias.to, *alias.type_ptr,
                                              *value, true);
            }
        }

        // a field path such as 'p.x' gets its array-ness from the field
        if (alias.is_element and is_single) {
            step.is_element = true;
        }

        step.lea_path.emplace_back(alias.lea);

        step.id =
            ident_builder::replace_alias_base(alias, step.id, step.lea_path);

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

    auto assert_const_not_defined(const token& src_loc_tk,
                                  const std::string_view name) const -> void {

        if (not scopes_.get().back().has_const(name)) {
            return;
        }

        const const_info& c{scopes_.get().back().get_const(name)};

        throw compiler_exception{
            src_loc_tk,
            std::format("constant '{}' already defined in this block at {}",
                        name, where(c.src_loc_tk))};
    }

    // 'generic_method' is the generic method that the function is an instance
    // of, empty if it is not
    auto assert_function_not_defined(
        const token& src_loc_tk, const std::string_view name,
        const std::string_view generic_method) const -> void {

        if (funcs_.get().has(name)) {
            const func_info& fn{funcs_.get().get(name)};

            // a built-in function has no source location
            if (fn.src_loc_tk.at_line() == 0) {
                throw compiler_exception{
                    src_loc_tk,
                    std::format("function '{}' is a built-in function", name)};
            }

            const bool is_generic{not generic_method.empty()};
            const bool is_existing_generic{not fn.generic_method.empty()};

            if (is_generic and is_existing_generic) {
                throw compiler_exception{
                    src_loc_tk,
                    std::format("generic method '{}' already defined at {}",
                                generic_method, where(fn.src_loc_tk))};
            }

            if (is_generic) {
                throw compiler_exception{
                    src_loc_tk,
                    std::format("generic method '{}' clashes with the method "
                                "'{}' defined at {}",
                                generic_method, name, where(fn.src_loc_tk))};
            }

            if (is_existing_generic) {
                throw compiler_exception{
                    src_loc_tk,
                    std::format("method '{}' clashes with the generic method "
                                "'{}' defined at {}",
                                name, fn.generic_method, where(fn.src_loc_tk))};
            }

            throw compiler_exception{
                src_loc_tk, std::format("function '{}' already defined at {}",
                                        name, where(fn.src_loc_tk))};
        }

        if (generics_.get().has_func(name)) {
            const generic_func_info& fn{generics_.get().get_func(name)};

            throw compiler_exception{
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

        throw compiler_exception{
            src_loc_tk,
            std::format("generic parameter '{}' hides the type '{}' defined "
                        "at {}, use another name",
                        name, name, where(types_.get().src_loc_tk_of(name)))};
    }

    auto assert_not_declared_in_scope(const token& src_loc_tk,
                                      const std::string_view name) const
        -> void {

        if (not scopes_.get().back().has_var(name)) {
            return;
        }

        const var_info& decl_var{scopes_.get().back().get_var_const_ref(name)};

        throw compiler_exception{
            src_loc_tk, std::format("variable '{}' already declared at {}",
                                    name, where(decl_var.src_loc_tk))};
    }

    // a generic type and a type share the namespace of types
    auto assert_type_not_defined(const token& src_loc_tk,
                                 const std::string_view name) const -> void {

        if (types_.get().has(name)) {
            throw compiler_exception{
                src_loc_tk,
                std::format("type '{}' already defined at {}", name,
                            where(types_.get().src_loc_tk_of(name)))};
        }

        if (generics_.get().has_type(name)) {
            throw compiler_exception{
                src_loc_tk,
                std::format("type '{}' already defined as a generic type at {}",
                            name,
                            where(generics_.get().get_type(name).src_loc_tk))};
        }
    }

  private:
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
            text += std::format("[{}]", var.array_len);
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
                                  name_info.is_array ? name_info.array_len : 1),
            name_info.operand);
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
    // above zero while the code is compiled only to be measured or checked
    size_t dry_run_depth_{};

  public:
    toc(::machine& backend, const std::string_view source,
        const size_t vars_capacity_bytes, const check_options& checks)
        : machine_{backend}, locations_{source},
          definitions_{scopes_, funcs_, types_, generics_, locations_},
          builtins_{backend}, resolver_{scopes_, backend, generics_, builtins_},
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
    [[nodiscard]] auto add_checked_noninline_call(std::string signature)
        -> bool {

        return funcs_.add_checked_call(std::move(signature));
    }

    auto add_const(const token& src_loc_tk, const size_t indent,
                   const std::string_view name, const int64_t value) -> void {

        definitions_.assert_const_not_defined(src_loc_tk, name);

        ::machine& x{machine()};

        x.comment(src_loc_tk, indent, "const {} = {}", name, value);

        scopes_.back().add_const(name, {
                                           .src_loc_tk{src_loc_tk},
                                           .value{value},
                                       });
    }

    auto add_dat(const statement* const stmt) -> void {
        if (scopes_.frames().size() != 1) {
            throw compiler_exception{stmt->tok(),
                                     "'dat' can only be added in global scope"};
        }

        if (scopes_.front().has_non_dat_var_been_added()) {
            throw compiler_exception{
                stmt->tok(), "'dat' can only be added before any 'var'"};
        }

        data_.add_dat(stmt, machine_.get().data_alignment());
    }

    auto add_func(const token& src_loc_tk, std::string name,
                  const type& return_type, const stmt_def_func* const func_def,
                  std::string generic_method) -> void {

        if (name == reserved_names::foo) {
            throw compiler_exception{src_loc_tk,
                                     "cannot name function 'foo' because it is "
                                     "a builtin iterator function"};
        }

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

    auto add_generic_type(const token& src_loc_tk, const std::string_view name,
                          const token& start_tk,
                          std::vector<generic_param> params) -> void {

        definitions_.assert_type_not_defined(src_loc_tk, name);

        generics_.add_type(src_loc_tk, name, start_tk, std::move(params));
    }

    // a body is emitted once per distinct instance, for a call that is
    // compiled for real: a dry run leaves no trace, the call is compiled again
    auto add_noninline_instance(const stmt_def_func& func,
                                std::vector<size_t> array_lengths) -> void {

        if (dry_run_depth_ != 0) {
            return;
        }

        funcs_.add_noninline_instance(func, std::move(array_lengths));
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

    auto add_var(const token& src_loc_tk, const size_t indent, var_info var,
                 const var_kind kind) -> void {

        definitions_.assert_not_declared_in_scope(src_loc_tk, var.name);

        // the value lives in its register for the whole scope
        const bool is_value_register{not var.value_register.is_empty()};

        // the variable is where the register points, it has no storage
        const bool is_pointer_register{not var.pointer_register.is_empty()};

        size_t allocated_size_bytes{};

        if (not is_value_register and not is_pointer_register) {
            allocated_size_bytes = storage_.allocate(src_loc_tk, var, kind);
        }

        scopes_.back().add_var(var, allocated_size_bytes, kind);

        var_comments_.comment(src_loc_tk, indent, var);
    }

    auto assert_generic_param_free(const token& src_loc_tk,
                                   const std::string_view name) const -> void {

        definitions_.assert_generic_param_free(src_loc_tk, name);
    }

    [[nodiscard]] auto bounds_check_options() const
        -> machine::bounds_check_options {

        return {
            .upper{checks_.bounds_upper},
            .lower{checks_.bounds_lower},
            .with_line{checks_.bounds_with_line},
        };
    }

    // runs 'compile' for its errors only: its output, string constants and
    // use of storage leave no trace
    auto check_only(const std::function_ref<void()> compile) -> void {
        std::ignore = measure_only(compile);
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

        if (has_const(tk.text())) {
            return get_const(tk.text());
        }

        return std::nullopt;
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

    // a function with a body of its own, its variables are placed from
    // 'storage_base_register' when not empty
    auto enter_noninline_func(const std::string_view name,
                              const std::string_view call_path,
                              const std::string_view storage_base_register)
        -> void {

        scopes_.enter_noninline_func(name, call_path, storage_base_register);
    }

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

    // the report is written, the frame usage is forgotten
    auto finish() -> void {
        assert(scopes_.is_empty());

        storage_.assert_released();

        scopes_.set_max_depth(0);
    }

    // empty outside of a function, e.g. in the initializer of a global variable
    // the scopes inside each other, the calls inlined in each other and their
    // blocks
    [[nodiscard]] auto frame_count() const -> size_t {
        return scopes_.frames().size();
    }

    [[nodiscard]] auto generics() -> generic_registry& { return generics_; }

    [[nodiscard]] auto generics() const -> const generic_registry& {
        return generics_;
    }

    [[nodiscard]] auto get_call_path() const -> std::string_view {
        if (not is_in_func()) {
            return {};
        }

        return scopes_.current_func_frame().call_path();
    }

    [[nodiscard]] auto get_const(const std::string_view name) const -> int64_t {
        const const_info* const c{scopes_.find_const(name)};

        assert(c != nullptr);

        return c->value;
    }

    [[nodiscard]] auto get_data() const
        -> const std::vector<const statement*>& {

        return data_.statements();
    }

    [[nodiscard]] auto get_func_defs() const
        -> std::span<const stmt_def_func* const> {

        return funcs_.defs();
    }

    [[nodiscard]] auto get_func_or_throw(const token& src_loc_tk,
                                         const std::string_view name) const
        -> const stmt_def_func& {

        return *get_func_info_or_throw(src_loc_tk, name).def;
    }

    [[nodiscard]] auto get_func_return_label() const -> std::string_view {
        return scopes_.current_func_frame().func_ret_label();
    }

    [[nodiscard]] auto
    get_func_return_type_or_throw(const token& src_loc_tk,
                                  const std::string_view name) const
        -> const type& {

        return *get_func_info_or_throw(src_loc_tk, name).type_ptr;
    }

    [[nodiscard]] auto get_lea_operand(const size_t indent,
                                       const statement& src,
                                       const ident_info& src_info,
                                       std::vector<operand>& lea_registers)
        -> operand {

        if (not src.is_indexed() and not src_info.has_lea() and
            not src_info.is_pointer) {

            return src_info.operand;
        }

        return src.compile_lea(*this, indent, src.tok(), lea_registers,
                               {
                                   .reg_count{},
                                   .lea_path{src_info.lea_path},
                                   .address_register{},
                               });
    }

    [[nodiscard]] auto get_looping_label_or_throw(const token& src_loc_tk) const
        -> std::string_view {

        return scopes_.looping_label_or_throw(src_loc_tk);
    }

    // no token names a missing 'main'
    [[nodiscard]] auto get_main_or_throw() const -> const stmt_def_func& {
        if (not funcs_.has(reserved_names::main)) {
            throw compiler_exception::file_level("function 'main' not found");
        }

        return *funcs_.get(reserved_names::main).def;
    }

    [[nodiscard]] auto get_string_constants() const
        -> std::span<const machine::string_constant> {

        return data_.constants();
    }

    [[nodiscard]] auto get_type_address() const -> const type& {
        return builtins_.address();
    }

    [[nodiscard]] auto get_type_bool() const -> const type& {
        return builtins_.boolean();
    }

    [[nodiscard]] auto get_type_default() const -> const type& {
        return builtins_.default_type();
    }

    [[nodiscard]] auto get_type_i8() const -> const type& {
        return builtins_.i8();
    }

    [[nodiscard]] auto get_type_or_throw(const token& src_loc_tk,
                                         const std::string_view name) const
        -> const type& {

        if (not types_.has(name)) {
            if (generics_.has_type(name)) {
                throw compiler_exception{
                    src_loc_tk,
                    std::format("generic type '{}' is not a type, name an "
                                "instance first, e.g. 'type str = {}<...>'",
                                name, name)};
            }

            throw compiler_exception{src_loc_tk,
                                     std::format("type '{}' not found", name)};
        }

        return types_.get(name);
    }

    [[nodiscard]] auto get_type_void() const -> const type& {
        return builtins_.void_type();
    }

    [[nodiscard]] auto has_const(const std::string_view name) const -> bool {
        return scopes_.find_const(name) != nullptr;
    }

    [[nodiscard]] auto has_lea(const statement& st) const -> bool {
        if (not st.is_identifier()) {
            return false;
        }

        std::string_view id_base{ident_path::root_of(st.identifier())};

        for (const frame& frm : scopes_.frames() | std::views::reverse) {
            if (frm.has_var(id_base)) {
                return frm.get_var_const_ref(id_base).is_pointer;
            }

            // aliases are declared at the root frame of a function
            if (not frm.is_func()) {
                continue;
            }

            if (not frm.has_alias(id_base)) {
                return false;
            }

            const alias_info& alias{frm.get_alias(id_base)};

            if (not alias.lea.is_empty()) {
                return true;
            }

            if (alias.register_operand.is_register()) {
                return false;
            }

            id_base = ident_path::root_of(alias.to);
        }

        // a constant is declared by no frame, e.g. in the initializer of a
        // global variable
        return false;
    }

    // whether the backend passes the address of the receiver of a method in a
    // register
    [[nodiscard]] auto has_slot_register() const -> bool {
        return not machine_.get().slot_register().empty();
    }

    [[nodiscard]] auto has_type(const std::string_view name) const -> bool {
        return types_.has(name);
    }

    [[nodiscard]] auto inlined_nesting(const std::string_view name) const
        -> size_t {

        return scopes_.inlined_nesting(name);
    }

    [[nodiscard]] auto is_alias_check() const -> bool { return checks_.alias; }

    [[nodiscard]] auto is_bounds_check_lower() const -> bool {
        return checks_.bounds_lower;
    }

    [[nodiscard]] auto is_bounds_check_upper() const -> bool {
        return checks_.bounds_upper;
    }

    [[nodiscard]] auto is_bounds_check_with_line() const -> bool {
        return checks_.bounds_with_line;
    }

    [[nodiscard]] auto is_frame_check() const -> bool { return checks_.frame; }

    [[nodiscard]] auto is_func(const std::string_view name) const -> bool {
        return funcs_.has(name) or generics_.has_func(name);
    }

    [[nodiscard]] auto is_func_builtin(const std::string_view name) const
        -> bool {

        return funcs_.get(name).def == nullptr;
    }

    // a local with the same name shadows it, so this may report a local
    [[nodiscard]] auto is_global_var(const std::string_view name) const
        -> bool {

        return scopes_.front().has_var(name);
    }

    // the initializer of a global variable is outside of any function
    [[nodiscard]] auto is_in_func() const -> bool {
        return std::ranges::any_of(
            scopes_.frames(),
            [](const frame& frm) -> bool { return frm.is_func(); });
    }

    [[nodiscard]] auto is_in_loop_block() const -> bool {
        return scopes_.is_in_loop_block();
    }

    [[nodiscard]] auto is_inlined_func() const -> bool {
        return scopes_.current_func_frame().is_inlined_func();
    }

    // 'int' and the names of the builtin integer types
    [[nodiscard]] auto is_integer_type_name(const std::string_view name) const
        -> bool {

        return builtins_.is_integer_name(name);
    }

    [[nodiscard]] auto is_var_or_alias(const std::string_view name) const
        -> bool {

        return scopes_.is_var_or_alias(name);
    }

    [[nodiscard]] auto machine() -> ::machine& { return machine_.get(); }

    [[nodiscard]] auto make_ident_info(const statement& st) const
        -> ident_info {

        // the name refers to the declared array, 'ps[1]' accesses one element
        const ident_info declared{
            resolver_.resolve(st.tok(), st.identifier()),
        };

        const bool is_element{st.is_array_element()};
        ident_info info{ident_builder::as_element_if(is_element, declared)};

        info.src_loc_tk = st.name_token();

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

    // like 'check_only', returns the size of the code, in the unit of the
    // target; an error is not caught here, it ends the compilation, so the
    // state saved below is not restored
    [[nodiscard]] auto measure_only(const std::function_ref<void()> compile)
        -> size_t {

        const size_t max_frame_count{scopes_.max_depth()};
        const size_t string_constant_count{data_.constant_count()};

        const storage_layout::dry_run_state saved{storage_.begin_dry_run()};

        ++dry_run_depth_;
        const size_t size{machine_.get().measure_code_size(compile)};
        --dry_run_depth_;

        storage_.end_dry_run(saved);
        scopes_.set_max_depth(max_frame_count);
        data_.resize_constants(string_constant_count);

        return size;
    }

    // a callee frame starts aligned for any variable it holds
    [[nodiscard]] auto next_frame_address() const -> operand {
        return storage_.next_frame_address(get_type_address());
    }

    // a copy, the list grows while the instances compile
    [[nodiscard]] auto noninline_instance_at(const size_t index) const
        -> noninline_instance {

        return funcs_.noninline_instance_at(index);
    }

    [[nodiscard]] auto noninline_instance_count() const -> size_t {
        return funcs_.noninline_instance_count();
    }

    [[nodiscard]] auto peak_frame_size_bytes() const -> size_t {
        return storage_.peak_frame_size_bytes();
    }

    auto reset_usage() -> void {
        assert(scopes_.is_empty());

        storage_.reset_usage();
        scopes_.set_max_depth(0);
        funcs_.clear_noninline_instances();
    }

    auto set_builtin_types(const type& t_i64, const type& t_i32,
                           const type& t_i16, const type& t_i8) -> void {

        builtins_.set_integers(t_i64, t_i32, t_i16, t_i8);
    }

    auto set_type_bool(const type& tpe) -> void { builtins_.set_boolean(tpe); }

    auto set_type_void(const type& tpe) -> void { builtins_.set_void(tpe); }

    [[nodiscard]] auto source() const -> std::string_view {
        return locations_.source();
    }

    [[nodiscard]] auto
    source_location_for_use_in_label(const token& src_loc_tk) const
        -> std::string {

        return locations_.for_label(src_loc_tk);
    }

    [[nodiscard]] auto types() -> type_table& { return types_; }

    [[nodiscard]] auto usage() const -> usage_statistics {
        return {
            .max_frame_count{scopes_.max_depth()},
            .max_vars_size_bytes{storage_.max_vars_size_bytes()},
            .dat_size_bytes{data_.total_size_bytes()},
            .dat_var_padding_bytes{data_.entry_gap()},
            .uninstantiated_generics{generics_.uninstantiated_func_names()},
        };
    }

    //
    // statics
    //

    static auto assert_not_whole_array(const statement& st,
                                       const ident_info& info) -> void {

        if (not info.is_array) {
            return;
        }

        throw compiler_exception{
            st.tok(),
            std::format("array '{}' must be indexed", st.identifier())};
    }

    // a name is an identifier, 'self' is declared only by the compiler: the
    // receiver of a method and the value built by a constructor
    static auto assert_valid_name(const token& name_tk) -> void {
        if (name_tk.text().empty()) {
            throw compiler_exception{name_tk, "expected a name"};
        }

        if (not is_identifier(name_tk)) {
            throw compiler_exception{
                name_tk,
                std::format("'{}' is not a valid name, a name starts with a "
                            "letter or '_' and has only letters, digits and "
                            "'_'",
                            name_tk.text())};
        }

        if (name_tk.is_text(reserved_names::self)) {
            throw compiler_exception{name_tk, "'self' is reserved"};
        }

        // the value of a name that is a boolean value would be the function
        if (name_tk.is_text(reserved_names::true_value) or
            name_tk.is_text(reserved_names::false_value)) {

            throw compiler_exception{
                name_tk,
                std::format("'{}' is a boolean value", name_tk.text())};
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

    [[nodiscard]] static auto make_ident_info_from_register(const operand& reg)
        -> ident_info {

        return ident_info::make_register(reg.base_register(), reg);
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
            throw compiler_exception{
                src_loc_tk, std::format("function '{}' not found", name)};
        }

        return funcs_.get(name);
    }
};

// type names that stand for the arguments of a generic definition while an
// instance is parsed, they are unbound when the scope ends
class type_alias_scope final {
    toc& tc_;
    std::vector<std::string> names_;

  public:
    explicit type_alias_scope(toc& tc) : tc_{tc} {}

    type_alias_scope(const type_alias_scope&) = delete;
    type_alias_scope(type_alias_scope&&) = delete;
    auto operator=(const type_alias_scope&) -> type_alias_scope& = delete;
    auto operator=(type_alias_scope&&) -> type_alias_scope& = delete;

    ~type_alias_scope() {
        for (const std::string& name : names_) {
            tc_.types().unbind(name);
        }
    }

    auto bind(const token& src_loc_tk, const std::string_view name,
              const type& tpe) -> void {

        tc_.assert_generic_param_free(src_loc_tk, name);
        tc_.types().bind(src_loc_tk, name, tpe);
        names_.emplace_back(name);
    }

    // the generic type and the type arguments of the instance
    auto bind_instance(const token& src_loc_tk,
                       const generic_type_instance& instance) -> void {

        bind(src_loc_tk, instance.generic_name, *instance.type_ptr);

        for (const generic_binding& binding : instance.bindings) {
            if (binding.type_ptr != nullptr) {
                bind(src_loc_tk, binding.name, *binding.type_ptr);
            }
        }
    }
};
