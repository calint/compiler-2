#pragma once

#include <algorithm>
#include <cstdint>
#include <unordered_set>

#include "compiler_exception.hpp"
#include "lut.hpp"
#include "statement.hpp"
#include "type.hpp"

class stmt_def_func;
class stmt_def_field;
class stmt_def_type;

struct func_return_info final {
    const token type_tk;  // type token
    const token ident_tk; // identifier token
    const type& type_ref;
};

struct var_info final {
    std::string name;
    const type& type_ref;
    token declared_at; // token for position in sources
    int stack_idx{};   // location relative to rsp
    bool initiated{};  // true if variable has been initiated
};

class frame final {
    // optional name
    std::string name_;

    // a unique path of source locations of the inlined call
    std::string call_path_;

    // number of bytes used on the stack by this frame
    size_t allocated_stack_{};

    // variables declared in this frame
    lut<var_info> vars_;

    // aliases that refers to previous frame(s) alias or variable
    lut<std::string> aliases_;

    // the label to jump to when exiting an function
    std::string func_ret_label_;

    // info about the function returns
    const std::vector<func_return_info>& func_rets_;

  public:
    enum class frame_type : uint8_t { FUNC, BLOCK, LOOP };

  private:
    frame_type type_{frame_type::FUNC}; // frame type

  public:
    frame(std::string name, const frame_type frm_type,
          const std::vector<func_return_info>& func_rets = {},
          std::string call_path = "", std::string func_ret_label = "") noexcept
        : name_{std::move(name)}, call_path_{std::move(call_path)},
          func_ret_label_{std::move(func_ret_label)}, func_rets_{func_rets},
          type_{frm_type} {}

    auto add_var(token declared_at, const std::string& name, const type& tpe,
                 const int stack_idx, const bool initiated) -> void {
        if (stack_idx < 0) {
            // variable, increase allocated stack size
            allocated_stack_ += tpe.size();
        }
        vars_.put(name, {.name = name,
                         .type_ref = tpe,
                         .declared_at = std::move(declared_at),
                         .stack_idx = stack_idx,
                         .initiated = initiated});
    }

    auto allocated_stack_size() const -> size_t { return allocated_stack_; }

    auto has_var(const std::string& name) const -> bool {
        return vars_.has(name);
    }

    auto get_var_ref(const std::string& name) -> var_info& {
        return vars_.get_ref(name);
    }

    auto get_var_const_ref(const std::string& name) const -> const var_info& {
        return vars_.get_const_ref(name);
    }

    auto add_alias(const std::string& ident, const std::string& outside_ident)
        -> void {
        aliases_.put(ident, outside_ident);
    }

    auto is_func() const -> bool { return type_ == frame_type::FUNC; }

    auto is_block() const -> bool { return type_ == frame_type::BLOCK; }

    auto is_loop() const -> bool { return type_ == frame_type::LOOP; }

    auto is_name(const std::string& name) const -> bool {
        return name_ == name;
    }

    auto has_alias(const std::string& name) const -> bool {
        return aliases_.has(name);
    }

    auto get_alias(const std::string& name) const -> const std::string& {
        return aliases_.get_const_ref(name);
    }

    auto name() const -> const std::string& { return name_; }

    auto func_ret_label() const -> const std::string& {
        return func_ret_label_;
    }

    auto call_path() const -> const std::string& { return call_path_; }

    auto get_func_returns_infos() const
        -> const std::vector<func_return_info>& {
        return func_rets_;
    }
};

struct field_info final {
    const stmt_def_field* def{};
    const token declared_at; // token for position in sources
    const bool is_str{};
};

struct func_info final {
    const stmt_def_func* def{};
    const token declared_at; // token for position in sources
    const type& type_ref;
};

struct type_info final {
    const stmt_def_type& def;
    const token declared_at;
    const type& type_ref;
};

struct ident_resolved final {
    enum class ident_type : uint8_t { CONST, VAR, REGISTER, FIELD, IMPLIED };

    const std::string id;
    const std::string id_nasm;
    const int64_t const_value{};
    const type& type_ref;
    const ident_type ident_type{ident_type::CONST};

