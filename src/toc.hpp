#pragma once
// review: 2025-09-29

#include <algorithm>
#include <cstdint>
#include <format>
#include <iostream>
#include <optional>
#include <ranges>
#include <string_view>

#include "compiler_exception.hpp"
#include "lut.hpp"
#include "panic_exception.hpp"
#include "statement.hpp"
#include "type.hpp"

class stmt_def_func;
class stmt_def_field;
class stmt_def_type;

struct func_return_info {
    token type_tk;  // type token
    token ident_tk; // identifier token
    const type* type_ref{};
};

struct var_info {
    std::string_view name;
    const type* type_ref{};
    token declared_at_tk; // token for position in source
    int32_t stack_idx{};  // location relative to rsp
};

class frame final {
    // optional name
    std::string_view name_;

    // a unique path of source locations of the in-lined call
    std::string call_path_;

    // number of bytes used on the stack by this frame
    size_t allocated_stack_{};

    // variables declared in this frame
    lut<var_info> vars_;

    // aliases that refers to previous frame(s) alias or variable
    lut<std::string> aliases_;

    // the label to jump to when exiting a in-lined function
    std::string func_ret_label_;

    // info about the function returns
    const std::vector<func_return_info>& func_rets_;

  public:
    enum class frame_type : uint8_t { FUNC, BLOCK, LOOP };

  private:
    frame_type type_{frame_type::FUNC}; // frame type

  public:
    frame(std::string_view name, const frame_type frm_type,
          const std::vector<func_return_info>& func_rets = {},
          std::string call_path = "", std::string func_ret_label = "") noexcept
        : name_{name}, call_path_{std::move(call_path)},
          func_ret_label_{std::move(func_ret_label)}, func_rets_{func_rets},
          type_{frm_type} {}

    auto add_var(token declared_at_tk, std::string_view name,
                 const type& type_ref, const bool is_array,
                 const size_t array_size, const int stack_idx) -> void {

        if (stack_idx < 0) {
            // variable, increase allocated stack size
            allocated_stack_ += type_ref.size() * (is_array ? array_size : 1);
        }

        vars_.put(std::string{name}, {.name = std::string{name},
                                      .type_ref = &type_ref,
                                      .declared_at_tk = declared_at_tk,
                                      .stack_idx = stack_idx});
    }

    [[nodiscard]] auto allocated_stack_size() const -> size_t {
        return allocated_stack_;
    }

    [[nodiscard]] auto has_var(std::string_view name) const -> bool {
        return vars_.has(name);
    }

    auto get_var_ref(const std::string& name) -> var_info& {
        return vars_.get_ref(name);
    }

    [[nodiscard]] auto get_var_const_ref(std::string_view name) const
        -> const var_info& {
        return vars_.get_const_ref(name);
    }

    auto add_alias(std::string_view from, std::string to) -> void {
        aliases_.put(std::string{from}, std::move(to));
    }

    [[nodiscard]] auto is_func() const -> bool {
        return type_ == frame_type::FUNC;
    }

    [[nodiscard]] auto is_block() const -> bool {
        return type_ == frame_type::BLOCK;
    }

    [[nodiscard]] auto is_loop() const -> bool {
        return type_ == frame_type::LOOP;
    }

    [[nodiscard]] auto is_name(std::string_view name) const -> bool {
        return name_ == name;
    }

    [[nodiscard]] auto has_alias(const std::string& name) const -> bool {
        return aliases_.has(name);
    }

    [[nodiscard]] auto get_alias(const std::string& name) const
        -> const std::string& {
        return aliases_.get_const_ref(name);
    }

    [[nodiscard]] auto name() const -> std::string_view { return name_; }

    [[nodiscard]] auto func_ret_label() const -> const std::string& {
        return func_ret_label_;
    }

    [[nodiscard]] auto call_path() const -> const std::string& {
        return call_path_;
    }

    [[nodiscard]] auto get_func_returns_infos() const
        -> const std::vector<func_return_info>& {
        return func_rets_;
    }
};

struct field_info {
    const stmt_def_field* def{};
    token declared_at_tk; // token for position in source
    bool is_str{};
};

struct func_info {
    const stmt_def_func* def{};
    token declared_at_tk; // token for position in source
    const type* type_ref;
};

struct type_info {
    const stmt_def_type* def{};
    token declared_at_tk;
    const type& type_ref;
};

struct ident_info {
    enum class ident_type : uint8_t { CONST, VAR, REGISTER, FIELD, IMPLIED };

    const std::string id;
    const std::string id_nasm; // NASM valid source
    const int64_t const_value{};
    const type& type_ref;
    const int32_t stack_ix;
    const ident_type ident_type{ident_type::CONST};

    [[nodiscard]] auto is_const() const -> bool {
        return ident_type == ident_type::CONST;
    }

    [[nodiscard]] auto is_register() const -> bool {
        return ident_type == ident_type::REGISTER;
    }
};

class identifier final {
    std::string_view id_;
    std::vector<std::string> path_;

  public:
    explicit identifier(std::string_view id) : id_{id} {
        auto parts{id_ | std::views::split('.')};
        for (auto part : parts) {
            path_.emplace_back(part.begin(), part.end());
        }
    }

    ~identifier() = default;

    identifier() = default;
    identifier(identifier&&) = default;
    identifier(const identifier&) = default;
    auto operator=(const identifier&) -> identifier& = default;
    auto operator=(identifier&&) -> identifier& = default;

    [[nodiscard]] auto id_base() const -> const std::string& {
        return path_.at(0);
    }

    [[nodiscard]] auto path() const -> const std::vector<std::string>& {
        return path_;
    }

