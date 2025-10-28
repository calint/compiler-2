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
#include <string_view>

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

    // info about the function returns
    std::optional<func_return_info> func_ret_;

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

    auto add_var(const var_info& var) -> void {
        if (var.stack_idx < 0) {
            // variable, increase allocated stack size
            allocated_stack_ +=
                var.type_ptr->size() * (var.is_array ? var.array_size : 1);
        }

        vars_.put(var.name, var);
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

    [[nodiscard]] auto get_func_return_info() const
        -> const std::optional<func_return_info>& {
        return func_ret_;
    }

    [[nodiscard]] auto get_var_const_ref(const std::string_view name) const
        -> const var_info& {

        return vars_.get_const_ref(name);
    }

    [[nodiscard]] auto has_alias(const std::string_view name) const -> bool {
        return aliases_.has(name);
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

struct field_info {
    const stmt_def_field* def{};
    token declared_at_tk; // token for position in the source
    bool is_str{};
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

    [[nodiscard]] auto path() const -> std::span<const std::string> {
        return path_;
    }

    [[nodiscard]] auto str() const -> const std::string& { return id_; }

    auto append(const std::string_view path_elem) {
        id_ = std::format("{}.{}", id_, path_elem);
        refresh_path();
    }

    auto set_base(std::string_view name) -> void { path_[0] = name; }
};

struct operand {
    std::string base_register;
    std::string index_register;
    int32_t displacement;
    uint8_t scale;

    operand() : displacement{0}, scale{1} {}

    explicit operand(const std::string_view operand_sv)
        : displacement{0}, scale{1} {

        if (operand_sv.empty()) {
            return;
        }
        // note: regex handles only "base + index * scale +/- displacement" with
        //       optional index, scale and displacement
        const std::regex pattern(
            // whitespace
            R"(^\s*)"
            // base register (must start with a letter)
            R"(([a-z][a-z0-9]*)?)"
            // +index (must start with a letter)
            R"((?:\s*\+\s*([a-z][a-z0-9]*?))?)"
            // *scale
            R"((?:\s*\*\s*([1248]))?)"
            // +/- displacement
            R"((?:\s*([+-])\s*(\d+))?)"
            // whitespace
            R"(\s*$)");

        constexpr size_t match_base_register{1};
        constexpr size_t match_index_register{2};
        constexpr size_t match_scale{3};
        constexpr size_t match_displacement_op{4};
        constexpr size_t match_displacement{5};

        std::smatch matches;
        const std::string operand_str{operand_sv};
        if (not std::regex_match(operand_str, matches, pattern)) {
            throw std::invalid_argument(
                std::format("invalid NASM operand format: {}", operand_str));
        }

        if (matches[match_base_register].matched) {
            base_register = matches[match_base_register].str();
        }

        if (matches[match_index_register].matched) {
            index_register = matches[match_index_register].str();
        }

        if (matches[match_scale].matched) {
            scale = static_cast<uint8_t>(std::stoi(matches[match_scale].str()));
        }

        if (matches[match_displacement_op].matched and
            matches[match_displacement].matched) {

            const int disp_value = std::stoi(matches[match_displacement].str());
            if (matches[match_displacement_op].str() == "-") {
                displacement = -disp_value;
            } else {
                displacement = disp_value;
            }
        }
    }

    [[nodiscard]] auto is_indexed() const -> bool {
        return not index_register.empty() or displacement != 0;
    }

    [[nodiscard]] auto to_string() const -> std::string {
        std::string result;

        if (not base_register.empty()) {
            result += base_register;
        }

        if (not index_register.empty()) {
            if (not result.empty()) {
                result += " + ";
            }
            result += index_register;
            if (scale > 1) {
                result += " * " + std::to_string(scale);
            }
        }

        if (displacement != 0) {
            if (not result.empty()) {
                if (displacement > 0) {
                    result += " + ";
                } else {
                    result += " - ";
                }
            }
            result +=
                std::to_string(displacement < 0 ? -displacement : displacement);
        }

        return result;
    }
};

class toc final {
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
    std::vector<std::string> allocated_registers_;
    std::vector<std::string> allocated_registers_src_locs_; // source locations
    std::vector<const stmt_def_func*> func_defs_;
    lut<field_info> fields_;
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

    std::regex regex_ws_{R"(\s+)"};
    std::regex regex_trim_{R"(^\s+|\s+$)"};
    std::regex regex_nasm_comment_{R"(^\s*;.*$)"};
    std::regex regex_nasm_number_register_{R"(r(\d+))"};

  public:
    static constexpr size_t size_qword{8};
    static constexpr size_t size_dword{4};
    static constexpr size_t size_word{2};
    static constexpr size_t size_byte{1};

    toc(const std::string_view source, const bool bounds_check_upper,
        const bool bounds_check_lower, const bool bounds_check_with_line)
        : source_{source}, bounds_check_upper_{bounds_check_upper},
          bounds_check_with_line_{bounds_check_with_line},
          bounds_check_lower_{bounds_check_lower} {}

    ~toc() = default;

    toc() = delete;
    toc(const toc&) = default;
    toc(toc&&) = default;
    auto operator=(const toc&) -> toc& = delete;
    auto operator=(toc&&) -> toc& = delete;

    // -------------------------------------------------------------------------
    // public non-special functions (sorted alphabetically)
    // -------------------------------------------------------------------------

    auto add_alias(const alias_info& ai) -> void {
        frames_.back().add_alias(ai);
    }

    auto add_field(const token& src_loc_tk, std::string name,
                   const stmt_def_field& fld_def, const bool is_str_field)
        -> void {

        if (fields_.has(name)) {
            throw compiler_exception{
                src_loc_tk,
                std::format("field '{}' already defined at {}", name,
                            source_location_hr(
                                fields_.get_const_ref(name).declared_at_tk))};
        }

        fields_.put(std::move(name), {.def = &fld_def,
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
                                     .type_ptr = &return_type});

        if (func_def) {
            func_defs_.emplace_back(func_def);
        }
    }

    auto add_type(const token& src_loc_tk, const type& tpe) -> void {
        if (types_.has(tpe.name())) {
            //? todo. specify where the type has been defined
            throw compiler_exception{
                src_loc_tk,
                std::format("type '{}' already defined", tpe.name())};
        }

        types_.put(tpe.name(), &tpe);
    }

    auto add_var(const token& src_loc_tk, std::ostream& os, const size_t indnt,
                 var_info var) -> void {

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
        frames_.back().add_var(var);

        const size_t total_stack_size{get_stack_size()};
        usage_max_stack_size_ =
            std::max(total_stack_size, usage_max_stack_size_);

        // comment the resolved name
        const ident_info& name_info{make_ident_info(src_loc_tk, var.name)};

        comment_start(src_loc_tk, os, indnt);
        std::print(os, "{}: {}", var.name, name_info.type_ptr->name());
        if (var.array_size) {
            std::print(os, "[{}]", var.array_size);
        }
        std::println(os, " ({}B @ {})", name_info.type_ptr->size(),
                     name_info.operand);
    }

    auto alloc_named_register(const token& src_loc_tk, std::ostream& os,
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

        allocated_registers_.emplace_back(std::move(*reg_iter));
        allocated_registers_src_locs_.emplace_back(
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
            const size_t n{allocated_registers_.size()};
            for (size_t i{}; i < n; i++) {
                if (allocated_registers_[i] == reg) {
                    loc = allocated_registers_src_locs_[i];
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
                                const size_t indnt) -> std::string {

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

            // not both operands are memory references, `src_op` might be a
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
                // special case for `mov` which needs sign extended move
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

            // sign-extend `src_op` to scratch register, then do the op, then
            // mov the sign-extended register to destination

            // note: when doing arithmetic between 2 memory locations, this code
            //       path is triggered by the use of an in-between scratch
            //       register

            const std::string reg_sx{
                alloc_scratch_register(src_loc_tk, os, indnt)};

            indent(os, indnt);
            std::println(os, "movsx {}, {}", reg_sx, src_op);

            // todo fix for dst_op is memory operand
            // note: this path is never taken?
            if (is_memory_operand(dst_op)) {
                throw panic_exception{"toc:6"};
            }

            // dst_op is a register

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
        //? todo: check for overflow
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
        std::println(os, ";               max stack size: {}B",
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
                             const size_t indnt, const std::string_view reg)
        -> void {

        comment_start(src_loc_tk, os, indnt);
        std::println(os, "free named register '{}'", reg);

        assert(allocated_registers_.back() == reg);

        named_registers_.emplace_back(std::move(allocated_registers_.back()));
        allocated_registers_.pop_back();
        allocated_registers_src_locs_.pop_back();
    }

    auto free_scratch_register(const token& src_loc_tk, std::ostream& os,
                               const size_t indnt, const std::string_view reg)
        -> void {

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

    auto get_lea_operand(std::ostream& os, const size_t indent,
                         const statement& src, const ident_info& src_info,
                         std::vector<std::string>& lea_registers)
        -> std::string {

        if (not src.is_indexed() and not src_info.has_lea()) {
            return src_info.operand;
        }

        std::string lea{src.compile_lea(src.tok(), *this, os, indent,
                                        lea_registers, "", src_info.lea_path)};

        return std::format(
            "{} [{}]", toc::get_size_specifier(src_info.type_ptr->size()), lea);
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
        size_t i{frames_.size()};
        while (i) {
            i--;
            const frame& frm{frames_[i]};
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

        return make_ident_info_or_throw(src_loc_tk, ident);
    }

    auto rep_movs(const token& src_loc_tk, std::ostream& os, const size_t indnt,
                  const std::string_view src, std::string_view dst,
                  const size_t bytes_count) -> void {

        // ; Copy RCX bytes from RSI to RDI
        // mov rsi, source_addr    ; source pointer
        // mov rdi, dest_addr      ; destination pointer
        // mov rcx, byte_count     ; number of bytes
        // rep movsb               ; repeat: copy byte [RSI++] to [RDI++]

        alloc_named_register_or_throw(src_loc_tk, os, indnt, "rsi");
        alloc_named_register_or_throw(src_loc_tk, os, indnt, "rdi");
        alloc_named_register_or_throw(src_loc_tk, os, indnt, "rcx");

        toc::asm_lea(os, indnt, "rsi", src);
        toc::asm_lea(os, indnt, "rdi", dst);

        // try moving qwords
        const size_t qword_count{bytes_count / toc::size_qword};
        const size_t rest_bytes_count{bytes_count -
                                      (qword_count * toc::size_qword)};
        const size_t reps{rest_bytes_count == 0 ? qword_count : bytes_count};
        const char rep_size{rest_bytes_count ? 'b' : 'q'};

        asm_cmd(src_loc_tk, os, indnt, "mov", "rcx", std::format("{}", reps));

        toc::asm_rep_movs(os, indnt, rep_size);

        free_named_register(src_loc_tk, os, indnt, "rcx");
        free_named_register(src_loc_tk, os, indnt, "rdi");
        free_named_register(src_loc_tk, os, indnt, "rsi");
    }

    auto rep_movs(const token& src_loc_tk, std::ostream& os, const size_t indnt,
                  const statement& src, const ident_info& src_info,
                  std::string_view dst, const size_t bytes_count) -> void {

        // ; Copy RCX bytes from RSI to RDI
        // mov rsi, source_addr    ; source pointer
        // mov rdi, dest_addr      ; destination pointer
        // mov rcx, byte_count     ; number of bytes
        // rep movsb               ; repeat: copy byte [RSI++] to [RDI++]

        alloc_named_register_or_throw(src_loc_tk, os, indnt, "rsi");
        alloc_named_register_or_throw(src_loc_tk, os, indnt, "rdi");
        alloc_named_register_or_throw(src_loc_tk, os, indnt, "rcx");

        std::vector<std::string> allocated_registers;
        if (src.is_indexed() or src_info.has_lea()) {
            const std::string& addr{src.compile_lea(src_loc_tk, *this, os,
                                                    indnt, allocated_registers,
                                                    "", src_info.lea_path)};
            toc::asm_lea(os, indnt, "rsi", addr);
        } else {
            toc::asm_lea(os, indnt, "rsi",
                         get_operand_address_str(src_info.operand));
        }
        toc::asm_lea(os, indnt, "rdi", dst);

        // try moving qwords
        const size_t qword_count{bytes_count / toc::size_qword};
        const size_t rest_bytes_count{bytes_count -
                                      (qword_count * toc::size_qword)};
        const size_t reps{rest_bytes_count == 0 ? qword_count : bytes_count};
        const char rep_size{rest_bytes_count ? 'b' : 'q'};

        asm_cmd(src_loc_tk, os, indnt, "mov", "rcx", std::format("{}", reps));

        toc::asm_rep_movs(os, indnt, rep_size);

        for (const std::string& reg :
             allocated_registers | std::views::reverse) {
            free_scratch_register(src_loc_tk, os, indnt, reg);
        }
        free_named_register(src_loc_tk, os, indnt, "rcx");
        free_named_register(src_loc_tk, os, indnt, "rdi");
        free_named_register(src_loc_tk, os, indnt, "rsi");
    }

    [[nodiscard]] auto regex_ws() const -> const std::regex& {
        return regex_ws_;
    }

    [[nodiscard]] auto regex_nams_number_register() const -> const std::regex& {
        return regex_nasm_number_register_;
    }

    [[nodiscard]] auto regex_trim() const -> const std::regex& {
        return regex_trim_;
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

    // todo formalize this
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

    [[nodiscard]] auto
    make_ident_info_or_empty(const token& src_loc,
                             const std::string_view ident) const -> ident_info {

        ident_path id{std::string{ident}};
        // get the root of an identifier: example p.x -> p
        // traverse the frames and resolve the `ident` (which might be an alias)
        // to a variable, field, register or constant
        size_t i{frames_.size()};
        std::vector<std::string> lea_path;
        // ignore the elements after the base:
        //  e.g.: lnks[1].pos.y
        //   ignore pos.y since those cannot have a lea, add empty leas for
        //   those
        const size_t n{id.path().size()};
        for (size_t j{1}; j < n; j++) {
            lea_path.emplace_back("");
        }

        while (i) {
            i--;
            const frame& frm{frames_[i]};
            // does scope contain the variable?
            if (frm.has_var(id.base())) {
                // yes, found
                // lea_path.emplace_back("");
                break;
            }
            if (frm.is_func()) {
                if (not frm.has_alias(id.base())) {
                    lea_path.emplace_back("");
                    break;
                }

                // yes, continue resolving alias until it is a variable,
                // field, register or constant
                const alias_info& alias{frm.get_alias(id.base())};

                lea_path.emplace_back(alias.lea);

                ident_path new_id{std::string{alias.to}};

                // this is an alias of the type:
                //   res -> pt.x becomes pt.x
                //   pt.x -> p becomes p.x
                //   lnk.count -> world.roome.link becomes
                //      world.roome.link.count
                for (const std::string& s : id.path() | std::views::drop(1)) {
                    new_id.append(s);
                }
                id = new_id;
                continue;
            }
        }

        const frame& frm{frames_[i]};

        // is it a variable?
        if (frm.has_var(id.base())) {
            // std::cerr << src_loc.at_line() << ": " << ident << " -> "
            //           << id.str() << "\n";
            const var_info& var{frm.get_var_const_ref(id.base())};
            std::vector<const type*> type_path;
            ident_info ii{var.type_ptr->accessor(src_loc, ident, id.path(), var,
                                                 type_path)};

            ii.elem_path = std::vector(id.path().begin(), id.path().end());
            //? fishy stuff adjusting lea_path size
            // pad the lea path to have the same size as the other vectors
            ii.type_path = type_path;
            for (size_t j{lea_path.size()}; j < id.path().size(); j++) {
                lea_path.emplace_back("");
            }
            while (lea_path.size() != id.path().size()) {
                lea_path.pop_back();
            }
            std::ranges::reverse(lea_path);
            ii.lea_path = lea_path;

            return ii;
        }

        // is it a register?
        if (const size_t reg_size{get_size_from_register_operand(id.str())};
            reg_size != 0) {
            const type& tpe{get_builtin_type_for_size(reg_size)};
            //? unary ops?
            return {
                .id{ident},
                .operand{id.str()},
                .type_ptr = &tpe,
                .elem_path{id.str()},
                .type_path{&tpe},
                .lea_path{""},
                .ident_type = ident_info::ident_type::REGISTER,
            };
        }

        // is it a register reference to memory?
        if (is_operand_indirect_addressing(id.str())) {
            // get the size: e.g. "dword [r15]"
            return {
                .id{ident},
                .operand{id.str()},
                .type_ptr = &get_builtin_type_for_operand(src_loc, id.str()),
                .elem_path{id.str()},
                .type_path{&get_type_default()},
                .lea_path{""},
                .ident_type = ident_info::ident_type::REGISTER,
            };
        }

        // is it a field?
        if (fields_.has(id.base())) {
            const std::string after_dot =
                id.path().size() == 1 ? "" : id.path()[1]; //? bug. not correct
            if (after_dot == "len") {
                return {
                    .id{ident},
                    .operand{std::format("{}.len", id.base())},
                    .type_ptr = &get_type_default(),
                    .elem_path{id.str()},
                    .type_path{&get_type_default()},
                    .lea_path{""},
                    .ident_type = ident_info::ident_type::IMPLIED,
                };
            }
            const field_info& fi{fields_.get_const_ref(id.base())};
            if (fi.is_str) {
                return {
                    .id{ident},
                    .operand{id.base()},
                    .type_ptr = &get_type_default(),
                    .elem_path{id.str()},
                    .type_path{&get_type_default()},
                    .lea_path{""},
                    .ident_type = ident_info::ident_type::FIELD,
                };
            }
            //? assumes qword
            return {
                .id{ident},
                .operand{std::format("qword [{}]", id.base())},
                .type_ptr = &get_type_default(),
                .elem_path{id.str()},
                .type_path{&get_type_default()},
                .lea_path{""},
                .ident_type = ident_info::ident_type::FIELD,
            };
        }

        // is 'id' an integer?
        if (const std::optional<int64_t> value{parse_to_constant(id.str())};
            value) {
            return {
                .id{ident},
                .operand{id.str()},
                .const_value = *value,
                .type_ptr = &get_type_default(),
                .elem_path{id.str()},
                .type_path{&get_type_default()},
                .lea_path{""},
            };
        }

        // is it a boolean constant?
        if (id.base() == "true") {
            return {
                .id{ident},
                .operand{"true"},
                .const_value = 1,
                .type_ptr = &get_type_bool(),
                .elem_path{id.str()},
                .type_path{&get_type_default()},
                .lea_path{""},
            };
        }

        if (id.base() == "false") {
            return {
                .id{ident},
                .operand{"false"},
                .const_value = 0,
                .type_ptr = &get_type_bool(),
                .elem_path{id.str()},
                .type_path{&get_type_default()},
                .lea_path{""},
            };
        }

        // not resolved, return empty info
        return {
            .id{},
            .operand{},
            .type_ptr = &get_type_void(),
            .elem_path{},
            .type_path{},
            .lea_path{},
        };
    }

    // helper: call make_ident_info_or_empty and throw if unresolved
    [[nodiscard]] auto
    make_ident_info_or_throw(const token& src_loc_tk,
                             const std::string_view ident) const -> ident_info {

        const ident_info id_info{make_ident_info_or_empty(src_loc_tk, ident)};
        if (not id_info.operand.empty()) {
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

    static auto get_field_offset_in_type(const token& src_loc_tk,
                                         const type& tp,
                                         const std::string_view field_name)
        -> size_t {

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

    static auto get_operand_address_str(const std::string_view op)
        -> std::string {

        std::optional<std::string> between_brackets{
            get_text_between_brackets(op)};

        if (not between_brackets) {
            return std::string{op};
        }

        return *between_brackets;
    }

    static auto get_size_specifier(const size_t size) -> std::string_view {
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
        for (size_t i{1}; i < indnt; i++) {
            std::print(os, "    ");
        }
    }

    [[nodiscard]] static auto
    is_operand_indirect_addressing(const std::string_view op) -> bool {
        if (auto expr{toc::get_text_between_brackets(op)}; expr) {
            const std::string reg{get_operand_base_register(*expr)};
            if (is_operand_register(reg)) {
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] static auto is_operand_register(const std::string_view op)
        -> bool {
        const size_t size{get_size_from_register_operand(op)};
        return size != 0;
    }

    static auto line_and_col_num_for_char_index(const size_t at_line,
                                                size_t char_index_in_source,
                                                const std::string_view src)
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
    static auto parse_to_constant(const std::string_view str)
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
            if (result.ec == std::errc{}) {
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
            if (result.ec == std::errc{}) {
                return value;
            }
        }

        // try decimal digit
        {
            int64_t value{};
            const std::string_view sv{str};
            // note: using 'std::string_view' for 'clang-tidy' to not trigger
            //       the warning
            //       'cppcoreguidelines-pro-bounds-pointer-arithmetic'

            auto parse_result{
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
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

    [[nodiscard]] static auto get_before_dot(std::string_view str)
        -> std::string_view {

        if (const size_t pos{str.find('.')}; pos != std::string_view::npos) {
            return str.substr(0, pos);
        }

        return str;
    }

    static auto get_operand_base_register(const std::string_view addressing)
        -> std::string_view {

        auto pos{addressing.find_first_of(" +")};
        if (pos == std::string_view::npos) {
            return addressing;
        }

        return addressing.substr(0, pos);
    }

    [[nodiscard]] static auto get_operand_size(const size_t size)
        -> std::string_view {
        switch (size) {
        case size_byte:
            return "byte";
        case size_word:
            return "word";
        case size_dword:
            return "dword";
        case size_qword:
            return "qword";
        default:
            std::unreachable();
        }
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

    static auto get_text_between_brackets(const std::string_view str)
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

    static auto is_memory_operand(const std::string_view operand) -> bool {
        return operand.find_first_of('[') != std::string::npos;
    }

    static auto get_sized_memory_operand(const std::string_view operand,
                                         const size_t new_size) -> std::string {

        const size_t pos = operand.find('[');
        assert(pos != std::string_view::npos);
        return std::format("{} {}", get_size_specifier(new_size),
                           operand.substr(pos));
    }
};