    [[nodiscard]] auto is_const() const -> bool {
        return ident_type == ident_type::CONST;
    }
    [[nodiscard]] auto is_var() const -> bool {
        return ident_type == ident_type::VAR;
    }
    [[nodiscard]] auto is_register() const -> bool {
        return ident_type == ident_type::REGISTER;
    }
    [[nodiscard]] auto is_field() const -> bool {
        return ident_type == ident_type::FIELD;
    }
    [[nodiscard]] auto is_implied() const -> bool {
        return ident_type == ident_type::IMPLIED;
    }
};

class identifier final {
    std::string id_;
    std::vector<std::string> path_;

  public:
    explicit identifier(std::string id) : id_{std::move(id)} {
        size_t start{};
        size_t end{};
        while ((end = id_.find('.', start)) != std::string::npos) {
            path_.emplace_back(id_.substr(start, end - start));
            start = end + 1;
        }
        path_.emplace_back(id_.substr(start));
    }

    ~identifier() = default;

    identifier() = default;
    identifier(identifier&&) = default;
    identifier(const identifier&) = default;
    auto operator=(const identifier&) -> identifier& = default;
    auto operator=(identifier&&) -> identifier& = default;

    [[nodiscard]] auto id() const -> const std::string& { return id_; }

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
    std::vector<std::string> named_registers_{"rax", "rbx", "rcx", "rdx",
                                              "rsi", "rdi", "rbp"};
    size_t named_registers_initial_size_{named_registers_.size()};
    std::vector<std::string> scratch_registers_{"r8",  "r9",  "r10", "r11",
                                                "r12", "r13", "r14", "r15"};
    size_t scratch_registers_initial_size_{scratch_registers_.size()};
    std::vector<std::string> allocated_registers_;
    std::vector<std::string> allocated_registers_src_locs_; // source locations
    std::unordered_set<std::string> initiated_registers_;
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

    auto source() const -> const std::string& { return source_; }

    auto add_field(const token& src_loc, const std::string& name,
                   const stmt_def_field* fld, const bool is_str_field) -> void {

        if (fields_.has(name)) {
            throw compiler_exception(
                src_loc, "field '" + name + "' already defined at " +
                             source_location_hr(
                                 fields_.get_const_ref(name).declared_at));
        }
        fields_.put(
            name, {.def = fld, .declared_at = src_loc, .is_str = is_str_field});
    }

    auto add_func(const token& src_loc, const std::string& name,
                  const type& return_type, const stmt_def_func* func) -> void {

        if (funcs_.has(name)) {
            const func_info& fn{funcs_.get_const_ref(name)};
            const std::string loc{source_location_hr(fn.declared_at)};
            throw compiler_exception(
                src_loc, "function '" + name + "' already defined at " + loc);
        }
        funcs_.put(
            name,
            {.def = func, .declared_at = src_loc, .type_ref = return_type});
    }

    auto get_func_or_throw(const token& src_loc, const std::string& name) const
        -> const stmt_def_func& {

        if (not funcs_.has(name)) {
            throw compiler_exception(src_loc,
                                     "function '" + name + "' not found");
        }

        return *funcs_.get_const_ref(name).def;
    }

    auto is_func_builtin(const token& src_loc, const std::string& name) const
        -> bool {

        if (not funcs_.has(name)) {
            throw compiler_exception(src_loc,
                                     "function '" + name + "' not found");
        }

        return funcs_.get_const_ref(name).def == nullptr;
    }

    auto get_func_return_type_or_throw(const token& src_loc,
                                       const std::string& name) const
        -> const type& {

        if (not funcs_.has(name)) {
            throw compiler_exception(src_loc,
                                     "function '" + name + "' not found");
        }

        return funcs_.get_const_ref(name).type_ref;
    }

    auto add_type(const token& src_loc, const type& tp) -> void {
        if (types_.has(tp.name())) {
            //? todo. specify where the type has been defined
            throw compiler_exception(src_loc, "type '" + tp.name() +
                                                  "' already defined");
        }

        types_.put(tp.name(), tp);
    }

    auto get_type_or_throw(const token& src_loc, const std::string& name) const
        -> const type& {

        if (not types_.has(name)) {
            throw compiler_exception(src_loc, "type '" + name + "' not found");
        }

        return types_.get_const_ref(name);
    }