    auto set_base(std::string name) -> void { path_[0] = std::move(name); }
};

class toc final {
    const std::string& source_;
    std::vector<frame> frames_;
    std::vector<std::string> all_registers_{
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
        "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"};
    size_t all_registers_initial_size_{all_registers_.size()};
    std::vector<std::string> named_registers_{"rax", "rbx", "rcx", "rdx",
                                              "rsi", "rdi", "rbp"};
    size_t named_registers_initial_size_{named_registers_.size()};
    std::vector<std::string> scratch_registers_{"r8",  "r9",  "r10", "r11",
                                                "r12", "r13", "r14", "r15"};
    size_t scratch_registers_initial_size_{scratch_registers_.size()};
    std::vector<std::string> allocated_registers_;
    std::vector<std::string> allocated_registers_src_locs_; // source locations
    lut<field_info> fields_;
    lut<func_info> funcs_;
    lut<const type&> types_;
    const type* type_void_{};
    const type* type_default_{};
    const type* type_bool_{};
    size_t usage_max_scratch_regs_{};
    size_t usage_max_frame_count_{};
    size_t usage_max_stack_size_{};
    const std::regex regex_ws{R"(\s+)"};

  public:
    explicit toc(const std::string& source) : source_{source} {}

    toc() = delete;
    toc(const toc&) = default;
    toc(toc&&) = default;
    auto operator=(const toc&) -> toc& = delete;
    auto operator=(toc&&) -> toc& = delete;

    ~toc() = default;

    auto add_field(const token& src_loc_tk, std::string_view name,
                   const stmt_def_field* fld_def, const bool is_str_field)
        -> void {

        if (fields_.has(name)) {
            throw compiler_exception{
                src_loc_tk,
                std::format("field '{}' already defined at {}", name,
                            source_location_hr(
                                fields_.get_const_ref(name).declared_at_tk))};
        }

        fields_.put(std::string{name}, {.def = fld_def,
                                        .declared_at_tk = src_loc_tk,
                                        .is_str = is_str_field});
    }

    auto add_func(const token& src_loc_tk, std::string_view name,
                  const type& return_type, const stmt_def_func* func_def)
        -> void {

        if (funcs_.has(name)) {
            const func_info& fn{funcs_.get_const_ref(name)};
            throw compiler_exception{
                src_loc_tk,
                std::format("function '{}' already defined at {}", name,
                            source_location_hr(fn.declared_at_tk))};
        }

        funcs_.put(std::string{name}, {.def = func_def,
                                       .declared_at_tk = src_loc_tk,
                                       .type_ref = &return_type});
    }

    [[nodiscard]] auto is_func(const std::string& name) const -> bool {
        return funcs_.has(name);
    }

    [[nodiscard]] auto get_func_or_throw(const token& src_loc_tk,
                                         std::string_view name) const
        -> const stmt_def_func& {

        if (not funcs_.has(name)) {
            throw compiler_exception{
                src_loc_tk, std::format("function '{}' not found", name)};
        }

        return *funcs_.get_const_ref(name).def;
    }

    [[nodiscard]] auto is_func_builtin(const token& src_loc_tk,
                                       std::string_view name) const -> bool {

        if (not funcs_.has(name)) {
            throw compiler_exception{
                src_loc_tk, std::format("function '{}' not found", name)};
        }

        return funcs_.get_const_ref(name).def == nullptr;
    }

    [[nodiscard]] auto
    get_func_return_type_or_throw(const token& src_loc_tk,
                                  std::string_view name) const -> const type& {

        if (not funcs_.has(name)) {
            throw compiler_exception{
                src_loc_tk, std::format("function '{}' not found", name)};
        }

        return *funcs_.get_const_ref(name).type_ref;
    }

    auto add_type(const token& src_loc_tk, const type& tpe) -> void {
        if (types_.has(tpe.name())) {
            //? todo. specify where the type has been defined
            throw compiler_exception{
                src_loc_tk,
                std::format("type '{}' already defined", tpe.name())};
        }

        types_.put(std::string{tpe.name()}, tpe);
    }

    [[nodiscard]] auto get_type_or_throw(const token& src_loc_tk,
                                         std::string_view name) const
        -> const type& {
        std::string const name_str{name};
        if (not types_.has(name_str)) {
            throw compiler_exception{src_loc_tk,
                                     std::format("type '{}' not found", name)};
        }

        return types_.get_const_ref(name_str);
    }

    [[nodiscard]] auto
    source_location_for_use_in_label(const token& src_loc_tk) const
        -> std::string {
        const auto [line, col]{line_and_col_num_for_char_index(
            src_loc_tk.at_line(), src_loc_tk.start_index(), source_)};

        return std::format("{}_{}", line, col);
    }

    // human readable source location
    [[nodiscard]] auto source_location_hr(const token& src_loc_tk) const
        -> std::string {
        const auto [line, col]{line_and_col_num_for_char_index(
            src_loc_tk.at_line(), src_loc_tk.start_index(), source_)};

        return std::format("{}:{}", line, col);
    }

    static auto get_field_offset_in_type(const token& src_loc_tk_,
                                         const type& tp,
                                         std::string_view field_name)
        -> size_t {
        size_t accum{};
        for (const type_field& f : tp.fields()) {
            if (f.name == field_name) {
                return accum;
            }
            accum += f.size;
        }
        throw compiler_exception{src_loc_tk_,
                                 "unexpected code path stmt_assign_var:1"};
    }

