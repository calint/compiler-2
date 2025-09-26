#pragma once

class panic_exception final : public exception {
  const string msg_{};

public:
  inline explicit panic_exception(string msg) : msg_{move(msg)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return msg_.c_str();
  }
};
