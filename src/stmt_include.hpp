#pragma once

#include <cstddef>
#include <ostream>

#include "decouple.hpp"
#include "statement.hpp"
#include "token.hpp"

// 'include "lib.baz"', the definitions of the file are parsed and compiled
// where the statement is, this statement only keeps the source
class stmt_include final : public statement {
    token path_tk_;

  public:
    stmt_include(const token src_loc_tk, const token path_tk)
        : statement{src_loc_tk}, path_tk_{path_tk} {}

    stmt_include() = default;

    //
    // overridden methods
    //

    auto source_to(std::ostream& os) const -> void override {
        statement::source_to(os);
        path_tk_.source_to(os);
    }

    auto compile([[maybe_unused]] toc& tc, [[maybe_unused]] const size_t indent,
                 [[maybe_unused]] const ident_info& dst_info) const
        -> void override {}
};
