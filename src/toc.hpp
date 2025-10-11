#pragma once
// review: 2025-09-29

#include <algorithm>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <ranges>
#include <string_view>
#include <utility>

#include "compiler_exception.hpp"
#include "lut.hpp"
#include "panic_exception.hpp"
#include "statement.hpp"
#include "type.hpp"

class stmt_def_func;
class stmt_def_field;
class stmt_def_type;

struct func_info {
    const stmt_def_func* def{}; // null if built-in function
    token declared_at_tk;       // token for position in the source
    const type& type_ref;       // return type or void
};

struct func_return_info {
    token type_tk;        // type token
    token ident_tk;       // identifier token
    const type& type_ref; // type
};

struct var_info {
    std::string_view name;
    const type& type_ref;
    token declared_at_tk; // token for position in the source
    int32_t stack_idx{};  // location relative to register rsp
    bool is_array{};
    size_t array_size{};
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

    // aliases that refer to previous frame(s) alias or variable
    lut<std::string> aliases_;

    // the label to jump to when exiting an in-lined function
    std::string func_ret_label_;

    // info about the function returns
    std::optional<func_return_info> func_rets_;

  public:
    enum class frame_type : uint8_t { FUNC, BLOCK, LOOP };

  private:
    frame_type type_{frame_type::FUNC}; // frame type

  public:
    frame(std::string_view name, frame_type frm_type,
          const std::optional<func_return_info>& func_rets = {},
          std::string call_path = "", std::string func_ret_label = "") noexcept
        : name_{name}, call_path_{std::move(call_path)},
          func_ret_label_{std::move(func_ret_label)}, func_rets_{func_rets},
          type_{frm_type} {}

    // -------------------------------------------------------------------------
    // public non-special functions (sorted alphabetically)
    // -------------------------------------------------------------------------

    auto add_alias(std::string from, std::string to) -> void {
        aliases_.put(std::move(from), std::move(to));
    }

    auto add_var(token declared_at_tk, const std::string& name,
                 const type& type_ref, bool is_array, size_t array_size,
                 int stack_idx) -> void {

        if (stack_idx < 0) {
            // variable, increase allocated stack size
            allocated_stack_ += type_ref.size() * (is_array ? array_size : 1);
        }

        vars_.put(name, {.name = name,
                         .type_ref = type_ref,
                         .declared_at_tk = declared_at_tk,
                         .stack_idx = stack_idx,
                         .is_array = is_array,
                         .array_size = array_size});
    }

    [[nodiscard]] auto allocated_stack_size() const -> size_t {
        return allocated_stack_;
    }

    [[nodiscard]] auto call_path() const -> std::string_view {
        return call_path_;
    }

    [[nodiscard]] auto func_ret_label() const -> std::string_view {
        return func_ret_label_;
    }

    [[nodiscard]] auto get_alias(std::string_view name) const
        -> const std::string& {
        return aliases_.get_const_ref(name);
    }

    [[nodiscard]] auto get_func_returns_infos() const
        -> const std::optional<func_return_info>& {
        return func_rets_;
    }

    [[nodiscard]] auto get_var_const_ref(std::string_view name) const
        -> const var_info& {

        return vars_.get_const_ref(name);
    }

    auto get_var_ref(std::string_view name) -> var_info& {
        return vars_.get_ref(name);
    }

    [[nodiscard]] auto has_alias(std::string_view name) const -> bool {
        return aliases_.has(name);
    }

    [[nodiscard]] auto has_var(std::string_view name) const -> bool {
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

    [[nodiscard]] auto is_name(std::string_view name) const -> bool {
        return name_ == name;
    }

    [[nodiscard]] auto name() const -> std::string_view { return name_; }
};

struct field_info {
    const stmt_def_field& def;
    token declared_at_tk; // token for position in the source
    bool is_str{};
};

struct type_info {
    const stmt_def_type& def;
    token declared_at_tk;
    const type& type_ref;
};

struct ident_info {
    enum class ident_type : uint8_t { CONST, VAR, REGISTER, FIELD, IMPLIED };

