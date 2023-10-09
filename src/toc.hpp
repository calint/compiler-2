#pragma once
#include "compiler_exception.hpp"
#include "lut.hpp"
#include "type.hpp"

class stmt_def_func;
class stmt_def_field;
class stmt_def_type;

struct var_meta final {
  string name;
  const type &tp;
  token declared_at;
  int stack_idx{};   // location relative to rbp
  string nasm_ident; // nasm usable literal
  bool initiated{};  // true if variable has been initiated
};

class frame final {
public:
  enum class frame_type { FUNC, BLOCK, LOOP };

  inline frame(string name, const frame_type tpe, string call_path = "",
               string func_ret_label = "", const bool func_is_inline = false,
               string func_ret_var = "") noexcept
      : name_{move(name)}, call_path_{move(call_path)},
        func_ret_label_{move(func_ret_label)}, func_ret_var_{move(
                                                   func_ret_var)},
        func_is_inline_{func_is_inline}, type_{tpe} {}

  inline void add_var(const token &declared_at, const string &name,
                      const type &tpe, const int stack_idx,
                      const bool initiated) {
    string nasm_ident;
    if (stack_idx > 0) {
      // function argument
      nasm_ident = "[rbp+" + to_string(stack_idx) + "]";
    } else if (stack_idx < 0) {
      // variable
      nasm_ident = "[rbp" + to_string(stack_idx) + "]";
      allocated_stack_ += tpe.size();
    } else {
      throw panic_exception("toc:fram:add_var");
    }
    //		if(size==8){
    //			nasm_ident="qword"+nasm_ident;
    // }else if(size==4){
    // 	ident="dword"+ident;
    // }else if(size==2){
    // 	ident="word"+ident;
    // }else if(size==1){
    // 	ident="byte"+ident;
    //		}else{
    nasm_ident = "qword" + nasm_ident;
    //			throw"unexpected variable size: "+to_string(size);
    //		}
    vars_.put(name, {name, tpe, declared_at, stack_idx, nasm_ident, initiated});
  }

  inline auto allocated_stack_size() const -> size_t {
    return allocated_stack_;
  }

  inline auto has_var(const string &name) const -> bool {
    return vars_.has(name);
  }

  inline auto get_var_ref(const string &name) -> var_meta & {
    return vars_.get_ref(name);
  }

  inline auto get_var_const_ref(const string &name) const -> const var_meta & {
    return vars_.get_const_ref(name);
  }

  inline void add_alias(const string &ident, const string &outside_ident) {
    aliases_.put(ident, outside_ident);
  }

  inline auto is_func() const -> bool { return type_ == frame_type::FUNC; }

  inline auto is_block() const -> bool { return type_ == frame_type::BLOCK; }

  inline auto is_loop() const -> bool { return type_ == frame_type::LOOP; }

  inline auto is_name(const string &nm) const -> bool { return name_ == nm; }

  inline auto has_alias(const string &nm) const -> bool {
    return aliases_.has(nm);
  }

  inline auto get_alias(const string &nm) const -> string {
    return aliases_.get(nm);
  }

  inline auto name() const -> const string & { return name_; }

  inline auto func_ret_label() const -> const string & {
    return func_ret_label_;
  }

  inline auto inline_call_path() const -> const string & { return call_path_; }

  inline auto func_is_inline() const -> bool { return func_is_inline_; }

  inline auto func_ret_var() const -> const string & { return func_ret_var_; }

private:
  string name_{};      // optional name
  string call_path_{}; // a unique path of source locations of the inlined call
  size_t allocated_stack_{}; // number of bytes used on the stack by this frame
  lut<var_meta> vars_{};     // variables declared in this frame
  lut<string>
      aliases_{}; // aliases that refers to previous frame(s) alias or variable
  string func_ret_label_{}; // the label to jump to when exiting an inlined
                            // function
  string func_ret_var_{};   // the variable that contains the return value
  bool func_is_inline_{};
  frame_type type_{frame_type::FUNC}; // frame type
};

struct field_meta final {
  const stmt_def_field *def{};
  const token declared_at;
  const bool is_str{};
};

struct func_meta final {
  const type &tp;
  const stmt_def_func *def{};
  const token declared_at;
};

struct call_meta final {
  size_t nregs_pushed{};
  size_t alloc_reg_idx{};
  size_t nbytes_of_vars{};
};

struct type_meta final {
  const stmt_def_type &def;
  const token declared_at;
  const type &tp;
};

struct ident_resolved final {
  enum class ident_type { CONST, VAR, REGISTER, FIELD, IMPLIED };

  string id;
  string id_nasm;
  long int const_value{};
  const type &tp;
  ident_type ident_type{ident_type::CONST};

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

class baz_ident final {
public:
  inline explicit baz_ident(const string &id) : id_{id} {
    size_t start{0};
    size_t end{0};
    while ((end = id.find('.', start)) != string::npos) {
      path_.push_back(id.substr(start, end - start));
      start = end + 1;
    }
    path_.push_back(id.substr(start));
  }

