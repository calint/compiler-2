#pragma once

class type;
struct type_field final {
  const string name;
  const type &tp;
  const size_t offset{};
};

class type final {
  string name_{};
  size_t size_{};
  vector<type_field> fields_{};
  bool is_built_in_{};

public:
  inline type(string name, const size_t size, const bool is_built_in)
      : name_{move(name)}, size_{size}, is_built_in_{is_built_in} {}

  inline type() = default;
  inline type(const type &) = default;
  inline type(type &&) = default;
  inline auto operator=(const type &) -> type & = default;
  inline auto operator=(type &&) -> type & = default;

  inline ~type() = default;

  inline void add_field([[maybe_unused]] const token &tk, string name,
                        const type &tp) {
    fields_.emplace_back(type_field{move(name), tp, size_});
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
                                     const int stack_idx_base) const
      -> pair<const type &, string> {

    const type *tp{this};
    size_t offset{0};
    for (size_t path_idx{1}; path_idx != path.size(); path_idx++) {
      const type_field &fld{tp->field(tk, path[path_idx])};
      offset += fld.offset;
      tp = &fld.tp;
    }
    const int stack_idx{stack_idx_base + int(offset)};

    // find first built-in type to have a valid operand size for the address
    const type *tp_first_field{tp};
    while (not tp_first_field->is_built_in()) {
      tp_first_field = &tp_first_field->fields_.at(0).tp;
    }

    const string &memsize{
        get_memory_operand_for_size(tk, tp_first_field->size())};

    return {*tp, memsize + "[rbp" + string{stack_idx > 0 ? "+" : ""} +
                     to_string(stack_idx) + "]"};
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

  [[nodiscard]] inline auto fields() const -> const vector<type_field> & {
    return fields_;
  }
};
