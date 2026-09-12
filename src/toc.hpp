#pragma once
// review: 2025-09-29

#include <algorithm>
#include <cstdint>
#include <format>
#include <optional>
#include <ostream>
#include <print>
#include <ranges>
#include <regex>
#include <span>
#include <sstream>
#include <string_view>
#include <utility>

#include "compiler_exception.hpp"
#include "decouple.hpp"
#include "lut.hpp"
#include "statement.hpp"
#include "type.hpp"

class stmt_def_func;
class stmt_def_field;
class stmt_def_type;

struct func_info {
    const stmt_def_func* def{}; // null if built-in function
    token declared_at_tk;       // token for position in the source
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
    lut<alias_info> aliases_;

    // the label to jump to when exiting an in-lined function
    std::string func_ret_label_;

    // info about the function return
    std::optional<func_return_info> func_ret_;

    // true if var that is not dat has been added
    bool non_dat_var_has_been_added_{};

  public:
    enum class frame_type : uint8_t { FUNC, BLOCK, LOOP };

  private:
    frame_type type_{frame_type::FUNC}; // frame type

  public:
    frame(const std::string_view name, const frame_type frm_type,
          const std::optional<func_return_info>& func_ret_info = {},
          std::string call_path = "", std::string func_ret_label = "") noexcept
        : name_{name}, call_path_{std::move(call_path)},
          func_ret_label_{std::move(func_ret_label)}, func_ret_{func_ret_info},
          type_{frm_type} {}

    // -------------------------------------------------------------------------
    // public non-special functions (sorted alphabetically)
    // -------------------------------------------------------------------------

    auto add_alias(const alias_info& ai) -> void {
        aliases_.put(std::string{ai.from}, ai);
    }