  [[nodiscard]] inline auto id() const -> const string & { return id_; }

  [[nodiscard]] inline auto id_base() const -> const string & {
    return path_[0];
  }

  [[nodiscard]] inline auto path() const -> const vector<string> & {
    return path_;
  }

private:
  string id_{};
  vector<string> path_{};
};

class toc final {
public:
  inline explicit toc(const string &source)
      : all_registers_{"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
                       "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"},
        scratch_registers_{"r8",  "r9",  "r10", "r11",
                           "r12", "r13", "r14", "r15"},
        named_registers_{"rax", "rbx", "rcx", "rdx", "rsi", "rdi"},
        source_str_{source} {}

  inline toc() = delete;
  inline toc(const toc &) = delete;
  inline toc(toc &&) = delete;
  inline auto operator=(const toc &) -> toc & = delete;
  inline auto operator=(toc &&) -> toc & = delete;

  inline ~toc() = default;

  inline auto source() const -> const string & { return source_str_; }

  inline void add_field(const statement &st, const string &ident,
                        const stmt_def_field *f, const bool is_str_field) {
    if (fields_.has(ident)) {
      const field_meta &fld = fields_.get(ident);
      throw compiler_exception(st.tok(),
                               "field '" + ident + "' already defined at " +
                                   source_location_hr(fld.declared_at));
    }
    fields_.put(ident, {f, st.tok(), is_str_field});
  }

  inline void add_func(const statement &st, const string &name,
                       const type &return_type, const stmt_def_func *ref) {
    if (funcs_.has(name)) {
      const func_meta &func = funcs_.get(name);
      throw compiler_exception(st.tok(),
                               "function '" + name + "' already defined at " +
                                   source_location_hr(func.declared_at));
    }
    funcs_.put(name, {return_type, ref, st.tok()});
  }

  inline auto get_func_or_throw(const statement &st, const string &name) const
      -> const stmt_def_func & {
    if (not funcs_.has(name)) {
      throw compiler_exception(st.tok(), "function '" + name + "' not found");
    }

    return *funcs_.get_const_ref(name).def;
  }

  inline auto is_func_builtin(const statement &st, const string &name) const
      -> bool {
    if (not funcs_.has(name)) {
      throw compiler_exception(st.tok(), "function '" + name + "' not found");
    }

    return funcs_.get_const_ref(name).def == nullptr;
  }

  inline auto get_func_return_type_or_throw(const statement &st,
                                            const string &name) const
      -> const type & {
    if (not funcs_.has(name)) {
      throw compiler_exception(st.tok(), "function '" + name + "' not found");
    }

    return funcs_.get_const_ref(name).tp;
  }

  inline void add_type(const statement &st, const type &tp) {
    if (types_.has(tp.name())) {
      throw compiler_exception(st.tok(),
                               "type '" + tp.name() + "' already defined");
    }

    types_.put(tp.name(), tp);
  }

  inline auto get_type_or_throw(const statement &st, const string &name) const
      -> const type & {
    if (not types_.has(name)) {
      throw compiler_exception(st.tok(), "type '" + name + "' not found");
    }

    return types_.get_const_ref(name);
  }

  inline auto source_location_for_label(const token &tk) const -> string {
    size_t char_in_line{};
    const size_t n{line_number_for_char_index(
        tk.char_index(), source_str_.c_str(), char_in_line)};
    return to_string(n) + "_" + to_string(char_in_line);
  }

  inline auto source_location_hr(const token &tk) -> string {
    size_t char_in_line{};
    const size_t n{line_number_for_char_index(
        tk.char_index(), source_str_.c_str(), char_in_line)};
    return to_string(n) + ":" + to_string(char_in_line);
  }

  inline static auto line_number_for_char_index(const size_t char_index,
                                                const char *ptr,
                                                size_t &char_in_line)
      -> size_t {
    size_t ix{0};
    size_t lix{0};
    size_t lineno{1};
    while (true) {
      if (ix == char_index) {
        break;
      }
      if (*ptr++ == '\n') {
        lineno++;
        ix++;
        lix = ix;
        continue;
      }
      ix++;
    }
    char_in_line = ix - lix + 1;
    return lineno;
  }

  inline void finish(ostream &os) {
    os << "\n; max scratch registers in use: " << max_usage_scratch_regs_
       << endl;
    os << ";            max frames in use: " << max_frame_count_ << endl;
    //		os<<";          max stack in use: "<<tc.max_stack_usage_<<endl;
    assert(all_registers_.size() == 16);
    assert(allocated_registers_.empty());
    assert(allocated_registers_loc_.empty());
    assert(call_metas_.empty());
    assert(frames_.empty());
    //		assert(initiated_registers_.empty());
    initiated_registers_.clear();
    assert(named_registers_.size() == 6);
    assert(scratch_registers_.size() == 8);
    max_frame_count_ = 0;
    max_usage_scratch_regs_ = 0;
  }