    static auto line_and_col_num_for_char_index(const size_t at_line,
                                                size_t char_index_in_source,
                                                const std::string& src)
        -> std::pair<size_t, size_t> {
        size_t at_col{0};
        while (src[char_index_in_source] != '\n') {
            at_col++;
            if (char_index_in_source == 0) {
                break;
            }
            char_index_in_source--;
        }

        return {at_line, at_col};
    }

    auto finish(std::ostream& os) -> void {
        os << "\n; max scratch registers in use: " << usage_max_scratch_regs_
           << '\n';
        os << ";            max frames in use: " << usage_max_frame_count_
           << '\n';
        os << ";               max stack size: " << usage_max_stack_size_
           << " B\n";
        assert(all_registers_.size() == all_registers_initial_size_);
        assert(allocated_registers_.empty());
        assert(allocated_registers_src_locs_.empty());
        assert(frames_.empty());
        assert(named_registers_.size() == named_registers_initial_size_);
        assert(scratch_registers_.size() == scratch_registers_initial_size_);
        usage_max_frame_count_ = 0;
        usage_max_scratch_regs_ = 0;
    }

    [[nodiscard]] auto
    make_ident_info(const statement& st,
                    [[maybe_unused]] const bool must_be_initiated) const
        -> ident_info {
        return make_ident_info_or_throw(st.tok(), st.identifier(),
                                        must_be_initiated);
    }

    [[nodiscard]] auto
    make_ident_info(const token& src_loc_tk, std::string_view ident,
                    [[maybe_unused]] const bool must_be_initiated) const
        -> ident_info {
        return make_ident_info_or_throw(src_loc_tk, ident, must_be_initiated);
    }

  private:
    // helper: call make_ident_info_or_empty and throw if unresolved
    [[nodiscard]] auto make_ident_info_or_throw(
        const token& src_loc_tk, std::string_view ident,
        [[maybe_unused]] const bool must_be_initiated) const -> ident_info {
        const ident_info id_info{make_ident_info_or_empty(src_loc_tk, ident)};
        if (not id_info.id_nasm.empty()) {
            return id_info;
        }

        throw compiler_exception{
            src_loc_tk, std::format("cannot resolve identifier '{}'", ident)};
    }

  public:
    // note: 'to' is alias or variable in parent frame
    auto add_alias(std::string_view from, std::string to) -> void {
        frames_.back().add_alias(from, std::move(to));
    }

    auto enter_func(std::string_view name,
                    const std::vector<func_return_info>& returns = {},
                    const std::string& call_path = "",
                    const std::string& return_jmp_label = "") -> void {
        frames_.emplace_back(name, frame::frame_type::FUNC, returns, call_path,
                             return_jmp_label);
        refresh_usage();
    }

    auto enter_block() -> void {
        frames_.emplace_back("", frame::frame_type::BLOCK);
        refresh_usage();
    }

    auto enter_loop(const std::string& name) -> void {
        frames_.emplace_back(name, frame::frame_type::LOOP);
        refresh_usage();
    }

    auto exit_func(std::string_view name) -> void {
        const frame& frm{frames_.back()};
        assert(frm.is_func() and frm.is_name(name));
        frames_.pop_back();
    }

    auto exit_loop(const std::string& name) -> void {
        const frame& frm{frames_.back()};
        assert(frm.is_loop() and frm.is_name(name));
        frames_.pop_back();
    }

    auto exit_block() -> void {
        const frame& frm{frames_.back()};
        assert(frm.is_block());
        frames_.pop_back();
    }

    auto add_var(const token& src_loc_tk, std::ostream& os, size_t indnt,
                 std::string_view name, const type& var_type,
                 const bool is_array, const size_t array_size) -> void {
        // check if variable is already declared in this scope
        if (frames_.back().has_var(name)) {
            const var_info& var{frames_.back().get_var_const_ref(name)};
            throw compiler_exception{
                src_loc_tk,
                std::format("variable '{}' already declared at {}", name,
                            source_location_hr(var.declared_at_tk))};
        }

        // check if variable shadows previously declared variable
        const auto [id, frm]{get_id_and_frame_for_identifier(name)};
        if (not id.empty()) {
            const var_info& var{frm.get_var_const_ref(id)};
            throw compiler_exception{
                src_loc_tk,
                std::format("variable '{}' shadows variable declared at {}",
                            name, source_location_hr(var.declared_at_tk))};
        }

        const int stack_idx{
            static_cast<int>(get_current_function_stack_size() +
                             (var_type.size() * (is_array ? array_size : 1)))};

        frames_.back().add_var(src_loc_tk, name, var_type, is_array, array_size,
                               -stack_idx);

        const size_t total_stack_size{get_total_stack_size()};
        usage_max_stack_size_ =
            std::max(total_stack_size, usage_max_stack_size_);

        // comment the resolved name
        const ident_info& name_info{make_ident_info(src_loc_tk, name, false)};
        indent(os, indnt, true);
        os << "var " << name << ": " << name_info.type_ref.name();
        if (array_size) {
            os << '[' << array_size << ']';
        }
        os << " @ " << name_info.id_nasm << '\n';
    }

    auto alloc_scratch_register(const token& src_loc_tk, std::ostream& os,
                                const size_t indnt) -> std::string {
        if (scratch_registers_.empty()) {
            throw compiler_exception{src_loc_tk,
                                     "out of scratch registers. try to reduce "
                                     "expression complexity"};
        }

        std::string reg{std::move(scratch_registers_.back())};
        scratch_registers_.pop_back();

        indent(os, indnt, true);
        os << "allocate scratch register -> " << reg << "\n";

        const size_t n{scratch_registers_initial_size_ -
                       scratch_registers_.size()};
        usage_max_scratch_regs_ = std::max(n, usage_max_scratch_regs_);

        allocated_registers_.emplace_back(std::move(reg));
        allocated_registers_src_locs_.emplace_back(
            source_location_hr(src_loc_tk));

        return allocated_registers_.back();
    }

