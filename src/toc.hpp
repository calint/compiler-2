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
class stmt_def_field;
class stmt_def_type;

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
        };
    }
};

struct const_info {
    token src_loc_tk; // token for position in the source
    int64_t value;
};

class frame final {
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

  public:
    enum class frame_type : uint8_t { FUNC, BLOCK, LOOP, FOO };

  private:
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

    [[nodiscard]] auto storage_base_register() const -> std::string_view {
        return storage_base_register_;
    }

    auto record_storage_size_bytes(const size_t size_bytes) -> void {
        assert(not storage_base_register_.empty());

        peak_storage_size_bytes_ =
            std::max(peak_storage_size_bytes_, size_bytes);
    }

    [[nodiscard]] auto peak_storage_size_bytes() const -> size_t {
        assert(not storage_base_register_.empty());

        return peak_storage_size_bytes_;
    }

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

    auto set_padding_between_dats_and_vars(const size_t size_bytes) -> void {
        assert(stack_padding_size_bytes_ == 0);

        stack_padding_size_bytes_ = size_bytes;
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

    [[nodiscard]] auto is_foo() const -> bool {
        return type_ == frame_type::FOO;
    }

    [[nodiscard]] auto is_name(const std::string_view name) const -> bool {
        return name_ == name;
    }

    [[nodiscard]] auto name() const -> std::string_view { return name_; }
};

class ident_path final {
    std::string id_;
    std::vector<std::string> path_;

    auto refresh_path() -> void {
        path_.clear();
        for (auto part : id_ | std::views::split('.')) {
            path_.emplace_back(part.begin(), part.end());
        }
    }

  public:
    explicit ident_path(std::string id) : id_{std::move(id)} {
        assert(not id_.empty());

        refresh_path();

        assert(not path_.empty());
    }

    [[nodiscard]] auto base() const -> std::string_view {
        assert(not path_.empty());

        return path_[0];
    }

    [[nodiscard]] auto path() const -> const std::vector<std::string>& {
        return path_;
    }

    [[nodiscard]] auto str() const -> const std::string& { return id_; }

    auto append(const std::string_view path_elem) -> void {
        id_ = std::format("{}.{}", id_, path_elem);
        refresh_path();
    }
};

class toc final {

    struct type_info {
        token src_loc_tk;
        const type* type_ptr;
    };

    std::reference_wrapper<::machine> machine_;
    std::string_view source_;
    std::vector<frame> frames_;
    std::vector<const stmt_def_func*> func_defs_;
    std::vector<const statement*> data_;
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

  public:
    toc(::machine& backend, const std::string_view source,
        const size_t vars_capacity_bytes, const bool bounds_check_upper,
        const bool bounds_check_lower, const bool bounds_check_with_line,
        const bool frame_check = {})
        : machine_{backend}, source_{source},
          vars_capacity_bytes_{vars_capacity_bytes},
          bounds_check_upper_{bounds_check_upper},
          bounds_check_with_line_{bounds_check_with_line},
          bounds_check_lower_{bounds_check_lower}, frame_check_{frame_check} {}

    [[nodiscard]] auto machine() -> ::machine& { return machine_.get(); }

    [[nodiscard]] auto machine() const -> const ::machine& {
        return machine_.get();
    }

