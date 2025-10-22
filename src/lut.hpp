#pragma once
// reviewed: 2025-09-28

#include <algorithm>
#include <format>
#include <string_view>
#include <utility>
#include <vector>

#include "panic_exception.hpp"

template <class T> class lut final {
    struct elem {
        std::string key;
        T data;

        [[nodiscard]] auto is_key(std::string_view k) const -> bool {
            return k == key;
        }
    };

    std::vector<elem> elems_;

  public:
    [[nodiscard]] auto has(std::string_view key) const -> bool {
        return std::ranges::any_of(
            elems_, [&key](const elem& e) -> bool { return e.is_key(key); });
    }

    auto put(std::string key, T data) -> void {
        elems_.emplace_back(std::move(key), std::move(data));
    }

    // note: for clarity get_const_ref instead of overloading get_ref
    [[nodiscard]] auto get_const_ref(std::string_view key) const -> const T& {
        for (const elem& e : elems_) {
            if (e.is_key(key)) {
                return e.data;
            }
        }
        throw panic_exception(std::format("element not found: {}", key));
    }
};
