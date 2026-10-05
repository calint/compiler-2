#pragma once
// reviewed: 2025-09-29

#include <algorithm>
#include <charconv>
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

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "generics.hpp"
#include "lut.hpp"
#include "machine.hpp"
#include "statement.hpp"
#include "type.hpp"

class stmt_def_func;

struct func_info {
    token src_loc_tk;           // token for position in the source
    const stmt_def_func* def{}; // null if built-in function
    const type* type_ptr{};     // return type or void
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

class frame final {
  public:
    enum class frame_type : uint8_t { FUNC, BLOCK, LOOP, FOO };

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

    frame_type type_{frame_type::FUNC}; // frame type
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
                 const bool is_dat = false) -> void {

        allocated_stack_size_bytes_ =
            sum_storage_size(allocated_stack_size_bytes_, allocated_size_bytes);

        vars_.put(var.name, var);

        if (not is_dat) {
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
        return type_ == frame_type::BLOCK;
    }

    [[nodiscard]] auto is_foo() const -> bool {
        return type_ == frame_type::FOO;
    }

    [[nodiscard]] auto is_func() const -> bool {
        return type_ == frame_type::FUNC;
    }

    [[nodiscard]] auto is_inlined_func() const -> bool {
        assert(is_func());

        return is_inlined_;
    }

    [[nodiscard]] auto is_loop() const -> bool {
        return type_ == frame_type::LOOP;
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
    static auto place_operand_from_lea(const token& src_loc_tk, ident_info& ii)
        -> void {

        // find the first element from the top that has a 'lea' and get
        // accessor relative to that
        operand lea;
        size_t lea_index{ii.elem_path.size()};
        while (lea_index--) {
            if (not ii.lea_path.at(lea_index).is_empty()) {
                lea = ii.lea_path.at(lea_index);
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
            std::span{ii.elem_path}.subspan(lea_index),
        };

        // navigate to referred element and get offset
        const size_t offset{
            ii.type_path.at(lea_index)->field_offset(src_loc_tk,
                                                     elem_path_from_lea),
        };

        ii.operand = operand::mem(lea, ii.type_ref());
        if (offset != 0) {
            ii.operand.increment_offset(address_offset(offset));
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
        } else if (str.starts_with("0b") or str.starts_with("0B")) {
            base = base_binary;
            digits.remove_prefix(2);
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
             const stmt_def_func* const func_def) -> void {

        funcs_.put(std::move(name), {
                                        .src_loc_tk{src_loc_tk},
                                        .def{func_def},
                                        .type_ptr{&return_type},
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

class toc final {
    // where a variable is placed, 'storage_frame' is null at the variables base
    struct storage_location {
        frame* storage_frame;
        size_t base_offset;
    };

    std::reference_wrapper<::machine> machine_;
    std::string_view source_;
    std::vector<frame> frames_;
    data_table data_;
    function_table funcs_;
    generic_registry generics_;
    type_table types_;
    const type* type_void_{};
    const type* type_bool_{};
    size_t usage_max_frame_count_{};
    size_t usage_max_vars_size_bytes_{};
    size_t vars_size_bytes_{};
    size_t vars_capacity_bytes_;
    bool vars_entry_gap_applied_{};
    check_options checks_;
    // the locals of a dry run live in the callee's own frame
    bool capacity_unchecked_{};

  public:
    toc(::machine& backend, const std::string_view source,
        const size_t vars_capacity_bytes, const bool bounds_check_upper,
        const bool bounds_check_lower, const bool bounds_check_with_line,
        const bool frame_check = {}, const bool alias_check = {})
        : machine_{backend}, source_{source},
          vars_capacity_bytes_{vars_capacity_bytes},
          checks_{
              .bounds_upper{bounds_check_upper},
              .bounds_lower{bounds_check_lower},
              .bounds_with_line{bounds_check_with_line},
              .frame{frame_check},
              .alias{alias_check},
          } {}

    auto add_alias(const alias_info& ai) -> void {
        frames_.back().add_alias(ai);
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
                   const std::string_view name, const int64_t value) {

        if (has_const_in_current_block(name)) {
            const const_info& c{frames_.back().get_const(name)};

            throw compiler_exception{
                src_loc_tk,
                std::format("constant '{}' already defined in this block at {}",
                            name, source_location_hr(c.src_loc_tk))};
        }

        ::machine& x{machine()};

        x.comment(src_loc_tk, indent, "const {} = {}", name, value);
        frames_.back().add_const(name, {
                                           .src_loc_tk{src_loc_tk},
                                           .value{value},
                                       });
    }

    auto add_dat(const statement* const stmt) -> void {
        if (frames_.size() != 1) {
            throw compiler_exception{stmt->tok(),
                                     "'dat' can only be added in global scope"};
        }
        if (frames_.front().has_non_dat_var_been_added()) {
            throw compiler_exception{
                stmt->tok(), "'dat' can only be added before any 'var'"};
        }

        data_.add_dat(stmt, machine_.get().data_alignment());
    }

    auto add_func(const token& src_loc_tk, std::string name,
                  const type& return_type, const stmt_def_func* const func_def)
        -> void {

        if (name == "foo") {
            throw compiler_exception{src_loc_tk,
                                     "cannot name function 'foo' because it is "
                                     "a builtin iterator function"};
        }

        assert_function_not_defined(src_loc_tk, name);

        funcs_.add(src_loc_tk, std::move(name), return_type, func_def);
    }

    // keeps the instance alive, 'funcs_' refers to it
    auto add_func_instance(std::shared_ptr<const stmt_def_func> instance)
        -> void {

        funcs_.add_instance(std::move(instance));
    }

    auto add_generic_func(
        const token& src_loc_tk, std::string name, const token& func_tk,
        const token& start_tk, std::string report_name,
        std::vector<std::string> param_names,
        std::vector<generic_deduction> deductions,
        std::optional<generic_type_instance> receiver_instance = {}) -> void {

        assert_function_not_defined(src_loc_tk, name);

        generics_.add_func(src_loc_tk, std::move(name), func_tk, start_tk,
                           std::move(report_name), std::move(param_names),
                           std::move(deductions), std::move(receiver_instance));
    }

    auto add_generic_type(const token& src_loc_tk, const std::string_view name,
                          const token& start_tk,
                          std::vector<generic_param> params) -> void {

        assert_type_not_defined(src_loc_tk, name);

        generics_.add_type(src_loc_tk, name, start_tk, std::move(params));
    }

    // a body is emitted once per distinct instance
    auto add_noninline_instance(const stmt_def_func& func,
                                std::vector<size_t> array_lengths) -> void {

        funcs_.add_noninline_instance(func, std::move(array_lengths));
    }

    // identical strings share the label of the first one compiled
    [[nodiscard]] auto add_string_constant(const token& string_tk)
        -> std::string {

        return add_read_only_constant("str", string_tk,
                                      string_tk.string_text());
    }

    auto add_type(const token& src_loc_tk, const type& tpe) -> void {
        assert_type_not_defined(src_loc_tk, tpe.name());

        types_.add(src_loc_tk, tpe);
    }

    auto add_var(const token& src_loc_tk, const size_t indent, var_info var,
                 const bool is_dat) -> void {

        assert_not_declared_in_scope(src_loc_tk, var.name);

        // the value lives in its register for the whole scope
        if (not var.value_register.is_empty()) {
            frames_.back().add_var(var, 0, is_dat);
            comment_var(src_loc_tk, indent, var);

            return;
        }

        const size_t var_size_bytes{var_storage_size_bytes(src_loc_tk, var)};
        const size_t var_alignment{var_storage_alignment(var)};

        if (not is_dat) {
            apply_entry_gap(src_loc_tk);
        }

        // offsets are relative to the variables base or to the nearest frame
        // with its own storage base, both are aligned
        const storage_location location{find_storage_location(is_dat)};

        frame* const storage_frame{location.storage_frame};
        const size_t base_offset{location.base_offset};

        const size_t padding_bytes{
            align_storage_size(base_offset, var_alignment) - base_offset,
        };

        const size_t allocated_size_bytes{
            add_storage_size(src_loc_tk, padding_bytes, var_size_bytes),
        };

        if (not is_dat) {
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
        frames_.back().add_var(var, allocated_size_bytes, is_dat);

        // stats
        if (not is_dat) {
            usage_max_vars_size_bytes_ =
                std::max(used_vars_size_bytes(), usage_max_vars_size_bytes_);
        }

        comment_var(src_loc_tk, indent, var);
    }

    // a generic parameter names its argument, so it cannot be the name of a
    // type
    auto assert_generic_param_free(const token& src_loc_tk,
                                   const std::string_view name) const -> void {

        if (not types_.has(name)) {
            return;
        }

        throw compiler_exception{
            src_loc_tk,
            std::format("generic parameter '{}' hides the type '{}' defined "
                        "at {}, use another name",
                        name, name,
                        source_location_hr(types_.src_loc_tk_of(name)))};
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
        frame* const storage_frame{find_storage_location(false).storage_frame};

        const size_t max_frame_count{usage_max_frame_count_};
        const size_t max_vars_size_bytes{usage_max_vars_size_bytes_};
        const size_t string_constant_count{data_.constant_count()};
        const bool was_capacity_unchecked{capacity_unchecked_};
        const size_t peak_storage_size_bytes{
            storage_frame ? storage_frame->peak_storage_size_bytes() : 0,
        };

        capacity_unchecked_ = true;

        machine_.get().discard_output(compile);

        capacity_unchecked_ = was_capacity_unchecked;
        usage_max_frame_count_ = max_frame_count;
        usage_max_vars_size_bytes_ = max_vars_size_bytes;
        data_.resize_constants(string_constant_count);

        if (storage_frame) {
            storage_frame->restore_peak_storage_size_bytes(
                peak_storage_size_bytes);
        }
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

    [[nodiscard]] auto create_unique_label(const token& src_loc_tk,
                                           const std::string_view prefix) const
        -> std::string {

        const std::string_view call_path{get_call_path()};
        const std::string src_loc{source_location_for_use_in_label(src_loc_tk)};
        const std::string lbl{
            std::format("{}.{}{}", prefix, src_loc,
                        (call_path.empty() ? std::string{}
                                           : std::format(".{}", call_path))),
        };

        return lbl;
    }

    auto enter_block() -> void {
        frames_.emplace_back("", frame::frame_type::BLOCK);
        refresh_usage();
    }

    auto enter_foo(const std::string_view name) -> void {
        frames_.emplace_back(name, frame::frame_type::FOO);
        refresh_usage();
    }

    auto enter_func(const std::string_view name,
                    const std::string_view call_path = {},
                    const std::string_view return_jmp_label = {},
                    const bool is_inlined = true,
                    const std::string_view storage_base_register = {}) -> void {

        assert(storage_base_register.empty() or not is_inlined);

        frames_.emplace_back(
            name, frame::frame_type::FUNC, std::string{call_path},
            std::string{return_jmp_label}, is_inlined, storage_base_register);

        refresh_usage();
    }

    auto enter_loop(const std::string_view name) -> void {
        frames_.emplace_back(name, frame::frame_type::LOOP);
        refresh_usage();
    }

    auto exit_block() -> void {
        assert(frames_.back().is_block());

        pop_frame();
    }

    auto exit_foo([[maybe_unused]] const std::string_view name) -> void {
        assert(frames_.back().is_foo() and frames_.back().is_name(name));

        pop_frame();
    }

    auto exit_func([[maybe_unused]] const std::string_view name) -> void {
        assert(frames_.back().is_func() and frames_.back().is_name(name));

        pop_frame();
    }

    auto exit_loop([[maybe_unused]] const std::string_view name) -> void {
        assert(frames_.back().is_loop() and frames_.back().is_name(name));

        pop_frame();
    }

    // the report after the code, written as comments through the machine
    auto finish() -> void {
        assert(frames_.empty());
        assert(vars_size_bytes_ == 0);

        usage_max_frame_count_ = 0;
    }

    [[nodiscard]] auto generics() -> generic_registry& { return generics_; }

    [[nodiscard]] auto generics() const -> const generic_registry& {
        return generics_;
    }

    [[nodiscard]] auto get_call_path() const -> std::string_view {
        return current_func_frame().call_path();
    }

    [[nodiscard]] auto get_const(const std::string_view name) const -> int64_t {
        const const_info* const c{find_const(name)};

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
        return current_func_frame().func_ret_label();
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

        return src.compile_lea(*this, indent, src.tok(), lea_registers, {},
                               src_info.lea_path, {});
    }

    [[nodiscard]] auto get_looping_label_or_throw(const token& src_loc_tk) const
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

    // no token names a missing 'main'
    [[nodiscard]] auto get_main_or_throw() const -> const stmt_def_func& {
        if (not funcs_.has("main")) {
            throw compiler_exception::file_level("function 'main' not found");
        }

        return *funcs_.get("main").def;
    }

    [[nodiscard]] auto get_string_constants() const
        -> std::span<const machine::string_constant> {

        return data_.constants();
    }

    [[nodiscard]] auto get_type_address() const -> const type& {
        return get_type_or_throw(
            token{}, machine_.get().address_size_bytes() == 4 ? "i32" : "i64");
    }

    [[nodiscard]] auto get_type_bool() const -> const type& {
        return *type_bool_;
    }

    [[nodiscard]] auto get_type_default() const -> const type& {
        return machine_.get().default_type();
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
        return *type_void_;
    }

    [[nodiscard]] auto has_const(const std::string_view name) const -> bool {
        return find_const(name) != nullptr;
    }

    [[nodiscard]] auto
    has_const_in_current_block(const std::string_view name) const -> bool {

        return frames_.back().has_const(name);
    }

    [[nodiscard]] auto has_lea(const statement& st) const -> bool {
        if (not st.is_identifier()) {
            return false;
        }

        std::string_view id_base{ident_path::root_of(st.identifier())};

        for (const frame& frm : frames_ | std::views::reverse) {
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

        std::unreachable();
    }

    [[nodiscard]] auto has_type(const std::string_view name) const -> bool {
        return types_.has(name);
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

        return frames_.front().has_var(name);
    }

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

    [[nodiscard]] auto is_inlined_func() const -> bool {
        return current_func_frame().is_inlined_func();
    }

    // same scoping as 'make_ident_info', e.g. a variable 'limits' hides the
    // type 'limits'
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

    [[nodiscard]] auto machine() -> ::machine& { return machine_.get(); }

    [[nodiscard]] auto make_ident_info(const statement& st) const
        -> ident_info {

        // the name refers to the declared array, 'ps[1]' accesses one element
        const ident_info declared{
            make_ident_info_or_throw(st.tok(), st.identifier()),
        };
        const bool is_element{st.is_array_element()};
        ident_info info{ident_builder::as_element_if(is_element, declared)};

        info.src_loc_tk = st.name_token();

        return info;
    }

    [[nodiscard]] auto make_ident_info(const token& src_loc_tk,
                                       const std::string_view ident) const
        -> ident_info {

        return make_ident_info_or_throw(src_loc_tk, ident);
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

        frames_.back().make_var_read_only(name, cause);
    }

    // a callee frame starts aligned for any variable it holds
    [[nodiscard]] auto next_frame_address() const -> operand {
        const size_t frame_alignment{machine_.get().address_size_bytes()};

        size_t local_size_bytes{};
        for (const frame& frm : frames_ | std::views::reverse) {
            local_size_bytes = sum_storage_size(
                local_size_bytes, frm.allocated_stack_size_bytes());
            if (not frm.storage_base_register().empty()) {
                return operand::mem(frm.storage_base_register(), {}, 1,
                                    address_offset(align_storage_size(
                                        local_size_bytes, frame_alignment)),
                                    get_type_address());
            }
        }

        const size_t root_size_bytes{
            sum_storage_size(vars_size_bytes_,
                             vars_entry_gap_applied_ ? 0 : data_.entry_gap()),
        };

        const size_t aligned_size_bytes{
            align_storage_size(root_size_bytes, frame_alignment),
        };

        const int64_t offset_from_dat{address_offset(aligned_size_bytes)};

        const int64_t offset_from_base{
            add_address_offset(offset_from_dat, -variables_base_shift_bytes()),
        };

        return operand::mem(machine_.get().variables_base_register(), {}, 1,
                            offset_from_base, get_type_address());
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
        for (const frame& frm : frames_ | std::views::reverse) {
            if (not frm.storage_base_register().empty()) {
                return frm.peak_storage_size_bytes();
            }
        }

        std::unreachable();
    }

    auto reset_usage() -> void {
        assert(frames_.empty());
        assert(vars_size_bytes_ == 0);

        usage_max_frame_count_ = 0;
        usage_max_vars_size_bytes_ = 0;
        funcs_.clear_noninline_instances();
    }

    auto set_type_bool(const type& tpe) -> void { type_bool_ = &tpe; }

    auto set_type_void(const type& tpe) -> void { type_void_ = &tpe; }

    [[nodiscard]] auto source() const -> std::string_view { return source_; }

    [[nodiscard]] auto
    source_location_for_use_in_label(const token& src_loc_tk) const
        -> std::string {

        const auto [line, col]{
            line_and_col_num_for_char_index(src_loc_tk.at_line(),
                                            src_loc_tk.start_index(), source_),
        };

        return std::format("{}.{}", line, col);
    }

    // human-readable source location
    [[nodiscard]] auto source_location_hr(const token& src_loc_tk) const
        -> std::string {

        const auto [line, col]{
            line_and_col_num_for_char_index(src_loc_tk.at_line(),
                                            src_loc_tk.start_index(), source_),
        };

        return std::format("{}:{}", line, col);
    }

    [[nodiscard]] auto types() -> type_table& { return types_; }

    [[nodiscard]] auto usage() const -> usage_statistics {
        return {
            .max_frame_count{usage_max_frame_count_},
            .max_vars_size_bytes{usage_max_vars_size_bytes_},
            .dat_size_bytes{data_.total_size_bytes()},
            .dat_var_padding_bytes{data_.entry_gap()},
            .uninstantiated_generics{generics_.uninstantiated_func_names()},
        };
    }

    //
    // statics
    //

    // 'self' is declared only by the compiler: the receiver of a method and the
    // value built by a constructor
    static auto assert_name_not_reserved(const token& name_tk) -> void {
        if (not name_tk.is_text("self")) {
            return;
        }

        throw compiler_exception{name_tk, "'self' is reserved"};
    }

    static auto assert_not_whole_array(const statement& st,
                                       const ident_info& info) -> void {

        if (not info.is_array) {
            return;
        }

        throw compiler_exception{
            st.tok(),
            std::format("array '{}' must be indexed", st.identifier())};
    }

    [[nodiscard]] static auto make_ident_info_from_register(const operand& reg)
        -> ident_info {

        return ident_info::make_register(reg.base_register(), reg);
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

    // a local starts after the storage in use of the nearest frame with its own
    // storage base and of the frames inside it, other variables and dats start
    // at the variables base
    // the first variable after the dats starts past the entry gap
    auto apply_entry_gap(const token& src_loc_tk) -> void {
        if (vars_entry_gap_applied_) {
            return;
        }

        frames_.front().set_padding_between_dats_and_vars(data_.entry_gap());
        vars_size_bytes_ =
            add_storage_size(src_loc_tk, vars_size_bytes_, data_.entry_gap());
        vars_entry_gap_applied_ = true;
    }

    auto assert_function_not_defined(const token& src_loc_tk,
                                     const std::string_view name) const
        -> void {

        if (funcs_.has(name)) {
            const func_info& fn{funcs_.get(name)};

            // a built-in function has no source location
            if (fn.src_loc_tk.at_line() == 0) {
                throw compiler_exception{
                    src_loc_tk,
                    std::format("function '{}' is a built-in function", name)};
            }

            throw compiler_exception{
                src_loc_tk,
                std::format("function '{}' already defined at {}", name,
                            source_location_hr(fn.src_loc_tk))};
        }

        if (generics_.has_func(name)) {
            const generic_func_info& fn{generics_.get_func(name)};
            throw compiler_exception{
                src_loc_tk,
                std::format("function '{}' already defined at {}", name,
                            source_location_hr(fn.src_loc_tk))};
        }
    }

    auto assert_not_declared_in_scope(const token& src_loc_tk,
                                      const std::string_view name) const
        -> void {

        if (not frames_.back().has_var(name)) {
            return;
        }

        const var_info& decl_var{frames_.back().get_var_const_ref(name)};

        throw compiler_exception{
            src_loc_tk,
            std::format("variable '{}' already declared at {}", name,
                        source_location_hr(decl_var.src_loc_tk))};
    }

    // a generic type and a type share the namespace of types
    auto assert_type_not_defined(const token& src_loc_tk,
                                 const std::string_view name) const -> void {

        if (types_.has(name)) {
            throw compiler_exception{
                src_loc_tk,
                std::format("type '{}' already defined at {}", name,
                            source_location_hr(types_.src_loc_tk_of(name)))};
        }

        if (generics_.has_type(name)) {
            throw compiler_exception{
                src_loc_tk,
                std::format(
                    "type '{}' already defined as a generic type at {}", name,
                    source_location_hr(generics_.get_type(name).src_loc_tk))};
        }
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

    // the resolved name shows where the variable is stored
    auto comment_var(const token& src_loc_tk, const size_t indent,
                     const var_info& var) -> void {

        const ident_info& name_info{make_ident_info(src_loc_tk, var.name)};

        ::machine& x{machine()};

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

    // blocks and loops belong to the function frame below them
    [[nodiscard]] auto current_func_frame() const -> const frame& {
        for (const frame& frm : frames_ | std::views::reverse) {
            if (frm.is_func()) {
                return frm;
            }
        }

        std::unreachable();
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

    [[nodiscard]] auto find_storage_location(const bool is_dat)
        -> storage_location {

        if (is_dat) {
            return {.storage_frame{}, .base_offset{vars_size_bytes_}};
        }

        size_t local_size_bytes{};

        for (frame& frm : frames_ | std::views::reverse) {
            local_size_bytes = sum_storage_size(
                local_size_bytes, frm.allocated_stack_size_bytes());

            if (not frm.storage_base_register().empty()) {
                return {.storage_frame{&frm}, .base_offset{local_size_bytes}};
            }
        }

        return {.storage_frame{}, .base_offset{vars_size_bytes_}};
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

    [[nodiscard]] auto
    make_ident_info_const_or_empty(const token& src_loc_tk,
                                   const std::string_view ident,
                                   const ident_path& id) const -> ident_info {

        // is 'id' an integer?
        if (const std::optional<int64_t> value{
                constant_parser::parse_constant(src_loc_tk, id.str()),
            };
            value) {

            return ident_info::make_const(ident, id.str(), get_type_default(),
                                          *value);
        }

        // is it a boolean constant?
        if (id.base() == "true") {
            return ident_info::make_const(ident, id.str(), get_type_bool(), 1);
        }

        if (id.base() == "false") {
            return ident_info::make_const(ident, id.str(), get_type_bool(), 0);
        }

        // is 'id' a constant?
        if (has_const(id.str())) {
            return ident_info::make_const(ident, id.str(), get_type_default(),
                                          get_const(id.str()));
        }

        // not resolved, return empty info
        return ident_info::make_empty();
    }

    [[nodiscard]] auto make_ident_info_from_frame(
        const frame& frm, const token& src_loc_tk, const std::string_view ident,
        const ident_path& id, std::vector<operand> lea_path) const
        -> ident_info {

        // try function scope
        if (frm.has_var(id.base())) {
            return make_ident_info_from_var_info(
                src_loc_tk, ident, id, frm.get_var_const_ref(id.base()),
                std::move(lea_path));
        }

        // try global scope
        if (frames_.front().has_var(id.base())) {
            return make_ident_info_from_var_info(
                src_loc_tk, ident, id,
                frames_.front().get_var_const_ref(id.base()), lea_path);
        }

        // try constant
        return make_ident_info_const_or_empty(src_loc_tk, ident, id);
    }

    [[nodiscard]] auto make_ident_info_from_var_info(
        const token& src_loc_tk, const std::string_view ident,
        const ident_path& id, const var_info& var,
        std::vector<operand> lea_path) const -> ident_info {

        if (var.value_register.is_register() and id.path().size() == 1) {
            return ident_info::make_register(ident, var.value_register);
        }

        ident_info ii{
            var.type_ptr->accessor(src_loc_tk, ident, id.path(), var,
                                   machine_.get().variables_base_register()),
        };

        ii.read_only_why = var.read_only_why;

        lea_path.resize(id.path().size());
        // note: pad with empty for the remaining elements in the id path

        std::ranges::reverse(lea_path);
        // note: reverse it since it was constructed while traversing
        //       upwards in the frame stack but 'elem_path' and 'type_path'
        //       are ordered from the top down

        ii.lea_path = std::move(lea_path);

        if (not ii.type_ref().is_builtin()) {
            return ii;
        }

        ident_builder::place_operand_from_lea(src_loc_tk, ii);

        return ii;
    }

    // reviewed: 2026-09-09
    [[nodiscard]] auto
    make_ident_info_or_empty(const token& src_loc_tk,
                             const std::string_view ident) const -> ident_info {

        assert(not ident.empty());

        ident_path id{std::string{ident}};

        assert(not id.path().empty());

        // get the base of the identifier: e.g. lnks[1].pos.y -> lnks
        // traverse the frames and resolve to a variable, register or constant

        std::vector<operand> lea_path;

        // note: 'lea' describes the effective address of an identifier's data
        //       'lea_path' associates address operands with identifier
        //       components; it is built while walking frames from the
        //       innermost outwards and reversed before use

        // ignore the elements after the first element:
        //  e.g.: lnks[1].pos.y
        //   ignore pos.y since those cannot have a lea
        //   add empty leas for those
        //   note: 'lea_path' will be reversed when complete so that
        //          'ident_path' elements have corresponding lea

        lea_path.insert(lea_path.end(), id.path().size() - 1, operand{});
        // note: -1 to exclude the first element

        // an alias of an element names the element, not the array holding it
        bool is_element{};

        for (const frame& cur_frame : frames_ | std::views::reverse) {

            // does this frame contain the variable?
            if (cur_frame.has_var(id.base())) {
                ident_info info{
                    make_ident_info_from_frame(cur_frame, src_loc_tk, ident, id,
                                               std::move(lea_path)),
                };

                return ident_builder::as_element_if(is_element,
                                                    std::move(info));
            }

            // from the root frame of a function aliases are followed to the
            // actual variable referred to
            if (not cur_frame.is_func()) {
                continue;
            }

            if (not cur_frame.has_alias(id.base())) {
                lea_path.emplace_back();

                ident_info info{
                    make_ident_info_from_frame(cur_frame, src_loc_tk, ident, id,
                                               std::move(lea_path)),
                };

                return ident_builder::as_element_if(is_element,
                                                    std::move(info));
            }

            // this is an alias, continue resolving until it is a variable,
            // register or constant

            const alias_info& alias{cur_frame.get_alias(id.base())};

            if (alias.register_operand.is_register() and
                id.path().size() == 1) {

                return ident_info::make_register(ident, alias.register_operand);
            }

            // a field path such as 'p.x' gets its array-ness from the field
            if (alias.is_element and id.path().size() == 1) {
                is_element = true;
            }

            lea_path.emplace_back(alias.lea);

            id = ident_builder::replace_alias_base(alias, id, lea_path);
        }

        return make_ident_info_const_or_empty(src_loc_tk, ident, id);
    }

    // helper: call make_ident_info_or_empty and throw if unresolved
    [[nodiscard]] auto
    make_ident_info_or_throw(const token& src_loc_tk,
                             const std::string_view ident) const -> ident_info {

        const ident_info id_info{make_ident_info_or_empty(src_loc_tk, ident)};

        if (not id_info.is_empty()) {
            return id_info;
        }

        if (generics_.has_type(ident)) {
            throw compiler_exception{
                src_loc_tk,
                std::format("generic type '{}' is not a value, name an "
                            "instance first, e.g. 'type str = {}<...>'",
                            ident, ident)};
        }

        if (generics_.has_func(ident)) {
            throw compiler_exception{
                src_loc_tk,
                std::format("generic function '{}' needs type arguments, "
                            "e.g. '{}<...>'",
                            ident, ident)};
        }

        throw compiler_exception{
            src_loc_tk, std::format("cannot resolve identifier '{}'", ident)};
    }

    // the root frame applies the dat var gap again when it is entered anew
    auto pop_frame() -> void {
        vars_size_bytes_ -= frames_.back().allocated_stack_size_bytes();
        frames_.pop_back();
        if (not frames_.empty()) {
            return;
        }

        assert(vars_size_bytes_ == 0);

        vars_entry_gap_applied_ = false;
    }

    auto refresh_usage() -> void {
        usage_max_frame_count_ =
            std::max(frames_.size(), usage_max_frame_count_);
    }

    // bytes of variables, without the dats and the gap after them
    [[nodiscard]] auto used_vars_size_bytes() const -> size_t {
        return vars_size_bytes_ - data_.total_size_bytes() - data_.entry_gap();
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
            sum_storage_size(data_.total_size_bytes(), data_.entry_gap()),
        };

        return address_offset(sum_storage_size(dats_bytes, *past_vars_bytes));
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
