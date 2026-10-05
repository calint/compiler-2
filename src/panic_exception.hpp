#pragma once
// reviewed: 2025-09-28

#include <exception>
#include <string>
#include <utility>

class panic_exception final : public std::exception {
    std::string msg_;

  public:
    explicit panic_exception(std::string msg) : msg_{std::move(msg)} {}

    //
    // overridden methods
    //

    [[nodiscard]] auto what() const noexcept -> const char* override {
        return msg_.c_str();
    }
};