    auto alloc_named_register_or_throw(const statement& st, std::ostream& os,
                                       const size_t indnt,
                                       const std::string& reg) -> void {
        indent(os, indnt, true);
        os << "allocate named register '" << reg << "'\n";

        auto reg_iter{std::ranges::find(named_registers_, reg)};
        if (reg_iter == named_registers_.end()) {
            // not found
            std::string loc;
            const size_t n{allocated_registers_.size()};
            for (size_t i{}; i < n; i++) {
                if (allocated_registers_.at(i) == reg) {
                    loc = allocated_registers_src_locs_.at(i);
                    break;
                }
            }
            throw compiler_exception{
                st.tok(), std::format("cannot allocate register '{}' because "
                                      "it was allocated at {}",
                                      reg, loc)};
        }

        allocated_registers_.emplace_back(std::move(*reg_iter));
        allocated_registers_src_locs_.emplace_back(
            source_location_hr(st.tok()));
        named_registers_.erase(reg_iter);
    }

    auto alloc_named_register(const token& src_loc_tk, std::ostream& os,
                              const size_t indnt, const std::string& reg)
        -> bool {
        indent(os, indnt, true);
        os << "allocate named register '" << reg << '\'';

        auto reg_iter{std::ranges::find(named_registers_, reg)};
        if (reg_iter == named_registers_.end()) {
            os << ": not available\n";
            return false;
        }

        os << '\n';

        allocated_registers_.emplace_back(std::move(*reg_iter));
        allocated_registers_src_locs_.emplace_back(
            source_location_hr(src_loc_tk));
        named_registers_.erase(reg_iter);

        return true;
    }

    auto free_named_register(std::ostream& os, const size_t indnt,
                             const std::string& reg) -> void {
        indent(os, indnt, true);
        os << "free named register '" << reg << "'\n";

        assert(allocated_registers_.back() == reg);

        named_registers_.emplace_back(std::move(allocated_registers_.back()));
        allocated_registers_.pop_back();
        allocated_registers_src_locs_.pop_back();
    }

    auto free_scratch_register(std::ostream& os, const size_t indnt,
                               const std::string& reg) -> void {
        indent(os, indnt, true);
        os << "free scratch register '" << reg << "'\n";

        assert(allocated_registers_.back() == reg);

        scratch_registers_.emplace_back(std::move(allocated_registers_.back()));
        allocated_registers_.pop_back();
        allocated_registers_src_locs_.pop_back();
    }

    [[nodiscard]] auto get_loop_label_or_throw(const token& src_loc_tk) const
        -> std::string_view {
        for (const auto& frm : frames_ | std::views::reverse) {
            if (frm.is_loop()) {
                return frm.name();
            }
            if (frm.is_func()) {
                throw compiler_exception{src_loc_tk, "not in a loop"};
            }
        }

        throw compiler_exception{src_loc_tk, "unexpected frames"};
    }

    [[nodiscard]] auto get_call_path(const token& src_loc_tk) const
        -> const std::string& {
        for (const auto& frm : frames_ | std::views::reverse) {
            if (frm.is_func()) {
                return frm.call_path();
            }
        }

        throw compiler_exception{src_loc_tk, "not in a function"};
    }

    [[nodiscard]] auto
    get_func_return_label_or_throw(const token& src_loc_tk) const
        -> const std::string& {
        for (const auto& frm : frames_ | std::views::reverse) {
            if (frm.is_func()) {
                return frm.func_ret_label();
            }
        }

        throw compiler_exception{src_loc_tk, "not in a function"};
    }

    [[nodiscard]] auto get_func_returns(const token& src_loc_tk) const
        -> const std::vector<func_return_info>& {
        for (const auto& frm : frames_ | std::views::reverse) {
            if (frm.is_func()) {
                return frm.get_func_returns_infos();
            }
        }

        throw compiler_exception{src_loc_tk, "not in a function"};
    }

    auto comment_source(const statement& st, std::ostream& os,
                        const size_t indnt) const -> void {
        const token& tk{st.tok()};
        const auto [line, col]{line_and_col_num_for_char_index(
            tk.at_line(), tk.start_index(), source_)};

        indent(os, indnt, true);
        os << "[" << line << ":" << col << "] ";

        std::stringstream ss;
        st.source_to(ss);
        std::string res{std::regex_replace(ss.str(), regex_ws, " ")};
        // trim end of string
        if (not res.empty() && res.back() == ' ') {
            res.pop_back();
        }
        os << res << "\n";
    }

    auto comment_source(std::ostream& os, const std::string& dst,
                        const std::string& op, const statement& st) const
        -> void {
        const token& tk{st.tok()};
        const auto [line, col]{line_and_col_num_for_char_index(
            tk.at_line(), tk.start_index(), source_)};

        os << "[" << line << ":" << col << "]";

        std::stringstream ss;
        ss << " " << dst << " " << op << " ";
        st.source_to(ss);
        std::string res{std::regex_replace(ss.str(), regex_ws, " ")};
        // trim end of string
        if (not res.empty() && res.back() == ' ') {
            res.pop_back();
        }
        os << res << '\n';
    }

    auto comment_token(std::ostream& os, const token& tk) const -> void {
        const auto [line, col]{line_and_col_num_for_char_index(
            tk.at_line(), tk.start_index(), source_)};

        os << "[" << line << ":" << col << "]";
        os << " " << tk.name() << '\n';
    }

