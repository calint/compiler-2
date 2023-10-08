#pragma once
#include "tokenizer.hpp"
#include "unexpected_exception.hpp"

class toc;

class unary_ops final {
  vector<char> ops_{};

public:
  inline explicit unary_ops(tokenizer &t) {
    while (true) {
      if (t.is_next_char('~')) {
        ops_.push_back('~');
      } else if (t.is_next_char('-')) {
        ops_.push_back('-');
        //			}else if(t.is_next_char('!')){
        //				ops_.push_back('!');
      } else {
        break;
      }
    }
  }

  inline unary_ops() = default;
  inline unary_ops(const unary_ops &) = default;
  inline unary_ops(unary_ops &&) = default;
  inline auto operator=(const unary_ops &) -> unary_ops & = default;
  inline auto operator=(unary_ops &&) -> unary_ops & = default;

  inline ~unary_ops() = default;

  [[nodiscard]] inline auto is_only_negated() const -> bool {
    return ops_.size() == 1 && ops_.back() == '-';
  }

  inline void put_back(tokenizer &t) const {
    size_t i{ops_.size()};
    while (i--) {
      t.put_back_char(ops_[i]);
    }
  }

  inline void source_to(ostream &os) const {
    const size_t n{ops_.size()};
    for (size_t i = 0; i < n; i++) {
      os << ops_[i];
    }
  }

  inline void compile(toc &tc, ostream &os, size_t indent_level,
                      const string &dst_resolved) const;

  [[nodiscard]] inline auto is_empty() const -> bool { return ops_.empty(); }

  [[nodiscard]] inline auto as_string() const -> string {
    return {ops_.begin(), ops_.end()};
  }

  [[nodiscard]] inline auto evaluate_constant(long int v) const -> long int {
    size_t i{ops_.size()};
    while (i--) {
      switch (ops_[i]) {
      case '-':
        v = -v;
        break;
      case '~':
        v = ~v; // NOLINT(hicpp-signed-bitwise)
        break;
        //			case'!':v=v^1;break;
      default:
        throw unexpected_exception("unexpected code path " + string{__FILE__} +
                                   ":" + to_string(__LINE__));
      }
    }
    return v;
  }
};
