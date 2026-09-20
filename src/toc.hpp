#pragma once
// reviewed: 2025-09-29

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "lut.hpp"
#include "statement.hpp"
#include "type.hpp"
#include "x86.hpp"

class stmt_def_func;
class stmt_def_field;
class stmt_def_type;

struct func_info {
    token declared_at_tk;       // token for position in the source
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
    std::string lea;
    const type* type_ptr{};
};

struct const_info {
    token declared_at_tk; // token for position in the source
    int64_t value;
};

class frame final {
    // optional name
    std::string_view name_;

    // a unique path of source locations of the in-lined call
    std::string call_path_;

    // number of bytes used on the stack by this frame
    size_t allocated_stack_{};

    // stack padding assigned to this frame
    size_t stack_padding_{};

    // constants
    lut<const_info> consts_;

    // variables declared in this frame
    lut<var_info> vars_;

    // aliases that refer to previous frame(s) alias or variable
    lut<alias_info> aliases_;

    // the label to jump to when exiting an in-lined function
    std::string func_ret_label_;

    // info about the function return
    std::optional<func_return_info> func_ret_;

    // true if var that is not dat has been added
    bool non_dat_var_has_been_added_{};

  public:
    enum class frame_type : uint8_t { FUNC, BLOCK, LOOP, FOO };

  private:
    frame_type type_{frame_type::FUNC}; // frame type

  public:
    frame(const std::string_view name, const frame_type frm_type,
          const std::optional<func_return_info>& func_ret_info = {},
          std::string call_path = "", std::string func_ret_label = "") noexcept
        : name_{name}, call_path_{std::move(call_path)},
          func_ret_label_{std::move(func_ret_label)}, func_ret_{func_ret_info},
          type_{frm_type} {}

    auto add_alias(const alias_info& ai) -> void {
        aliases_.put(std::string{ai.from}, ai);
    }

    auto add_const(const std::string_view name, const const_info& ci) -> void {
        consts_.put(std::string{name}, ci);
    }

    auto add_var(const var_info& var, const size_t allocated_size,
                 bool is_data = false) -> void {

        allocated_stack_ += allocated_size;

        vars_.put(var.name, var);

        if (not is_data) {
            non_dat_var_has_been_added_ = true;
        }
    }

    [[nodiscard]] auto allocated_stack_size() const -> size_t {
        return allocated_stack_ + stack_padding_;
    }