    const std::string id;
    const std::string id_nasm; // NASM valid source
    const int64_t const_value{};
    const type& type_ref;
    const int32_t stack_ix{};
    const bool is_array{};
    const size_t array_size{};
    const size_t size{};
    const ident_type ident_type{ident_type::CONST};

    [[nodiscard]] auto is_const() const -> bool {
        return ident_type == ident_type::CONST;
    }

    [[nodiscard]] auto is_register() const -> bool {
        return ident_type == ident_type::REGISTER;
    }

    [[nodiscard]] auto is_var() const -> bool {
        return ident_type == ident_type::VAR;
    }
};

class identifier final {
    std::string_view id_;
    std::vector<std::string_view> path_;

  public:
    explicit identifier(std::string_view id) : id_{id} {
        auto parts{id_ | std::views::split('.')};
        for (auto part : parts) {
            path_.emplace_back(part.begin(), part.end());
        }
    }

    [[nodiscard]] auto id_base() const -> std::string_view {
        return path_.at(0);
    }

    [[nodiscard]] auto path() const -> const std::vector<std::string_view>& {
        return path_;
    }

    auto set_base(std::string_view name) -> void { path_[0] = name; }
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
    const std::regex regex_trim{R"(^\s+|\s+$)"};
    const bool bounds_check_{};

  public:
    toc(const std::string& source, const bool bounds_check)
        : source_{source}, bounds_check_(bounds_check) {}

    ~toc() = default;

    toc() = delete;
    toc(const toc&) = default;
    toc(toc&&) = default;
    auto operator=(const toc&) -> toc& = delete;
    auto operator=(toc&&) -> toc& = delete;

    // -------------------------------------------------------------------------
    // public non-special functions (sorted alphabetically)
    // -------------------------------------------------------------------------

    auto add_alias(std::string from, std::string to) -> void {
        frames_.back().add_alias(std::move(from), std::move(to));
    }

    auto add_field(const token& src_loc_tk, std::string name,
                   const stmt_def_field& fld_def, bool is_str_field) -> void {

        if (fields_.has(name)) {
            throw compiler_exception{
                src_loc_tk,
                std::format("field '{}' already defined at {}", name,
                            source_location_hr(
                                fields_.get_const_ref(name).declared_at_tk))};
        }

        fields_.put(std::move(name), {.def = fld_def,
                                      .declared_at_tk = src_loc_tk,
                                      .is_str = is_str_field});
    }

