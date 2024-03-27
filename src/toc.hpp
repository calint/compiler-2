#pragma once
#include "compiler_exception.hpp"
#include "lut.hpp"
#include "type.hpp"

class stmt_def_func;
class stmt_def_field;
class stmt_def_type;

struct func_return_info final {
  const token type_tk{};  // type token
  const token ident_tk{}; // identifier token
  const type &type_ref;
};

struct var_info final {
  string name{};
  const type &type_ref;
  token declared_at{}; // token for position in sources
  int stack_idx{};     // location relative to rbp
  bool initiated{};    // true if variable has been initiated
};

class frame final {
  // optional name
  string name_{};

  // a unique path of source locations of the inlined call
  string call_path_{};

  // number of bytes used on the stack by this frame
  size_t allocated_stack_{};

  // variables declared in this frame
  lut<var_info> vars_{};

  // aliases that refers to previous frame(s) alias or variable
  lut<string> aliases_{};

  // the label to jump to when exiting an inlined function
  string func_ret_label_{};

  // info about the function returns
  const vector<func_return_info> &func_rets_{};

  // true if function is inlined
  bool func_is_inline_{};

public:
  enum class frame_type { FUNC, BLOCK, LOOP };

private:
  frame_type type_{frame_type::FUNC}; // frame type

public:
  inline frame(string name, const frame_type frm_type,
               const vector<func_return_info> &func_rets = {},
               const bool func_is_inline = false, string call_path = "",
               string func_ret_label = "") noexcept
      : name_{move(name)}, call_path_{move(call_path)},
        func_ret_label_{move(func_ret_label)}, func_rets_{func_rets},
        func_is_inline_{func_is_inline}, type_{frm_type} {}

  inline void add_var(token declared_at, const string &name, const type &tpe,
                      const int stack_idx, const bool initiated) {
    if (stack_idx < 0) {
      // variable, increase allocated stack size
      allocated_stack_ += tpe.size();
    }
    vars_.put(name, {name, tpe, move(declared_at), stack_idx, initiated});
  }

  inline auto allocated_stack_size() const -> size_t {
    return allocated_stack_;
  }

  inline auto has_var(const string &name) const -> bool {
    return vars_.has(name);
  }

  inline auto get_var_ref(const string &name) -> var_info & {
    return vars_.get_ref(name);
  }

  inline auto get_var_const_ref(const string &name) const -> const var_info & {
    return vars_.get_const_ref(name);
  }

  inline void add_alias(const string &ident, const string &outside_ident) {
    aliases_.put(ident, outside_ident);
  }

  inline auto is_func() const -> bool { return type_ == frame_type::FUNC; }

  inline auto is_block() const -> bool { return type_ == frame_type::BLOCK; }

  inline auto is_loop() const -> bool { return type_ == frame_type::LOOP; }

  inline auto is_name(const string &name) const -> bool {
    return name_ == name;
  }

  inline auto has_alias(const string &name) const -> bool {
    return aliases_.has(name);
  }

  inline auto get_alias(const string &name) const -> string {
    return aliases_.get(name);
  }

  inline auto name() const -> const string & { return name_; }

  inline auto func_ret_label() const -> const string & {
    return func_ret_label_;
  }

  inline auto inline_call_path() const -> const string & { return call_path_; }

  inline auto func_is_inline() const -> bool { return func_is_inline_; }

  inline auto get_func_returns_infos() const
      -> const vector<func_return_info> & {
    return func_rets_;
  }
};

struct field_info final {
  const stmt_def_field *def{};
  const token declared_at{}; // token for position in sources
  const bool is_str{};
};

struct func_info final {
  const stmt_def_func *def{};
  const token declared_at{}; // token for position in sources
  const type &type_ref;
};

struct call_info final {
  const size_t nregs_pushed{};
  const size_t alloc_reg_idx{};
  const size_t nbytes_of_vars{};
};

struct type_info final {
  const stmt_def_type &def;
  const token declared_at{};
  const type &type_ref;
};

struct ident_resolved final {
  enum class ident_type { CONST, VAR, REGISTER, FIELD, IMPLIED };

  const string id{};
  const string id_nasm{};
  const long const_value{};
  const type &type_ref;
  const ident_type ident_type{ident_type::CONST};

  [[nodiscard]] inline auto is_const() const -> bool {
    return ident_type == ident_type::CONST;
  }
  [[nodiscard]] inline auto is_var() const -> bool {
    return ident_type == ident_type::VAR;
  }
  [[nodiscard]] inline auto is_register() const -> bool {
    return ident_type == ident_type::REGISTER;
  }
  [[nodiscard]] inline auto is_field() const -> bool {
    return ident_type == ident_type::FIELD;
  }
  [[nodiscard]] inline auto is_implied() const -> bool {
    return ident_type == ident_type::IMPLIED;
  }
};