    auto source_location_for_use_in_label(const token& src_loc) const
        -> std::string {

        const auto [line, col]{line_and_col_num_for_char_index(
            src_loc.start_index(), source_.c_str())};

        return std::to_string(line) + "_" + std::to_string(col);
    }

    // human readable source location
    auto source_location_hr(const token& src_loc) -> std::string {

        const auto [line, col]{line_and_col_num_for_char_index(
            src_loc.start_index(), source_.c_str())};

        return std::to_string(line) + ":" + std::to_string(col);
    }

    static auto line_and_col_num_for_char_index(const size_t char_index,
                                                const char* src)
        -> std::pair<size_t, size_t> {

        size_t ix{};
        size_t lix{};
        size_t line_num{1};
        while (true) {
            if (ix == char_index) {
                break;
            }
            if (*src++ == '\n') {
                line_num++;
                ix++;
                lix = ix;
                continue;
            }
            ix++;
        }
        return {line_num, ix - lix + 1};
    }

    auto finish(std::ostream& os) -> void {
        os << "\n; max scratch registers in use: " << usage_max_scratch_regs_
           << '\n';
        os << ";            max frames in use: " << usage_max_frame_count_
           << '\n';
        os << ";               max stack size: " << usage_max_stack_size_
           << " B\n";
        assert(all_registers_.size() == 16);
        assert(allocated_registers_.empty());
        assert(allocated_registers_src_locs_.empty());
        assert(frames_.empty());
        //		assert(initiated_registers_.empty());
        initiated_registers_.clear();
        assert(named_registers_.size() == named_registers_initial_size_);
        assert(scratch_registers_.size() == scratch_registers_initial_size_);
        usage_max_frame_count_ = 0;
        usage_max_scratch_regs_ = 0;
    }

    auto resolve_identifier(const statement& st,
                            const bool must_be_initiated) const
        -> ident_resolved {

        const ident_resolved& id_resolved{resolve_identifier_or_empty(
            st.tok(), st.identifier(), must_be_initiated)};

        if (not id_resolved.id_nasm.empty()) {
            return id_resolved;
        }

        throw compiler_exception(st.tok(), "cannot resolve identifier '" +
                                               st.identifier() + "'");
    }

    auto resolve_identifier(const token& src_loc, const std::string& name,
                            const bool must_be_initiated) const
        -> ident_resolved {

        const ident_resolved& name_resolved{
            resolve_identifier_or_empty(src_loc, name, must_be_initiated)};

        if (not name_resolved.id_nasm.empty()) {
            return name_resolved;
        }

        throw compiler_exception(src_loc,
                                 "cannot resolve identifier '" + name + "'");
    }

    auto add_alias(const std::string& name,
                   const std::string& name_in_parent_frame) -> void {

        frames_.back().add_alias(name, name_in_parent_frame);
    }