    auto set_padding_between_dats_and_vars(const size_t nbytes) -> void {
        assert(stack_padding_ == 0);
        stack_padding_ = nbytes;
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

    [[nodiscard]] auto has_non_data_var_been_added() const -> bool {
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
        token declared_at_tk;
        const type* type_ptr;
    };

    x86 x86_;
    std::string_view source_;
    std::vector<frame> frames_;
    std::vector<const stmt_def_func*> func_defs_;
    std::vector<const statement*> data_;
    lut<func_info> funcs_;
    lut<type_info> types_;
    const type* type_void_{};
    const type* type_default_{};
    const type* type_bool_{};
    size_t usage_max_frame_count_{};
    size_t usage_max_vars_size_{};
    size_t total_dat_size_{};
    size_t vars_entry_gap_{};
    size_t vars_size_{};
    size_t vars_capacity_;
    bool vars_entry_gap_applied_{};
    bool bounds_check_upper_{};
    bool bounds_check_with_line_{};
    bool bounds_check_lower_{};

    static constexpr size_t stack_alignment{16};

  public:
    toc(std::ostream& os, const std::string_view source,
        const size_t vars_capacity, const bool bounds_check_upper,
        const bool bounds_check_lower, const bool bounds_check_with_line)
        : x86_{os, source}, source_{source}, vars_capacity_{vars_capacity},
          bounds_check_upper_{bounds_check_upper},
          bounds_check_with_line_{bounds_check_with_line},
          bounds_check_lower_{bounds_check_lower} {}

    [[nodiscard]] auto machine() -> x86& { return x86_; }

    [[nodiscard]] auto machine() const -> const x86& { return x86_; }

    auto add_alias(const alias_info& ai) -> void {
        frames_.back().add_alias(ai);
    }

    auto add_const(const token& src_loc_tk, const size_t indent,
                   const std::string_view name, const int64_t value) {

        if (has_const_in_current_block(name)) {
            const const_info& c{frames_.back().get_const(name)};

            throw compiler_exception(
                src_loc_tk,
                std::format("constant '{}' already defined in this block at {}",
                            name, source_location_hr(c.declared_at_tk)));
        }

        x86& x{machine()};

        x.comment(src_loc_tk, indent, "const {} = {}", name, value);
        frames_.back().add_const(name,
                                 {.declared_at_tk{src_loc_tk}, .value{value}});
    }

    auto add_dat(const statement* stmt) -> void {
        if (frames_.size() != 1) {
            throw compiler_exception(stmt->tok(),
                                     "'dat' can only be added in global scope");
        }
        if (frames_.front().has_non_data_var_been_added()) {
            throw compiler_exception(
                stmt->tok(), "'dat' can only be added before any 'var'");
        }
        data_.emplace_back(stmt);
        total_dat_size_ += stmt->dat_size_bytes();
        vars_entry_gap_ =
            (stack_alignment - (total_dat_size_ % stack_alignment)) %
            stack_alignment;
    }

    auto add_func(const token& src_loc_tk, std::string name,
                  const type& return_type, const stmt_def_func* func_def)
        -> void {

        if (name == "foo") {
            throw compiler_exception(src_loc_tk,
                                     "cannot name function 'foo' because it is "
                                     "a builtin iterator function");
        }

        if (funcs_.has(name)) {
            const func_info& fn{funcs_.get_const_ref(name)};
            throw compiler_exception{
                src_loc_tk,
                std::format("function '{}' already defined at {}", name,
                            source_location_hr(fn.declared_at_tk))};
        }

        funcs_.put(std::move(name), {.declared_at_tk{src_loc_tk},
                                     .def{func_def},
                                     .type_ptr{&return_type}});

        if (func_def) {
            func_defs_.emplace_back(func_def);
        }
    }

    auto add_type(const token& src_loc_tk, const type& tpe) -> void {
        if (types_.has(tpe.name())) {
            throw compiler_exception{
                src_loc_tk,
                std::format(
                    "type '{}' already defined at {}", tpe.name(),
                    source_location_hr(
                        types_.get_const_ref(tpe.name()).declared_at_tk))};
        }

        types_.put(tpe.name(), {
                                   .declared_at_tk{src_loc_tk},
                                   .type_ptr{&tpe},
                               });
    }

    auto add_var(const token& src_loc_tk, const size_t indent, var_info var,
                 bool is_dat) -> void {

        if (operand::register_size(var.name) != 0) {
            throw compiler_exception{
                src_loc_tk,
                std::format("cannot use register name '{}' as a variable name",
                            var.name)};
        }

        // check if the variable is already declared in this scope
        if (frames_.back().has_var(var.name)) {
            const var_info& decl_var{
                frames_.back().get_var_const_ref(var.name)};

            throw compiler_exception{
                src_loc_tk,
                std::format("variable '{}' already declared at {}", var.name,
                            source_location_hr(decl_var.declared_at_tk))};
        }

        const size_t var_size{var.type_ptr->size() *
                              (var.is_array ? var.array_size : 1)};

        if (not is_dat and not vars_entry_gap_applied_) {
            frames_.front().set_padding_between_dats_and_vars(vars_entry_gap_);
            vars_size_ += vars_entry_gap_;
            vars_entry_gap_applied_ = true;
        }

        if (not is_dat) {
            const size_t used{vars_size_ - total_dat_size_ - vars_entry_gap_};
            if (var_size > vars_capacity_ - used) {
                throw compiler_exception{
                    src_loc_tk,
                    std::format("variable '{}' would overflow allocated vars "
                                "section",
                                var.name)};
            }
        }

        var.stack_idx = static_cast<int32_t>(vars_size_);

        frames_.back().add_var(var, var_size, is_dat);
        vars_size_ += var_size;

        // stats
        if (not is_dat) {
            usage_max_vars_size_ =
                std::max(vars_size_ - total_dat_size_ - vars_entry_gap_,
                         usage_max_vars_size_);
        }

        // comment the resolved name
        const ident_info& name_info{
            make_ident_info_parsing(src_loc_tk, var.name)};

        x86& x{machine()};

        std::string text{
            std::format("{}: {}", var.name, name_info.type_ref().name())};

        if (var.array_size) {
            text += std::format("[{}]", var.array_size);
        }
        if (not var.reg.empty()) {
            x.comment(src_loc_tk, indent, "{} ({})", text, var.reg);
            return;
        }
        x.comment(src_loc_tk, indent, "{} ({} B @ [{}])", text,
                  name_info.type_ref().size() *
                      (name_info.is_array ? name_info.array_size : 1),
                  name_info.operand.address_str());
    }

    [[nodiscard]] auto create_unique_label(const token& tk,
                                           const std::string_view prefix) const
        -> std::string {

        const std::string_view call_path{get_call_path()};
        const std::string src_loc{source_location_for_use_in_label(tk)};
        const std::string lbl{
            std::format("{}_{}{}", prefix, src_loc,
                        (call_path.empty() ? std::string{}
                                           : std::format("_{}", call_path)))};

        return lbl;
    }

    auto enter_block() -> void {
        frames_.emplace_back("", frame::frame_type::BLOCK);
        refresh_usage();
    }

    auto enter_func(const std::string_view name,
                    const std::optional<func_return_info>& returns,
                    const std::string_view call_path = {},
                    const std::string_view return_jmp_label = {}) -> void {

        frames_.emplace_back(name, frame::frame_type::FUNC, returns,
                             std::string{call_path},
                             std::string{return_jmp_label});

        refresh_usage();
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
        vars_size_ -= frm.allocated_stack_size();
        frames_.pop_back();
        if (frames_.empty()) {
            assert(vars_size_ == 0);
            vars_entry_gap_applied_ = false;
        }
    }

    auto exit_block() -> void {
        const frame& frm{frames_.back()};
        assert(frm.is_block());
        vars_size_ -= frm.allocated_stack_size();
        frames_.pop_back();
        if (frames_.empty()) {
            assert(vars_size_ == 0);
            vars_entry_gap_applied_ = false;
        }
    }

    auto exit_func(const std::string_view name) -> void {
        const frame& frm{frames_.back()};
        assert(frm.is_func() and frm.is_name(name));
        vars_size_ -= frm.allocated_stack_size();
        frames_.pop_back();
        if (frames_.empty()) {
            assert(vars_size_ == 0);
            vars_entry_gap_applied_ = false;
        }
    }

    auto exit_loop(const std::string_view name) -> void {
        const frame& frm{frames_.back()};
        assert(frm.is_loop() and frm.is_name(name));
        vars_size_ -= frm.allocated_stack_size();
        frames_.pop_back();
        if (frames_.empty()) {
            assert(vars_size_ == 0);
            vars_entry_gap_applied_ = false;
        }
    }

    auto reset_usage() -> void {
        assert(frames_.empty());
        assert(vars_size_ == 0);
        usage_max_frame_count_ = 0;
        usage_max_vars_size_ = 0;
    }

    auto finish(std::ostream& os) -> void {
        std::println(os, ";            max frames in use: {}",
                     usage_max_frame_count_);

        std::println(os, ";              dat var padding: {} B",
                     vars_entry_gap_);

        std::println(os, ";                max vars size: {} B",
                     usage_max_vars_size_);

        assert(frames_.empty());
        assert(vars_size_ == 0);
        usage_max_frame_count_ = 0;
    }

    [[nodiscard]] auto get_call_path() const -> std::string_view {

        for (const auto& frm : frames_ | std::views::reverse) {
            if (frm.is_func()) {
                return frm.call_path();
            }
        }

        std::unreachable();
    }

    [[nodiscard]] auto get_call_path_extend(const token& src_loc_tk,
                                            const std::string& name) const
        -> std::string {

        const std::string_view call_path{get_call_path()};
        return std::format("{}_{}{}", name,
                           source_location_for_use_in_label(src_loc_tk),
                           (call_path.empty() ? std::string{}
                                              : std::format("_{}", call_path)));
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
        for (const auto& frm : frames_ | std::views::reverse) {
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
                                       std::vector<std::string>& lea_registers)
        -> operand {

        if (not src.is_indexed() and not src_info.has_lea()) {
            return src_info.operand;
        }

        operand op{src.compile_lea(*this, indent, src.tok(), lea_registers, "",
                                   src_info.lea_path)};

        op.size = src_info.operand.size;

        return op;
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
        return *type_default_;
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
                return false;
            }
            if (frm.is_func()) {
                if (not frm.has_alias(id_base)) {
                    return false;
                }
                const alias_info& alias{frm.get_alias(id_base)};
                if (not alias.lea.empty()) {
                    return true;
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

    [[nodiscard]] auto is_func(const std::string_view name) const -> bool {
        return funcs_.has(name);
    }

    [[nodiscard]] auto is_func_builtin(const std::string_view name) const
        -> bool {

        return funcs_.get_const_ref(name).def == nullptr;
    }

    [[nodiscard]] auto make_ident_info(const statement& st) const
        -> ident_info {

        return make_ident_info_or_throw(true, st.tok(), st.identifier());
    }

    [[nodiscard]] auto make_ident_info(const token& src_loc_tk,
                                       const std::string_view ident) const
        -> ident_info {

        return make_ident_info_or_throw(true, src_loc_tk, ident);
    }

    [[nodiscard]] auto make_ident_info_parsing(const statement& st) const
        -> ident_info {

        return make_ident_info_or_throw(false, st.tok(), st.identifier());
    }

    [[nodiscard]] auto
    make_ident_info_parsing(const token& src_loc_tk,
                            const std::string_view ident) const -> ident_info {

        return make_ident_info_or_throw(false, src_loc_tk, ident);
    }

    [[nodiscard]] static auto
    make_ident_info_register(const std::string_view ident,
                             const std::string_view reg, const type& tpe)
        -> ident_info {

        // unary ops are applied by callers where relevant
        return ident_info::make_register(ident, reg, tpe);
    }

    [[nodiscard]] auto
    make_ident_info_from_register(const std::string_view reg) const
        -> ident_info {

        const x86& x{machine()};

        return make_ident_info_register(reg, reg,
                                        x.get_allocated_register_type(reg));
    }

    auto set_type_bool(const type& tpe) -> void { type_bool_ = &tpe; }

    auto set_type_default(const type& tpe) -> void { type_default_ = &tpe; }

    auto set_type_void(const type& tpe) -> void { type_void_ = &tpe; }

    [[nodiscard]] auto source() const -> std::string_view { return source_; }

    [[nodiscard]] auto
    source_location_for_use_in_label(const token& src_loc_tk) const
        -> std::string {

        const auto [line, col]{line_and_col_num_for_char_index(
            src_loc_tk.at_line(), src_loc_tk.start_index(), source_)};

        return std::format("{}_{}", line, col);
    }

    // human-readable source location
    [[nodiscard]] auto source_location_hr(const token& src_loc_tk) const
        -> std::string {

        const auto [line, col]{line_and_col_num_for_char_index(
            src_loc_tk.at_line(), src_loc_tk.start_index(), source_)};

        return std::format("{}:{}", line, col);
    }

    [[nodiscard]] auto get_stack_size() const -> size_t { return vars_size_; }

    [[nodiscard]] auto get_builtin_type_for_size(const size_t size) const
        -> const type& {

        switch (size) {
        case operand::size_qword:
            return *types_.get_const_ref("i64").type_ptr;
        case operand::size_dword:
            return *types_.get_const_ref("i32").type_ptr;
        case operand::size_word:
            return *types_.get_const_ref("i16").type_ptr;
        case operand::size_byte:
            return *types_.get_const_ref("i8").type_ptr;
        default:
            std::unreachable();
        }
    }

    [[nodiscard]] static auto
    get_field_offset_in_type(const type& tp, const std::string_view field_name)
        -> size_t {

        size_t accum{};
        for (const type_field& f : tp.fields()) {
            if (f.name == field_name) {
                return accum;
            }
            accum += f.size;
        }

        std::unreachable();
    }

// pragma below for clang++ to not generate warning stemming from
// 'std::from_chars' requiring pointers
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
    [[nodiscard]] static auto parse_constant(const token& src_loc_tk,
                                             const std::string_view str)
        -> std::optional<int64_t> {

        // is it hex?
        if (str.starts_with("0x") or str.starts_with("0X")) {
            constexpr size_t base_hex{16};

            std::string_view sv{str};
            sv.remove_prefix(2); // skip "0x" or "0X"

            int64_t value{};
            const std::from_chars_result result{std::from_chars(
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                sv.data(), sv.data() + sv.size(), value, base_hex)};

            if (result.ec == std::errc::result_out_of_range) {
                throw compiler_exception{
                    src_loc_tk,
                    std::format("constant '{}' is out of range", str)};
            }

            if (result.ec == std::errc{} and
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                result.ptr == sv.data() + sv.size()) {

                return value;
            }
        }

        // is it binary?
        if (str.starts_with("0b") or str.starts_with("0B")) {
            constexpr size_t base_binary{2};

            std::string_view sv{str};
            sv.remove_prefix(2); // skip "0b" or "0B"

            int64_t value{};
            const std::from_chars_result result{std::from_chars(
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                sv.data(), sv.data() + sv.size(), value, base_binary)};

            if (result.ec == std::errc::result_out_of_range) {
                throw compiler_exception{
                    src_loc_tk,
                    std::format("constant '{}' is out of range", str)};
            }

            if (result.ec == std::errc{} and
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                result.ptr == sv.data() + sv.size()) {

                return value;
            }
        }

        // try decimal digit
        {
            const std::string_view sv{str};
            // note: using 'std::string_view' for 'clang-tidy' to not
            // trigger the warning
            // 'cppcoreguidelines-pro-bounds-pointer-arithmetic'

            int64_t value{};
            const std::from_chars_result result{
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                std::from_chars(sv.data(), sv.data() + sv.size(), value)};

            if (result.ec == std::errc::result_out_of_range) {
                throw compiler_exception{
                    src_loc_tk,
                    std::format("constant '{}' is out of range", str)};
            }

            if (result.ec == std::errc{} and
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                result.ptr == sv.data() + sv.size()) {

                return value;
            }
        }

        return std::nullopt;
    }

  private:
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
    make_ident_info_or_empty(const bool use_allocated_register_type,
                             const token& src_loc_tk,
                             const std::string_view ident) const -> ident_info {

        assert(not ident.empty());
        ident_path id{std::string{ident}};
        assert(not id.path().empty());

        // get the base of the identifier: e.g. lnks[1].pos.y -> lnks
        // traverse the frames and resolve to a variable, register or constant

        std::vector<std::string> lea_path;
        // note: 'lea' is a register operand pointing to the data of the
        //       identifier combined which combined assembler instruction 'lea'
        //       to loads the effective address of that data
        //       'lea_path' elements will match components of the identifier
        //       using the top most being the most recent in the call stack

        // ignore the elements after the first element:
        //  e.g.: lnks[1].pos.y
        //   ignore pos.y since those cannot have a lea
        //   add empty leas for those
        //   note: 'lea_path' will be reversed when complete so that
        //          'ident_path' elements have corresponding lea

        lea_path.insert(lea_path.end(), id.path().size() - 1, "");
        // note: -1 to exclude the first element

        for (const frame& f : frames_ | std::views::reverse) {

            // does this frame contain the variable?
            if (f.has_var(id.base())) {
                return make_ident_info_from_frame(use_allocated_register_type,
                                                  f, src_loc_tk, ident, id,
                                                  std::move(lea_path));
            }

            if (f.is_func()) {

                // root frame of the function
                // from here on aliases are followed to the actual variable
                // referred to

                if (not f.has_alias(id.base())) {
                    // is not an alias

                    // add an empty
                    lea_path.emplace_back("");
                    return make_ident_info_from_frame(
                        use_allocated_register_type, f, src_loc_tk, ident, id,
                        std::move(lea_path));
                }

                // this is an alias, continue resolving until it is a variable,
                // register or constant

                const alias_info& alias{f.get_alias(id.base())};

                lea_path.emplace_back(alias.lea);

                ident_path new_id{std::string{alias.to}};

                // big note: the fishy resizing of the 'lea_path' happens when
                //           the 'new_id' extended past fields that do not need
                //           lea
                //           if 'lea_path' is not extended then the types, id
                //           path elements and lea path vectors are not in sync

                const size_t nid_sz{new_id.path().size()};
                const size_t lea_sz{lea_path.size()};
                if ((nid_sz > lea_sz) and (nid_sz - lea_sz > 1)) {
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

        if (const ident_info reg_info{make_ident_info_register_or_empty(
                use_allocated_register_type, ident, id)};
            not reg_info.is_empty()) {

            return reg_info;
        }

        return make_ident_info_const_or_empty(src_loc_tk, ident, id);
    }

    [[nodiscard]] auto make_ident_info_from_frame(
        const bool use_allocated_register_type, const frame& frm,
        const token& src_loc_tk, const std::string_view ident,
        const ident_path& id, std::vector<std::string> lea_path) const
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

        // try register
        if (const ident_info reg_info{make_ident_info_register_or_empty(
                use_allocated_register_type, ident, id)};
            not reg_info.is_empty()) {

            return reg_info;
        }

        // try constant
        return make_ident_info_const_or_empty(src_loc_tk, ident, id);
    }

    [[nodiscard]] static auto make_ident_info_from_var_info(
        const token& src_loc_tk, const std::string_view ident,
        const ident_path& id, const var_info& var,
        std::vector<std::string> lea_path) -> ident_info {

        ident_info ii{
            var.type_ptr->accessor(src_loc_tk, ident, id.path(), var)};

        lea_path.resize(id.path().size());
        // note: pad with empty for the remaining elements in the id path

        std::ranges::reverse(lea_path);
        // note: reverse it since it was constructed while traversing
        //       upwards in the frame stack but 'elem_path' and 'type_path'
        //       are ordered from the top down

        ii.lea_path = lea_path;

        if (not ii.type_ref().is_built_in()) {
            return ii;
        }

        // identifier is built-in type

        // find the first element from the top that has a 'lea' and get
        // accessor relative to that

        std::string lea;
        size_t lea_index{ii.elem_path.size()};
        while (lea_index--) {
            if (not ii.lea_path[lea_index].empty()) {
                lea = ii.lea_path[lea_index];
                break;
            }
        }

        if (lea.empty()) {
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

        ii.operand = {lea, false};
        if (offset != 0) {
            ii.operand.displacement += static_cast<int32_t>(offset);
        }
        ii.operand.size = ii.type_ref().size();

        return ii;
    }

    [[nodiscard]] auto make_ident_info_register_or_empty(
        const bool use_allocated_register_type, const std::string_view ident,
        const ident_path& id) const -> ident_info {

        // is it a register?
        if (const size_t reg_size{operand::register_size(id.str())};
            reg_size != 0) {

            if (use_allocated_register_type) {
                const x86& x{machine()};

                return make_ident_info_register(
                    ident, id.str(), x.get_allocated_register_type(id.str()));
            }

            return make_ident_info_register(
                ident, id.str(), get_builtin_type_for_size(reg_size));
        }

        // not resolved, return empty info
        return ident_info::make_empty();
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
    make_ident_info_or_throw(const bool use_allocated_register_type,
                             const token& src_loc_tk,
                             const std::string_view ident) const -> ident_info {

        const ident_info id_info{make_ident_info_or_empty(
            use_allocated_register_type, src_loc_tk, ident)};

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

#pragma clang diagnostic pop
};
