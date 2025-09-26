#pragma once

#include <exception>
#include <string>

class panic_exception final : public std::exception {
    const std::string msg_;

  public:
    explicit panic_exception(std::string msg) : msg_{std::move(msg)} {}

    [[nodiscard]] auto what() const noexcept -> const char* override {
        return msg_.c_str();
    }
};