    auto enter_func(const std::string& name,
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

    auto exit_func(const std::string& name) -> void {
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

    auto add_var(const token& src_loc, std::ostream& os, size_t indnt,
                 const std::string& name, const type& var_type,
                 const bool initiated) -> void {

        // check if variable is already declared in this scope
        if (frames_.back().has_var(name)) {
            const var_info& var{frames_.back().get_var_const_ref(name)};
            throw compiler_exception(
                src_loc, "variable '" + name + "' already declared at " +
                             source_location_hr(var.declared_at));
        }
        // check if variable shadows previously declared variable
        const auto [id, frm]{get_id_and_frame_for_identifier(name)};
        if (frm.has_var(id)) {
            const var_info& var{frm.get_var_const_ref(id)};
            throw compiler_exception(src_loc,
                                     "variable '" + name +
                                         "' shadows variable declared at " +
                                         source_location_hr(var.declared_at));
        }

        const int stack_idx{
            static_cast<int>(get_current_stack_size() + var_type.size())};
        frames_.back().add_var(src_loc, name, var_type, -stack_idx, initiated);

        const size_t total_stack_size{get_total_stack_size()};
        usage_max_stack_size_ =
            std::max(total_stack_size, usage_max_stack_size_);

        // comment the resolved name
        const ident_resolved& name_resolved{
            resolve_identifier(src_loc, name, false)};
        indent(os, indnt, true);
        os << name << ": " << name_resolved.id_nasm << '\n';
    }

    auto alloc_scratch_register(const token& src_loc, std::ostream& os,
                                const size_t indnt) -> const std::string& {

        if (scratch_registers_.empty()) {
            throw compiler_exception(src_loc,
                                     "out of scratch registers. try to reduce "
                                     "expression complexity");
        }

        const std::string& reg{scratch_registers_.back()};
        scratch_registers_.pop_back();

        indent(os, indnt, true);
        os << "alloc " << reg << '\n';

        const size_t n{scratch_registers_initial_size_ -
                       scratch_registers_.size()};
        usage_max_scratch_regs_ = std::max(n, usage_max_scratch_regs_);

        allocated_registers_.emplace_back(reg);
        allocated_registers_src_locs_.emplace_back(source_location_hr(src_loc));
        return reg;
    }

    auto alloc_named_register_or_throw(const statement& st, std::ostream& os,
                                       const size_t indnt,
                                       const std::string& reg) -> void {
        indent(os, indnt, true);
        os << "alloc " << reg << '\n';
        auto reg_iter{std::ranges::find(named_registers_, reg)};
        if (reg_iter == named_registers_.end()) {
            std::string loc;
            const size_t n{allocated_registers_.size()};
            for (size_t i{}; i < n; i++) {
                if (allocated_registers_.at(i) == reg) {
                    loc = allocated_registers_src_locs_.at(i);
                    break;
                }
            }
            throw compiler_exception(
                st.tok(), "cannot allocate register '" + reg +
                              "' because it was allocated at " + loc);
        }
        named_registers_.erase(reg_iter);
        allocated_registers_.emplace_back(reg);
        allocated_registers_src_locs_.emplace_back(
            source_location_hr(st.tok()));
    }

    auto alloc_named_register(const token& src_loc, std::ostream& os,
                              const size_t indnt, const std::string& reg)
        -> bool {

        indent(os, indnt, true);
        os << "alloc " << reg;
        auto reg_iter{std::ranges::find(named_registers_, reg)};
        if (reg_iter == named_registers_.end()) {
            os << ": false\n";
            return false;
        }
        named_registers_.erase(reg_iter);
        allocated_registers_.emplace_back(reg);
        allocated_registers_src_locs_.emplace_back(source_location_hr(src_loc));
        os << '\n';
        return true;
    }

    auto free_named_register(std::ostream& os, const size_t indnt,
                             const std::string& reg) -> void {
        indent(os, indnt, true);
        os << "free " << reg << '\n';
        assert(allocated_registers_.back() == reg);
        allocated_registers_.pop_back();
        allocated_registers_src_locs_.pop_back();
        named_registers_.emplace_back(reg);
        initiated_registers_.erase(reg);
    }

    auto free_scratch_register(std::ostream& os, const size_t indnt,
                               const std::string& reg) -> void {
        indent(os, indnt, true);
        os << "free " << reg << '\n';
        assert(allocated_registers_.back() == reg);
        allocated_registers_.pop_back();
        allocated_registers_src_locs_.pop_back();
        scratch_registers_.emplace_back(reg);
        initiated_registers_.erase(reg);
    }

    auto get_loop_label_or_throw(const token& src_loc) const
        -> const std::string& {

        size_t i{frames_.size()};
        while (i--) {
            if (frames_.at(i).is_loop()) {
                return frames_.at(i).name();
            }
            if (frames_.at(i).is_func()) {
                throw compiler_exception(src_loc, "not in a loop");
            }
        }
        throw compiler_exception(src_loc, "unexpected frames");
    }

    auto get_call_path(const token& src_loc) const -> const std::string& {

        size_t i{frames_.size()};
        while (i--) {
            if (frames_.at(i).is_func()) {
                return frames_.at(i).call_path();
            }
        }
        throw compiler_exception(src_loc, "not in a function");
    }

    auto get_func_return_label_or_throw(const token& src_loc) const
        -> const std::string& {

        size_t i{frames_.size()};
        while (i--) {
            if (frames_.at(i).is_func()) {
                return frames_.at(i).func_ret_label();
            }
        }
        throw compiler_exception(src_loc, "not in a function");
    }

    auto get_func_returns(const token& src_loc) const
        -> const std::vector<func_return_info>& {

        size_t i{frames_.size()};
        while (i--) {
            if (frames_.at(i).is_func()) {
                return frames_.at(i).get_func_returns_infos();
            }
        }
        throw compiler_exception(src_loc, "not in a function");
    }

    auto comment_source(const statement& st, std::ostream& os,
                        const size_t indnt) const -> void {

        const auto [line, col]{line_and_col_num_for_char_index(
            st.tok().start_index(), source_.c_str())};

        indent(os, indnt, true);
        os << "[" << line << ":" << col << "] ";

        std::stringstream ss;
        st.source_to(ss);
        const std::string s{ss.str()};
        std::string res{std::regex_replace(s, regex_ws, " ")};
        if (!res.empty() && res.back() == ' ') {
            res.pop_back();
        }
        os << res << "\n";
    }

    auto comment_source(std::ostream& os, const std::string& dst,
                        const std::string& op, const statement& st) const
        -> void {

        const auto [line, col]{line_and_col_num_for_char_index(
            st.tok().start_index(), source_.c_str())};

        os << "[" << line << ":" << col << "]";

        std::stringstream ss;
        ss << " " << dst << " " << op << " ";
        st.source_to(ss);
        const std::string& s{ss.str()};
        const std::string& res{regex_replace(s, std::regex(R"(\s+)"), " ")};
        os << res << '\n';
    }

    auto comment_token(std::ostream& os, const token& tk) const -> void {
        const auto [line, col]{
            line_and_col_num_for_char_index(tk.start_index(), source_.c_str())};

        os << "[" << line << ":" << col << "]";
        os << " " << tk.name() << '\n';
    }

    auto is_identifier_register(const std::string& id) const -> bool {
        return std::ranges::find(all_registers_, id) != all_registers_.end();
    }

    auto asm_cmd(const token& src_loc, std::ostream& os, const size_t indnt,
                 const std::string& op, const std::string& dst_nasm,
                 const std::string& src_nasm) -> void {
        if (op == "mov") {
            if (dst_nasm == src_nasm) {
                return;
            }

            if (is_identifier_register(dst_nasm)) {
                // for optimization of push/pop when calling a function
                initiated_registers_.insert(dst_nasm);
            }
        }

        const size_t dst_size{get_size_from_operand(src_loc, dst_nasm)};
        const size_t src_size{get_size_from_operand(src_loc, src_nasm)};

        if (dst_size == src_size) {
            if (not(is_operand_memory(dst_nasm) and
                    is_operand_memory(src_nasm))) {
                // both operands are not memory references
                indent(os, indnt);
                os << op << " " << dst_nasm << ", " << src_nasm << '\n';
                return;
            }
            // both operands are memory references
            //   use scratch register for transfer
            const std::string& reg{alloc_scratch_register(src_loc, os, indnt)};
            const std::string& reg_sized{
                get_register_operand_for_size(src_loc, reg, src_size)};
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
            //   use scratch register for transfer
            const std::string& reg{alloc_scratch_register(src_loc, os, indnt)};
            const std::string& reg_sized{
                get_register_operand_for_size(src_loc, reg, dst_size)};
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
                get_register_operand_for_size(src_loc, src_nasm, dst_size)};
            indent(os, indnt);
            os << op << " " << dst_nasm << ", " << reg_sized << '\n';
            return;
        }

        if (is_operand_memory(src_nasm)) {
            //? todo. this displays nasm identifiers but should be human
            // readable
            // identifiers
            throw compiler_exception(
                src_loc, "cannot move '" + src_nasm + "' to '" + dst_nasm +
                             "' because it would be truncated");
        }

        // constant
        indent(os, indnt);
        os << op << " " << dst_nasm << ", " << src_nasm << '\n';
    }

    auto set_var_is_initiated(const std::string& name) -> void {
        const auto [id, frm]{get_id_and_frame_for_identifier(name)};
        // FIX
        const identifier found_ident{id};
        // is 'id_nasm' a variable?
        if (frm.has_var(found_ident.id_base())) {
            frm.get_var_ref(id).initiated = true;
            return;
        }

        if (fields_.has(id)) {
            return; // a field, it is initiated
        }

        if (is_identifier_register(id)) {
            initiated_registers_.insert(id);
            return;
        }

        throw panic_exception("unexpected code path " + std::string{__FILE__} +
                              ":" + std::to_string(__LINE__));
    }

    auto is_var_initiated(const std::string& name) -> bool {
        const identifier ident{name};
        const auto [id, frm]{get_id_and_frame_for_identifier(ident.id_base())};
        // is 'id_nasm' a variable?
        if (frm.has_var(id)) {
            return frm.get_var_const_ref(id).initiated;
        }

        if (fields_.has(id)) {
            return true; // a field, it is initiated
        }

        if (is_identifier_register(id)) {
            auto it = std::ranges::find(initiated_registers_, id);
            return it != initiated_registers_.end();
        }

        throw panic_exception("unexpected code path " + std::string{__FILE__} +
                              ":" + std::to_string(__LINE__));
    }

    auto set_type_void(const type& tp) -> void { type_void_ = &tp; }

    auto get_type_void() const -> const type& { return *type_void_; }

    auto set_type_default(const type& tp) -> void { type_default_ = &tp; }

    auto get_type_default() const -> const type& { return *type_default_; }

    auto set_type_bool(const type& tp) -> void { type_bool_ = &tp; }

    auto get_type_bool() const -> const type& { return *type_bool_; }

    static auto is_operand_memory(const std::string& operand) -> bool {
        return operand.find_first_of('[') != std::string::npos;
    }

    auto get_size_from_operand(const token& src_loc,
                               const std::string& operand) const -> size_t {
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
            return get_size_from_operand_register(src_loc, operand);
        }

        // constant
        //? calculate the smallest size of the constant?
        return get_type_default().size();
    }