  inline auto resolve_identifier(const statement &st,
                                 const bool must_be_initiated) const
      -> ident_resolved {
    const ident_resolved &ir{
        resolve_ident_or_empty(st, st.identifier(), must_be_initiated)};
    if (not ir.id_nasm.empty()) {
      return ir;
    }

    throw compiler_exception(st.tok(), "cannot resolve identifier '" +
                                           st.identifier() + "'");
  }

  inline auto resolve_identifier(const statement &st, const string &ident,
                                 const bool must_be_initiated) const
      -> ident_resolved {
    const ident_resolved &ir{
        resolve_ident_or_empty(st, ident, must_be_initiated)};
    if (not ir.id_nasm.empty()) {
      return ir;
    }

    throw compiler_exception(st.tok(),
                             "cannot resolve identifier '" + ident + "'");
  }

  inline void add_alias(const string &ident, const string &parent_frame_ident) {
    frames_.back().add_alias(ident, parent_frame_ident);
  }

  inline void enter_func(const string &name, const string &call_loc,
                         const string &ret_jmp, const bool is_inline,
                         const string &ret_var) {
    frames_.emplace_back(name, frame::frame_type::FUNC, call_loc, ret_jmp,
                         is_inline, ret_var);
    check_usage();
  }

  inline void enter_block() {
    frames_.emplace_back("", frame::frame_type::BLOCK);
    check_usage();
  }

  inline void enter_loop(const string &name) {
    frames_.emplace_back(name, frame::frame_type::LOOP);
    check_usage();
  }

  inline void exit_func(const string &name) {
    const frame &f{frames_.back()};
    assert(f.is_func() and f.is_name(name));
    frames_.pop_back();
  }

  inline void exit_loop(const string &name) {
    const frame &f{frames_.back()};
    assert(f.is_loop() and f.is_name(name));
    frames_.pop_back();
  }

  inline void exit_block() {
    const frame &f{frames_.back()};
    assert(f.is_block());
    frames_.pop_back();
  }

  inline void add_var(const statement &st, ostream &os, size_t indnt,
                      const string &name, const type &var_type,
                      const bool initiated) {
    // check if variable already declared in this scope
    if (frames_.back().has_var(name)) {
      const var_meta &var = frames_.back().get_var_const_ref(name);
      throw compiler_exception(st.tok(),
                               "variable '" + name + "' already declared at " +
                                   source_location_hr(var.declared_at));
    }
    // check if variable shadows previously declared variable
    const pair<string, frame &> idfrm{get_id_and_frame_for_identifier(name)};
    const string &id = idfrm.first;
    const frame &frm = idfrm.second;
    if (frm.has_var(id)) {
      const var_meta &var{frm.get_var_const_ref(id)};
      throw compiler_exception(
          st.tok(), "variable '" + name + "' shadows variable declared at " +
                        source_location_hr(var.declared_at));
    }

    const int stack_idx{int(get_current_stack_size() + var_type.size())};
    frames_.back().add_var(st.tok(), name, var_type, -stack_idx, initiated);
    // comment the resolved name
    const ident_resolved &ir{resolve_identifier(st, name, false)};
    indent(os, indnt, true);
    os << name << ": " << ir.id_nasm << endl;
  }

  inline void add_func_arg(const statement &st, ostream &os, size_t indnt,
                           const string &name, const type &arg_type,
                           const int stack_idx) {
    assert(frames_.back().is_func() && not frames_.back().func_is_inline());
    frames_.back().add_var(st.tok(), name, arg_type, stack_idx, true);
    // comment the resolved name
    const ident_resolved &ir{resolve_identifier(st, name, false)};
    indent(os, indnt, true);
    os << name << ": " << ir.id_nasm << endl;
  }

  inline auto alloc_scratch_register(const statement &st, ostream &os,
                                     const size_t indnt) -> const string & {
    if (scratch_registers_.empty()) {
      throw compiler_exception(
          st.tok(),
          "out of scratch registers. try to reduce expression complexity");
    }

    const string &r{scratch_registers_.back()};
    scratch_registers_.pop_back();

    indent(os, indnt, true);
    os << "alloc " << r << endl;

    const size_t n{8 - scratch_registers_.size()};
    if (n > max_usage_scratch_regs_) {
      // stmt_assign_var tries 2 different methods making this metric inaccurate
      // if a discarded method uses more registers than the selected method
      max_usage_scratch_regs_ = n;
    }

    allocated_registers_.push_back(r);
    allocated_registers_loc_.push_back(source_location_hr(st.tok()));
    return r;
  }

