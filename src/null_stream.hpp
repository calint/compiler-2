#pragma once
// reviewed: 2025-09-28

#include <ostream>

class null_stream final : public std::ostream {
    class null_buffer : public std::streambuf {
      protected:
        auto overflow(int c) -> int override { return c; }
    } nb_{};

  public:
    null_stream() : std::ostream(&nb_) {}
};
