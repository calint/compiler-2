class unexpected_exception final : public std::exception {
  const string msg{};

public:
  inline explicit unexpected_exception(string message)
      : msg{std::move(message)} {}

  const char *what() const noexcept override { return msg.c_str(); }
};
