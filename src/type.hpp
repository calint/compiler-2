#pragma once

class type;
struct type_field final {
  const string name;
  const type &tp;
  const size_t offset{};
};

class type final {
public:
  inline type(std::string name, const size_t size, const bool is_built_in)
      : name_{std::move(name)}, size_{size}, is_built_in_{is_built_in} {}

  inline type() = default;
  inline type(const type &) = default;
  inline type(type &&) = default;
  inline auto operator=(const type &) -> type & = default;
  inline auto operator=(type &&) -> type & = default;

  inline ~type() = default;

  inline void add_field([[maybe_unused]] const token &tk, const string &name,
                        const type &tp) {
    fields_.emplace_back(type_field{name, tp, size_});
    size_ += tp.size_;
  }
  [[nodiscard]] inline auto field(const token &tk, const string &name) const
      -> const type_field & {
    for (const type_field &fld : fields_) {
      if (fld.name == name) {
        return fld;
      }
    }
    throw compiler_exception(tk, "field '" + name + "' not found in type '" +
                                 name_ + "'");
  }

  [[nodiscard]] inline auto accessor(const token &tk,
                                     const vector<string> &path,
                                     const int stack_idx_base) const -> string {
    const pair<size_t, size_t> res{field_size_and_offset(tk, path)};
    const size_t offset{res.second};
    const size_t fldsize{res.first};
    const int stack_idx = stack_idx_base + int(offset);

    const string &memsize{get_memory_operand_for_size(tk, fldsize)};

    return memsize + "[rbp" + string{stack_idx > 0 ? "+" : ""} +
           to_string(stack_idx) + "]";
  }

  inline static auto get_memory_operand_for_size(const token &tk,
                                                 const size_t size) -> string {
    switch (size) {
    case 8:
      return "qword";
    case 4:
      return "dword";
    case 2:
      return "word";
    case 1:
      return "byte";
    default:
      throw compiler_exception(tk, "illegal size for memory operand: " +
                                   to_string(size));
    }
  }

  [[nodiscard]] inline auto size() const -> size_t { return size_; }

  inline void set_size(const size_t nbytes) { size_ = nbytes; }

  [[nodiscard]] inline auto name() const -> const string & { return name_; }

  inline void set_name(const string &nm) { name_ = nm; }

  [[nodiscard]] inline auto is_built_in() const -> bool { return is_built_in_; }

  inline void clear_fields() {
    fields_.clear();
    size_ = 0;
  }

private:
  [[nodiscard]] inline auto
  field_size_and_offset(const token &tk, const vector<string> &path) const
      -> pair<size_t, size_t> {
    if (path.size() == 1) {
      // path contains only a reference to the variable
      // find first primitive type
      const type *t{this};
      while (true) {
        if (t->fields_.empty()) {
          return {t->size_, 0};
        }
        t = &t->fields_[0].tp;
      }
    }
    const type *tp{this};
    size_t offset{0};
    size_t path_idx{1};
    while (true) {
      if (path_idx == path.size()) {
        return {tp->fields_.empty() ? size_ : tp->fields_[0].tp.size_, offset};
      }

      const type_field &fld = tp->field(tk, path[path_idx]);
      offset += fld.offset;
      if (fld.tp.is_built_in_) {
        return {fld.tp.size_, offset};
      }
      tp = &fld.tp;
      path_idx++;
    }
  }

  std::string name_{};
  size_t size_{};
  vector<type_field> fields_{};
  bool is_built_in_{};
};
