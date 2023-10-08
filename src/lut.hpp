#pragma once

template <class T> class lut final {
public:
  inline auto get(const string &key) const -> T {
    if (last_has_el) {
      if (last_has_el->is_key(key)) {
        return last_has_el->data;
      }
    }
    for (const el &e : elems_) {
      if (e.is_key(key)) {
        return e.data;
      }
    }
    throw unexpected_exception("element not found: " + key);
  }

  inline auto has(const string &key) const -> bool {
    for (const el &e : elems_) {
      if (e.is_key(key)) {
        last_has_el = &e;
        return true;
      }
    }
    return false;
  }

  inline void put(const string &key, T data) {
    elems_.emplace_back(el{key, data});
  }

  inline auto get_ref(const string &key) -> T & {
    for (el &e : elems_) {
      if (e.is_key(key)) {
        return e.data;
      }
    }
    throw unexpected_exception("element not found: " + key);
  }

  // for clarity get_const_ref instead of overloading get_ref
  inline auto get_const_ref(const string &key) const -> const T & {
    if (last_has_el) {
      if (last_has_el->is_key(key)) {
        return last_has_el->data;
      }
    }
    for (const el &e : elems_) {
      if (e.is_key(key)) {
        return e.data;
      }
    }
    throw unexpected_exception("element not found: " + key);
  }

private:
  struct el {
    string key;
    T data;
    [[nodiscard]] inline auto is_key(const string &k) const -> bool {
      return k == key;
    }
  };
  vector<el> elems_{};
  mutable const el *last_has_el{};
};