  inline void alloc_named_register_or_throw(const statement &st, ostream &os,
                                            const size_t indnt,
                                            const string &reg) {
    indent(os, indnt, true);
    os << "alloc " << reg << endl;
    auto r{find(named_registers_.begin(), named_registers_.end(), reg)};
    if (r == named_registers_.end()) {
      const size_t n{allocated_registers_.size()};
      string loc;
      for (size_t i{0}; i < n; i++) {
        if (allocated_registers_[i] == reg) {
          loc = allocated_registers_loc_[i];
        }
      }
      throw compiler_exception(st.tok(), "cannot allocate register '" + reg +
                                             "' because it was allocated at " +
                                             loc);
    }
    named_registers_.erase(r);
    allocated_registers_.push_back(reg);
    allocated_registers_loc_.push_back(source_location_hr(st.tok()));
  }

  inline auto alloc_named_register(const statement &st, ostream &os,
                                   const size_t indnt, const string &reg)
      -> bool {
    indent(os, indnt, true);
    os << "alloc " << reg;
    auto r{find(named_registers_.begin(), named_registers_.end(), reg)};
    if (r == named_registers_.end()) {
      os << ": false" << endl;
      return false;
    }
    named_registers_.erase(r);
    allocated_registers_.push_back(reg);
    allocated_registers_loc_.push_back(source_location_hr(st.tok()));
    os << endl;
    return true;
  }

  inline void free_named_register(ostream &os, const size_t indnt,
                                  const string &reg) {
    indent(os, indnt, true);
    os << "free " << reg << endl;
    assert(allocated_registers_.back() == reg);
    allocated_registers_.pop_back();
    allocated_registers_loc_.pop_back();
    named_registers_.push_back(reg);
    initiated_registers_.erase(reg);
  }

  inline void free_scratch_register(ostream &os, const size_t indnt,
                                    const string &reg) {
    indent(os, indnt, true);
    os << "free " << reg << endl;
    assert(allocated_registers_.back() == reg);
    allocated_registers_.pop_back();
    allocated_registers_loc_.pop_back();
    scratch_registers_.push_back(reg);
    initiated_registers_.erase(reg);
  }

  inline auto get_loop_label_or_throw(const statement &st) const
      -> const string & {
    size_t i{frames_.size()};
    while (i--) {
      if (frames_[i].is_loop()) {
        return frames_[i].name();
      }
      if (frames_[i].is_func()) {
        throw compiler_exception(st.tok(), "not in a loop");
      }
    }
    throw compiler_exception(st.tok(), "unexpected frames");
  }

  inline auto get_inline_call_path(const token &tk) const -> const string & {
    size_t i{frames_.size()};
    while (i--) {
      if (frames_[i].is_func()) {
        return frames_[i].inline_call_path();
      }
    }
    throw compiler_exception(tk, "not in a function");
  }

  inline auto get_func_return_label_or_throw(const statement &st) const
      -> const string & {
    size_t i{frames_.size()};
    while (i--) {
      if (frames_[i].is_func()) {
        return frames_[i].func_ret_label();
      }
    }
    throw compiler_exception(st.tok(), "not in a function");
  }

  inline auto get_func_return_var_name_or_throw(const statement &st) const
      -> const string & {
    size_t i{frames_.size()};
    while (i--) {
      if (frames_[i].is_func()) {
        return frames_[i].func_ret_var();
      }
    }
    throw compiler_exception(st.tok(), "not in a function");
  }

  inline void source_comment(const statement &st, ostream &os,
                             const size_t indnt) const {
    size_t char_in_line{};
    const size_t n{line_number_for_char_index(
        st.tok().char_index(), source_str_.c_str(), char_in_line)};

    indent(os, indnt, true);
    os << "[" << to_string(n) << ":" << char_in_line << "]";

    stringstream ss;
    ss << " ";
    st.source_to(ss);
    const string &s{ss.str()};
    const string &res{regex_replace(s, regex("\\s+"), " ")};
    os << res << endl;
  }

  inline void source_comment(ostream &os, const string &dst, const string &op,
                             const statement &st) const {
    size_t char_in_line{};
    const size_t n{line_number_for_char_index(
        st.tok().char_index(), source_str_.c_str(), char_in_line)};
    os << "[" << to_string(n) << ":" << char_in_line << "]";

    stringstream ss;
    ss << " " << dst << op;
    st.source_to(ss);
    const string &s{ss.str()};
    const string &res{regex_replace(s, regex("\\s+"), " ")};
    os << res << endl;
  }

  inline void token_comment(ostream &os, const token &tk) const {
    size_t char_in_line{};
    const size_t n{line_number_for_char_index(
        tk.char_index(), source_str_.c_str(), char_in_line)};
    os << "[" << to_string(n) << ":" << char_in_line << "]";
    os << " " << tk.name() << endl;
  }