    auto add_var(const var_info& var, bool is_data = false) -> void {
        if (var.stack_idx < 0) {
            // variable, increase allocated stack size
            allocated_stack_ +=
                var.type_ptr->size() * (var.is_array ? var.array_size : 1);
        }

        vars_.put(var.name, var);

        if (not is_data) {
            non_dat_var_has_been_added_ = true;
        }
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
        -> const alias_info& {

        return aliases_.get_const_ref(name);
    }

    [[nodiscard]] auto get_var_const_ref(const std::string_view name) const
        -> const var_info& {

        return vars_.get_const_ref(name);
    }

    [[nodiscard]] auto has_alias(const std::string_view name) const -> bool {
        return aliases_.has(name);
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

    [[nodiscard]] auto is_name(const std::string_view name) const -> bool {
        return name_ == name;
    }

    [[nodiscard]] auto name() const -> std::string_view { return name_; }
};

class ident_path final {
    std::string id_;
    std::vector<std::string> path_;

    auto refresh_path() {
        path_.clear();
        for (auto part : id_ | std::views::split('.')) {
            path_.emplace_back(part.begin(), part.end());
        }
    }

  public:
    explicit ident_path(std::string id) : id_{std::move(id)} { refresh_path(); }

    [[nodiscard]] auto base() const -> std::string_view { return path_[0]; }

    [[nodiscard]] auto path() const -> const std::vector<std::string>& {
        return path_;
    }

    [[nodiscard]] auto str() const -> const std::string& { return id_; }

    auto append(const std::string_view path_elem) {
        id_ = std::format("{}.{}", id_, path_elem);
        refresh_path();
    }
};

class toc final {
    struct allocated_register {
        std::string name;
        std::string source_location;
    };

    std::string_view source_;
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
    std::vector<allocated_register> allocated_registers_;
    std::vector<const stmt_def_func*> func_defs_;
    lut<func_info> funcs_;
    lut<const type*> types_;
    const type* type_void_{};
    const type* type_default_{};
    const type* type_bool_{};
    size_t usage_max_scratch_regs_{};
    size_t usage_max_frame_count_{};
    size_t usage_max_stack_size_{};
    bool bounds_check_upper_{};
    bool bounds_check_with_line_{};
    bool bounds_check_lower_{};
    std::vector<const statement*> data_;
    struct constant {
        token src_loc_tk;
        int64_t value{};
    };
    lut<constant> constants_;

    std::regex regex_ws_{R"(\s+)"};
    std::regex regex_trim_{R"(^\s+|\s+$)"};
    std::regex regex_nasm_comment_{R"(^\s*;.*$)"};
    std::regex regex_nasm_number_register_{R"(r(\d+))"};

  public:
    static constexpr size_t size_qword{8};
    static constexpr size_t size_dword{4};
    static constexpr size_t size_word{2};
    static constexpr size_t size_byte{1};
    static constexpr std::string_view def_data_qword{"dq"};
    static constexpr std::string_view def_data_dword{"dd"};
    static constexpr std::string_view def_data_word{"dw"};
    static constexpr std::string_view def_data_byte{"db"};
    static constexpr size_t threshold_for_rep_stos{32};
    static constexpr size_t threshold_for_rep_movs{16};

    toc(const std::string_view source, const bool bounds_check_upper,
        const bool bounds_check_lower, const bool bounds_check_with_line)
        : source_{source}, bounds_check_upper_{bounds_check_upper},
          bounds_check_with_line_{bounds_check_with_line},
          bounds_check_lower_{bounds_check_lower} {}

    // -------------------------------------------------------------------------
    // public non-special functions (sorted alphabetically)
    // -------------------------------------------------------------------------

    auto add_alias(const alias_info& ai) -> void {
        frames_.back().add_alias(ai);
    }

    auto add_const(const token& src_loc_tk, const std::string_view name,
                   const int64_t value) {

        if (constants_.has(name)) {
            const constant c{constants_.get_const_ref(name)};
            throw compiler_exception(
                src_loc_tk,
                std::format("constant '{}' already defined at {}", name,
                            source_location_hr(c.src_loc_tk)));
        }
        constants_.put(std::string{name},
                       {.src_loc_tk{src_loc_tk}, .value{value}});
    }

    auto add_dat(const statement* stmt) -> void {
        if (not is_in_main()) {
            throw compiler_exception(stmt->tok(),
                                     "dat can only be added in function main");
        }
        if (frames_.back().has_non_data_var_been_added()) {
            throw compiler_exception(stmt->tok(),
                                     "dat can only be added before any var");
        }
        data_.emplace_back(stmt);
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

        funcs_.put(std::move(name), {.def{func_def},
                                     .declared_at_tk{src_loc_tk},
                                     .type_ptr{&return_type}});

        if (func_def) {
            func_defs_.emplace_back(func_def);
        }
    }

    auto add_type(const token& src_loc_tk, const type& tpe) -> void {
        if (types_.has(tpe.name())) {
            // todo: specify where the type has been defined
            throw compiler_exception{
                src_loc_tk,
                std::format("type '{}' already defined", tpe.name())};
        }

        types_.put(tpe.name(), &tpe);
    }

    auto add_var(const token& src_loc_tk, std::ostream& os, const size_t indnt,
                 var_info var, bool is_dat) -> void {

        // check if the variable is already declared in this scope
        if (frames_.back().has_var(var.name)) {
            const var_info& decl_var{
                frames_.back().get_var_const_ref(var.name)};
            throw compiler_exception{
                src_loc_tk,
                std::format("variable '{}' already declared at {}", var.name,
                            source_location_hr(decl_var.declared_at_tk))};
        }

        const int stack_idx{static_cast<int>(
            get_stack_size() +
            (var.type_ptr->size() * (var.is_array ? var.array_size : 1)))};

        var.stack_idx = -stack_idx;
        frames_.back().add_var(var, is_dat);

        const size_t total_stack_size{get_stack_size()};
        usage_max_stack_size_ =
            std::max(total_stack_size, usage_max_stack_size_);

        // comment the resolved name
        const ident_info& name_info{make_ident_info(src_loc_tk, var.name)};

        comment_start(src_loc_tk, os, indnt);
        std::print(os, "{}: {}", var.name, name_info.type().name());
        if (var.array_size) {
            std::print(os, "[{}]", var.array_size);
        }
        std::println(os, " ({} B @ [{}])",
                     name_info.type().size() *
                         (name_info.is_array ? name_info.array_size : 1),
                     name_info.operand.address_str());
    }

    [[nodiscard]] auto
    alloc_named_register(const token& src_loc_tk, std::ostream& os,
                         const size_t indnt, const std::string_view reg)
        -> bool {

        comment_start(src_loc_tk, os, indnt);
        std::print(os, "allocate named register '{}'", reg);

        auto reg_iter{std::ranges::find(named_registers_, reg)};
        if (reg_iter == named_registers_.end()) {
            std::println(os, ": not available");
            return false;
        }

        std::println(os, "");

        allocated_registers_.emplace_back(std::move(*reg_iter),
                                          source_location_hr(src_loc_tk));
        named_registers_.erase(reg_iter);

        return true;
    }

    auto alloc_named_register_or_throw(const token& src_loc_tk,
                                       std::ostream& os, const size_t indnt,
                                       const std::string_view reg) -> void {

        comment_start(src_loc_tk, os, indnt);
        std::println(os, "allocate named register '{}'", reg);

        auto reg_iter{std::ranges::find(named_registers_, reg)};
        if (reg_iter == named_registers_.end()) {
            // not found
            std::string loc;
            const auto allocated{std::ranges::find(allocated_registers_, reg,
                                                   &allocated_register::name)};
            if (allocated != allocated_registers_.end()) {
                loc = allocated->source_location;
            }
            throw compiler_exception{
                src_loc_tk, std::format("cannot allocate register '{}' because "
                                        "it was allocated at {}",
                                        reg, loc)};
        }

        allocated_registers_.emplace_back(std::move(*reg_iter),
                                          source_location_hr(src_loc_tk));
        named_registers_.erase(reg_iter);
    }

    [[nodiscard]] auto alloc_scratch_register(const token& src_loc_tk,
                                              std::ostream& os,
                                              const size_t indnt)
        -> std::string {

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

        allocated_registers_.emplace_back(std::move(reg),
                                          source_location_hr(src_loc_tk));

        return allocated_registers_.back().name;
    }

    auto asm_cmd(const token& src_loc_tk, std::ostream& os, const size_t indnt,
                 const std::string_view op, const std::string_view dst_op,
                 const std::string_view src_op) -> void {

        if (op == "mov" and dst_op == src_op) {
            return;
        }

        const size_t dst_size{get_operand_size(dst_op)};
        const size_t src_size{get_operand_size(src_op)};

        if (dst_size == src_size) {
            if (is_memory_operand(dst_op) and is_memory_operand(src_op)) {
                // both operands are memory references
                // use scratch register for transfer
                const std::string reg{
                    alloc_scratch_register(src_loc_tk, os, indnt)};
                const std::string reg_sized{
                    get_sized_register_operand(reg, dst_size)};
                indent(os, indnt);
                std::println(os, "mov {}, {}", reg_sized, src_op);
                indent(os, indnt);
                std::println(os, "{} {}, {}", op, dst_op, reg_sized);
                free_scratch_register(src_loc_tk, os, indnt, reg);
                return;
            }

            // not both operands are memory references, 'src_op' might be a
            // constant or a register or a constant

            indent(os, indnt);
            std::println(os, "{} {}, {}", op, dst_op, src_op);
            return;
        }

        if (dst_size > src_size) {
            // destination is larger than source
            // mov rax,byte[b] -> movsx
            if (is_memory_operand(dst_op) and is_memory_operand(src_op)) {
                // both operands refer to memory
                // use in-between scratch register
                const std::string reg{
                    alloc_scratch_register(src_loc_tk, os, indnt)};
                const std::string reg_sized{
                    get_sized_register_operand(reg, dst_size)};
                indent(os, indnt);
                std::println(os, "movsx {}, {}", reg_sized, src_op);
                indent(os, indnt);
                std::println(os, "{} {}, {}", op, dst_op, reg_sized);
                free_scratch_register(src_loc_tk, os, indnt, reg);
                return;
            }

            // not both of the operands are memory references

            if (op == "mov") {
                // special case for 'mov' which needs sign extended move
                indent(os, indnt);
                std::println(os, "movsx {}, {}", dst_op, src_op);
                return;
            }

            // note: special case for shift operations which have the shift
            // amount in 'cl'
            if (op == "sal" or op == "sar") {
                indent(os, indnt);
                std::println(os, "{} {}, {}", op, dst_op, src_op);
                return;
            }

            // sign-extend 'src_op' to scratch register, then do the op, then
            // mov the sign-extended register to destination

            // note: when doing arithmetic between 2 memory locations, this code
            //       path is triggered by the use of an in-between scratch
            //       register

            const std::string reg_sx{
                alloc_scratch_register(src_loc_tk, os, indnt)};

            indent(os, indnt);
            std::println(os, "movsx {}, {}", reg_sx, src_op);

            indent(os, indnt);
            std::println(os, "{} {}, {}", op, dst_op, reg_sx);

            free_scratch_register(src_loc_tk, os, indnt, reg_sx);
            return;
        }

        // dst_size < src_size
        // truncation will happen

        if (is_memory_operand(dst_op) and is_memory_operand(src_op)) {
            // both operands are memory references
            // use scratch register for transfer
            const std::string reg{
                alloc_scratch_register(src_loc_tk, os, indnt)};
            const std::string reg_sized{
                get_sized_register_operand(reg, dst_size)};
            indent(os, indnt);
            std::println(os, "mov {}, {}", reg_sized,
                         get_sized_memory_operand(src_op, dst_size));
            indent(os, indnt);
            std::println(os, "{} {}, {}", op, dst_op, reg_sized);
            free_scratch_register(src_loc_tk, os, indnt, reg);
            return;
        }

        // not both operands are memory references
        const bool dst_is_reg{is_operand_register(dst_op)};
        const bool src_is_reg{is_operand_register(src_op)};

        if (dst_is_reg and src_is_reg) {
            indent(os, indnt);
            std::println(os, "{} {}, {}", op, dst_op,
                         get_sized_register_operand(src_op, dst_size));
            return;
        }

        if (dst_is_reg) {
            indent(os, indnt);
            std::println(os, "{} {}, {}", op, dst_op,
                         is_memory_operand(src_op)
                             ? get_sized_memory_operand(src_op, dst_size)
                             : src_op);
            // note: handle constants by not applying size on 'src_op'
            return;
        }

        if (src_is_reg) {
            indent(os, indnt);
            std::println(os, "{} {}, {}", op, dst_op,
                         get_sized_register_operand(src_op, dst_size));
            return;
        }

        // constant to memory

        // note: constants have the default size of qword so when assigning
        //       to smaller sizes this code path is taken
        // todo: check for overflow
        indent(os, indnt);
        std::println(os, "{} {}, {}", op, dst_op, src_op);
    }

    auto comment_source(const statement& st, std::ostream& os,
                        const size_t indnt) const -> void {

        comment_start(st.tok(), os, indnt);
        std::stringstream ss;
        st.source_to(ss);
        std::println(
            os, "{}",
            std::regex_replace(std::regex_replace(ss.str(), regex_trim_, ""),
                               regex_ws_, " "));
    }

    auto comment_source(const statement& st, std::ostream& os,
                        const size_t indnt, const std::string_view dst,
                        const std::string_view op) const -> void {

        comment_start(st.tok(), os, indnt);
        std::stringstream ss;
        std::print(ss, "{} {} ", dst, op);
        st.source_to(ss);
        std::string res{std::regex_replace(ss.str(), regex_ws_, " ")};
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

    auto exit_block() -> void {
        const frame& frm{frames_.back()};
        assert(frm.is_block());
        frames_.pop_back();
    }

    auto exit_func(const std::string_view name) -> void {
        const frame& frm{frames_.back()};
        assert(frm.is_func() and frm.is_name(name));
        frames_.pop_back();
    }

    auto exit_loop(const std::string_view name) -> void {
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
        assert(frames_.empty());
        assert(named_registers_.size() == named_registers_initial_size_);
        assert(scratch_registers_.size() == scratch_registers_initial_size_);
        usage_max_frame_count_ = 0;
        usage_max_scratch_regs_ = 0;
    }

    auto free_named_register(const token& src_loc_tk, std::ostream& os,
                             const size_t indnt, const std::string_view reg)
        -> void {

        comment_start(src_loc_tk, os, indnt);
        std::println(os, "free named register '{}'", reg);

        assert(allocated_registers_.back().name == reg);

        named_registers_.emplace_back(
            std::move(allocated_registers_.back().name));
        allocated_registers_.pop_back();
    }

    auto free_scratch_register(const token& src_loc_tk, std::ostream& os,
                               const size_t indnt, const std::string_view reg)
        -> void {

        comment_start(src_loc_tk, os, indnt);
        std::println(os, "free scratch register '{}'", reg);

        assert(allocated_registers_.back().name == reg);

        scratch_registers_.emplace_back(
            std::move(allocated_registers_.back().name));
        allocated_registers_.pop_back();
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
                                  const std::string_view name) const
        -> const type& {

        if (not funcs_.has(name)) {
            throw compiler_exception{
                src_loc_tk, std::format("function '{}' not found", name)};
        }

        return *funcs_.get_const_ref(name).type_ptr;
    }

    [[nodiscard]] auto get_lea_operand(std::ostream& os, const size_t indent,
                                       const statement& src,
                                       const ident_info& src_info,
                                       std::vector<std::string>& lea_registers)
        -> operand {

        if (not src.is_indexed() and not src_info.has_lea()) {
            return src_info.operand;
        }

        operand op{src.compile_lea(src.tok(), *this, os, indent, lea_registers,
                                   "", src_info.lea_path)};
        op.size = src_info.type().size();
        return op;
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

    [[nodiscard]] auto
    get_sized_register_operand(const std::string_view operand,
                               const size_t size) const -> std::string {

        //? sort of ugly
        if (operand == "rax") {
            switch (size) {
            case size_qword:
                return "rax";
            case size_dword:
                return "eax";
            case size_word:
                return "ax";
            case size_byte:
                return "al";
            default:
                std::unreachable();
            }
        }
        if (operand == "rbx") {
            switch (size) {
            case size_qword:
                return "rbx";
            case size_dword:
                return "ebx";
            case size_word:
                return "bx";
            case size_byte:
                return "bl";
            default:
                std::unreachable();
            }
        }
        if (operand == "rcx") {
            switch (size) {
            case size_qword:
                return "rcx";
            case size_dword:
                return "ecx";
            case size_word:
                return "cx";
            case size_byte:
                return "cl";
            default:
                std::unreachable();
            }
        }
        if (operand == "rdx") {
            switch (size) {
            case size_qword:
                return "rdx";
            case size_dword:
                return "edx";
            case size_word:
                return "dx";
            case size_byte:
                return "dl";
            default:
                std::unreachable();
            }
        }
        if (operand == "rbp") {
            switch (size) {
            case size_qword:
                return "rbp";
            case size_dword:
                return "ebp";
            case size_word:
                return "bp";
            default:
                std::unreachable();
            }
        }
        if (operand == "rsi") {
            switch (size) {
            case size_qword:
                return "rsi";
            case size_dword:
                return "esi";
            case size_word:
                return "si";
            default:
                std::unreachable();
            }
        }
        if (operand == "rdi") {
            switch (size) {
            case size_qword:
                return "rdi";
            case size_dword:
                return "edi";
            case size_word:
                return "di";
            default:
                std::unreachable();
            }
        }
        if (operand == "rsp") {
            switch (size) {
            case size_qword:
                return "rsp";
            case size_dword:
                return "esp";
            case size_word:
                return "sp";
            default:
                std::unreachable();
            }
        }

        std::smatch match;
        const std::string operand_str{operand};
        if (not std::regex_search(operand_str, match,
                                  regex_nasm_number_register_)) {
            std::unreachable();
        }
        const std::string rnbr{match[1]};
        switch (size) {
        case size_qword:
            return std::format("r{}", rnbr);
        case size_dword:
            return std::format("r{}d", rnbr);
        case size_word:
            return std::format("r{}w", rnbr);
        case size_byte:
            return std::format("r{}b", rnbr);
        default:
            std::unreachable();
        }
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

        return *types_.get_const_ref(name_str);
    }

    [[nodiscard]] auto get_type_void() const -> const type& {
        return *type_void_;
    }

    [[nodiscard]] auto has_lea(const statement& st) const -> bool {
        if (not st.is_identifier()) {
            return false;
        }

        std::string_view id_base{get_before_dot(st.identifier())};

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
                id_base = get_before_dot(alias.to);
            }
        }

        return false;
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

    [[nodiscard]] auto is_func_builtin(const token& src_loc_tk,
                                       const std::string_view name) const
        -> bool {

        if (not funcs_.has(name)) {
            throw compiler_exception{
                src_loc_tk, std::format("function '{}' not found", name)};
        }

        return funcs_.get_const_ref(name).def == nullptr;
    }

    [[nodiscard]] auto make_ident_info(const statement& st) const
        -> ident_info {

        return make_ident_info_or_throw(st.tok(), st.identifier());
    }

    [[nodiscard]] auto make_ident_info(const token& src_loc_tk,
                                       const std::string_view ident) const
        -> ident_info {

        if (ident.empty()) {
            throw compiler_exception(src_loc_tk, "identifier is empty");
        }

        return make_ident_info_or_throw(src_loc_tk, ident);
    }

    [[nodiscard]] auto
    make_ident_info_for_register(const std::string_view reg) const
        -> ident_info {

        const size_t reg_size{get_size_from_register_operand(reg)};
        const type& tpe{get_builtin_type_for_size(reg_size)};
        //? unary ops?
        return {
            .id{reg},
            .elem_path{std::string{reg}},
            .type_path{&tpe},
            .lea_path{""},
            .operand{reg},
            .ident_type{ident_info::ident_type::REGISTER},
        };
    }

    auto rep_movs(const token& src_loc_tk, std::ostream& os, const size_t indnt,
                  const std::string_view& src, const std::string_view& dst,
                  const size_t bytes_count) -> void {

        if (bytes_count > threshold_for_rep_movs) {
            alloc_named_register_or_throw(src_loc_tk, os, indnt, "rsi");
            alloc_named_register_or_throw(src_loc_tk, os, indnt, "rdi");
            alloc_named_register_or_throw(src_loc_tk, os, indnt, "rcx");

            toc::asm_lea(os, indnt, "rsi", src);
            toc::asm_lea(os, indnt, "rdi", dst);
            toc::asm_cmd(src_loc_tk, os, indnt, "mov", "rcx",
                         std::format("{}", bytes_count));
            toc::asm_rep_movs(os, indnt, 'b');

            free_named_register(src_loc_tk, os, indnt, "rcx");
            free_named_register(src_loc_tk, os, indnt, "rdi");
            free_named_register(src_loc_tk, os, indnt, "rsi");
            return;
        }

        comment_start(src_loc_tk, os, indnt);
        std::println(os, "size <= {} B, use mov", threshold_for_rep_movs);

        alloc_named_register_or_throw(src_loc_tk, os, indnt, "rax");

        size_t rest{bytes_count};
        const size_t qword_movs{rest / toc::size_qword};

        operand op_src{src};
        operand op_dst{dst};

        for (size_t i{}; i < qword_movs; ++i) {
            asm_cmd(src_loc_tk, os, indnt, "mov", "rax",
                    op_src.str(operand::size_qword));
            asm_cmd(src_loc_tk, os, indnt, "mov",
                    op_dst.str(operand::size_qword), "rax");
            op_src.displacement += operand::size_qword;
            op_dst.displacement += operand::size_qword;
            rest -= operand::size_qword;
        }

        // mov the reminder
        if ((rest / toc::size_dword) != 0) {
            asm_cmd(src_loc_tk, os, indnt, "mov", "eax",
                    op_src.str(operand::size_dword));
            asm_cmd(src_loc_tk, os, indnt, "mov",
                    op_dst.str(operand::size_dword), "eax");
            op_src.displacement += operand::size_dword;
            op_dst.displacement += operand::size_dword;
            rest -= operand::size_dword;
        }

        if ((rest / toc::size_word) != 0) {
            asm_cmd(src_loc_tk, os, indnt, "mov", "ax",
                    op_src.str(operand::size_word));
            asm_cmd(src_loc_tk, os, indnt, "mov",
                    op_dst.str(operand::size_word), "ax");
            op_src.displacement += operand::size_word;
            op_dst.displacement += operand::size_word;
            rest -= operand::size_word;
        }

        if (rest) {
            asm_cmd(src_loc_tk, os, indnt, "mov", "al",
                    op_src.str(operand::size_byte));
            asm_cmd(src_loc_tk, os, indnt, "mov",
                    op_dst.str(operand::size_byte), "al");
        }

        free_named_register(src_loc_tk, os, indnt, "rax");
    }

    auto rep_movs(const token& src_loc_tk, std::ostream& os, const size_t indnt,
                  const statement& src_stmt, const ident_info& src_info,
                  std::string_view dst, const size_t bytes_count) -> void {

        std::vector<std::string> allocated_registers;
        std::string src;
        if (src_stmt.is_indexed() or src_info.has_lea()) {
            const operand addr{src_stmt.compile_lea(src_loc_tk, *this, os,
                                                    indnt, allocated_registers,
                                                    "", src_info.lea_path)};
            src = addr.address_str();
        } else {
            src = src_info.operand.address_str();
        }

        rep_movs(src_loc_tk, os, indnt, src, dst, bytes_count);

        for (const std::string& reg :
             allocated_registers | std::views::reverse) {
            free_scratch_register(src_loc_tk, os, indnt, reg);
        }
    }

    auto rep_stos(const token& src_loc_tk, std::ostream& os, const size_t indnt,
                  const std::string_view& dst, const size_t bytes_count,
                  const int8_t value) -> void {

        // todo: heuristics to use mov when less than 64 bytes

        if (bytes_count > threshold_for_rep_stos) {
            // mov al, byte_val        ; byte value to store (e.g., 0x00)
            // mov rdi, dest_addr      ; destination pointer
            // mov rcx, byte_count     ; number of bytes to write
            // rep stosb               ; store al into [rdi], rcx times (rdi++)

            alloc_named_register_or_throw(src_loc_tk, os, indnt, "rax");
            alloc_named_register_or_throw(src_loc_tk, os, indnt, "rdi");
            alloc_named_register_or_throw(src_loc_tk, os, indnt, "rcx");

            if (value == 0) {
                toc::asm_cmd(src_loc_tk, os, indnt, "xor", "al", "al");
            } else {
                toc::asm_cmd(src_loc_tk, os, indnt, "mov", "al",
                             std::format("{}", value));
            }
            toc::asm_lea(os, indnt, "rdi", dst);
            toc::asm_cmd(src_loc_tk, os, indnt, "mov", "rcx",
                         std::format("{}", bytes_count));
            toc::asm_rep_stos(os, indnt, 'b');

            free_named_register(src_loc_tk, os, indnt, "rcx");
            free_named_register(src_loc_tk, os, indnt, "rdi");
            free_named_register(src_loc_tk, os, indnt, "rax");
            return;
        }

        comment_start(src_loc_tk, os, indnt);
        std::println(os, "size <= {} B, use mov", threshold_for_rep_stos);

        const uint8_t byte_u8{static_cast<uint8_t>(value)};
        const uint64_t val_qword{static_cast<uint64_t>(byte_u8) *
                                 0x0101010101010101ULL};
        const uint32_t val_dword{static_cast<uint32_t>(val_qword)};
        const uint16_t val_word{static_cast<uint16_t>(val_qword)};

        const std::string str_qword{val_qword ? std::format("0x{:x}", val_qword)
                                              : "0"};
        const std::string str_dword{val_dword ? std::format("0x{:x}", val_dword)
                                              : "0"};
        const std::string str_word{val_word ? std::format("0x{:x}", val_word)
                                            : "0"};
        const std::string str_byte{byte_u8 ? std::format("0x{:x}", byte_u8)
                                           : "0"};

        size_t rest{bytes_count};
        const size_t qword_movs{rest / toc::size_qword};

        operand op{dst};

        for (size_t i{}; i < qword_movs; ++i) {
            asm_cmd(src_loc_tk, os, indnt, "mov", op.str(operand::size_qword),
                    str_qword);
            op.displacement += operand::size_qword;
            rest -= operand::size_qword;
        }

        // mov the reminder
        if ((rest / toc::size_dword) != 0) {
            asm_cmd(src_loc_tk, os, indnt, "mov", op.str(operand::size_dword),
                    str_dword);
            op.displacement += operand::size_dword;
            rest -= operand::size_dword;
        }

        if ((rest / toc::size_word) != 0) {
            asm_cmd(src_loc_tk, os, indnt, "mov", op.str(operand::size_word),
                    str_word);
            op.displacement += operand::size_word;
            rest -= operand::size_word;
        }

        if (rest) {
            asm_cmd(src_loc_tk, os, indnt, "mov", op.str(operand::size_byte),
                    str_byte);
        }
    }

    [[nodiscard]] auto regex_ws() const -> const std::regex& {
        return regex_ws_;
    }

    [[nodiscard]] auto regex_nasm_comment() const -> const std::regex& {
        return regex_nasm_comment_;
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

    [[nodiscard]] static auto make_ident_info_empty() -> ident_info {
        return {
            .id{},
            .elem_path{},
            .type_path{},
            .lea_path{},
            .operand{},
        };
    }

  private:
    // -------------------------------------------------------------------------
    // private non-special functions (sorted alphabetically)
    // -------------------------------------------------------------------------

    [[nodiscard]] auto
    get_builtin_type_for_operand(const token& src_loc_tk,
                                 const std::string_view op) const
        -> const type& {

        //? sort of ugly
        if (op.starts_with("qword")) {
            return get_type_or_throw(src_loc_tk, "i64");
        }
        if (op.starts_with("dword")) {
            return get_type_or_throw(src_loc_tk, "i32");
        }
        if (op.starts_with("word")) {
            return get_type_or_throw(src_loc_tk, "i16");
        }
        if (op.starts_with("byte")) {
            return get_type_or_throw(src_loc_tk, "i8");
        }

        std::unreachable();
    }

    // todo: formalize this
    [[nodiscard]] auto get_builtin_type_for_size(const size_t size) const
        -> const type& {

        switch (size) {
        case size_qword:
            return *types_.get_const_ref("i64");
        case size_dword:
            return *types_.get_const_ref("i32");
        case size_word:
            return *types_.get_const_ref("i16");
        case size_byte:
            return *types_.get_const_ref("i8");
        default:
            std::unreachable();
        }
    }

    [[nodiscard]] auto get_stack_size() const -> size_t {
        size_t nbytes{};
        for (const auto& frm : frames_) {
            nbytes += frm.allocated_stack_size();
        }
        return nbytes;
    }

    [[nodiscard]] auto is_in_main() const -> bool {
        for (const auto& frm : frames_ | std::views::reverse) {
            if (frm.is_func()) {
                return frm.name() == "main";
            }
        }

        return false;
    }

    // reviewed: 2026-09-09
    [[nodiscard]] auto
    make_ident_info_or_empty(const token& src_loc_tk,
                             const std::string_view ident) const -> ident_info {

        ident_path id{std::string{ident}};

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

        size_t i{frames_.size()};
        while (i) {
            --i;

            const frame& frm{frames_[i]};

            // does this frame contain the variable?
            if (frm.has_var(id.base())) {
                break;
            }

            if (frm.is_func()) {

                // root frame of the function
                // from here on aliases are followed to the actual variable
                // referred to

                if (not frm.has_alias(id.base())) {
                    // add an empty
                    lea_path.emplace_back("");
                    break;
                }

                // this is an alias, continue resolving until it is a variable,
                // register or constant

                const alias_info& alias{frm.get_alias(id.base())};

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

                // this is an alia
                // e.g.
                //   res -> pt.x becomes pt.x
                //   pt.x -> p becomes p.x
                //   lnk.count -> world.roome.link becomes
                //   world.roome.link.count

                for (const std::string& s : id.path() | std::views::drop(1)) {
                    new_id.append(s);
                }
                id = new_id;

                continue;
            }
        }

        const frame& frm{frames_[i]};

        if (frm.has_var(id.base())) {

            const var_info& var{frm.get_var_const_ref(id.base())};

            ident_info ii{
                var.type_ptr->accessor(src_loc_tk, ident, id.path(), var)};

            lea_path.resize(id.path().size());
            // note: pad with empty for the remaining elements in the id path

            std::ranges::reverse(lea_path);
            // note: reverse it since it was constructed while traversing
            //       upwards in the frame stack but 'elem_path' and 'type_path'
            //       are ordered from the top down

            ii.lea_path = lea_path;

            if (not ii.type().is_built_in()) {
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
            // location of 'rooms[2]' cannot  be deduced statically, thus the
            // last lea encountered is the starting point when accessing
            // identifiers

            // start from the lea address and calculate offset to referred field
            const std::span<std::string> elem_path_from_lea{
                std::span{ii.elem_path}.subspan(lea_index)};

            // navigate to referred element and get offset
            const size_t offset{ii.type_path[lea_index]->field_offset(
                src_loc_tk, elem_path_from_lea)};

            ii.operand = operand{lea};
            if (offset != 0) {
                ii.operand.displacement += static_cast<int>(offset);
            }
            ii.operand.size = ii.type().size();

            return ii;
        }

        // is it a register?
        if (const size_t reg_size{get_size_from_register_operand(id.str())};
            reg_size != 0) {
            const type& tpe{get_builtin_type_for_size(reg_size)};
            //? unary ops?
            return {
                .id{ident},
                .elem_path{id.str()},
                .type_path{&tpe},
                .lea_path{""},
                .operand{id.str()},
                .ident_type{ident_info::ident_type::REGISTER},
            };
        }

        // is it a register reference to memory?
        if (is_operand_indirect_addressing(id.str())) {
            // get the size: e.g. "dword [r15]"
            return {
                .id{ident},
                .elem_path{id.str()},
                .type_path{&get_builtin_type_for_operand(src_loc_tk, id.str())},
                .lea_path{""},
                .operand{id.str()},
                .ident_type{ident_info::ident_type::VAR},
            };
        }

        // if ident is "bracketed" extract the contents between the brackets
        std::optional<std::string> ident_bracketed{
            get_text_between_brackets(ident)};

        if (ident_bracketed) {
            id = ident_path{*ident_bracketed};
        }

        // is 'id' an integer?
        if (const std::optional<int64_t> value{
                parse_to_constant(src_loc_tk, id.str())};
            value) {
            return {
                .id{ident},
                .elem_path{id.str()},
                .type_path{&get_type_default()},
                .lea_path{""},
                .operand{id.str(), true},
                .const_value{*value},
                .ident_type{ident_info::ident_type::CONST},
            };
        }

        // is it a boolean constant?
        if (id.base() == "true") {
            return {
                .id{ident},
                .elem_path{id.str()},
                .type_path{&get_type_default()},
                .lea_path{""},
                .operand{"true", true},
                .const_value{1},
                .ident_type{ident_info::ident_type::CONST},
            };
        }

        if (id.base() == "false") {
            return {
                .id{ident},
                .elem_path{id.str()},
                .type_path{&get_type_default()},
                .lea_path{""},
                .operand{"false", true},
                .const_value{},
                .ident_type{ident_info::ident_type::CONST},
            };
        }

        // is 'id' a constant?
        if (constants_.has(id.str())) {
            const constant& c{constants_.get_const_ref(id.str())};
            return {
                .id{ident},
                .elem_path{id.str()},
                .type_path{&get_type_default()},
                .lea_path{""},
                .operand{id.str(), true},
                .const_value{c.value},
                .ident_type{ident_info::ident_type::CONST},
            };
        }

        // not resolved, return empty info
        return {.id{}, .elem_path{}, .type_path{}, .lea_path{}, .operand{}};
    }

    // helper: call make_ident_info_or_empty and throw if unresolved
    [[nodiscard]] auto
    make_ident_info_or_throw(const token& src_loc_tk,
                             const std::string_view ident) const -> ident_info {

        const ident_info id_info{make_ident_info_or_empty(src_loc_tk, ident)};
        if (not id_info.id.empty()) {
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

    static auto asm_jmp(std::ostream& os, const size_t indnt,
                        const std::string_view label) -> void {

        indent(os, indnt);
        std::println(os, "jmp {}", label);
    }

    static auto asm_jcc(std::ostream& os, const size_t indnt,
                        const std::string_view comparison,
                        const std::string_view label) -> void {

        indent(os, indnt);
        std::println(os, "j{} {}", comparison, label);
    }

    static auto asm_label(std::ostream& os, const size_t indnt,
                          const std::string_view label) -> void {

        indent(os, indnt);
        std::println(os, "{}:", label);
    }

    static auto asm_lea(std::ostream& os, const size_t indnt,
                        const std::string_view dst,
                        const std::string_view operand) -> void {

        indent(os, indnt);
        std::println(os, "lea {}, [{}]", dst, operand);
    }

    static auto asm_neg(std::ostream& os, const size_t indnt,
                        const std::string_view operand) -> void {

        indent(os, indnt);
        std::println(os, "neg {}", operand);
    }

    static auto asm_not(std::ostream& os, const size_t indnt,
                        const std::string_view operand) -> void {

        indent(os, indnt);
        std::println(os, "not {}", operand);
    }

    static auto asm_pop(std::ostream& os, const size_t indnt,
                        const std::string_view operand) -> void {

        indent(os, indnt);
        std::println(os, "pop {}", operand);
    }

    static auto asm_push(std::ostream& os, const size_t indnt,
                         const std::string_view operand) -> void {

        indent(os, indnt);
        std::println(os, "push {}", operand);
    }

    // size: b, w, d, q for different sizings
    static auto asm_rep_movs(std::ostream& os, const size_t indnt,
                             const char size) -> void {

        indent(os, indnt);
        std::println(os, "rep movs{}", size);
    }

    // size: b, w, d, q for different sizings
    static auto asm_rep_stos(std::ostream& os, const size_t indnt,
                             const char size) -> void {

        indent(os, indnt);
        std::println(os, "rep stos{}", size);
    }

    // size: b, w, d, q for different sizings
    static auto asm_repe_cmps(std::ostream& os, const size_t indnt,
                              const char size) -> void {

        indent(os, indnt);
        std::println(os, "repe cmps{}", size);
    }

    static auto asm_setcc(std::ostream& os, const size_t indnt,
                          const std::string_view comparison,
                          const std::string_view operand) -> void {

        indent(os, indnt);
        std::println(os, "set{} {}", comparison, operand);
    }

    [[nodiscard]] static auto get_data_def(const size_t size)
        -> std::string_view {
        switch (size) {
        case size_qword:
            return def_data_qword;
        case size_dword:
            return def_data_dword;
        case size_word:
            return def_data_word;
        case size_byte:
            return def_data_byte;
        default:
            std::unreachable();
        }
    }

    [[nodiscard]] static auto
    get_field_offset_in_type(const token& src_loc_tk, const type& tp,
                             const std::string_view field_name) -> size_t {

        size_t accum{};
        for (const type_field& f : tp.fields()) {
            if (f.name == field_name) {
                return accum;
            }
            accum += f.size;
        }

        throw compiler_exception(
            src_loc_tk, std::format("field '{}' not found in type '{}'",
                                    field_name, tp.name()));
    }

    [[nodiscard]] static auto get_size_specifier(const size_t size)
        -> std::string_view {

        switch (size) {
        case size_qword:
            return "qword";
        case size_dword:
            return "dword";
        case size_word:
            return "word";
        case size_byte:
            return "byte";
        default:
            std::unreachable();
        }
    }

    static auto indent(std::ostream& os, const size_t indnt,
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
        for (size_t i{1}; i < indnt; ++i) {
            std::print(os, "    ");
        }
    }

    [[nodiscard]] static auto
    is_operand_indirect_addressing(const std::string_view op) -> bool {
        if (auto expr{toc::get_text_between_brackets(op)}; expr) {
            return true;
        }

        return false;
    }

    [[nodiscard]] static auto is_operand_register(const std::string_view op)
        -> bool {

        const size_t size{get_size_from_register_operand(op)};
        return size != 0;
    }

    [[nodiscard]] static auto line_and_col_num_for_char_index(
        const size_t at_line, size_t char_index_in_source,
        const std::string_view src) -> std::pair<size_t, size_t> {

        if (char_index_in_source >= src.size()) {
            return {at_line, 0};
        }

        size_t at_col{};
        while (src[char_index_in_source] != '\n') {
            ++at_col;
            if (char_index_in_source == 0) {
                break;
            }
            --char_index_in_source;
        }

        return {at_line, at_col};
    }

// pragma below for clang++ to not generate warning stemming from
// 'std::from_chars' requiring pointers
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
    [[nodiscard]] static auto parse_to_constant(const token& src_loc_tk,
                                                const std::string_view str)
        -> std::optional<int64_t> {

        // is it hex?
        if (str.starts_with("0x") or str.starts_with("0X")) {
            constexpr size_t base_hex{16};
            // hex
            int64_t value{};
            std::string_view sv{str};
            sv.remove_prefix(2); // skip "0x" or "0X"

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto result{std::from_chars(sv.data(), sv.data() + sv.size(), value,
                                        base_hex)};
            if (result.ec == std::errc::result_out_of_range) {
                throw compiler_exception{
                    src_loc_tk,
                    std::format("constant '{}' is out of range", str)};
            }
            const std::string_view remainder{
                result.ptr,
                static_cast<size_t>(sv.data() + sv.size() - result.ptr)};
            if (result.ec == std::errc{} and
                (remainder.empty() or remainder == ";")) {
                return value;
            }
        }

        // is it binary?
        if (str.starts_with("0b") or str.starts_with("0B")) {
            constexpr size_t base_binary{2};
            // binary
            int64_t value{};
            std::string_view sv{str};
            sv.remove_prefix(2); // skip "0b" or "0B"

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto result{std::from_chars(sv.data(), sv.data() + sv.size(), value,
                                        base_binary)};
            if (result.ec == std::errc::result_out_of_range) {
                throw compiler_exception{
                    src_loc_tk,
                    std::format("constant '{}' is out of range", str)};
            }
            const std::string_view remainder{
                result.ptr,
                static_cast<size_t>(sv.data() + sv.size() - result.ptr)};
            if (result.ec == std::errc{} and
                (remainder.empty() or remainder == ";")) {
                return value;
            }
        }

        // try decimal digit
        {
            int64_t value{};
            const std::string_view sv{str};
            // note: using 'std::string_view' for 'clang-tidy' to not
            // trigger the warning
            // 'cppcoreguidelines-pro-bounds-pointer-arithmetic'

            auto result{
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                std::from_chars(sv.data(), sv.data() + sv.size(), value)};
            if (result.ec == std::errc::result_out_of_range) {
                throw compiler_exception{
                    src_loc_tk,
                    std::format("constant '{}' is out of range", str)};
            }
            const std::string_view remainder{
                result.ptr,
                static_cast<size_t>(sv.data() + sv.size() - result.ptr)};
            if (result.ec == std::errc{} and
                (remainder.empty() or remainder == ";")) {
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

    [[nodiscard]] static auto get_before_dot(std::string_view str)
        -> std::string_view {

        if (const size_t pos{str.find('.')}; pos != std::string_view::npos) {
            return str.substr(0, pos);
        }

        return str;
    }

    [[nodiscard]] auto get_operand_size(const std::string_view operand) const
        -> size_t {

        //? sort of ugly
        if (operand.starts_with("qword")) {
            return size_qword;
        }
        if (operand.starts_with("dword")) {
            return size_dword;
        }
        if (operand.starts_with("word")) {
            return size_word;
        }
        if (operand.starts_with("byte")) {
            return size_byte;
        }
        if (is_operand_register(operand)) {
            return get_size_from_register_operand(operand);
        }
        if (const size_t reg_size{get_size_from_register_operand(operand)}) {
            return reg_size;
        }
        // constant
        return get_type_default().size();
    }

    // returns 0 if not a register
    static auto get_size_from_register_operand(const std::string_view operand)
        -> size_t {

        if (operand == "rax" || operand == "rbx" || operand == "rcx" ||
            operand == "rdx" || operand == "rbp" || operand == "rsi" ||
            operand == "rdi" || operand == "rsp" || operand == "r8" ||
            operand == "r9" || operand == "r10" || operand == "r11" ||
            operand == "r12" || operand == "r13" || operand == "r14" ||
            operand == "r15") {
            return size_qword;
        }

        if (operand == "eax" || operand == "ebx" || operand == "ecx" ||
            operand == "edx" || operand == "ebp" || operand == "esi" ||
            operand == "edi" || operand == "esp" || operand == "r8d" ||
            operand == "r9d" || operand == "r10d" || operand == "r11d" ||
            operand == "r12d" || operand == "r13d" || operand == "r14d" ||
            operand == "r15d") {
            return size_dword;
        }

        if (operand == "ax" || operand == "bx" || operand == "cx" ||
            operand == "dx" || operand == "bp" || operand == "si" ||
            operand == "di" || operand == "sp" || operand == "r8w" ||
            operand == "r9w" || operand == "r10w" || operand == "r11w" ||
            operand == "r12w" || operand == "r13w" || operand == "r14w" ||
            operand == "r15w") {
            return size_word;
        }

        if (operand == "al" || operand == "ah" || operand == "bl" ||
            operand == "bh" || operand == "cl" || operand == "ch" ||
            operand == "dl" || operand == "dh" || operand == "r8b" ||
            operand == "r9b" || operand == "r10b" || operand == "r11b" ||
            operand == "r12b" || operand == "r13b" || operand == "r14b" ||
            operand == "r15b") {
            return size_byte;
        }

        return 0;
    }

    [[nodiscard]] static auto
    get_text_between_brackets(const std::string_view str)
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

    [[nodiscard]] static auto
    get_sized_memory_operand(const std::string_view operand,
                             const size_t new_size) -> std::string {

        const size_t pos{operand.find('[')};
        assert(pos != std::string_view::npos);
        return std::format("{} {}", get_size_specifier(new_size),
                           operand.substr(pos));
    }

    [[nodiscard]] static auto is_memory_operand(const std::string_view operand)
        -> bool {

        return operand.find_first_of('[') != std::string::npos;
    }
};
