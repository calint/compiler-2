#pragma once

#include "token.hpp"

class expr_any;

struct identifier_elem final {
    token name_tk;
    expr_any* array_index_expr;
    token ws1;
    bool has_array_index_expr{};
};

class identifier_path final {
  public:
    std::vector<identifier_elem> elems;
};
