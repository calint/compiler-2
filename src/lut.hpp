#pragma once

#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

// a table of values by name in the order they were put, looked up by a hash
// index: a scope or a program-wide table (functions, types) may hold tens of
// thousands of names; a name is put once and looked up only when it is in the
// table
template <class T> class lut final {
    std::vector<T> values_;
    // the index in 'values_' of each name; a lookup copies the name, which
    // for a short name allocates nothing
    std::unordered_map<std::string, size_t> index_;

  public:
    // note: for clarity, 'get_const_ref' instead of overloading 'get_ref'
    [[nodiscard]] auto get_const_ref(const std::string_view key) const
        -> const T& {

        // callers look up keys they have defined
        assert(has(key));

        return values_.at(index_.at(std::string{key}));
    }

    [[nodiscard]] auto get_ref(const std::string_view key) -> T& {
        // callers look up keys they have defined
        assert(has(key));

        return values_.at(index_.at(std::string{key}));
    }

    [[nodiscard]] auto has(const std::string_view key) const -> bool {
        return index_.contains(std::string{key});
    }

    auto put(std::string key, T data) -> void {
        assert(not has(key));

        index_.emplace(std::move(key), values_.size());
        values_.push_back(std::move(data));
    }

    // the values in the order they were put
    [[nodiscard]] auto values() const -> const std::vector<T>& {
        return values_;
    }
};