    [[nodiscard]] auto is_identifier_register(std::string_view id) const
        -> bool {
        return std::ranges::find(all_registers_, id) != all_registers_.end();
    }

    [[nodiscard]] auto is_identifier_direct_register_indirect_addressing(
        const std::string& id) const -> bool {
        if (auto expr{toc::extract_between_brackets(id)}; expr) {
            const std::string reg{
                extract_base_register_from_indirect_addressing(*expr)};
            if (is_identifier_register(reg)) {
                return true;
            }
        }

        return false;
    }

  private:
    static auto
    extract_base_register_from_indirect_addressing(std::string_view addressing)
        -> std::string_view {

        auto pos{addressing.find_first_of(" +")};
        if (pos == std::string_view::npos) {
            return addressing;
        }

        return addressing.substr(0, pos);
    }

  public:
    auto asm_cmd(const token& src_loc_tk, std::ostream& os, size_t indnt,
                 std::string_view op, std::string_view dst_nasm,
                 std::string_view src_nasm) -> void {
        if (op == "mov" and dst_nasm == src_nasm) {
            return;
        }

        const size_t dst_size{get_size_from_operand(src_loc_tk, dst_nasm)};
        const size_t src_size{get_size_from_operand(src_loc_tk, src_nasm)};

        if (dst_size == src_size) {
            if (not(is_operand_memory(dst_nasm) and
                    is_operand_memory(src_nasm))) {
                // both operands are not memory references
                indent(os, indnt);
                os << op << " " << dst_nasm << ", " << src_nasm << '\n';
                return;
            }

            // both operands are memory references
            // use scratch register for transfer
            const std::string& reg{
                alloc_scratch_register(src_loc_tk, os, indnt)};
            const std::string& reg_sized{
                get_register_operand_for_size(src_loc_tk, reg, src_size)};
            indent(os, indnt);
            os << "mov " << reg_sized << ", " << src_nasm << '\n';
            indent(os, indnt);
            os << op << " " << dst_nasm << ", " << reg_sized << '\n';
            free_scratch_register(os, indnt, reg);
            return;
        }

        if (dst_size > src_size) {
            // mov rax,byte[b] -> movsx
            if (not(is_operand_memory(dst_nasm) and
                    is_operand_memory(src_nasm))) {
                // not both operands memory references
                if (op == "mov") {
                    indent(os, indnt);
                    os << "movsx " << dst_nasm << ", " << src_nasm << '\n';
                    return;
                }
            }

            // both operands refer to memory
            // use scratch register for transfer
            const std::string& reg{
                alloc_scratch_register(src_loc_tk, os, indnt)};
            const std::string& reg_sized{
                get_register_operand_for_size(src_loc_tk, reg, dst_size)};
            indent(os, indnt);
            os << "movsx " << reg_sized << ", " << src_nasm << '\n';
            indent(os, indnt);
            os << op << " " << dst_nasm << ", " << reg_sized << '\n';
            free_scratch_register(os, indnt, reg);
            return;
        }

        // dst_size < src_size
        //? truncation might change value of signed number
        if (is_identifier_register(src_nasm)) {
            const std::string& reg_sized{
                get_register_operand_for_size(src_loc_tk, src_nasm, dst_size)};
            indent(os, indnt);
            os << op << " " << dst_nasm << ", " << reg_sized << '\n';
            return;
        }

        if (is_operand_memory(src_nasm)) {
            //? todo. this displays nasm identifiers but should be human
            // readable identifiers
            throw compiler_exception{
                src_loc_tk, std::format("cannot move '{}' to '{}' because "
                                        "it would be truncated",
                                        src_nasm, dst_nasm)};
        }

        // constant
        indent(os, indnt);
        os << op << " " << dst_nasm << ", " << src_nasm << '\n';
    }

    auto set_type_void(const type& tpe) -> void { type_void_ = &tpe; }

    [[nodiscard]] auto get_type_void() const -> const type& {
        return *type_void_;
    }

    auto set_type_default(const type& tpe) -> void { type_default_ = &tpe; }

    [[nodiscard]] auto get_type_default() const -> const type& {
        return *type_default_;
    }

    auto set_type_bool(const type& tpe) -> void { type_bool_ = &tpe; }

    [[nodiscard]] auto get_type_bool() const -> const type& {
        return *type_bool_;
    }

    [[nodiscard]] auto get_size_from_operand(const token& src_loc_tk,
                                             std::string_view operand) const
        -> size_t {
        //? sort of ugly
        if (operand.starts_with("qword")) {
            return 8;
        }
        if (operand.starts_with("dword")) {
            return 4;
        }
        if (operand.starts_with("word")) {
            return 2;
        }
        if (operand.starts_with("byte")) {
            return 1;
        }
        if (is_identifier_register(operand)) {
            return get_size_from_operand_register(src_loc_tk, operand);
        }

        // constant
        return get_type_default().size();
    }

    [[nodiscard]] auto
    get_builtin_type_for_operand(const token& src_loc_tk_,
                                 const std::string& operand) const
        -> const type& {
        //? sort of ugly
        if (operand.starts_with("qword")) {
            return get_type_or_throw(src_loc_tk_, "i64");
        }
        if (operand.starts_with("dword")) {
            return get_type_or_throw(src_loc_tk_, "i32");
        }
        if (operand.starts_with("word")) {
            return get_type_or_throw(src_loc_tk_, "i16");
        }
        if (operand.starts_with("byte")) {
            return get_type_or_throw(src_loc_tk_, "i8");
        }

        throw panic_exception{"toc:1"};
    }

