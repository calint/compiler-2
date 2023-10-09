#pragma once

class panic_exception final : public exception {
  const string msg_{};

public:
  inline explicit panic_exception(const string& msg)
      : msg_{msg} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return msg_.c_str();
  }
};