    auto add_func(const token& src_loc_tk, std::string name,
                  const type& return_type, const stmt_def_func* func_def)
        -> void {

        if (funcs_.has(name)) {
            const func_info& fn{funcs_.get_const_ref(name)};
            throw compiler_exception{
                src_loc_tk,
                std::format("function '{}' already defined at {}", name,
                            source_location_hr(fn.declared_at_tk))};
        }

        funcs_.put(std::move(name), {.def = func_def,
                                     .declared_at_tk = src_loc_tk,
                                     .type_ref = return_type});
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

    auto add_var(const token& src_loc_tk, std::ostream& os, size_t indnt,
                 std::string name, const type& var_type, bool is_array,
                 size_t array_size) -> void {

        // check if the variable is already declared in this scope
        if (frames_.back().has_var(name)) {
            const var_info& var{frames_.back().get_var_const_ref(name)};
            throw compiler_exception{
                src_loc_tk,
                std::format("variable '{}' already declared at {}", name,
                            source_location_hr(var.declared_at_tk))};
        }

        const int stack_idx{
            static_cast<int>(get_stack_size() +
                             (var_type.size() * (is_array ? array_size : 1)))};

        frames_.back().add_var(src_loc_tk, name, var_type, is_array, array_size,
                               -stack_idx);

        const size_t total_stack_size{get_stack_size()};
        usage_max_stack_size_ =
            std::max(total_stack_size, usage_max_stack_size_);

        // comment the resolved name
        const ident_info& name_info{make_ident_info(src_loc_tk, name, false)};

        comment_start(src_loc_tk, os, indnt);
        std::print(os, "var {}: {}", name, name_info.type_ref.name());
        if (array_size) {
            std::print(os, "[{}]", array_size);
        }
        std::println(os, " @ {}", name_info.id_nasm);
    }

    auto alloc_named_register(const token& src_loc_tk, std::ostream& os,
                              size_t indnt, std::string_view reg) -> bool {

        comment_start(src_loc_tk, os, indnt);
        std::print(os, "allocate named register '{}'", reg);

        auto reg_iter{std::ranges::find(named_registers_, reg)};
        if (reg_iter == named_registers_.end()) {
            std::println(os, ": not available");
            return false;
        }

        std::println(os, "");

        allocated_registers_.emplace_back(std::move(*reg_iter));
        allocated_registers_src_locs_.emplace_back(
            source_location_hr(src_loc_tk));
        named_registers_.erase(reg_iter);

        return true;
    }

    auto alloc_named_register_or_throw(const token& src_loc_tk,
                                       std::ostream& os, size_t indnt,
                                       std::string_view reg) -> void {

        comment_start(src_loc_tk, os, indnt);
        std::println(os, "allocate named register '{}'", reg);

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
                src_loc_tk, std::format("cannot allocate register '{}' because "
                                        "it was allocated at {}",
                                        reg, loc)};
        }

        allocated_registers_.emplace_back(std::move(*reg_iter));
        allocated_registers_src_locs_.emplace_back(
            source_location_hr(src_loc_tk));
        named_registers_.erase(reg_iter);
    }

    auto alloc_scratch_register(const token& src_loc_tk, std::ostream& os,
                                size_t indnt) -> std::string {

        if (scratch_registers_.empty()) {
            throw compiler_exception{src_loc_tk,
                                     "out of scratch registers. try to reduce "
                                     "expression complexity"};
        }

        std::string reg{std::move(scratch_registers_.back())};
        scratch_registers_.pop_back();

        comment_start(src_loc_tk, os, indnt);
        std::println(os, "allocate scratch register -> {}", reg);

        const size_t n{scratch_registers_initial_size_ -
                       scratch_registers_.size()};
        usage_max_scratch_regs_ = std::max(n, usage_max_scratch_regs_);

        allocated_registers_.emplace_back(std::move(reg));
        allocated_registers_src_locs_.emplace_back(
            source_location_hr(src_loc_tk));

        return allocated_registers_.back();
    }

