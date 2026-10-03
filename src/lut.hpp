#pragma once
// reviewed: 2025-09-28

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

template <class T> class lut final {
    struct elem {
        std::string key;
        T data;
    };

    std::vector<elem> elems_;

  public:
    // note: for clarity, get_const_ref instead of overloading get_ref
    [[nodiscard]] auto get_const_ref(const std::string_view key) const
        -> const T& {

        for (const elem& e : elems_) {
            if (e.key == key) {
                return e.data;
            }
        }

        // callers look up keys they have defined
        std::unreachable();
    }

    [[nodiscard]] auto get_ref(const std::string_view key) -> T& {
        for (elem& e : elems_) {
            if (e.key == key) {
                return e.data;
            }
        }

        // callers look up keys they have defined
        std::unreachable();
    }

    [[nodiscard]] auto has(const std::string_view key) const -> bool {
        return std::ranges::contains(elems_, key, &elem::key);
    }

    auto put(std::string key, T data) -> void {
        elems_.emplace_back(std::move(key), std::move(data));
    }
};