    // -------------------------------------------------------------------------
    // statics
    // -------------------------------------------------------------------------

    static auto get_size_from_operand_register(const token& src_loc,
                                               const std::string& operand)
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

        throw compiler_exception(src_loc, "unknown register '" + operand + "'");
    }

    static auto get_register_operand_for_size(const token& src_loc,
                                              const std::string& operand,
                                              const size_t size)
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
                throw compiler_exception(
                    src_loc, "illegal size " + std::to_string(size) +
                                 " for register operand '" + operand + "'");
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
                throw compiler_exception(
                    src_loc, "illegal size " + std::to_string(size) +
                                 " for register '" + operand + "'");
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
                throw compiler_exception(
                    src_loc, "illegal size " + std::to_string(size) +
                                 " for register '" + operand + "'");
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
                throw compiler_exception(
                    src_loc, "illegal size " + std::to_string(size) +
                                 " for register '" + operand + "'");
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
                throw compiler_exception(
                    src_loc, "illegal size " + std::to_string(size) +
                                 " for register '" + operand + "'");
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
                throw compiler_exception(
                    src_loc, "illegal size " + std::to_string(size) +
                                 " for register '" + operand + "'");
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
                throw compiler_exception(
                    src_loc, "illegal size " + std::to_string(size) +
                                 " for register '" + operand + "'");
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
                throw compiler_exception(
                    src_loc, "illegal size " + std::to_string(size) +
                                 " for register '" + operand + "'");
            }
        }
        const std::regex rx{R"(r(\d+))"};
        std::smatch match;
        if (!regex_search(operand, match, rx)) {
            throw compiler_exception(src_loc, "unknown register " + operand);
        }
        const std::string rnbr{match[1]};
        switch (size) {
        case 8:
            return "r" + rnbr;
        case 4:
            return "r" + rnbr + "d";
        case 2:
            return "r" + rnbr + "w";
        case 1:
            return "r" + rnbr + "b";
        default:
            throw compiler_exception(src_loc,
                                     "illegal size " + std::to_string(size) +
                                         " for register '" + operand + "'");
        }
    }

    static auto asm_push([[maybe_unused]] const token& src_loc,
                         std::ostream& os, const size_t indnt,
                         const std::string& operand) -> void {
        indent(os, indnt);
        os << "push " << operand << '\n';
    }

    static auto asm_pop([[maybe_unused]] const token& src_loc, std::ostream& os,
                        const size_t indnt, const std::string& operand)
        -> void {
        indent(os, indnt);
        os << "pop " << operand << '\n';
    }

    static auto asm_ret([[maybe_unused]] const token& src_loc, std::ostream& os,
                        const size_t indnt) -> void {
        indent(os, indnt);
        os << "ret\n";
    }

    static auto asm_jmp([[maybe_unused]] const token& src_loc, std::ostream& os,
                        const size_t indnt, const std::string& label) -> void {
        indent(os, indnt);
        os << "jmp " << label << '\n';
    }

    static auto asm_label([[maybe_unused]] const token& src_loc,
                          std::ostream& os, const size_t indnt,
                          const std::string& label) -> void {
        indent(os, indnt);
        os << label << ":\n";
    }

    static auto asm_call([[maybe_unused]] const token& src_loc,
                         std::ostream& os, const size_t indnt,
                         const std::string& label) -> void {
        indent(os, indnt);
        os << "call " << label << '\n';
    }

    static auto asm_neg([[maybe_unused]] const token& src_loc, std::ostream& os,
                        const size_t indnt, const std::string& dst_resolved)
        -> void {
        indent(os, indnt);
        os << "neg " << dst_resolved << '\n';
    }

    static auto asm_not([[maybe_unused]] const token& src_loc, std::ostream& os,
                        const size_t indnt, const std::string& dst_resolved)
        -> void {
        indent(os, indnt);
        os << "not " << dst_resolved << '\n';
    }

    static auto indent(std::ostream& os, const size_t indnt,
                       const bool comment = false) -> void {
        if (indnt == 0) {
            if (comment) {
                os << ";";
            }
            return;
        }
        os << (comment ? ";" : " ");
        for (size_t i{}; i < indnt; i++) {
            os << "  ";
        }
    }

  private:
    auto get_id_and_frame_for_identifier(const std::string& ident)
        -> std::pair<std::string, frame&> {

        identifier bid{ident};
        std::string id{bid.id_base()};
        // traverse the frames and resolve the id_nasm (which might be an alias)
        // to a variable, field, register or constant
        size_t i{frames_.size()};
        while (i) {
            i--;
            // is the frame a function?
            if (frames_.at(i).is_func()) {
                // is it an alias defined by an argument in the function?
                if (not frames_.at(i).has_alias(id)) {
                    break;
                }
                // yes, continue resolving alias until it is
                // a variable, field, register or constant
                id = frames_.at(i).get_alias(id);
                bid = identifier{id};
                id = bid.id_base();
                continue;
            }
            // does scope contain the variable
            if (frames_.at(i).has_var(id)) {
                break;
            }
        }
        return {std::move(id), frames_.at(i)};
    }

    auto is_register_initiated(const std::string& reg) const -> bool {
        return initiated_registers_.contains(reg);
    }

    auto get_current_stack_size() const -> size_t {
        assert(!frames_.empty());
        size_t nbytes{};
        size_t i{frames_.size()};
        while (i--) {
            const frame& frm{frames_.at(i)};
            nbytes += frm.allocated_stack_size();
            if (frm.is_func()) {
                return nbytes;
            }
        }
        // top frame, 'main'
        return nbytes;
    }

    auto get_total_stack_size() const -> size_t {
        assert(!frames_.empty());
        size_t nbytes{};
        size_t i{frames_.size()};
        while (i--) {
            const frame& frm{frames_.at(i)};
            nbytes += frm.allocated_stack_size();
        }
        return nbytes;
    }

    auto resolve_identifier_or_empty(const token& src_loc,
                                     const std::string& ident,
                                     const bool must_be_initiated) const
        -> ident_resolved {

        identifier id{ident};
        // get the root of an identifier: example p.x -> p
        std::string base_id{id.id_base()};
        // traverse the frames and resolve the id_nasm (which might be an
        // alias) to a variable, field, register or constant
        size_t i{frames_.size()};
        while (i) {
            i--;
            // is the frame a function?
            if (frames_.at(i).is_func()) {
                // is it an alias defined by an argument in the function?
                if (not frames_.at(i).has_alias(base_id)) {
                    // no, it is not
                    break;
                }
                // yes, continue resolving alias until it is a variable, field,
                // register or constant
                const identifier new_id{frames_.at(i).get_alias(base_id)};
                base_id = new_id.id_base();
                if (id.path().size() == 1) {
                    // this is an alias of type: res -> p.x
                    id = new_id;
                } else {
                    // this is an alias of type: pt.x -> p.x
                    id.set_base(base_id);
                }
                continue;
            }
            // does scope contain the variable
            if (frames_.at(i).has_var(base_id)) {
                break;
            }
        }

        // is 'id' a variable?
        if (frames_.at(i).has_var(base_id)) {
            const var_info& var{frames_.at(i).get_var_const_ref(base_id)};
            if (must_be_initiated and not var.initiated) {
                throw compiler_exception(src_loc, "variable '" + var.name +
                                                      "' is not initiated");
            }
            auto [tp, acc]{
                var.type_ref.accessor(src_loc, id.path(), var.stack_idx)};
            return {.id = ident,
                    .id_nasm = acc,
                    .const_value = 0,
                    .type_ref = tp,
                    .ident_type = ident_resolved::ident_type::VAR};
        }

        // is 'id' a register?
        if (is_identifier_register(base_id)) {
            if (must_be_initiated and not is_register_initiated(base_id)) {
                throw compiler_exception(src_loc, "register '" + base_id +
                                                      "' is not initiated");
            }

            //? unary ops?
            return {.id = ident,
                    .id_nasm = base_id,
                    .const_value = 0,
                    .type_ref = get_type_default(),
                    .ident_type = ident_resolved::ident_type::REGISTER};
        }

        // is 'id' a field?
        if (fields_.has(base_id)) {
            const std::string& after_dot =
                id.path().size() < 2 ? ""
                                     : id.path().at(1); //? bug. not correct
            if (after_dot == "len") {
                return {.id = ident,
                        .id_nasm = base_id + "." + after_dot,
                        .const_value = 0,
                        .type_ref = get_type_default(),
                        .ident_type = ident_resolved::ident_type::IMPLIED};
            }
            const field_info& fi{fields_.get_const_ref(base_id)};
            if (fi.is_str) {
                return {.id = ident,
                        .id_nasm = base_id,
                        .const_value = 0,
                        .type_ref = get_type_default(),
                        .ident_type = ident_resolved::ident_type::FIELD};
            }
            //? assumes qword
            return {.id = ident,
                    .id_nasm = "qword[" + base_id + "]",
                    .const_value = 0,
                    .type_ref = get_type_default(),
                    .ident_type = ident_resolved::ident_type::FIELD};
        }

        // is 'id' a constant?
        char* ep{};
        const int64_t const_value{strtol(base_id.c_str(), &ep, 10)};
        if (!*ep) {
            return {.id = ident,
                    .id_nasm = base_id,
                    .const_value = const_value,
                    .type_ref = get_type_default(),
                    .ident_type = ident_resolved::ident_type::CONST};
        }

        if (base_id.starts_with("0x")) { // hex
            const int64_t value{strtol(base_id.c_str() + 2, &ep, 16)};
            if (!*ep) {
                return {.id = ident,
                        .id_nasm = base_id,
                        .const_value = value,
                        .type_ref = get_type_default(),
                        .ident_type = ident_resolved::ident_type::CONST};
            }
        }

        if (base_id.starts_with("0b")) { // binary
            const int64_t value{strtol(base_id.c_str() + 2, &ep, 2)};
            if (!*ep) {
                return {.id = ident,
                        .id_nasm = base_id,
                        .const_value = value,
                        .type_ref = get_type_default(),
                        .ident_type = ident_resolved::ident_type::CONST};
            }
        }

        // is 'id' a boolean constant?
        if (base_id == "true") {
            return {.id = ident,
                    .id_nasm = base_id,
                    .const_value = 1,
                    .type_ref = get_type_bool(),
                    .ident_type = ident_resolved::ident_type::CONST};
        }

        if (base_id == "false") {
            return {.id = ident,
                    .id_nasm = base_id,
                    .const_value = 0,
                    .type_ref = get_type_bool(),
                    .ident_type = ident_resolved::ident_type::CONST};
        }

        // not resolved, return empty answer
        return {.id = "",
                .id_nasm = "",
                .const_value = 0,
                .type_ref = get_type_void(),
                .ident_type = ident_resolved::ident_type::CONST};
    }

    auto refresh_usage() -> void {
        usage_max_frame_count_ =
            std::max(frames_.size(), usage_max_frame_count_);
    }
};
