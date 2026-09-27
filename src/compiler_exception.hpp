#pragma once
// reviewed: 2025-09-28

#include <exception>
#include <string>

#include "token.hpp"

// 'tokenizer' throws this, so it is only declared here
class tokenizer;

class compiler_exception final : public std::exception {
  public:
    std::string msg;
    size_t line{};
    size_t start_index{};
    size_t end_index{};

    compiler_exception(const token& src_loc_tk, std::string message)
        : msg{std::move(message)}, line{src_loc_tk.at_line()},
          start_index{src_loc_tk.start_index()},
          end_index{src_loc_tk.end_index()} {}

    // defined in 'tokenizer.hpp'
    compiler_exception(const tokenizer& tz, std::string message);

    [[nodiscard]] auto what() const noexcept -> const char* override {
        return msg.c_str();
    }
};