  inline auto is_identifier_register(const string &id) const -> bool {
    return find(all_registers_.begin(), all_registers_.end(), id) !=
           all_registers_.end();
  }

  inline void enter_call(const statement &st, ostream &os, const size_t indnt) {
    const bool root_call{call_metas_.empty()};
    const size_t nbytes_of_vars_on_stack{root_call ? get_current_stack_size()
                                                   : 0};
    if (root_call) {
      // this call is not nested within another call's arguments
      if (nbytes_of_vars_on_stack) {
        // adjust stack past the allocated vars
        asm_cmd(st, os, indnt, "sub", "rsp",
                to_string(nbytes_of_vars_on_stack));
        // stack: <base>,.. vars ..,
      }
    }
    // index in the allocated registers that have been allocated but not pushed
    // prior to this call (that might clobber them)
    const size_t alloc_regs_idx{root_call ? 0
                                          : call_metas_.back().alloc_reg_idx};

    // push registers allocated prior to this call
    const size_t n{allocated_registers_.size()};
    size_t nregs_pushed_on_stack{0};
    for (size_t i{alloc_regs_idx}; i < n; i++) {
      const string &reg{allocated_registers_[i]};
      if (not is_register_initiated(reg)) {
        continue;
      }
      // push only registers that contain a valid value
      asm_push(st, os, indnt, reg);
      nregs_pushed_on_stack++;
    }
    call_metas_.push_back(call_meta{nregs_pushed_on_stack,
                                    allocated_registers_.size(),
                                    nbytes_of_vars_on_stack});
  }

  inline void exit_call(const statement &st, ostream &os, const size_t indnt,
                        const size_t nbytes_of_args_on_stack,
                        const vector<string> &allocated_args_registers) {
    const size_t nregs_pushed{call_metas_.back().nregs_pushed};
    const size_t nbytes_of_vars{call_metas_.back().nbytes_of_vars};
    call_metas_.pop_back();
    const bool restore_rsp_to_base{call_metas_.empty()};
    const size_t alloc_reg_idx{
        restore_rsp_to_base ? 0 : call_metas_.back().alloc_reg_idx};

    // optimization can be done if no registers need to be popped
    //   rsp is adjusted once
    if (nregs_pushed == 0) {
      // stack is: <base>,vars,args,
      if (restore_rsp_to_base) {
        const string &offset{
            to_string(nbytes_of_args_on_stack + nbytes_of_vars)};
        asm_cmd(st, os, indnt, "add", "rsp", offset);
        // stack is: <base>,
      } else {
        const string &offset{to_string(nbytes_of_args_on_stack)};
        asm_cmd(st, os, indnt, "add", "rsp", offset);
        // stack is: <base>,vars,
      }
      // free named registers
      size_t i{allocated_registers_.size()};
      while (true) {
        if (i == alloc_reg_idx) {
          break;
        }
        i--;
        const string &reg{allocated_registers_[i]};
        // don't pop registers used to pass arguments
        if (find(allocated_args_registers.begin(),
                 allocated_args_registers.end(),
                 reg) != allocated_args_registers.end()) {
          free_named_register(os, indnt, reg);
        }
      }
    } else {
      // stack is: <base>,vars,regs,args,
      if (nbytes_of_args_on_stack) {
        const string &offset{to_string(nbytes_of_args_on_stack)};
        asm_cmd(st, os, indnt, "add", "rsp", offset);
      }
      // stack is: <base>,vars,regs,

      // pop registers pushed prior to this call
      size_t i{allocated_registers_.size()};
      while (true) {
        if (i == alloc_reg_idx) {
          break;
        }
        i--;
        const string &reg{allocated_registers_[i]};
        // don't pop registers used to pass arguments
        if (find(allocated_args_registers.begin(),
                 allocated_args_registers.end(),
                 reg) == allocated_args_registers.end()) {
          if (is_register_initiated(reg)) {
            // pop only registers that were pushed
            asm_pop(st, os, indnt, reg);
          }
        } else {
          free_named_register(os, indnt, reg);
        }
      }

      // stack is: <base>,vars,
      if (restore_rsp_to_base) {
        // this was not a call within the arguments of another call
        // stack is: <base>,vars,
        if (nbytes_of_vars) {
          const string &offset{to_string(nbytes_of_vars)};
          asm_cmd(st, os, indnt, "add", "rsp", offset);
        }
        // stack is: <base>,
      }
    }
  }

