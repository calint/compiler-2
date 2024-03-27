#pragma once

template <class T> class lut final {
  struct el {
    string key;
    T data;

    [[nodiscard]] inline auto is_key(const string &k) const -> bool {
      return k == key;
    }
  };

  vector<el> elems_{};
  mutable const el *last_has_el_{};

public:
  inline auto get(const string &key) const -> T {
    if (last_has_el_) {
      if (last_has_el_->is_key(key)) {
        return last_has_el_->data;
      }
    }
    for (const el &e : elems_) {
      if (e.is_key(key)) {
        return e.data;
      }
    }
    throw panic_exception("element not found: " + key);
  }

  inline auto has(const string &key) const -> bool {
    for (const el &e : elems_) {
      if (e.is_key(key)) {
        last_has_el_ = &e;
        return true;
      }
    }
    last_has_el_ = nullptr;
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
    throw panic_exception("element not found: " + key);
  }

  // for clarity get_const_ref instead of overloading get_ref
  inline auto get_const_ref(const string &key) const -> const T & {
    if (last_has_el_) {
      if (last_has_el_->is_key(key)) {
        return last_has_el_->data;
      }
    }
    for (const el &e : elems_) {
      if (e.is_key(key)) {
        return e.data;
      }
    }
    throw panic_exception("element not found: " + key);
  }
};
