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
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "compiler_exception.hpp"
#include "decouple.hpp"
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

struct alias_info {
    std::string from;
    std::string to;
    operand lea;
    const type* type_ptr{};
    operand register_operand;
    // e.g. 'i' -> 'arr[ix]' names one element, not the array 'arr'
    bool is_element{};

    // the parameter was declared 'const', writes through it are rejected
    bool is_read_only{};

    //
    // statics
    //

    [[nodiscard]] static auto
    make_register(const std::string_view name, const type& alias_type,
                  const operand& reg, const bool is_read_only) -> alias_info {

        assert(reg.is_register());

        return {
            .from{std::string{name}},
            .to{reg.base_register()},
            .lea{},
            .type_ptr{&alias_type},
            .register_operand{reg},
            .is_element{},
            .is_read_only{is_read_only},
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

    // info about the function return
    std::optional<func_return_info> func_ret_;

    // true if var that is not dat has been added
    bool non_dat_var_has_been_added_{};

    frame_type type_{frame_type::FUNC}; // frame type
    bool is_inlined_{true};
    std::string_view storage_base_register_;
    size_t peak_storage_size_bytes_{};

  public:
    frame(const std::string_view name, const frame_type frm_type,
          const std::optional<func_return_info>& func_ret_info = {},
          std::string call_path = "", std::string func_ret_label = "",
          const bool is_inlined = true,
          const std::string_view storage_base_register = {}) noexcept
        : name_{name}, call_path_{std::move(call_path)},
          func_ret_label_{std::move(func_ret_label)}, func_ret_{func_ret_info},
          type_{frm_type}, is_inlined_{is_inlined},
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
            add_storage_size(allocated_stack_size_bytes_, allocated_size_bytes);

        vars_.put(var.name, var);

        if (not is_dat) {
            non_dat_var_has_been_added_ = true;
        }
    }

    [[nodiscard]] auto allocated_stack_size_bytes() const -> size_t {
        return add_storage_size(allocated_stack_size_bytes_,
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

        return path_[0];
    }

    [[nodiscard]] auto path() const -> const std::vector<std::string>& {
        return path_;
    }

    [[nodiscard]] auto str() const -> const std::string& { return id_; }

  private:
    auto refresh_path() -> void {
        path_.clear();
        for (auto part : id_ | std::views::split('.')) {
            path_.emplace_back(part.begin(), part.end());
        }
    }
};

class toc final {
    struct type_info {
        token src_loc_tk;
        const type* type_ptr;
    };

    // where a variable is placed, 'storage_frame' is null at the variables base
    struct storage_location {
        frame* storage_frame;
        size_t base_offset;
    };

    std::reference_wrapper<::machine> machine_;
    std::string_view source_;
    std::vector<frame> frames_;
    std::vector<const stmt_def_func*> func_defs_;
    std::vector<const statement*> data_;
    std::vector<machine::string_constant> string_constants_;
    lut<func_info> funcs_;
    lut<type_info> types_;
    const type* type_void_{};
    const type* type_bool_{};
    size_t usage_max_frame_count_{};
    size_t usage_max_vars_size_bytes_{};
    size_t total_dat_size_bytes_{};
    size_t vars_entry_gap_{};
    size_t vars_size_bytes_{};
    size_t vars_capacity_bytes_;
    bool vars_entry_gap_applied_{};
    bool bounds_check_upper_{};
    bool bounds_check_with_line_{};
    bool bounds_check_lower_{};
    bool frame_check_{};
    bool alias_check_{};

  public:
    toc(::machine& backend, const std::string_view source,
        const size_t vars_capacity_bytes, const bool bounds_check_upper,
        const bool bounds_check_lower, const bool bounds_check_with_line,
        const bool frame_check = {}, const bool alias_check = {})
        : machine_{backend}, source_{source},
          vars_capacity_bytes_{vars_capacity_bytes},
          bounds_check_upper_{bounds_check_upper},
          bounds_check_with_line_{bounds_check_with_line},
          bounds_check_lower_{bounds_check_lower}, frame_check_{frame_check},
          alias_check_{alias_check} {}

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
        data_.emplace_back(stmt);

        // same padding as 'add_var' places before the dat
        total_dat_size_bytes_ =
            add_storage_size(align_storage_size(total_dat_size_bytes_,
                                                stmt->get_type().alignment()),
                             stmt->dat_size_bytes());
        const size_t alignment{machine_.get().data_alignment()};
        vars_entry_gap_ =
            (alignment - (total_dat_size_bytes_ % alignment)) % alignment;
    }

    auto add_func(const token& src_loc_tk, std::string name,
                  const type& return_type, const stmt_def_func* const func_def)
        -> void {

        if (name == "foo") {
            throw compiler_exception{src_loc_tk,
                                     "cannot name function 'foo' because it is "
                                     "a builtin iterator function"};
        }

        if (funcs_.has(name)) {
            const func_info& fn{funcs_.get_const_ref(name)};
            throw compiler_exception{
                src_loc_tk,
                std::format("function '{}' already defined at {}", name,
                            source_location_hr(fn.src_loc_tk))};
        }

        funcs_.put(std::move(name), {
                                        .src_loc_tk{src_loc_tk},
                                        .def{func_def},
                                        .type_ptr{&return_type},
                                    });

        if (func_def) {
            func_defs_.emplace_back(func_def);
        }
    }

    // identical strings share the label of the first one compiled
    [[nodiscard]] auto add_string_constant(const token& string_tk)
        -> std::string {

        return add_read_only_constant("str", string_tk,
                                      string_tk.string_text());
    }

    auto add_type(const token& src_loc_tk, const type& tpe) -> void {
        if (types_.has(tpe.name())) {
            throw compiler_exception{
                src_loc_tk,
                std::format("type '{}' already defined at {}", tpe.name(),
                            source_location_hr(
                                types_.get_const_ref(tpe.name()).src_loc_tk))};
        }

        types_.put(tpe.name(), {
                                   .src_loc_tk{src_loc_tk},
                                   .type_ptr{&tpe},
                               });
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

        const size_t var_size_bytes{
            var.is_pointer
                ? machine_.get().address_size_bytes()
                : multiply_storage_size(var.type_ptr->size_bytes(),
                                        var.is_array ? var.array_len : 1)};

        const size_t var_alignment{var.is_pointer
                                       ? machine_.get().address_size_bytes()
                                       : var.type_ptr->alignment()};

        if (not is_dat and not vars_entry_gap_applied_) {
            frames_.front().set_padding_between_dats_and_vars(vars_entry_gap_);
            vars_size_bytes_ =
                add_storage_size(vars_size_bytes_, vars_entry_gap_);
            vars_entry_gap_applied_ = true;
        }

        // offsets are relative to the variables base or to the nearest frame
        // with its own storage base, both are aligned
        const storage_location location{find_storage_location(is_dat)};

        frame* const storage_frame{location.storage_frame};
        const size_t base_offset{location.base_offset};

        const size_t padding_bytes{
            align_storage_size(base_offset, var_alignment) - base_offset};

        const size_t allocated_size_bytes{
            add_storage_size(padding_bytes, var_size_bytes)};

        if (not is_dat) {
            assert_vars_capacity(src_loc_tk, var.name, allocated_size_bytes);
        }

        var.offset = address_offset(base_offset + padding_bytes);

        if (storage_frame) {
            var.base_register = storage_frame->storage_base_register();
            storage_frame->record_storage_size_bytes(
                add_storage_size(base_offset, allocated_size_bytes));
        }

        frames_.back().add_var(var, allocated_size_bytes, is_dat);
        vars_size_bytes_ =
            add_storage_size(vars_size_bytes_, allocated_size_bytes);

        // stats
        if (not is_dat) {
            usage_max_vars_size_bytes_ =
                std::max(used_vars_size_bytes(), usage_max_vars_size_bytes_);
        }

        comment_var(src_loc_tk, indent, var);
    }

    [[nodiscard]] auto bounds_check_options() const
        -> machine::bounds_check_options {

        return {
            .upper{bounds_check_upper_},
            .lower{bounds_check_lower_},
            .with_line{bounds_check_with_line_},
        };
    }

    [[nodiscard]] auto create_unique_label(const token& src_loc_tk,
                                           const std::string_view prefix) const
        -> std::string {

        const std::string_view call_path{get_call_path()};
        const std::string src_loc{source_location_for_use_in_label(src_loc_tk)};
        const std::string lbl{
            std::format("{}.{}{}", prefix, src_loc,
                        (call_path.empty() ? std::string{}
                                           : std::format(".{}", call_path)))};

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
                    const std::optional<func_return_info>& returns,
                    const std::string_view call_path = {},
                    const std::string_view return_jmp_label = {},
                    const bool is_inlined = true,
                    const std::string_view storage_base_register = {}) -> void {

        assert(storage_base_register.empty() or not is_inlined);

        frames_.emplace_back(
            name, frame::frame_type::FUNC, returns, std::string{call_path},
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

    auto finish() -> void {
        ::machine& x{machine_.get()};

        x.comment(token{}, 0, "           max frames in use: {}",
                  usage_max_frame_count_);

        x.comment(token{}, 0, "                    dat size: {} B",
                  total_dat_size_bytes_);

        x.comment(token{}, 0, "             dat var padding: {} B",
                  vars_entry_gap_);

        x.comment(token{}, 0, "               max vars size: {} B",
                  usage_max_vars_size_bytes_);

        assert(frames_.empty());
        assert(vars_size_bytes_ == 0);

        usage_max_frame_count_ = 0;
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

        return data_;
    }

    [[nodiscard]] auto get_func_defs() const
        -> std::span<const stmt_def_func* const> {

        return func_defs_;
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

    [[nodiscard]] auto get_string_constants() const
        -> std::span<const machine::string_constant> {

        return string_constants_;
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

        const std::string name_str{name};
        if (not types_.has(name_str)) {
            throw compiler_exception{src_loc_tk,
                                     std::format("type '{}' not found", name)};
        }

        return *types_.get_const_ref(name_str).type_ptr;
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

        std::string_view id_base{root_id_of(st.identifier())};

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

            id_base = root_id_of(alias.to);
        }

        std::unreachable();
    }

    [[nodiscard]] auto has_type(const std::string_view name) const -> bool {
        return types_.has(std::string{name});
    }

    [[nodiscard]] auto is_alias_check() const -> bool { return alias_check_; }

    [[nodiscard]] auto is_bounds_check_lower() const -> bool {
        return bounds_check_lower_;
    }

    [[nodiscard]] auto is_bounds_check_upper() const -> bool {
        return bounds_check_upper_;
    }

    [[nodiscard]] auto is_bounds_check_with_line() const -> bool {
        return bounds_check_with_line_;
    }

    [[nodiscard]] auto is_frame_check() const -> bool { return frame_check_; }

    [[nodiscard]] auto is_func(const std::string_view name) const -> bool {
        return funcs_.has(name);
    }

    [[nodiscard]] auto is_func_builtin(const std::string_view name) const
        -> bool {

        return funcs_.get_const_ref(name).def == nullptr;
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

    [[nodiscard]] auto machine() const -> const ::machine& {
        return machine_.get();
    }

    [[nodiscard]] auto make_ident_info(const statement& st) const
        -> ident_info {

        // the name refers to the declared array, 'ps[1]' accesses one element
        return as_element_if(
            st.is_array_element(),
            make_ident_info_or_throw(st.tok(), st.identifier()));
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

    // a callee frame starts aligned for any variable it holds
    [[nodiscard]] auto next_frame_address() const -> operand {
        const size_t frame_alignment{machine_.get().address_size_bytes()};

        size_t local_size_bytes{};
        for (const frame& frm : frames_ | std::views::reverse) {
            local_size_bytes = add_storage_size(
                local_size_bytes, frm.allocated_stack_size_bytes());
            if (not frm.storage_base_register().empty()) {
                return operand::mem(frm.storage_base_register(), {}, 1,
                                    address_offset(align_storage_size(
                                        local_size_bytes, frame_alignment)),
                                    get_type_address());
            }
        }

        const size_t root_size_bytes{add_storage_size(
            vars_size_bytes_, vars_entry_gap_applied_ ? 0 : vars_entry_gap_)};

        return operand::mem(machine_.get().variables_base_register(), {}, 1,
                            address_offset(align_storage_size(root_size_bytes,
                                                              frame_alignment)),
                            get_type_address());
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
    }

    auto set_type_bool(const type& tpe) -> void { type_bool_ = &tpe; }

    auto set_type_void(const type& tpe) -> void { type_void_ = &tpe; }

    [[nodiscard]] auto source() const -> std::string_view { return source_; }

    [[nodiscard]] auto
    source_location_for_use_in_label(const token& src_loc_tk) const
        -> std::string {

        const auto [line, col]{line_and_col_num_for_char_index(
            src_loc_tk.at_line(), src_loc_tk.start_index(), source_)};

        return std::format("{}.{}", line, col);
    }

    // human-readable source location
    [[nodiscard]] auto source_location_hr(const token& src_loc_tk) const
        -> std::string {

        const auto [line, col]{line_and_col_num_for_char_index(
            src_loc_tk.at_line(), src_loc_tk.start_index(), source_)};

        return std::format("{}:{}", line, col);
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

    // the tokenizer keeps the quotes in the text of a character literal
    [[nodiscard]] static auto is_character_literal(const std::string_view str)
        -> bool {

        return str.size() >= 2 and str.starts_with('\'') and
               str.ends_with('\'');
    }

    [[nodiscard]] static auto make_ident_info_from_register(const operand& reg)
        -> ident_info {

        return ident_info::make_register(reg.base_register(), reg);
    }

    // the value is the byte, e.g. 'a' is 97 and '\\xff' is 255
    [[nodiscard]] static auto parse_character(const token& src_loc_tk,
                                              const std::string_view str)
        -> int64_t {

        assert(is_character_literal(str));

        const std::string_view body{str.substr(1, str.size() - 2)};

        if (body.size() == 1 and body[0] != '\\') {
            return static_cast<unsigned char>(body[0]);
        }

        if (not body.starts_with('\\')) {
            throw compiler_exception{
                src_loc_tk,
                std::format("character literal {} must contain one character",
                            str)};
        }

        const std::optional<char> decoded{token::decode_escape(body.substr(1))};
        if (not decoded) {
            throw compiler_exception{
                src_loc_tk,
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
            std::from_chars(begin, end, value, base)};

        if (result.ec == std::errc::result_out_of_range) {
            throw compiler_exception{
                src_loc_tk, std::format("constant '{}' is out of range", str)};
        }

        if (result.ec == std::errc{} and result.ptr == end) {
            return value;
        }

        return std::nullopt;
    }

  private:
    // identical text shares the label of the first constant added
    [[nodiscard]] auto add_read_only_constant(const std::string_view kind,
                                              const token& src_loc_tk,
                                              std::string text) -> std::string {

        for (const machine::string_constant& s : string_constants_) {
            if (s.text == text) {
                return s.label;
            }
        }

        string_constants_.push_back({
            .label{std::format("{}.{}", kind,
                               source_location_for_use_in_label(src_loc_tk))},
            .text{std::move(text)},
        });

        return string_constants_.back().label;
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

    auto assert_vars_capacity(const token& src_loc_tk,
                              const std::string_view name,
                              const size_t allocated_size_bytes) const -> void {

        if (allocated_size_bytes <=
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
            std::format("{}: {}", var.name, name_info.type_ref().name())};

        if (var.array_len) {
            text += std::format("[{}]", var.array_len);
        }

        // the iterator 'e' is memory at its register, the counter 'i' is the
        // register
        const operand& reg{var.value_register.is_empty() ? var.pointer_register
                                                         : var.value_register};

        if (not reg.is_empty()) {
            x.comment(src_loc_tk, indent, "{} ({})", text, reg.base_register());
            return;
        }

        x.comment_variable(
            src_loc_tk, indent, text,
            multiply_storage_size(name_info.type_ref().size_bytes(),
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

    // a local starts after the storage in use of the nearest frame with its own
    // storage base and of the frames inside it, other variables and dats start
    // at the variables base
    [[nodiscard]] auto find_storage_location(const bool is_dat)
        -> storage_location {

        if (is_dat) {
            return {.storage_frame{}, .base_offset{vars_size_bytes_}};
        }

        size_t local_size_bytes{};

        for (frame& frm : frames_ | std::views::reverse) {
            local_size_bytes = add_storage_size(
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

        return funcs_.get_const_ref(name);
    }

    [[nodiscard]] auto
    make_ident_info_const_or_empty(const token& src_loc_tk,
                                   const std::string_view ident,
                                   const ident_path& id) const -> ident_info {

        // is 'id' an integer?
        if (const std::optional<int64_t> value{
                parse_constant(src_loc_tk, id.str())};
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
            ident_info reg_info{
                ident_info::make_register(ident, var.value_register)};

            reg_info.is_read_only = var.is_read_only;

            return reg_info;
        }

        ident_info ii{
            var.type_ptr->accessor(src_loc_tk, ident, id.path(), var,
                                   machine_.get().variables_base_register())};

        ii.is_read_only = var.is_read_only;

        lea_path.resize(id.path().size());
        // note: pad with empty for the remaining elements in the id path

        std::ranges::reverse(lea_path);
        // note: reverse it since it was constructed while traversing
        //       upwards in the frame stack but 'elem_path' and 'type_path'
        //       are ordered from the top down

        ii.lea_path = lea_path;

        if (not ii.type_ref().is_builtin()) {
            return ii;
        }

        place_operand_from_lea(src_loc_tk, ii);

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

        // note: 'lea' describes the effective address of an identifier's data.
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

        // one 'const' parameter in the alias chain makes the data read-only
        bool is_read_only{};

        for (const frame& cur_frame : frames_ | std::views::reverse) {

            // does this frame contain the variable?
            if (cur_frame.has_var(id.base())) {
                return as_read_only_if(
                    is_read_only,
                    as_element_if(is_element, make_ident_info_from_frame(
                                                  cur_frame, src_loc_tk, ident,
                                                  id, std::move(lea_path))));
            }

            // from the root frame of a function aliases are followed to the
            // actual variable referred to
            if (not cur_frame.is_func()) {
                continue;
            }

            if (not cur_frame.has_alias(id.base())) {
                lea_path.emplace_back();

                return as_read_only_if(
                    is_read_only,
                    as_element_if(is_element, make_ident_info_from_frame(
                                                  cur_frame, src_loc_tk, ident,
                                                  id, std::move(lea_path))));
            }

            // this is an alias, continue resolving until it is a variable,
            // register or constant

            const alias_info& alias{cur_frame.get_alias(id.base())};

            is_read_only = is_read_only or alias.is_read_only;

            if (alias.register_operand.is_register() and
                id.path().size() == 1) {

                return as_read_only_if(
                    is_read_only,
                    ident_info::make_register(ident, alias.register_operand));
            }

            // a field path such as 'p.x' gets its array-ness from the field
            if (alias.is_element and id.path().size() == 1) {
                is_element = true;
            }

            lea_path.emplace_back(alias.lea);

            id = replace_alias_base(alias, id, lea_path);
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
        return vars_size_bytes_ - total_dat_size_bytes_ - vars_entry_gap_;
    }

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

    [[nodiscard]] static auto as_read_only_if(const bool is_read_only,
                                              ident_info info) -> ident_info {

        info.is_read_only = info.is_read_only or is_read_only;
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
            if (not ii.lea_path[lea_index].is_empty()) {
                lea = ii.lea_path[lea_index];
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
            std::span{ii.elem_path}.subspan(lea_index)};

        // navigate to referred element and get offset
        const size_t offset{ii.type_path[lea_index]->field_offset(
            src_loc_tk, elem_path_from_lea)};

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

    // variable name without the field path
    [[nodiscard]] static auto root_id_of(const std::string_view id)
        -> std::string_view {

        return id.substr(0, id.find('.'));
    }
};