    auto asm_cmd(const token& src_loc_tk, std::ostream& os, size_t indnt,
                 std::string_view op, std::string_view dst_nasm,
                 std::string_view src_nasm) -> void {

        if (op == "mov" and dst_nasm == src_nasm) {
            return;
        }

        const size_t dst_size{get_size_from_operand(src_loc_tk, dst_nasm)};
        const size_t src_size{get_size_from_operand(src_loc_tk, src_nasm)};

        if (dst_size == src_size) {
            if (is_nasm_memory_operand(dst_nasm) and
                is_nasm_memory_operand(src_nasm)) {
                // both operands are memory references
                // use scratch register for transfer
                const std::string reg{
                    alloc_scratch_register(src_loc_tk, os, indnt)};
                const std::string reg_sized{
                    get_sized_register(src_loc_tk, reg, dst_size)};
                indent(os, indnt);
                std::println(os, "mov {}, {}", reg_sized, src_nasm);
                indent(os, indnt);
                std::println(os, "{} {}, {}", op, dst_nasm, reg_sized);
                free_scratch_register(src_loc_tk, os, indnt, reg);
                return;
            }

            // not both operands are memory references, `src_nasm` might be a
            // constant or a register src might be constant
            indent(os, indnt);
            std::println(os, "{} {}, {}", op, dst_nasm, src_nasm);
            return;
        }

        if (dst_size > src_size) {
            // destination is larger than source
            // mov rax,byte[b] -> movsx
            if (is_nasm_memory_operand(dst_nasm) and
                is_nasm_memory_operand(src_nasm)) {
                // both operands refer to memory
                // use in-between scratch register
                const std::string reg{
                    alloc_scratch_register(src_loc_tk, os, indnt)};
                const std::string reg_sized{
                    get_sized_register(src_loc_tk, reg, dst_size)};
                indent(os, indnt);
                std::println(os, "movsx {}, {}", reg_sized, src_nasm);
                indent(os, indnt);
                std::println(os, "{} {}, {}", op, dst_nasm, reg_sized);
                free_scratch_register(src_loc_tk, os, indnt, reg);
                return;
            }

            // not both of the operands are memory references
            if (op == "mov") {
                // special case for `mov` which needs sign extended move
                indent(os, indnt);
                std::println(os, "movsx {}, {}", dst_nasm, src_nasm);
                return;
            }

            // note: when doing arithmetic between 2 memory locations,
            //       this code path is triggered by the use of an
            //       in-between scratch register
            indent(os, indnt);
            std::println(os, "{} {}, {}", op,
                         get_sized_register(src_loc_tk, dst_nasm, src_size),
                         src_nasm);
            return;
        }

        // dst_size < src_size
        // truncation will happen

        if (is_nasm_memory_operand(dst_nasm) and
            is_nasm_memory_operand(src_nasm)) {
            // both operands are memory references
            // use scratch register for transfer
            const std::string reg{
                alloc_scratch_register(src_loc_tk, os, indnt)};
            const std::string reg_sized{
                get_sized_register(src_loc_tk, reg, dst_size)};
            indent(os, indnt);
            std::println(os, "mov {}, {}", reg_sized,
                         resize_nasm_memory_operand(
                             src_nasm, get_operand_size(dst_size)));
            indent(os, indnt);
            std::println(os, "{} {}, {}", op, dst_nasm, reg_sized);
            free_scratch_register(src_loc_tk, os, indnt, reg);
            return;
        }

        // not both operands are memory references
        if (is_nasm_register(src_nasm)) {
            indent(os, indnt);
            std::println(os, "{} {}, {}", op, dst_nasm,
                         get_sized_register(src_loc_tk, src_nasm, dst_size));
            return;
        }

        if (is_nasm_register(dst_nasm)) {
            //? destination is never a register
            throw panic_exception{"unexpected code path toc:5"};
        }

        // constant to memory
        // note: constants have the default size of qword so when assigning
        //       to smaller sizes this code path is taken
        //? todo: check for overflow
        indent(os, indnt);
        std::println(os, "{} {}, {}", op, dst_nasm, src_nasm);
    }

    auto comment_source(const statement& st, std::ostream& os,
                        size_t indnt) const -> void {

        comment_start(st.tok(), os, indnt);
        std::stringstream ss;
        st.source_to(ss);
        std::println(
            os, "{}",
            std::regex_replace(std::regex_replace(ss.str(), regex_trim, ""),
                               regex_ws, " "));
    }

    auto comment_source(const statement& st, std::ostream& os,
                        const size_t indnt, std::string_view dst,
                        std::string_view op) const -> void {

        comment_start(st.tok(), os, indnt);
        std::stringstream ss;
        std::print(ss, "{} {} ", dst, op);
        st.source_to(ss);
        std::string res{std::regex_replace(ss.str(), regex_ws, " ")};
        // trim end of string
        if (not res.empty() && res.back() == ' ') {
            res.pop_back();
        }
        std::println(os, "{}", res);
    }