    // -------------------------------------------------------------------------
    // statics
    // -------------------------------------------------------------------------

    // pragma below for clang++ to not generate warning stemming from
    // 'std::from_chars' requiring pointers
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
    static auto parse_to_constant(std::string_view str)
        -> std::optional<int64_t> {
        // is it hex?
        if (str.starts_with("0x") or str.starts_with("0X")) { // hex
            int64_t value{};
            std::string_view sv{str};
            sv.remove_prefix(2); // skip "0x" or "0X"
            auto result{
                std::from_chars(sv.data(), sv.data() + sv.size(), value, 16)};
            if (result.ec == std::errc{}) {
                return value;
            }
        }

        // is it binary?
        if (str.starts_with("0b") or str.starts_with("0B")) { // binary
            int64_t value{};
            std::string_view sv{str};
            sv.remove_prefix(2); // skip "0b" or "0B"
            auto result{
                std::from_chars(sv.data(), sv.data() + sv.size(), value, 2)};
            if (result.ec == std::errc{}) {
                return value;
            }
        }

        // try decimal digit
        {
            int64_t value{};
            const std::string_view sv{str};
            // note: using 'std::string_view' for 'clang-tidy' to not
            // trigger
            //       warning
            //       'cppcoreguidelines-pro-bounds-pointer-arithmetic'
            auto parse_result{
                std::from_chars(sv.data(), sv.data() + sv.size(), value)};
            if (parse_result.ec == std::errc{}) {
                return value;
            }
        }

        return std::nullopt;
    }
#pragma clang diagnostic pop

    static auto get_size_from_operand_register(const token& src_loc_tk,
                                               std::string_view operand)
        -> size_t {
        if (operand == "rax" || operand == "rbx" || operand == "rcx" ||
            operand == "rdx" || operand == "rbp" || operand == "rsi" ||
            operand == "rdi" || operand == "rsp" || operand == "r8" ||
            operand == "r9" || operand == "r10" || operand == "r11" ||
            operand == "r12" || operand == "r13" || operand == "r14" ||
            operand == "r15") {
            return 8;
        }

        if (operand == "eax" || operand == "ebx" || operand == "ecx" ||
            operand == "edx" || operand == "ebp" || operand == "esi" ||
            operand == "edi" || operand == "esp" || operand == "r8d" ||
            operand == "r9d" || operand == "r10d" || operand == "r11d" ||
            operand == "r12d" || operand == "r13d" || operand == "r14d" ||
            operand == "r15d") {
            return 4;
        }

        if (operand == "ax" || operand == "bx" || operand == "cx" ||
            operand == "dx" || operand == "bp" || operand == "si" ||
            operand == "di" || operand == "sp" || operand == "r8w" ||
            operand == "r9w" || operand == "r10w" || operand == "r11w" ||
            operand == "r12w" || operand == "r13w" || operand == "r14w" ||
            operand == "r15w") {
            return 2;
        }

        if (operand == "al" || operand == "ah" || operand == "bl" ||
            operand == "bh" || operand == "cl" || operand == "ch" ||
            operand == "dl" || operand == "dh" || operand == "r8b" ||
            operand == "r9b" || operand == "r10b" || operand == "r11b" ||
            operand == "r12b" || operand == "r13b" || operand == "r14b" ||
            operand == "r15b") {
            return 1;
        }

        throw compiler_exception{src_loc_tk,
                                 std::format("unknown register '{}'", operand)};
    }

    static auto get_register_operand_for_size(const token& src_loc_tk,
                                              std::string_view operand,
                                              size_t size) -> std::string {
        //? sort of ugly
        if (operand == "rax") {
            switch (size) {
            case 8:
                return "rax";
            case 4:
                return "eax";
            case 2:
                return "ax";
            case 1:
                return "al";
            default:
                throw compiler_exception{
                    src_loc_tk,
                    std::format("illegal size {} for register operand '{}'",
                                size, operand)};
            }
        }
        if (operand == "rbx") {
            switch (size) {
            case 8:
                return "rbx";
            case 4:
                return "ebx";
            case 2:
                return "bx";
            case 1:
                return "bl";
            default:
                throw compiler_exception{
                    src_loc_tk, std::format("illegal size {} for register '{}'",
                                            size, operand)};
            }
        }
        if (operand == "rcx") {
            switch (size) {
            case 8:
                return "rcx";
            case 4:
                return "ecx";
            case 2:
                return "cx";
            case 1:
                return "cl";
            default:
                throw compiler_exception{
                    src_loc_tk, std::format("illegal size {} for register '{}'",
                                            size, operand)};
            }
        }
        if (operand == "rdx") {
            switch (size) {
            case 8:
                return "rdx";
            case 4:
                return "edx";
            case 2:
                return "dx";
            case 1:
                return "dl";
            default:
                throw compiler_exception{
                    src_loc_tk, std::format("illegal size {} for register '{}'",
                                            size, operand)};
            }
        }
        if (operand == "rbp") {
            switch (size) {
            case 8:
                return "rbp";
            case 4:
                return "ebp";
            case 2:
                return "bp";
            default:
                throw compiler_exception{
                    src_loc_tk, std::format("illegal size {} for register '{}'",
                                            size, operand)};
            }
        }
        if (operand == "rsi") {
            switch (size) {
            case 8:
                return "rsi";
            case 4:
                return "esi";
            case 2:
                return "si";
            default:
                throw compiler_exception{
                    src_loc_tk, std::format("illegal size {} for register '{}'",
                                            size, operand)};
            }
        }
        if (operand == "rdi") {
            switch (size) {
            case 8:
                return "rdi";
            case 4:
                return "edi";
            case 2:
                return "di";
            default:
                throw compiler_exception{
                    src_loc_tk, std::format("illegal size {} for register '{}'",
                                            size, operand)};
            }
        }
        if (operand == "rsp") {
            switch (size) {
            case 8:
                return "rsp";
            case 4:
                return "esp";
            case 2:
                return "sp";
            default:
                throw compiler_exception{
                    src_loc_tk, std::format("illegal size {} for register '{}'",
                                            size, operand)};
            }
        }

        //? todo. move this to a static
        const std::regex rx{R"(r(\d+))"};
        std::smatch match;
        std::string const operand_str{operand};
        if (not std::regex_search(operand_str, match, rx)) {
            throw compiler_exception{
                src_loc_tk, std::format("unknown register {}", operand)};
        }
        const std::string rnbr{match[1]};
        switch (size) {
        case 8:
            return std::format("r{}", rnbr);
        case 4:
            return std::format("r{}d", rnbr);
        case 2:
            return std::format("r{}w", rnbr);
        case 1:
            return std::format("r{}b", rnbr);
        default:
            throw compiler_exception{
                src_loc_tk, std::format("illegal size {} for register '{}'",
                                        size, operand)};
        }
    }