class identifier final {
  const string id_{};
  vector<string> path_{};

public:
  inline explicit identifier(string id) : id_{move(id)} {
    size_t start{0};
    size_t end{0};
    while ((end = id_.find('.', start)) != string::npos) {
      path_.emplace_back(id_.substr(start, end - start));
      start = end + 1;
    }
    path_.emplace_back(id_.substr(start));
  }

  [[nodiscard]] inline auto id() const -> const string & { return id_; }

  [[nodiscard]] inline auto id_base() const -> const string & {
    return path_.at(0);
  }

  [[nodiscard]] inline auto path() const -> const vector<string> & {
    return path_;
  }
};

class toc final {
  const string &source_{};
  vector<frame> frames_{};
  vector<string> all_registers_{};
  vector<string> named_registers_{};
  vector<string> scratch_registers_{};
  vector<string> allocated_registers_{};
  vector<string> allocated_registers_src_locs_{}; // source locations
  unordered_set<string> initiated_registers_{};
  lut<field_info> fields_{};
  lut<func_info> funcs_{};
  vector<call_info> calls_{};
  lut<const type &> types_{};
  const type *type_void_{};
  const type *type_default_{};
  const type *type_bool_{};
  size_t usage_max_scratch_regs_{};
  size_t usage_max_frame_count_{};
  size_t usage_max_stack_size_{};

public:
  inline explicit toc(const string &source)
      : source_{source},
        all_registers_{"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
                       "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"},
        named_registers_{"rax", "rbx", "rcx", "rdx", "rsi", "rdi"},
        scratch_registers_{"r8",  "r9",  "r10", "r11",
                           "r12", "r13", "r14", "r15"} {}

  inline toc() = delete;
  inline toc(const toc &) = delete;
  inline toc(toc &&) = delete;
  inline auto operator=(const toc &) -> toc & = delete;
  inline auto operator=(toc &&) -> toc & = delete;

  inline ~toc() = default;

  inline auto source() const -> const string & { return source_; }

  inline void add_field(const token &src_loc, const string &name,
                        const stmt_def_field *fld, const bool is_str_field) {

    if (fields_.has(name)) {
      throw compiler_exception(
          src_loc, "field '" + name + "' already defined at " +
                       source_location_hr(fields_.get(name).declared_at));
    }
    fields_.put(name, {fld, src_loc, is_str_field});
  }

  inline void add_func(const token &src_loc, const string &name,
                       const type &return_type, const stmt_def_func *func) {

    if (funcs_.has(name)) {
      const func_info &fn{funcs_.get_const_ref(name)};
      const string loc{source_location_hr(fn.declared_at)};
      throw compiler_exception(src_loc, "function '" + name +
                                            "' already defined at " + loc);
    }
    funcs_.put(name, {func, src_loc, return_type});
  }

  inline auto get_func_or_throw(const token &src_loc, const string &name) const
      -> const stmt_def_func & {

    if (not funcs_.has(name)) {
      throw compiler_exception(src_loc, "function '" + name + "' not found");
    }

    return *funcs_.get_const_ref(name).def;
  }

  inline auto is_func_builtin(const token &src_loc, const string &name) const
      -> bool {

    if (not funcs_.has(name)) {
      throw compiler_exception(src_loc, "function '" + name + "' not found");
    }

    return funcs_.get_const_ref(name).def == nullptr;
  }

  inline auto get_func_return_type_or_throw(const token &src_loc,
                                            const string &name) const
      -> const type & {

    if (not funcs_.has(name)) {
      throw compiler_exception(src_loc, "function '" + name + "' not found");
    }

    return funcs_.get_const_ref(name).type_ref;
  }

  inline void add_type(const token &src_loc, const type &tp) {
    if (types_.has(tp.name())) {
      //? todo. specify where the type has been defined
      throw compiler_exception(src_loc,
                               "type '" + tp.name() + "' already defined");
    }

    types_.put(tp.name(), tp);
  }

  inline auto get_type_or_throw(const token &src_loc, const string &name) const
      -> const type & {

    if (not types_.has(name)) {
      throw compiler_exception(src_loc, "type '" + name + "' not found");
    }

    return types_.get_const_ref(name);
  }

  inline auto source_location_for_use_in_label(const token &src_loc) const
      -> string {

    const auto [line, col]{line_and_col_num_for_char_index(
        src_loc.start_index(), source_.c_str())};

    return to_string(line) + "_" + to_string(col);
  }

  inline auto source_location_hr(const token &src_loc) -> string {

    const auto [line, col]{line_and_col_num_for_char_index(
        src_loc.start_index(), source_.c_str())};

    return to_string(line) + ":" + to_string(col);
  }