    auto comment_start(const token& src_loc_tk, std::ostream& os,
                       const size_t indnt) const -> void {

        const auto [line, col]{line_and_col_num_for_char_index(
            src_loc_tk.at_line(), src_loc_tk.start_index(), source_)};
        indent(os, indnt, true);
        std::print(os, "[{}:{}] ", line, col);
    }

    auto comment_token(const token& tk, std::ostream& os,
                       const size_t indnt) const -> void {

        comment_start(tk, os, indnt);
        std::println(os, "{}", tk.text());
    }

    [[nodiscard]] auto create_unique_label(const token& tk,
                                           const std::string_view prefix) const
        -> std::string {

        const std::string_view call_path{get_call_path(tk)};
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

    auto enter_func(std::string_view name,
                    const std::optional<func_return_info>& returns,
                    std::string_view call_path = {},
                    std::string_view return_jmp_label = {}) -> void {

        frames_.emplace_back(name, frame::frame_type::FUNC, returns,
                             std::string{call_path},
                             std::string{return_jmp_label});
        refresh_usage();
    }

    auto enter_loop(std::string_view name) -> void {
        frames_.emplace_back(name, frame::frame_type::LOOP);
        refresh_usage();
    }

    auto exit_block() -> void {
        const frame& frm{frames_.back()};
        assert(frm.is_block());
        frames_.pop_back();
    }

    auto exit_func(std::string_view name) -> void {
        const frame& frm{frames_.back()};
        assert(frm.is_func() and frm.is_name(name));
        frames_.pop_back();
    }

    auto exit_loop(std::string_view name) -> void {
        const frame& frm{frames_.back()};
        assert(frm.is_loop() and frm.is_name(name));
        frames_.pop_back();
    }

    auto finish(std::ostream& os) -> void {
        std::println(os, "\n; max scratch registers in use: {}",
                     usage_max_scratch_regs_);
        std::println(os, ";            max frames in use: {}",
                     usage_max_frame_count_);
        std::println(os, ";               max stack size: {} B",
                     usage_max_stack_size_);
        assert(all_registers_.size() == all_registers_initial_size_);
        assert(allocated_registers_.empty());
        assert(allocated_registers_src_locs_.empty());
        assert(frames_.empty());
        assert(named_registers_.size() == named_registers_initial_size_);
        assert(scratch_registers_.size() == scratch_registers_initial_size_);
        usage_max_frame_count_ = 0;
        usage_max_scratch_regs_ = 0;
    }

    auto free_named_register(const token& src_loc_tk, std::ostream& os,
                             size_t indnt, std::string_view reg) -> void {

        comment_start(src_loc_tk, os, indnt);
        std::println(os, "free named register '{}'", reg);

        assert(allocated_registers_.back() == reg);

        named_registers_.emplace_back(std::move(allocated_registers_.back()));
        allocated_registers_.pop_back();
        allocated_registers_src_locs_.pop_back();
    }

    auto free_scratch_register(const token& src_loc_tk, std::ostream& os,
                               size_t indnt, std::string_view reg) -> void {

        comment_start(src_loc_tk, os, indnt);
        std::println(os, "free scratch register '{}'", reg);

        assert(allocated_registers_.back() == reg);

        scratch_registers_.emplace_back(std::move(allocated_registers_.back()));
        allocated_registers_.pop_back();
        allocated_registers_src_locs_.pop_back();
    }

    [[nodiscard]] auto get_call_path(const token& src_loc_tk) const
        -> std::string_view {

        for (const auto& frm : frames_ | std::views::reverse) {
            if (frm.is_func()) {
                return frm.call_path();
            }
        }

        throw compiler_exception{src_loc_tk, "not in a function"};
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

    [[nodiscard]] auto
    get_func_return_label_or_throw(const token& src_loc_tk) const
        -> std::string_view {

        for (const auto& frm : frames_ | std::views::reverse) {
            if (frm.is_func()) {
                return frm.func_ret_label();
            }
        }

        throw compiler_exception{src_loc_tk, "not in a function"};
    }

    [[nodiscard]] auto
    get_func_return_type_or_throw(const token& src_loc_tk,
                                  std::string_view name) const -> const type& {

        if (not funcs_.has(name)) {
            throw compiler_exception{
                src_loc_tk, std::format("function '{}' not found", name)};
        }

        return funcs_.get_const_ref(name).type_ref;
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
        if (is_nasm_register(operand)) {
            return get_size_from_operand_register(src_loc_tk, operand);
        }

        // constant
        return get_type_default().size();
    }

    [[nodiscard]] auto get_type_bool() const -> const type& {
        return *type_bool_;
    }

    [[nodiscard]] auto get_type_default() const -> const type& {
        return *type_default_;
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

    [[nodiscard]] auto get_type_void() const -> const type& {
        return *type_void_;
    }

    [[nodiscard]] auto is_bounds_check() const -> bool { return bounds_check_; }

    [[nodiscard]] auto is_func(std::string_view name) const -> bool {
        return funcs_.has(name);
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

    auto set_type_bool(const type& tpe) -> void { type_bool_ = &tpe; }

    auto set_type_default(const type& tpe) -> void { type_default_ = &tpe; }

    auto set_type_void(const type& tpe) -> void { type_void_ = &tpe; }

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

  private:
    // -------------------------------------------------------------------------
    // private non-special functions (sorted alphabetically)
    // -------------------------------------------------------------------------

    [[nodiscard]] auto
    get_builtin_type_for_operand(const token& src_loc_tk,
                                 std::string_view operand) const
        -> const type& {

        //? sort of ugly
        if (operand.starts_with("qword")) {
            return get_type_or_throw(src_loc_tk, "i64");
        }
        if (operand.starts_with("dword")) {
            return get_type_or_throw(src_loc_tk, "i32");
        }
        if (operand.starts_with("word")) {
            return get_type_or_throw(src_loc_tk, "i16");
        }
        if (operand.starts_with("byte")) {
            return get_type_or_throw(src_loc_tk, "i8");
        }

        throw panic_exception{"unexpected code path toc:1"};
    }

    [[nodiscard]] auto get_stack_size() const -> size_t {
        size_t nbytes{};
        for (const auto& frm : frames_) {
            nbytes += frm.allocated_stack_size();
        }
        return nbytes;
    }

    [[nodiscard]] auto is_nasm_indirect(std::string_view nasm) const -> bool {
        if (auto expr{toc::get_text_between_brackets(nasm)}; expr) {
            const std::string reg{
                get_base_register_from_indirect_addressing(*expr)};
            if (is_nasm_register(reg)) {
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] auto is_nasm_register(std::string_view nasm) const -> bool {
        return std::ranges::find(all_registers_, nasm) != all_registers_.end();
    }

    [[nodiscard]] auto make_ident_info_or_empty(const token& src_loc,
                                                std::string_view ident) const
        -> ident_info {

        identifier id{ident};
        // get the root of an identifier: example p.x -> p
        std::string_view id_base{id.id_base()};
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
                    // this is an alias of the type: res -> p.x
                    id = new_id;
                } else {
                    // this is an alias of the type: pt.x -> p.x
                    id.set_base(id_base);
                }
                continue;
            }
        }

        const frame& frm{frames_.at(i)};

        // is it a variable?
        if (frm.has_var(id_base)) {
            const var_info& var{frm.get_var_const_ref(id_base)};
            accessor_info ai{var.type_ref.accessor(
                src_loc, id.path(), var.stack_idx, var.is_array, var.array_size,
                var.type_ref.size() * (var.is_array ? var.array_size : 1))};
            return {.id = std::string{ident},
                    .id_nasm = ai.id_nasm,
                    .type_ref = ai.tp,
                    .stack_ix = var.stack_idx,
                    .is_array = ai.is_array,
                    .array_size = ai.array_size,
                    .size = ai.size,
                    .ident_type = ident_info::ident_type::VAR};
        }

        // is it a register?
        if (is_nasm_register(id_base)) {
            //? unary ops?
            return {.id = std::string{ident},
                    .id_nasm = std::string{id_base},
                    .type_ref = get_type_default(),
                    .size = get_type_default().size(),
                    .ident_type = ident_info::ident_type::REGISTER};
        }

        // is it a register reference to memory?
        if (is_nasm_indirect(id_base)) {
            // get the size: e.g. "dword [r15]"
            return {.id = std::string{ident},
                    .id_nasm = std::string{id_base},
                    .type_ref = get_builtin_type_for_operand(src_loc, id_base),
                    .size = get_size_from_operand(src_loc, id_base),
                    .ident_type = ident_info::ident_type::REGISTER};
        }

        // is it a field?
        if (fields_.has(id_base)) {
            const std::string_view after_dot =
                id.path().size() == 1 ? ""
                                      : id.path().at(1); //? bug. not correct
            if (after_dot == "len") {
                return {.id = std::string{ident},
                        .id_nasm = std::format("{}.len", id_base),
                        .type_ref = get_type_default(),
                        .size = get_type_default().size(),
                        .ident_type = ident_info::ident_type::IMPLIED};
            }
            const field_info& fi{fields_.get_const_ref(id_base)};
            if (fi.is_str) {
                return {.id = std::string{ident},
                        .id_nasm = std::string{id_base},
                        .type_ref = get_type_default(),
                        .ident_type = ident_info::ident_type::FIELD};
            }
            //? assumes qword
            return {.id = std::string{ident},
                    .id_nasm = std::format("qword [{}]", id_base),
                    .type_ref = get_type_default(),
                    .size = get_type_default().size(),
                    .ident_type = ident_info::ident_type::FIELD};
        }

        // is 'id' an integer?
        if (const std::optional<int64_t> value{parse_to_constant(id_base)};
            value) {
            return {.id = std::string{ident},
                    .id_nasm = std::string{id_base},
                    .const_value = *value,
                    .type_ref = get_type_default(),
                    .size = get_type_default().size()};
        }

        // is it a boolean constant?
        if (id_base == "true") {
            return {.id = std::string{ident},
                    .id_nasm = "true",
                    .const_value = 1,
                    .type_ref = get_type_bool(),
                    .size = get_type_bool().size()};
        }

        if (id_base == "false") {
            return {.id = std::string{ident},
                    .id_nasm = "false",
                    .const_value = 0,
                    .type_ref = get_type_bool(),
                    .size = get_type_bool().size()};
        }

        // not resolved, return empty info
        return {
            .id = "",
            .id_nasm = "",
            .type_ref = get_type_void(),
        };
    }

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

    auto refresh_usage() -> void {
        usage_max_frame_count_ =
            std::max(frames_.size(), usage_max_frame_count_);
    }

  public:
    // -------------------------------------------------------------------------
    // public statics (sorted alphabetically)
    // -------------------------------------------------------------------------

    static auto asm_jmp([[maybe_unused]] const token& src_loc_tk,
                        std::ostream& os, size_t indnt, std::string_view label)
        -> void {

        indent(os, indnt);
        std::println(os, "jmp {}", label);
    }

    static auto asm_jxx([[maybe_unused]] const token& src_loc_tk,
                        std::ostream& os, size_t indnt,
                        std::string_view comparison, std::string_view label)
        -> void {

        indent(os, indnt);
        std::println(os, "j{} {}", comparison, label);
    }

    static auto asm_label([[maybe_unused]] const token& src_loc_tk,
                          std::ostream& os, size_t indnt,
                          std::string_view label) -> void {

        indent(os, indnt);
        std::println(os, "{}:", label);
    }

    static auto asm_neg([[maybe_unused]] const token& src_loc_tk,
                        std::ostream& os, size_t indnt,
                        std::string_view operand) -> void {

        indent(os, indnt);
        std::println(os, "neg {}", operand);
    }

    static auto asm_not([[maybe_unused]] const token& src_loc_tk,
                        std::ostream& os, size_t indnt,
                        std::string_view operand) -> void {

        indent(os, indnt);
        std::println(os, "not {}", operand);
    }

    static auto asm_pop([[maybe_unused]] const token& src_loc_tk,
                        std::ostream& os, size_t indnt,
                        std::string_view operand) -> void {

        indent(os, indnt);
        std::println(os, "pop {}", operand);
    }

    static auto asm_push([[maybe_unused]] const token& src_loc_tk,
                         std::ostream& os, size_t indnt,
                         std::string_view operand) -> void {

        indent(os, indnt);
        std::println(os, "push {}", operand);
    }

    // size: b, w, d, q for different sizings
    static auto asm_rep_movs([[maybe_unused]] const token& src_loc_tk,
                             std::ostream& os, size_t indnt, char size)
        -> void {

        indent(os, indnt);
        std::println(os, "rep movs{}", size);
    }

    // size: b, w, d, q for different sizings
    static auto asm_rep_stos([[maybe_unused]] const token& src_loc_tk,
                             std::ostream& os, size_t indnt, char size)
        -> void {

        indent(os, indnt);
        std::println(os, "rep stos{}", size);
    }

    // size: b, w, d, q for different sizings
    static auto asm_repe_cmps([[maybe_unused]] const token& src_loc_tk,
                              std::ostream& os, size_t indnt, char size)
        -> void {

        indent(os, indnt);
        std::println(os, "repe cmps{}", size);
    }

    static auto get_field_offset_in_type(const token& src_loc_tk,
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
        throw compiler_exception{src_loc_tk,
                                 "unexpected code path stmt_assign_var:1"};
    }

    static auto indent(std::ostream& os, size_t indnt,
                       const bool comment = false) -> void {

        if (comment) {
            std::print(os, ";");
        }
        if (indnt == 0) {
            return;
        }
        if (comment) {
            std::print(os, "   ");
        } else {
            std::print(os, "    ");
        }
        for (size_t i{1}; i < indnt; i++) {
            std::print(os, "    ");
        }
    }

    static auto line_and_col_num_for_char_index(size_t at_line,
                                                size_t char_index_in_source,
                                                std::string_view src)
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
            //       trigger the warning
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

  private:
    //------------------------------------------------------------------------
    // private statics (sorted alphabetically)
    //------------------------------------------------------------------------

    static auto
    get_base_register_from_indirect_addressing(std::string_view addressing)
        -> std::string_view {

        auto pos{addressing.find_first_of(" +")};
        if (pos == std::string_view::npos) {
            return addressing;
        }

        return addressing.substr(0, pos);
    }

    static auto get_text_between_brackets(std::string_view str)
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

    [[nodiscard]] static auto get_operand_size(const size_t size)
        -> std::string_view {
        switch (size) {
        case 1:
            return "byte";
        case 2:
            return "word";
        case 4:
            return "dword";
        case 8:
            return "qword";
        default:
            throw panic_exception{"unexpected code path toc:3"};
        }
    }

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

    static auto get_sized_register(const token& src_loc_tk,
                                   std::string_view operand, size_t size)
        -> std::string {

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

    static auto is_nasm_memory_operand(std::string_view operand) -> bool {
        return operand.find_first_of('[') != std::string::npos;
    }

    static auto resize_nasm_memory_operand(std::string_view operand,
                                           std::string_view new_size)
        -> std::string {

        auto pos = operand.find('[');
        if (pos == std::string_view::npos) {
            throw panic_exception{"unexpected code path toc:4"};
        }
        return std::format("{} {}", new_size, operand.substr(pos));
    }
};
