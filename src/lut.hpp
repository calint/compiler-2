#pragma once
// reviewed: 2025-09-28

#include <string>
#include <utility>
#include <vector>

#include "panic_exception.hpp"

template <class T> class lut final {
    struct elem final {
        std::string key;
        T data;

        [[nodiscard]] auto is_key(const std::string& k) const -> bool {
            return k == key;
        }
    };

    std::vector<elem> elems_;
    mutable const elem* last_has_elem_{};

  public:
    auto get(const std::string& key) const -> T {
        if (last_has_elem_) {
            if (last_has_elem_->is_key(key)) {
                return last_has_elem_->data;
            }
        }
        for (const elem& e : elems_) {
            if (e.is_key(key)) {
                return e.data;
            }
        }
        throw panic_exception("element not found: " + key);
    }

    auto has(const std::string& key) const -> bool {
        for (const elem& e : elems_) {
            if (e.is_key(key)) {
                last_has_elem_ = &e;
                return true;
            }
        }
        last_has_elem_ = nullptr;
        return false;
    }

    auto put(std::string key, T data) -> void {
        elems_.emplace_back(elem{std::move(key), std::move(data)});
    }

    auto get_ref(const std::string& key) -> T& {
        for (elem& e : elems_) {
            if (e.is_key(key)) {
                return e.data;
            }
        }
        throw panic_exception("element not found: " + key);
    }

    // note: for clarity get_const_ref instead of overloading get_ref
    auto get_const_ref(const std::string& key) const -> const T& {
        if (last_has_elem_) {
            if (last_has_elem_->is_key(key)) {
                return last_has_elem_->data;
            }
        }
        for (const elem& e : elems_) {
            if (e.is_key(key)) {
                return e.data;
            }
        }
        throw panic_exception("element not found: " + key);
    }
};
