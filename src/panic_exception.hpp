class panic_exception final : public std::exception {
  const string msg{};

public:
  inline explicit panic_exception(string message)
      : msg{std::move(message)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return msg.c_str();
  }
};
