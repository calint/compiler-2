#pragma once

#include <functional>
#include <optional>
#include <string_view>

#include "token.hpp"
#include "type.hpp"
#include "ub_alias.hpp"

// the walk over the reads of a variable shared by the checks: each read hands
// out its bytes for the definite assignment and the constructor order checks
// and its path for the aliasing check

// the variable whose reads 'visit_reads' reports, none for every variable
using read_filter = std::optional<std::string_view>;

// receives each read of a variable: the bytes of the variable that it reaches,
// the whole array after a run-time index, and its path
using read_visitor = std::function_ref<void(
    const token& src_loc_tk, std::string_view read_text,
    const byte_range& accessed_bytes, const access_path& accessed_path)>;