  inline void asm_cmd(const statement &st, ostream &os, const size_t indnt,
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

    const size_t dst_size{get_size_from_operand(st, dst_resolved)};
    const size_t src_size{get_size_from_operand(st, src_resolved)};

    if (dst_size == src_size) {
      if (not(is_operand_memory(dst_resolved) and
              is_operand_memory(src_resolved))) {
        indent(os, indnt);
        os << op << " " << dst_resolved << ", " << src_resolved << endl;
        return;
      }

      const string &r{alloc_scratch_register(st, os, indnt)};
      const string &r_sized{get_register_operand_for_size(st, r, src_size)};
      indent(os, indnt);
      os << "mov " << r_sized << ", " << src_resolved << endl;
      indent(os, indnt);
      os << op << " " << dst_resolved << ", " << r_sized << endl;
      free_scratch_register(os, indnt, r);
      return;
    }

    if (dst_size > src_size) {
      // mov rax,byte[b] -> movsx
      if (not(is_operand_memory(dst_resolved) and
              is_operand_memory(src_resolved))) {
        if (op == "mov") {
          indent(os, indnt);
          os << "movsx " << dst_resolved << ", " << src_resolved << endl;
          return;
        }
      }
      const string &r{alloc_scratch_register(st, os, indnt)};
      const string &r_sized{get_register_operand_for_size(st, r, dst_size)};
      indent(os, indnt);
      os << "movsx " << r_sized << ", " << src_resolved << endl;
      indent(os, indnt);
      os << op << " " << dst_resolved << ", " << r_sized << endl;
      free_scratch_register(os, indnt, r);
      return;
    }

    // dst_size < src_size
    if (is_identifier_register(src_resolved)) {
      const string &r_sized{
          get_register_operand_for_size(st, src_resolved, dst_size)};
      indent(os, indnt);
      os << op << " " << dst_resolved << ", " << r_sized << endl;
      return;
    }

    if (is_operand_memory(src_resolved)) {
      throw compiler_exception(st.tok(), "cannot move '" + src_resolved +
                                             "' to '" + dst_resolved +
                                             "' because it would be truncated");
    }

    // constant
    indent(os, indnt);
    os << op << " " << dst_resolved << ", " << src_resolved << endl;
  }

  inline static auto is_operand_memory(const string &operand) -> bool {
    return operand.find_first_of('[') != string::npos;
  }
  inline auto get_size_from_operand(const statement &st,
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
      return get_size_from_operand_register(st, operand);
    }

    // constant
    return get_type_default().size();
    //		throw compiler_error(st,"size of operand '"+operand+"' could not
    // be deduced");
  }