  inline static auto line_and_col_num_for_char_index(const size_t char_index,
                                                     const char *src)
      -> pair<size_t, size_t> {

    size_t ix{0};
    size_t lix{0};
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

  inline void finish(ostream &os) {
    os << "\n; max scratch registers in use: " << usage_max_scratch_regs_
       << endl;
    os << ";            max frames in use: " << usage_max_frame_count_ << endl;
    os << ";        max inline stack size: " << usage_max_stack_size_ << " B"
       << endl;
    //		os<<";          max stack in use: "<<tc.max_stack_usage_<<endl;
    assert(all_registers_.size() == 16);
    assert(allocated_registers_.empty());
    assert(allocated_registers_src_locs_.empty());
    assert(calls_.empty());
    assert(frames_.empty());
    //		assert(initiated_registers_.empty());
    initiated_registers_.clear();
    assert(named_registers_.size() == 6);
    assert(scratch_registers_.size() == 8);
    usage_max_frame_count_ = 0;
    usage_max_scratch_regs_ = 0;
  }

  inline auto resolve_identifier(const statement &st,
                                 const bool must_be_initiated) const
      -> ident_resolved {

    const ident_resolved &id_resolved{
        resolve_ident_or_empty(st.tok(), st.identifier(), must_be_initiated)};

    if (not id_resolved.id_nasm.empty()) {
      return id_resolved;
    }

    throw compiler_exception(st.tok(), "cannot resolve identifier '" +
                                           st.identifier() + "'");
  }

  inline auto resolve_identifier(const token &src_loc, const string &name,
                                 const bool must_be_initiated) const
      -> ident_resolved {

    const ident_resolved &name_resolved{
        resolve_ident_or_empty(src_loc, name, must_be_initiated)};

    if (not name_resolved.id_nasm.empty()) {
      return name_resolved;
    }

    throw compiler_exception(src_loc,
                             "cannot resolve identifier '" + name + "'");
  }

  inline void add_alias(const string &name,
                        const string &name_in_parent_frame) {

    frames_.back().add_alias(name, name_in_parent_frame);
  }

  inline void enter_func(const string &name,
                         const vector<func_return_info> &returns = {},
                         const bool is_inline = false,
                         const string &call_path = "",
                         const string &return_jmp_label = "") {

    frames_.emplace_back(name, frame::frame_type::FUNC, returns, is_inline,
                         call_path, return_jmp_label);
    refresh_usage();
  }

  inline void enter_block() {
    frames_.emplace_back("", frame::frame_type::BLOCK);
    refresh_usage();
  }

  inline void enter_loop(const string &name) {
    frames_.emplace_back(name, frame::frame_type::LOOP);
    refresh_usage();
  }

  inline void exit_func(const string &name) {
    const frame &frm{frames_.back()};
    assert(frm.is_func() and frm.is_name(name));
    frames_.pop_back();
  }

  inline void exit_loop(const string &name) {
    const frame &frm{frames_.back()};
    assert(frm.is_loop() and frm.is_name(name));
    frames_.pop_back();
  }

  inline void exit_block() {
    const frame &frm{frames_.back()};
    assert(frm.is_block());
    frames_.pop_back();
  }

  inline void add_var(const token &src_loc, ostream &os, size_t indnt,
                      const string &name, const type &var_type,
                      const bool initiated) {

    // check if variable is already declared in this scope
    if (frames_.back().has_var(name)) {
      const var_info &var{frames_.back().get_var_const_ref(name)};
      throw compiler_exception(src_loc,
                               "variable '" + name + "' already declared at " +
                                   source_location_hr(var.declared_at));
    }
    // check if variable shadows previously declared variable
    const auto [id, frm]{get_id_and_frame_for_identifier(name)};
    if (frm.has_var(id)) {
      const var_info &var{frm.get_var_const_ref(id)};
      throw compiler_exception(
          src_loc, "variable '" + name + "' shadows variable declared at " +
                       source_location_hr(var.declared_at));
    }

    const int stack_idx{int(get_current_stack_size() + var_type.size())};
    frames_.back().add_var(src_loc, name, var_type, -stack_idx, initiated);

    const size_t total_stack_size{get_total_stack_size()};
    if (total_stack_size > usage_max_stack_size_) {
      usage_max_stack_size_ = total_stack_size;
    }

    // comment the resolved name
    const ident_resolved &name_resolved{
        resolve_identifier(src_loc, name, false)};
    indent(os, indnt, true);
    os << name << ": " << name_resolved.id_nasm << endl;
  }

  inline void add_func_arg(const token &src_loc, ostream &os, size_t indnt,
                           const string &name, const type &arg_type,
                           const int stack_idx) {

    assert(frames_.back().is_func() && not frames_.back().func_is_inline());

    frames_.back().add_var(src_loc, name, arg_type, stack_idx, true);
    // comment the resolved name
    const ident_resolved &name_resolved{
        resolve_identifier(src_loc, name, false)};
    indent(os, indnt, true);
    os << name << ": " << name_resolved.id_nasm << endl;
  }

  inline auto alloc_scratch_register(const token &src_loc, ostream &os,
                                     const size_t indnt) -> const string & {

    if (scratch_registers_.empty()) {
      throw compiler_exception(
          src_loc,
          "out of scratch registers. try to reduce expression complexity");
    }

    const string &reg{scratch_registers_.back()};
    scratch_registers_.pop_back();

    indent(os, indnt, true);
    os << "alloc " << reg << endl;

    //? 8 is magic number
    const size_t n{8 - scratch_registers_.size()};
    if (n > usage_max_scratch_regs_) {
      // stmt_assign_var tries 2 different methods making this metric inaccurate
      // if a discarded method uses more registers than the selected method
      usage_max_scratch_regs_ = n;
    }

    allocated_registers_.emplace_back(reg);
    allocated_registers_src_locs_.emplace_back(source_location_hr(src_loc));
    return reg;
  }

  inline void alloc_named_register_or_throw(const statement &st, ostream &os,
                                            const size_t indnt,
                                            const string &reg) {
    indent(os, indnt, true);
    os << "alloc " << reg << endl;
    auto reg_iter{find(named_registers_.begin(), named_registers_.end(), reg)};
    if (reg_iter == named_registers_.end()) {
      string loc{};
      const size_t n{allocated_registers_.size()};
      for (size_t i{0}; i < n; i++) {
        if (allocated_registers_.at(i) == reg) {
          loc = allocated_registers_src_locs_.at(i);
          break;
        }
      }
      throw compiler_exception(st.tok(), "cannot allocate register '" + reg +
                                             "' because it was allocated at " +
                                             loc);
    }
    named_registers_.erase(reg_iter);
    allocated_registers_.emplace_back(reg);
    allocated_registers_src_locs_.emplace_back(source_location_hr(st.tok()));
  }

  inline auto alloc_named_register(const token &src_loc, ostream &os,
                                   const size_t indnt, const string &reg)
      -> bool {

    indent(os, indnt, true);
    os << "alloc " << reg;
    auto reg_iter{find(named_registers_.begin(), named_registers_.end(), reg)};
    if (reg_iter == named_registers_.end()) {
      os << ": false" << endl;
      return false;
    }
    named_registers_.erase(reg_iter);
    allocated_registers_.emplace_back(reg);
    allocated_registers_src_locs_.emplace_back(source_location_hr(src_loc));
    os << endl;
    return true;
  }

  inline void free_named_register(ostream &os, const size_t indnt,
                                  const string &reg) {
    indent(os, indnt, true);
    os << "free " << reg << endl;
    assert(allocated_registers_.back() == reg);
    allocated_registers_.pop_back();
    allocated_registers_src_locs_.pop_back();
    named_registers_.emplace_back(reg);
    initiated_registers_.erase(reg);
  }

  inline void free_scratch_register(ostream &os, const size_t indnt,
                                    const string &reg) {
    indent(os, indnt, true);
    os << "free " << reg << endl;
    assert(allocated_registers_.back() == reg);
    allocated_registers_.pop_back();
    allocated_registers_src_locs_.pop_back();
    scratch_registers_.emplace_back(reg);
    initiated_registers_.erase(reg);
  }

  inline auto get_loop_label_or_throw(const token &src_loc) const
      -> const string & {

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

  inline auto get_inline_call_path(const token &src_loc) const
      -> const string & {

    size_t i{frames_.size()};
    while (i--) {
      if (frames_.at(i).is_func()) {
        return frames_.at(i).inline_call_path();
      }
    }
    throw compiler_exception(src_loc, "not in a function");
  }

  inline auto get_func_return_label_or_throw(const token &src_loc) const
      -> const string & {

    size_t i{frames_.size()};
    while (i--) {
      if (frames_.at(i).is_func()) {
        return frames_.at(i).func_ret_label();
      }
    }
    throw compiler_exception(src_loc, "not in a function");
  }

  inline auto get_func_returns(const token &src_loc) const
      -> const vector<func_return_info> & {

    size_t i{frames_.size()};
    while (i--) {
      if (frames_.at(i).is_func()) {
        return frames_.at(i).get_func_returns_infos();
      }
    }
    throw compiler_exception(src_loc, "not in a function");
  }

  inline void comment_source(const statement &st, ostream &os,
                             const size_t indnt) const {

    const auto [line, col]{line_and_col_num_for_char_index(
        st.tok().start_index(), source_.c_str())};

    indent(os, indnt, true);
    os << "[" << line << ":" << col << "]";

    stringstream ss{};
    ss << " ";
    st.source_to(ss);
    const string &s{ss.str()};
    const string &res{regex_replace(s, regex(R"(\s+)"), " ")};
    os << res << endl;
  }

  inline void comment_source(ostream &os, const string &dst, const string &op,
                             const statement &st) const {

    const auto [line, col]{line_and_col_num_for_char_index(
        st.tok().start_index(), source_.c_str())};

    os << "[" << line << ":" << col << "]";

    stringstream ss{};
    ss << " " << dst << " " << op << " ";
    st.source_to(ss);
    const string &s{ss.str()};
    const string &res{regex_replace(s, regex(R"(\s+)"), " ")};
    os << res << endl;
  }

  inline void comment_token(ostream &os, const token &tk) const {
    const auto [line, col]{
        line_and_col_num_for_char_index(tk.start_index(), source_.c_str())};

    os << "[" << line << ":" << col << "]";
    os << " " << tk.name() << endl;
  }

  inline auto is_identifier_register(const string &id) const -> bool {
    return find(all_registers_.begin(), all_registers_.end(), id) !=
           all_registers_.end();
  }

  inline void enter_call(const token &src_loc, ostream &os,
                         const size_t indnt) {
    const bool root_call{calls_.empty()};
    const size_t nbytes_of_vars_on_stack{root_call ? get_current_stack_size()
                                                   : 0};
    if (root_call) {
      // this call is not nested within another call's arguments
      if (nbytes_of_vars_on_stack != 0) {
        // adjust stack past the allocated vars
        asm_cmd(src_loc, os, indnt, "sub", "rsp",
                to_string(nbytes_of_vars_on_stack));
        // stack: <base>,.. vars ..,
      }
    }
    // index in the allocated registers that have been allocated but not pushed
    // prior to this call (that might clobber them)
    const size_t alloc_regs_idx{root_call ? 0 : calls_.back().alloc_reg_idx};

    // push registers allocated prior to this call
    const size_t n{allocated_registers_.size()};
    size_t nregs_pushed_on_stack{0};
    for (size_t i{alloc_regs_idx}; i < n; i++) {
      const string &reg{allocated_registers_.at(i)};
      if (not is_register_initiated(reg)) {
        // don't save uninitiated registers because their value is irrelevants
        continue;
      }
      // push only registers that contain a valid value
      asm_push(src_loc, os, indnt, reg);
      nregs_pushed_on_stack++;
    }
    calls_.emplace_back(call_info{nregs_pushed_on_stack,
                                  allocated_registers_.size(),
                                  nbytes_of_vars_on_stack});
  }

  inline void exit_call(const token &src_loc, ostream &os, const size_t indnt,
                        const size_t nbytes_of_args_on_stack,
                        const vector<string> &allocated_args_registers) {

    const size_t nregs_pushed{calls_.back().nregs_pushed};
    const size_t nbytes_of_vars{calls_.back().nbytes_of_vars};
    calls_.pop_back();
    const bool restore_rsp_to_base{calls_.empty()};
    const size_t alloc_reg_idx{
        restore_rsp_to_base ? 0 : calls_.back().alloc_reg_idx};

    // optimization can be done if no registers need to be popped
    //   rsp is adjusted once
    if (nregs_pushed == 0) {
      // stack is: <base>,vars,args,
      if (restore_rsp_to_base) {
        const string &offset{
            to_string(nbytes_of_args_on_stack + nbytes_of_vars)};
        asm_cmd(src_loc, os, indnt, "add", "rsp", offset);
        // stack is: <base>,
      } else {
        const string &offset{to_string(nbytes_of_args_on_stack)};
        asm_cmd(src_loc, os, indnt, "add", "rsp", offset);
        // stack is: <base>,vars,
      }
      // free named registers
      size_t i{allocated_registers_.size()};
      while (true) {
        if (i == alloc_reg_idx) {
          break;
        }
        i--;
        const string &reg{allocated_registers_.at(i)};
        // don't pop registers used to pass arguments
        if (find(allocated_args_registers.begin(),
                 allocated_args_registers.end(),
                 reg) != allocated_args_registers.end()) {
          free_named_register(os, indnt, reg);
        }
      }
    } else {
      // stack is: <base>,vars,regs,args,
      if (nbytes_of_args_on_stack != 0) {
        const string &offset{to_string(nbytes_of_args_on_stack)};
        asm_cmd(src_loc, os, indnt, "add", "rsp", offset);
      }
      // stack is: <base>,vars,regs,

      // pop registers pushed prior to this call
      size_t i{allocated_registers_.size()};
      while (true) {
        if (i == alloc_reg_idx) {
          break;
        }
        i--;
        const string &reg{allocated_registers_.at(i)};
        // don't pop registers used to pass arguments
        if (find(allocated_args_registers.begin(),
                 allocated_args_registers.end(),
                 reg) == allocated_args_registers.end()) {
          if (is_register_initiated(reg)) {
            // pop only registers that were pushed
            asm_pop(src_loc, os, indnt, reg);
          }
        } else {
          free_named_register(os, indnt, reg);
        }
      }

      // stack is: <base>,vars,
      if (restore_rsp_to_base) {
        // this was not a call within the arguments of another call
        // stack is: <base>,vars,
        if (nbytes_of_vars != 0) {
          const string &offset{to_string(nbytes_of_vars)};
          asm_cmd(src_loc, os, indnt, "add", "rsp", offset);
        }
        // stack is: <base>,
      }
    }
  }

  inline void asm_cmd(const token &src_loc, ostream &os, const size_t indnt,
                      const string &op, const string &dst_resolved,
                      const string &src_resolved) {
    if (op == "mov") {
      if (dst_resolved == src_resolved) {
        return;
      }

      if (is_identifier_register(dst_resolved)) {
        // for optimization of push/pop when calling a function
        initiated_registers_.insert(dst_resolved);
      }
    }

    const size_t dst_size{get_size_from_operand(src_loc, dst_resolved)};
    const size_t src_size{get_size_from_operand(src_loc, src_resolved)};

    if (dst_size == src_size) {
      if (not(is_operand_memory(dst_resolved) and
              is_operand_memory(src_resolved))) {
        // both operands are not memory references
        indent(os, indnt);
        os << op << " " << dst_resolved << ", " << src_resolved << endl;
        return;
      }
      // both operands are memory references
      //   use scratch register for transfer
      const string &reg{alloc_scratch_register(src_loc, os, indnt)};
      const string &reg_sized{
          get_register_operand_for_size(src_loc, reg, src_size)};
      indent(os, indnt);
      os << "mov " << reg_sized << ", " << src_resolved << endl;
      indent(os, indnt);
      os << op << " " << dst_resolved << ", " << reg_sized << endl;
      free_scratch_register(os, indnt, reg);
      return;
    }

    if (dst_size > src_size) {
      // mov rax,byte[b] -> movsx
      if (not(is_operand_memory(dst_resolved) and
              is_operand_memory(src_resolved))) {
        // not both operands memory references
        if (op == "mov") {
          indent(os, indnt);
          os << "movsx " << dst_resolved << ", " << src_resolved << endl;
          return;
        }
      }
      // both operands refer to memory
      //   use scratch register for transfer
      const string &reg{alloc_scratch_register(src_loc, os, indnt)};
      const string &reg_sized{
          get_register_operand_for_size(src_loc, reg, dst_size)};
      indent(os, indnt);
      os << "movsx " << reg_sized << ", " << src_resolved << endl;
      indent(os, indnt);
      os << op << " " << dst_resolved << ", " << reg_sized << endl;
      free_scratch_register(os, indnt, reg);
      return;
    }

    // dst_size < src_size
    //? truncation might change value of signed number
    if (is_identifier_register(src_resolved)) {
      const string &reg_sized{
          get_register_operand_for_size(src_loc, src_resolved, dst_size)};
      indent(os, indnt);
      os << op << " " << dst_resolved << ", " << reg_sized << endl;
      return;
    }

    if (is_operand_memory(src_resolved)) {
      //? todo. this displays nasm identifiers but should be human readable
      // identifiers
      throw compiler_exception(src_loc, "cannot move '" + src_resolved +
                                            "' to '" + dst_resolved +
                                            "' because it would be truncated");
    }

    // constant
    indent(os, indnt);
    os << op << " " << dst_resolved << ", " << src_resolved << endl;
  }

  inline void set_var_is_initiated(const string &name) {
    const identifier ident{name};
    const auto [id, frm]{get_id_and_frame_for_identifier(ident.id_base())};
    // is 'id_nasm' a variable?
    if (frm.has_var(id)) {
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

    throw panic_exception("unexpected code path " + string{__FILE__} + ":" +
                          to_string(__LINE__));
  }

  inline auto is_var_initiated(const string &name) -> bool {
    const identifier ident{name};
    const auto [id, frm]{get_id_and_frame_for_identifier(ident.id_base())};
    // is 'id_nasm' a variable?
    if (frm.has_var(id)) {
      return frm.get_var_ref(id).initiated;
    }

    if (fields_.has(id)) {
      return true; // a field, it is initiated
    }

    if (is_identifier_register(id)) {
      auto it = std::find(initiated_registers_.begin(),
                          initiated_registers_.end(), id);
      return it != initiated_registers_.end();
    }

    throw panic_exception("unexpected code path " + string{__FILE__} + ":" +
                          to_string(__LINE__));
  }

  inline void set_type_void(const type &tp) { type_void_ = &tp; }

  inline auto get_type_void() const -> const type & { return *type_void_; }

  inline void set_type_default(const type &tp) { type_default_ = &tp; }

  inline auto get_type_default() const -> const type & {
    return *type_default_;
  }

  inline void set_type_bool(const type &tp) { type_bool_ = &tp; }

  inline auto get_type_bool() const -> const type & { return *type_bool_; }

  inline static auto is_operand_memory(const string &operand) -> bool {
    return operand.find_first_of('[') != string::npos;
  }

  inline auto get_size_from_operand(const token &src_loc,
                                    const string &operand) const -> size_t {
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

  inline static auto get_size_from_operand_register(const token &src_loc,
                                                    const string &operand)
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

  inline static auto get_register_operand_for_size(const token &src_loc,
                                                   const string &operand,
                                                   const size_t size)
      -> string {
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
        throw compiler_exception(src_loc, "illegal size " + to_string(size) +
                                              " for register operand '" +
                                              operand + "'");
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
        throw compiler_exception(src_loc, "illegal size " + to_string(size) +
                                              " for register '" + operand +
                                              "'");
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
        throw compiler_exception(src_loc, "illegal size " + to_string(size) +
                                              " for register '" + operand +
                                              "'");
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
        throw compiler_exception(src_loc, "illegal size " + to_string(size) +
                                              " for register '" + operand +
                                              "'");
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
        throw compiler_exception(src_loc, "illegal size " + to_string(size) +
                                              " for register '" + operand +
                                              "'");
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
        throw compiler_exception(src_loc, "illegal size " + to_string(size) +
                                              " for register '" + operand +
                                              "'");
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
        throw compiler_exception(src_loc, "illegal size " + to_string(size) +
                                              " for register '" + operand +
                                              "'");
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
        throw compiler_exception(src_loc, "illegal size " + to_string(size) +
                                              " for register '" + operand +
                                              "'");
      }
    }
    const regex rx{R"(r(\d+))"};
    smatch match{};
    if (!regex_search(operand, match, rx)) {
      throw compiler_exception(src_loc, "unknown register " + operand);
    }
    const string &rnbr{match[1]};
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
      throw compiler_exception(src_loc, "illegal size " + to_string(size) +
                                            " for register '" + operand + "'");
    }

    throw compiler_exception(src_loc, "unknown register '" + operand + "'");
  }

  inline static void asm_push([[maybe_unused]] const token &src_loc,
                              ostream &os, const size_t indnt,
                              const string &operand) {
    indent(os, indnt);
    os << "push " << operand << endl;
  }

  inline static void asm_pop([[maybe_unused]] const token &src_loc, ostream &os,
                             const size_t indnt, const string &operand) {
    indent(os, indnt);
    os << "pop " << operand << endl;
  }

  inline static void asm_ret([[maybe_unused]] const token &src_loc, ostream &os,
                             const size_t indnt) {
    indent(os, indnt);
    os << "ret\n";
  }

  inline static void asm_jmp([[maybe_unused]] const token &src_loc, ostream &os,
                             const size_t indnt, const string &label) {
    indent(os, indnt);
    os << "jmp " << label << endl;
  }

  inline static void asm_label([[maybe_unused]] const token &src_loc,
                               ostream &os, const size_t indnt,
                               const string &label) {
    indent(os, indnt);
    os << label << ":" << endl;
  }

  inline static void asm_call([[maybe_unused]] const token &src_loc,
                              ostream &os, const size_t indnt,
                              const string &label) {
    indent(os, indnt);
    os << "call " << label << endl;
  }

  inline static void asm_neg([[maybe_unused]] const token &src_loc, ostream &os,
                             const size_t indnt, const string &dst_resolved) {
    indent(os, indnt);
    os << "neg " << dst_resolved << endl;
  }

  inline static void asm_not([[maybe_unused]] const token &src_loc, ostream &os,
                             const size_t indnt, const string &dst_resolved) {
    indent(os, indnt);
    os << "not " << dst_resolved << endl;
  }

  inline static void indent(ostream &os, const size_t indnt,
                            const bool comment = false) {
    if (indnt == 0) {
      if (comment) {
        os << ";";
      }
      return;
    }
    os << (comment ? ";" : " ");
    for (size_t i{0}; i < indnt; i++) {
      os << "  ";
    }
  }

private:
  inline auto get_id_and_frame_for_identifier(const string &name)
      -> pair<string, frame &> {

    string id{name};
    // traverse the frames and resolve the id_nasm (which might be an alias) to
    // a variable, field, register or constant
    size_t i{frames_.size()};
    while (i--) {
      // is the frame a function?
      if (frames_.at(i).is_func()) {
        // is it an alias defined by an argument in the function?
        if (not frames_.at(i).has_alias(id)) {
          break;
        }
        // yes, continue resolving alias until it is
        // a variable, field, register or constant
        id = frames_.at(i).get_alias(id);
        continue;
      }
      // does scope contain the variable
      if (frames_.at(i).has_var(id)) {
        break;
      }
    }
    return {move(id), frames_.at(i)};
  }

  inline auto is_register_initiated(const string &reg) const -> bool {
    return initiated_registers_.contains(reg);
  }

  inline auto get_current_stack_size() const -> size_t {
    assert(!frames_.empty());
    size_t nbytes{0};
    size_t i{frames_.size()};
    while (i--) {
      const frame &frm{frames_.at(i)};
      nbytes += frm.allocated_stack_size();
      if (frm.is_func() and not frm.func_is_inline()) {
        return nbytes;
      }
    }
    // top frame, 'main'
    return nbytes;
  }

  //? does not take into account non-inlined calls and the return address from a
  // function call
  inline auto get_total_stack_size() const -> size_t {
    assert(!frames_.empty());
    size_t nbytes{0};
    size_t i{frames_.size()};
    while (i--) {
      const frame &frm{frames_.at(i)};
      nbytes += frm.allocated_stack_size();
    }
    return nbytes;
  }

  inline auto resolve_ident_or_empty(const token &src_loc, const string &ident,
                                     const bool must_be_initiated) const
      -> ident_resolved {

    const identifier bid{ident};
    string id{bid.id_base()};
    // traverse the frames and resolve the id_nasm (which might be an alias) to
    // a variable, field, register or constant
    size_t i{frames_.size()};
    while (i--) {
      // is the frame a function?
      if (frames_.at(i).is_func()) {
        // is it an alias defined by an argument in the function?
        if (not frames_.at(i).has_alias(id)) {
          break;
        }
        // yes, continue resolving alias until it is
        // a variable, field, register or constant
        id = frames_.at(i).get_alias(id);
        if (i == 0) {
          break;
        }
        continue;
      }
      // does scope contain the variable
      if (frames_.at(i).has_var(id)) {
        break;
      }
      if (i == 0) {
        break;
      }
    }

    // is 'id_nasm' a variable?
    if (frames_.at(i).has_var(id)) {
      const var_info &var{frames_.at(i).get_var_const_ref(id)};
      if (must_be_initiated and not var.initiated) {
        throw compiler_exception(src_loc, "variable '" + var.name +
                                              "' is not initiated");
      }
      auto [tp, acc]{var.type_ref.accessor(src_loc, bid.path(), var.stack_idx)};
      return {ident, acc, 0, tp, ident_resolved::ident_type::VAR};
    }

    // is 'id_nasm' a register?
    if (is_identifier_register(id)) {
      if (must_be_initiated and not is_register_initiated(id)) {
        throw compiler_exception(src_loc,
                                 "register '" + id + "' is not initiated");
      }

      //? unary ops?
      return {ident, id, 0, get_type_default(),
              ident_resolved::ident_type::REGISTER};
    }

    // is it a field?
    if (fields_.has(id)) {
      const string &after_dot =
          bid.path().size() < 2 ? "" : bid.path().at(1); //? bug. not correct
      if (after_dot == "len") {
        return {ident, id + "." + after_dot, 0, get_type_default(),
                ident_resolved::ident_type::IMPLIED};
      }
      const field_info &fi{fields_.get_const_ref(id)};
      if (fi.is_str) {
        return {ident, id, 0, get_type_default(),
                ident_resolved::ident_type::FIELD};
      }
      //? assumes qword
      return {ident, "qword[" + id + "]", 0, get_type_default(),
              ident_resolved::ident_type::FIELD};
    }

    // is it a constant?
    char *ep{};
    const long const_value{strtol(id.c_str(), &ep, 10)};
    if (!*ep) {
      return {ident, id, const_value, get_type_default(),
              ident_resolved::ident_type::CONST};
    }

    if (id.starts_with("0x")) { // hex
      const long value{strtol(id.c_str() + 2, &ep, 16)};
      if (!*ep) {
        return {ident, id, value, get_type_default(),
                ident_resolved::ident_type::CONST};
      }
    }

    if (id.starts_with("0b")) { // binary
      const long value{strtol(id.c_str() + 2, &ep, 2)};
      if (!*ep) {
        return {ident, id, value, get_type_default(),
                ident_resolved::ident_type::CONST};
      }
    }

    // is it a boolean constant?
    if (id == "true") {
      return {ident, id, 1, get_type_bool(), ident_resolved::ident_type::CONST};
    }

    if (id == "false") {
      return {ident, id, 0, get_type_bool(), ident_resolved::ident_type::CONST};
    }

    // not resolved, return empty answer
    return {"", "", 0, get_type_void(), ident_resolved::ident_type::CONST};
  }

  inline void refresh_usage() {
    if (frames_.size() > usage_max_frame_count_) {
      usage_max_frame_count_ = frames_.size();
    }
  }
};
