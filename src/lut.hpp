#pragma once

#include <algorithm>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// a table of values by name in the order they were put, looked up by a linear
// search: the names of a scope are few, and the program-wide tables (functions,
// types) are searched once per use of a name; a name is put once and looked up
// only when it is in the table
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

    // the values in the order they were put
    [[nodiscard]] auto values() const -> auto {
        return elems_ | std::views::transform(&elem::data);
    }
};