    static auto asm_push([[maybe_unused]] const token& src_loc_tk,
                         std::ostream& os, const size_t indnt,
                         const std::string& operand) -> void {
        indent(os, indnt);
        os << "push " << operand << '\n';
    }

    static auto asm_pop([[maybe_unused]] const token& src_loc_tk,
                        std::ostream& os, const size_t indnt,
                        const std::string& operand) -> void {
        indent(os, indnt);
        os << "pop " << operand << '\n';
    }

    static auto asm_jmp([[maybe_unused]] const token& src_loc_tk,
                        std::ostream& os, const size_t indnt,
                        const std::string& label) -> void {
        indent(os, indnt);
        os << "jmp " << label << '\n';
    }

    static auto asm_label([[maybe_unused]] const token& src_loc_tk,
                          std::ostream& os, const size_t indnt,
                          const std::string& label) -> void {
        indent(os, indnt);
        os << label << ":\n";
    }

    static auto asm_neg([[maybe_unused]] const token& src_loc_tk,
                        std::ostream& os, const size_t indnt,
                        const std::string& operand) -> void {
        indent(os, indnt);
        os << "neg " << operand << '\n';
    }

    static auto asm_not([[maybe_unused]] const token& src_loc_tk,
                        std::ostream& os, const size_t indnt,
                        const std::string& operand) -> void {
        indent(os, indnt);
        os << "not " << operand << '\n';
    }

    static auto asm_rep_stosb([[maybe_unused]] const token& src_loc_tk,
                              std::ostream& os, const size_t indnt) -> void {
        indent(os, indnt);
        os << "rep stosb\n";
    }

    static auto asm_rep_movs([[maybe_unused]] const token& src_loc_tk,
                             std::ostream& os, const size_t indnt, char size)
        -> void {
        indent(os, indnt);
        os << "rep movs" << size << '\n';
    }

    static auto indent(std::ostream& os, const size_t indnt,
                       const bool comment = false) -> void {
        if (comment) {
            os << ';';
        }
        if (indnt == 0) {
            return;
        }
        for (size_t i{}; i < indnt; i++) {
            os << "    ";
        }
    }

  private:
    // returns empty id and frame[0] if 'ident' not found
    auto get_id_and_frame_for_identifier(std::string_view ident)
        -> std::pair<std::string, frame&> {
        identifier id{ident};
        std::string id_base{id.id_base()};

        // traverse the frames and try to find the identifier
        for (auto& frm : frames_ | std::views::reverse) {
            // does scope contain the variable?
            if (frm.has_var(id_base)) {
                // yes, return result
                return {id_base, frm};
            }

            // is the frame a function?
            if (frm.is_func()) {
                // yes, is identifier an alias?
                if (not frm.has_alias(id_base)) {
                    // no, return not found
                    return {"", frames_.at(0)};
                }

                // yes, continue resolving aliases until a variable, field
                // or register

                // note: when compiling in "dry-run" at 'stmt_def_func' the
                //       return variable is in the frame of the function as
                //       a variable, however when compiling the 'stmt_call'
                //       the function is inlined and the return is added as
                //       an alias to a variable in a higher context, thus
                //       aliases are followed to find the variable

                id = identifier{frm.get_alias(id_base)};
                id_base = id.id_base();

                if (is_identifier_register(id_base) or fields_.has(id_base)) {
                    return {id_base, frm};
                }

                // check if destination is of type: e.g. qword[r15]
                if (std::optional<std::string_view> reg{
                        toc::extract_between_brackets(id_base)};
                    reg) {
                    if (is_identifier_register(std::string{*reg})) {
                        // a memory reference, e.g. qword[r15]
                        return {id_base, frm};
                    }
                }
            }
        }

        throw panic_exception{"unexpected code path toc:3"};
    }

    [[nodiscard]] auto get_current_function_stack_size() const -> size_t {
        assert(not frames_.empty());
        size_t nbytes{};
        for (const auto& frm : frames_ | std::views::reverse) {
            nbytes += frm.allocated_stack_size();
            if (frm.is_func()) {
                return nbytes;
            }
        }
        // top frame, 'main'
        return nbytes;
    }

    [[nodiscard]] auto get_total_stack_size() const -> size_t {
        size_t nbytes{};
        for (const auto& frm : frames_) {
            nbytes += frm.allocated_stack_size();
        }
        return nbytes;
    }

