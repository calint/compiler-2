#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// a table of values by name in the order they were put, looked up by a linear
// search, which is fast for the few names of a scope; a name is put once and
// looked up only when it is in the table
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

    [[nodiscard]] auto keys() const -> std::vector<std::string> {
        std::vector<std::string> result;
        result.reserve(elems_.size());

        for (const elem& e : elems_) {
            result.push_back(e.key);
        }

        return result;
    }

    auto put(std::string key, T data) -> void {
        elems_.emplace_back(std::move(key), std::move(data));
    }
};