  inline static auto get_size_from_operand_register(const statement &st,
                                                    const string &operand)
      -> size_t {
    if (operand == "rax") {
      return 8;
    }
    if (operand == "rbx") {
      return 8;
    }
    if (operand == "rcx") {
      return 8;
    }
    if (operand == "rdx") {
      return 8;
    }
    if (operand == "rbp") {
      return 8;
    }
    if (operand == "rsi") {
      return 8;
    }
    if (operand == "rdi") {
      return 8;
    }
    if (operand == "rsp") {
      return 8;
    }
    if (operand == "r8") {
      return 8;
    }
    if (operand == "r9") {
      return 8;
    }
    if (operand == "r10") {
      return 8;
    }
    if (operand == "r11") {
      return 8;
    }
    if (operand == "r12") {
      return 8;
    }
    if (operand == "r13") {
      return 8;
    }
    if (operand == "r14") {
      return 8;
    }
    if (operand == "r15") {
      return 8;
    }

    if (operand == "eax") {
      return 4;
    }
    if (operand == "ebx") {
      return 4;
    }
    if (operand == "ecx") {
      return 4;
    }
    if (operand == "edx") {
      return 4;
    }
    if (operand == "ebp") {
      return 4;
    }
    if (operand == "esi") {
      return 4;
    }
    if (operand == "edi") {
      return 4;
    }
    if (operand == "esp") {
      return 4;
    }
    if (operand == "r8d") {
      return 4;
    }
    if (operand == "r9d") {
      return 4;
    }
    if (operand == "r10d") {
      return 4;
    }
    if (operand == "r11d") {
      return 4;
    }
    if (operand == "r12d") {
      return 4;
    }
    if (operand == "r13d") {
      return 4;
    }
    if (operand == "r14d") {
      return 4;
    }
    if (operand == "r15d") {
      return 4;
    }

    if (operand == "ax") {
      return 2;
    }
    if (operand == "bx") {
      return 2;
    }
    if (operand == "cx") {
      return 2;
    }
    if (operand == "dx") {
      return 2;
    }
    if (operand == "bp") {
      return 2;
    }
    if (operand == "si") {
      return 2;
    }
    if (operand == "di") {
      return 2;
    }
    if (operand == "sp") {
      return 2;
    }
    if (operand == "r8w") {
      return 2;
    }
    if (operand == "r9w") {
      return 2;
    }
    if (operand == "r10w") {
      return 2;
    }
    if (operand == "r11w") {
      return 2;
    }
    if (operand == "r12w") {
      return 2;
    }
    if (operand == "r13w") {
      return 2;
    }
    if (operand == "r14w") {
      return 2;
    }
    if (operand == "r15w") {
      return 2;
    }

    if (operand == "al") {
      return 1;
    }
    if (operand == "ah") {
      return 1;
    }
    if (operand == "bl") {
      return 1;
    }
    if (operand == "bh") {
      return 1;
    }
    if (operand == "cl") {
      return 1;
    }
    if (operand == "ch") {
      return 1;
    }
    if (operand == "dl") {
      return 1;
    }
    if (operand == "dh") {
      return 1;
    }
    if (operand == "r8b") {
      return 1;
    }
    if (operand == "r9b") {
      return 1;
    }
    if (operand == "r10b") {
      return 1;
    }
    if (operand == "r11b") {
      return 1;
    }
    if (operand == "r12b") {
      return 1;
    }
    if (operand == "r13b") {
      return 1;
    }
    if (operand == "r14b") {
      return 1;
    }
    if (operand == "r15b") {
      return 1;
    }

    throw compiler_exception(st.tok(), "unknown register '" + operand + "'");
  }
  inline static auto get_register_operand_for_size(const statement &st,
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
        throw compiler_exception(st.tok(), "illegal size " + to_string(size) +
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
        throw compiler_exception(st.tok(), "illegal size " + to_string(size) +
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
        throw compiler_exception(st.tok(), "illegal size " + to_string(size) +
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
        throw compiler_exception(st.tok(), "illegal size " + to_string(size) +
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
        throw compiler_exception(st.tok(), "illegal size " + to_string(size) +
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
        throw compiler_exception(st.tok(), "illegal size " + to_string(size) +
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
        throw compiler_exception(st.tok(), "illegal size " + to_string(size) +
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
        throw compiler_exception(st.tok(), "illegal size " + to_string(size) +
                                               " for register '" + operand +
                                               "'");
      }
    }
    const regex rx{R"(r(\d+))"};
    smatch match;
    if (!regex_search(operand, match, rx)) {
      throw compiler_exception(st.tok(), "unknown register " + operand);
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
      throw compiler_exception(st.tok(), "illegal size " + to_string(size) +
                                             " for register '" + operand + "'");
    }

    throw compiler_exception(st.tok(), "unknown register '" + operand + "'");
  }
  inline static void asm_push([[maybe_unused]] const statement &st, ostream &os,
                              const size_t indnt, const string &operand) {
    indent(os, indnt);
    os << "push " << operand << endl;
  }

  inline static void asm_pop([[maybe_unused]] const statement &st, ostream &os,
                             const size_t indnt, const string &operand) {
    indent(os, indnt);
    os << "pop " << operand << endl;
  }

  inline static void asm_ret([[maybe_unused]] const statement &st, ostream &os,
                             const size_t indnt) {
    indent(os, indnt);
    os << "ret\n";
  }

  inline static void asm_jmp([[maybe_unused]] const statement &st, ostream &os,
                             const size_t indnt, const string &label) {
    indent(os, indnt);
    os << "jmp " << label << endl;
  }

  inline static void asm_label([[maybe_unused]] const statement &st,
                               ostream &os, const size_t indnt,
                               const string &label) {
    indent(os, indnt);
    os << label << ":" << endl;
  }

  inline static void asm_call([[maybe_unused]] const statement &st, ostream &os,
                              const size_t indnt, const string &label) {
    indent(os, indnt);
    os << "call " << label << endl;
  }

  inline static void asm_neg([[maybe_unused]] const statement &st, ostream &os,
                             const size_t indnt, const string &dst_resolved) {
    indent(os, indnt);
    os << "neg " << dst_resolved << endl;
  }

  inline void set_var_is_initiated(const string &name) {
    const baz_ident bid{name};
    pair<string, frame &> idfrm{get_id_and_frame_for_identifier(bid.id_base())};
    const string &id = idfrm.first;
    frame &frm = idfrm.second;
    // is 'id_nasm' a variable?
    if (frm.has_var(id)) {
      frm.get_var_ref(id).initiated = true;
      return;
    }

    if (fields_.has(id)) {
      return; // a field, it is initiated
    }

    if (is_identifier_register(id)) {
      //? this might not be necessary since it is updated at
      // asm_cmd(...,"mov",...)
      initiated_registers_.insert(id);
      return;
    }

    throw panic_exception("should not be reached: " + string{__FILE__} + ":" +
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
      if (frames_[i].is_func()) {
        // is it an alias defined by an argument in the function?
        if (not frames_[i].has_alias(id)) {
          break;
        }
        // yes, continue resolving alias until it is
        // a variable, field, register or constant
        id = frames_[i].get_alias(id);
        continue;
      }
      // does scope contain the variable
      if (frames_[i].has_var(id)) {
        break;
      }
    }
    return {move(id), frames_[i]};
  }

  inline auto is_register_initiated(const string &reg) const -> bool {
    return initiated_registers_.contains(reg);
  }

  inline auto get_current_stack_size() const -> size_t {
    assert(!frames_.empty());
    size_t n{0};
    size_t i{frames_.size()};
    while (i--) {
      const frame &f{frames_[i]};
      n += f.allocated_stack_size();
      if (f.is_func() && not f.func_is_inline()) {
        break;
      }
    }
    return n;
  }

  inline auto resolve_ident_or_empty(const statement &st, const string &ident,
                                     const bool must_be_initiated) const
      -> ident_resolved {
    const baz_ident bid{ident};
    string id{bid.id_base()};
    // traverse the frames and resolve the id_nasm (which might be an alias) to
    // a variable, field, register or constant
    size_t i{frames_.size()};
    while (i--) {
      // is the frame a function?
      if (frames_[i].is_func()) {
        // is it an alias defined by an argument in the function?
        if (not frames_[i].has_alias(id)) {
          break;
        }
        // yes, continue resolving alias until it is
        // a variable, field, register or constant
        id = frames_[i].get_alias(id);
        if (i == 0) {
          break;
        }
        continue;
      }
      // does scope contain the variable
      if (frames_[i].has_var(id)) {
        break;
      }
      if (i == 0) {
        break;
      }
    }

    // is 'id_nasm' a variable?
    if (frames_[i].has_var(id)) {
      const var_meta &var = frames_[i].get_var_const_ref(id);
      if (must_be_initiated and not var.initiated) {
        throw compiler_exception(st.tok(), "variable '" + var.name +
                                               "' is not initiated");
      }
      const string &acc = var.tp.accessor(st.tok(), bid.path(), var.stack_idx);
      return {ident, acc, 0, var.tp, ident_resolved::ident_type::VAR};
    }

    // is 'id_nasm' a register?
    if (is_identifier_register(id)) {
      if (must_be_initiated and not is_register_initiated(id)) {
        throw compiler_exception(st.tok(),
                                 "register '" + id + "' is not initiated");
      }

      //? unary ops?
      return {ident, id, 0, get_type_default(),
              ident_resolved::ident_type::REGISTER};
    }

    // is 'id_nasm' a field?

    if (fields_.has(id)) {
      const string &after_dot =
          bid.path().size() < 2 ? "" : bid.path()[1]; // ! not correct
      if (after_dot == "len") {
        return {ident, id + "." + after_dot, 0, get_type_default(),
                ident_resolved::ident_type::IMPLIED};
      }
      const field_meta &fm = fields_.get_const_ref(id);
      if (fm.is_str) {
        return {ident, id, 0, get_type_default(),
                ident_resolved::ident_type::FIELD};
      }
      //? assumes qword
      return {ident, "qword[" + id + "]", 0, get_type_default(),
              ident_resolved::ident_type::FIELD};
    }

    char *ep{};
    const long int const_value{strtol(id.c_str(), &ep, 10)};
    if (!*ep) {
      return {ident, id, const_value, get_type_default(),
              ident_resolved::ident_type::CONST};
    }

    if (id.starts_with("0x")) { // hex
      const long int value{strtol(id.c_str() + 2, &ep, 16)};
      if (!*ep) {
        return {ident, id, value, get_type_default(),
                ident_resolved::ident_type::CONST};
      }
    }

    if (id.starts_with("0b")) { // binary
      const long int value{strtol(id.c_str() + 2, &ep, 2)};
      if (!*ep) {
        return {ident, id, value, get_type_default(),
                ident_resolved::ident_type::CONST};
      }
    }

    if (funcs_.has(id)) {
      const func_meta &func{funcs_.get_const_ref(id)};
      //? type is func
      return {ident, id, 0, func.tp, ident_resolved::ident_type::CONST};
    }

    if (id == "true") {
      return {ident, id, 1, get_type_bool(), ident_resolved::ident_type::CONST};
    }

    if (id == "false") {
      return {ident, id, 0, get_type_bool(), ident_resolved::ident_type::CONST};
    }

    // not resolved, return empty answer
    return {"", "", 0, get_type_void(), ident_resolved::ident_type::CONST};
  }

  inline void check_usage() {
    if (frames_.size() > max_frame_count_) {
      max_frame_count_ = frames_.size();
    }
  }

  vector<frame> frames_{};
  vector<string> all_registers_{};
  vector<string> scratch_registers_{};
  vector<string> allocated_registers_{};
  vector<string>
      allocated_registers_loc_{}; // source locations of allocated_registers_
  vector<string> named_registers_{};
  size_t max_usage_scratch_regs_{0};
  size_t max_frame_count_{0};
  const string &source_str_{};
  lut<field_meta> fields_{};
  lut<func_meta> funcs_{};
  lut<const type &> types_{};
  vector<call_meta> call_metas_{};
  unordered_set<string> initiated_registers_{};
  const type *type_void_{};
  const type *type_default_{};
  const type *type_bool_{};
};
