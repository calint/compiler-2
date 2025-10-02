#pragma once

#include "token.hpp"

class expr_any;

struct identifier_elem final {
    token name;
    expr_any* array_index_expr; //? TODO unique_ptr give ciruclar reference
    token ws1;                  // whitespace
    bool has_array_index_expr{};
};

using identifier_path = std::vector<identifier_elem>;
