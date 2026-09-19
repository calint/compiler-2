#pragma once
// reviewed: 2025-09-28

#include <algorithm>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "panic_exception.hpp"

template <class T> class lut final {
    struct elem {
        std::string key;
        T data;
    };

    std::vector<elem> elems_;

  public:
    [[nodiscard]] auto has(const std::string_view key) const -> bool {
        return std::ranges::contains(elems_, key, &elem::key);
    }

    auto put(std::string key, T data) -> void {
        elems_.emplace_back(std::move(key), std::move(data));
    }

    // note: for clarity, get_const_ref instead of overloading get_ref
    [[nodiscard]] auto get_const_ref(const std::string_view key) const
        -> const T& {

        for (const elem& entry : elems_) {
            if (entry.key == key) {
                return entry.data;
            }
        }
        throw panic_exception(std::format("element not found: {}", key));
    }
};