    [[nodiscard]] auto make_ident_info_or_empty(const token& src_loc,
                                                std::string_view ident) const
        -> ident_info {
        identifier id{std::string{ident}};
        // get the root of an identifier: example p.x -> p
        std::string id_base{id.id_base()};
        // traverse the frames and resolve the id_nasm (which might be an
        // alias) to a variable, field, register or constant
        size_t i{frames_.size()};
        while (i) {
            i--;
            const frame& frm{frames_.at(i)};
            // does scope contain the variable?
            if (frm.has_var(id_base)) {
                // yes, found
                break;
            }
            // is the frame a function?
            if (frm.is_func()) {
                // is it an alias defined by the function?
                if (not frm.has_alias(id_base)) {
                    // no, it is not
                    break;
                }
                // yes, continue resolving alias until it is a variable,
                // field, register or constant
                const identifier new_id{frm.get_alias(id_base)};
                id_base = new_id.id_base();
                // is this a path e.g. 'pt.x' or just 'res'?
                if (id.path().size() == 1) {
                    // this is an alias of type: res -> p.x
                    id = new_id;
                } else {
                    // this is an alias of type: pt.x -> p.x
                    id.set_base(id_base);
                }
                continue;
            }
        }

        const frame& frm{frames_.at(i)};
        // is it a variable?
        if (frm.has_var(id_base)) {
            const var_info& var{frm.get_var_const_ref(id_base)};
            auto [tp, acc]{
                var.type_ref->accessor(src_loc, id.path(), var.stack_idx)};
            return {.id = std::string{ident},
                    .id_nasm = acc,
                    .const_value = 0,
                    .type_ref = tp,
                    .stack_ix = var.stack_idx,
                    .ident_type = ident_info::ident_type::VAR};
        }

        // is it a register?
        if (is_identifier_register(id_base)) {
            //? unary ops?
            return {.id = std::string{ident},
                    .id_nasm = id_base,
                    .const_value = 0,
                    .type_ref = get_type_default(),
                    .stack_ix = 0,
                    .ident_type = ident_info::ident_type::REGISTER};
        }

        // is it a register reference to memory?
        if (is_identifier_direct_register_indirect_addressing(id_base)) {
            // get the size: e.g. "dword [r15]"
            return {.id = std::string{ident},
                    .id_nasm = id_base,
                    .const_value = 0,
                    .type_ref = get_builtin_type_for_operand(src_loc, id_base),
                    .stack_ix = 0,
                    .ident_type = ident_info::ident_type::REGISTER};
        }

        // is it a field?
        if (fields_.has(id_base)) {
            const std::string& after_dot =
                id.path().size() == 1 ? ""
                                      : id.path().at(1); //? bug. not correct
            if (after_dot == "len") {
                return {.id = std::string{ident},
                        .id_nasm = std::format("{}.len", id_base),
                        .const_value = 0,
                        .type_ref = get_type_default(),
                        .stack_ix = 0,
                        .ident_type = ident_info::ident_type::IMPLIED};
            }
            const field_info& fi{fields_.get_const_ref(id_base)};
            if (fi.is_str) {
                return {.id = std::string{ident},
                        .id_nasm = id_base,
                        .const_value = 0,
                        .type_ref = get_type_default(),
                        .stack_ix = 0,
                        .ident_type = ident_info::ident_type::FIELD};
            }
            //? assumes qword
            return {.id = std::string{ident},
                    .id_nasm = std::format("qword [{}]", id_base),
                    .const_value = 0,
                    .type_ref = get_type_default(),
                    .stack_ix = 0,
                    .ident_type = ident_info::ident_type::FIELD};
        }

        // is 'id' an integer?
        if (const std::optional<int64_t> value{parse_to_constant(id_base)};
            value) {
            return {.id = std::string{ident},
                    .id_nasm = id_base,
                    .const_value = *value, // * dereference is safe
                                           // inside the if body
                    .type_ref = get_type_default(),
                    .stack_ix = 0,
                    .ident_type = ident_info::ident_type::CONST};
        }

        // is it a boolean constant?
        if (id_base == "true") {
            return {.id = std::string{ident},
                    .id_nasm = "true",
                    .const_value = 1,
                    .type_ref = get_type_bool(),
                    .stack_ix = 0,
                    .ident_type = ident_info::ident_type::CONST};
        }

        if (id_base == "false") {
            return {.id = std::string{ident},
                    .id_nasm = "false",
                    .const_value = 0,
                    .type_ref = get_type_bool(),
                    .stack_ix = 0,
                    .ident_type = ident_info::ident_type::CONST};
        }

        // not resolved, return empty info
        return {.id = "",
                .id_nasm = "",
                .const_value = 0,
                .type_ref = get_type_void(),
                .stack_ix = 0,
                .ident_type = ident_info::ident_type::CONST};
    }

    auto refresh_usage() -> void {
        usage_max_frame_count_ =
            std::max(frames_.size(), usage_max_frame_count_);
    }

    //------------------------------------------------------------------------
    // statics
    //------------------------------------------------------------------------
    static auto is_operand_memory(std::string_view operand) -> bool {
        return operand.find_first_of('[') != std::string::npos;
    }

    static auto extract_between_brackets(std::string_view str)
        -> std::optional<std::string_view> {
        auto start{str.find('[')};
        if (start == std::string_view::npos) {
            return std::nullopt;
        }

        auto end{str.find(']', start)};
        if (end == std::string_view::npos) {
            return std::nullopt;
        }

        return str.substr(start + 1, end - start - 1);
    }
};
