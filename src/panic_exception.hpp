#pragma once

class panic_exception final : public exception {
  const string msg{};

public:
  inline explicit panic_exception(string message)
      : msg{move(message)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return msg.c_str();
  }
};
