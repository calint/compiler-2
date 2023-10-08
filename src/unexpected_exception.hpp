class unexpected_exception final : public std::exception {
  const string msg{};

public:
  inline explicit unexpected_exception(string message)
      : msg{std::move(message)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override { return msg.c_str(); }
};