    auto add_alias(const alias_info& ai) -> void {
        frames_.back().add_alias(ai);
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
        total_dat_size_bytes_ =
            add_storage_size(total_dat_size_bytes_, stmt->dat_size_bytes());
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

        // check if the variable is already declared in this scope
        if (frames_.back().has_var(var.name)) {
            const var_info& decl_var{
                frames_.back().get_var_const_ref(var.name)};

            throw compiler_exception{
                src_loc_tk,
                std::format("variable '{}' already declared at {}", var.name,
                            source_location_hr(decl_var.src_loc_tk))};
        }

        const size_t var_size_bytes{
            var.is_pointer
                ? machine_.get().address_size_bytes()
                : multiply_storage_size(var.type_ptr->size_bytes(),
                                        var.is_array ? var.array_len : 1)};

        if (not is_dat and not vars_entry_gap_applied_) {
            frames_.front().set_padding_between_dats_and_vars(vars_entry_gap_);
            vars_size_bytes_ =
                add_storage_size(vars_size_bytes_, vars_entry_gap_);
            vars_entry_gap_applied_ = true;
        }

        if (not is_dat) {
            const size_t used_size_bytes{
                vars_size_bytes_ - total_dat_size_bytes_ - vars_entry_gap_};
            if (var_size_bytes > vars_capacity_bytes_ - used_size_bytes) {
                throw compiler_exception{
                    src_loc_tk,
                    std::format("variable '{}' would overflow allocated vars "
                                "section",
                                var.name)};
            }
        }

        var.offset = address_offset(vars_size_bytes_);

        if (not is_dat) {
            size_t local_size_bytes{};
            for (frame& frm : frames_ | std::views::reverse) {
                local_size_bytes = add_storage_size(
                    local_size_bytes, frm.allocated_stack_size_bytes());
                if (not frm.storage_base_register().empty()) {
                    var.base_register = frm.storage_base_register();
                    var.offset = address_offset(local_size_bytes);
                    frm.record_storage_size_bytes(
                        add_storage_size(local_size_bytes, var_size_bytes));

                    break;
                }
            }
        }

        frames_.back().add_var(var, var_size_bytes, is_dat);
        vars_size_bytes_ = add_storage_size(vars_size_bytes_, var_size_bytes);

        // stats
        if (not is_dat) {
            usage_max_vars_size_bytes_ = std::max(
                vars_size_bytes_ - total_dat_size_bytes_ - vars_entry_gap_,
                usage_max_vars_size_bytes_);
        }

        // comment the resolved name
        const ident_info& name_info{make_ident_info(src_loc_tk, var.name)};

        ::machine& x{machine()};

        std::string text{
            std::format("{}: {}", var.name, name_info.type_ref().name())};

        if (var.array_len) {
            text += std::format("[{}]", var.array_len);
        }
        if (not var.reg.is_empty()) {
            x.comment(src_loc_tk, indent, "{} ({})", text,
                      var.reg.base_register());

            return;
        }
        x.comment_variable(
            src_loc_tk, indent, text,
            multiply_storage_size(name_info.type_ref().size_bytes(),
                                  name_info.is_array ? name_info.array_len : 1),
            name_info.operand);
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

    [[nodiscard]] auto is_inlined_func() const -> bool {
        for (const frame& frm : frames_ | std::views::reverse) {
            if (frm.is_func()) {
                return frm.is_inlined_func();
            }
        }

        std::unreachable();
    }

    [[nodiscard]] auto peak_frame_size_bytes() const -> size_t {
        for (const frame& frm : frames_ | std::views::reverse) {
            if (not frm.storage_base_register().empty()) {
                return frm.peak_storage_size_bytes();
            }
        }

        std::unreachable();
    }

    [[nodiscard]] auto next_frame_address() const -> operand {
        size_t local_size_bytes{};
        for (const frame& frm : frames_ | std::views::reverse) {
            local_size_bytes = add_storage_size(
                local_size_bytes, frm.allocated_stack_size_bytes());
            if (not frm.storage_base_register().empty()) {
                return operand::mem(frm.storage_base_register(), {}, 1,
                                    address_offset(local_size_bytes),
                                    get_type_address());
            }
        }

        const size_t root_size_bytes{add_storage_size(
            vars_size_bytes_, vars_entry_gap_applied_ ? 0 : vars_entry_gap_)};

        return operand::mem(machine_.get().variables_base_register(), {}, 1,
                            address_offset(root_size_bytes),
                            get_type_address());
    }

    auto enter_loop(const std::string_view name) -> void {
        frames_.emplace_back(name, frame::frame_type::LOOP);
        refresh_usage();
    }

    auto enter_foo(const std::string_view name) -> void {
        frames_.emplace_back(name, frame::frame_type::FOO);
        refresh_usage();
    }

    auto exit_foo(const std::string_view name) -> void {
        const frame& frm{frames_.back()};

        assert(frm.is_foo() and frm.is_name(name));

        vars_size_bytes_ -= frm.allocated_stack_size_bytes();
        frames_.pop_back();
        if (frames_.empty()) {

            assert(vars_size_bytes_ == 0);

            vars_entry_gap_applied_ = false;
        }
    }

    auto exit_block() -> void {
        const frame& frm{frames_.back()};

        assert(frm.is_block());

        vars_size_bytes_ -= frm.allocated_stack_size_bytes();
        frames_.pop_back();
        if (frames_.empty()) {

            assert(vars_size_bytes_ == 0);

            vars_entry_gap_applied_ = false;
        }
    }

    auto exit_func(const std::string_view name) -> void {
        const frame& frm{frames_.back()};

        assert(frm.is_func() and frm.is_name(name));

        vars_size_bytes_ -= frm.allocated_stack_size_bytes();
        frames_.pop_back();
        if (frames_.empty()) {

            assert(vars_size_bytes_ == 0);

            vars_entry_gap_applied_ = false;
        }
    }

    auto exit_loop(const std::string_view name) -> void {
        const frame& frm{frames_.back()};

        assert(frm.is_loop() and frm.is_name(name));

        vars_size_bytes_ -= frm.allocated_stack_size_bytes();
        frames_.pop_back();
        if (frames_.empty()) {

            assert(vars_size_bytes_ == 0);

            vars_entry_gap_applied_ = false;
        }
    }

    auto reset_usage() -> void {
        assert(frames_.empty());
        assert(vars_size_bytes_ == 0);

        usage_max_frame_count_ = 0;
        usage_max_vars_size_bytes_ = 0;
    }

    auto finish() -> void {
        ::machine& x{machine_.get()};

        x.comment(token{}, 0, "           max frames in use: {}",
                  usage_max_frame_count_);

        x.comment(token{}, 0, "             dat var padding: {} B",
                  vars_entry_gap_);

        x.comment(token{}, 0, "               max vars size: {} B",
                  usage_max_vars_size_bytes_);

        assert(frames_.empty());
        assert(vars_size_bytes_ == 0);

        usage_max_frame_count_ = 0;
    }

    [[nodiscard]] auto get_call_path() const -> std::string_view {

        for (const frame& frm : frames_ | std::views::reverse) {
            if (frm.is_func()) {
                return frm.call_path();
            }
        }

        std::unreachable();
    }

    [[nodiscard]] auto get_const(const std::string_view name) const -> int64_t {

        for (const frame& f : frames_ | std::views::reverse) {
            if (f.has_const(name)) {
                return f.get_const(name).value;
            }
            if (f.is_func()) {
                break;
            }
        }

        if (frames_.front().has_const(name)) {
            return frames_.front().get_const(name).value;
        }

        std::unreachable();
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

        if (not funcs_.has(name)) {
            throw compiler_exception{
                src_loc_tk, std::format("function '{}' not found", name)};
        }

        return *funcs_.get_const_ref(name).def;
    }

    [[nodiscard]] auto get_func_return_label() const -> std::string_view {
        for (const frame& frm : frames_ | std::views::reverse) {
            if (frm.is_func()) {
                return frm.func_ret_label();
            }
        }

        std::unreachable();
    }

    [[nodiscard]] auto
    get_func_return_type_or_throw(const token& src_loc_tk,
                                  const std::string_view name) const
        -> const type& {

        if (not funcs_.has(name)) {
            throw compiler_exception{
                src_loc_tk, std::format("function '{}' not found", name)};
        }

        return *funcs_.get_const_ref(name).type_ptr;
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

    [[nodiscard]] auto get_type_bool() const -> const type& {
        return *type_bool_;
    }

    [[nodiscard]] auto get_type_default() const -> const type& {
        return machine_.get().default_type();
    }

    [[nodiscard]] auto get_type_address() const -> const type& {
        return get_type_or_throw(
            token{}, machine_.get().address_size_bytes() == 4 ? "i32" : "i64");
    }

    [[nodiscard]] auto has_type(const std::string_view name) const -> bool {
        return types_.has(std::string{name});
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
        for (const frame& f : frames_ | std::views::reverse) {
            if (f.has_const(name)) {
                return true;
            }
            if (f.is_func()) {
                break;
            }
        }

        return frames_.front().has_const(name);
    }

    [[nodiscard]] auto
    has_const_in_current_block(const std::string_view name) const -> bool {

        return frames_.back().has_const(name);
    }

    [[nodiscard]] auto has_lea(const statement& st) const -> bool {
        if (not st.is_identifier()) {
            return false;
        }

        std::string_view id_base{st.identifier()};
        id_base = id_base.substr(0, id_base.find('.'));

        for (const frame& frm : frames_ | std::views::reverse) {
            if (frm.has_var(id_base)) {
                return frm.get_var_const_ref(id_base).is_pointer;
            }
            if (frm.is_func()) {
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
                id_base = alias.to;
                id_base = id_base.substr(0, id_base.find('.'));
            }
        }

        std::unreachable();
    }

    [[nodiscard]] auto is_bounds_check_upper() const -> bool {
        return bounds_check_upper_;
    }

    [[nodiscard]] auto is_bounds_check_with_line() const -> bool {
        return bounds_check_with_line_;
    }

    [[nodiscard]] auto is_bounds_check_lower() const -> bool {
        return bounds_check_lower_;
    }

    [[nodiscard]] auto is_frame_check() const -> bool { return frame_check_; }

    [[nodiscard]] auto is_func(const std::string_view name) const -> bool {
        return funcs_.has(name);
    }

    [[nodiscard]] auto is_func_builtin(const std::string_view name) const
        -> bool {

        return funcs_.get_const_ref(name).def == nullptr;
    }

    [[nodiscard]] auto make_ident_info(const statement& st) const
        -> ident_info {

        return make_ident_info_or_throw(st.tok(), st.identifier());
    }

    [[nodiscard]] auto make_ident_info(const token& src_loc_tk,
                                       const std::string_view ident) const
        -> ident_info {

        return make_ident_info_or_throw(src_loc_tk, ident);
    }

    [[nodiscard]] static auto make_ident_info_from_register(const operand& reg)
        -> ident_info {

        return ident_info::make_register(reg.base_register(), reg);
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

    [[nodiscard]] auto get_stack_size_bytes() const -> size_t {
        return vars_size_bytes_;
    }

    [[nodiscard]] static auto
    field_offset_in_type(const type& tp, const std::string_view field_name)
        -> size_t {

        for (const type_field& f : tp.fields()) {
            if (f.name == field_name) {
                return f.offset;
            }
        }

        std::unreachable();
    }

    [[nodiscard]] static auto parse_constant(const token& src_loc_tk,
                                             const std::string_view str)
        -> std::optional<int64_t> {

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
    [[nodiscard]] auto
    builtin_type_for_size_bytes(const size_t size_bytes) const -> const type& {

        for (const char* const name : {"i64", "i32", "i16", "i8"}) {
            const type& value_type{*types_.get_const_ref(name).type_ptr};
            if (size_bytes == value_type.size_bytes()) {
                return value_type;
            }
        }

        std::unreachable();
    }

    [[nodiscard]] auto is_in_main() const -> bool {
        for (const frame& frm : frames_ | std::views::reverse) {
            if (frm.is_func()) {
                return frm.name() == "main";
            }
        }

        std::unreachable();
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

        for (const frame& cur_frame : frames_ | std::views::reverse) {

            // does this frame contain the variable?
            if (cur_frame.has_var(id.base())) {
                return make_ident_info_from_frame(cur_frame, src_loc_tk, ident,
                                                  id, std::move(lea_path));
            }

            if (cur_frame.is_func()) {

                // root frame of the function
                // from here on aliases are followed to the actual variable
                // referred to

                if (not cur_frame.has_alias(id.base())) {
                    // is not an alias

                    // add an empty
                    lea_path.emplace_back();

                    return make_ident_info_from_frame(
                        cur_frame, src_loc_tk, ident, id, std::move(lea_path));
                }

                // this is an alias, continue resolving until it is a variable,
                // register or constant

                const alias_info& alias{cur_frame.get_alias(id.base())};

                if (alias.register_operand.is_register() and
                    id.path().size() == 1) {
                    return ident_info::make_register(ident,
                                                     alias.register_operand);
                }

                lea_path.emplace_back(alias.lea);

                ident_path new_id{std::string{alias.to}};

                // big note: the fishy resizing of the 'lea_path' happens when
                //           the 'new_id' extended past fields that do not need
                //           lea
                //           if 'lea_path' is not extended then the types, id
                //           path elements and lea path vectors are not in sync

                const size_t new_id_count{new_id.path().size()};
                const size_t lea_count{lea_path.size()};
                if ((new_id_count > lea_count) and
                    (new_id_count - lea_count > 1)) {
                    lea_path.resize(lea_path.size() + new_id.path().size() - 2);
                    // note: -2 because last element is current element and
                    //       first will be processed
                }

                // this is an alias
                // e.g.
                //   res -> pt.x becomes pt.x
                //   pt.x -> p becomes p.x
                //   lnk.count -> world.room.link becomes
                //   world.room.link.count

                for (const std::string& s : id.path() | std::views::drop(1)) {
                    new_id.append(s);
                }

                id = new_id;

                assert(not id.path().empty());
            }
        }

        return make_ident_info_const_or_empty(src_loc_tk, ident, id);
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

        ident_info ii{
            var.type_ptr->accessor(src_loc_tk, ident, id.path(), var,
                                   machine_.get().variables_base_register())};

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

        // identifier is built-in type

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
            return ii;
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

        return ii;
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
            return ident_info::make_const(ident, id.str(), get_type_default(),
                                          1);
        }

        if (id.base() == "false") {
            return ident_info::make_const(ident, id.str(), get_type_default(),
                                          0);
        }

        // is 'id' a constant?
        if (has_const(id.str())) {
            return ident_info::make_const(ident, id.str(), get_type_default(),
                                          get_const(id.str()));
        }

        // not resolved, return empty info
        return ident_info::make_empty();
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

    auto refresh_usage() -> void {
        usage_max_frame_count_ =
            std::max(frames_.size(), usage_max_frame_count_);
    }
};
