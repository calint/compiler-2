#pragma once

template <class T> class lut final {
public:
  inline T get(const string &key) const {
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
    throw "element not found: " + key;
  }

  inline bool has(const string &key) const {
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

  inline T &get_ref(const string &key) {
    for (el &e : elems_) {
      if (e.is_key(key)) {
        return e.data;
      }
    }
    throw "element not found: " + key;
  }

  // for clarity get_const_ref instead of overloading get_ref
  inline const T &get_const_ref(const string &key) const {
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
    throw "element not found: " + key;
  }

private:
  struct el {
    string key;
    T data;
    inline bool is_key(const string &k) const { return k == key; }
  };
  vector<el> elems_{};
  mutable const el *last_has_el{};
};
