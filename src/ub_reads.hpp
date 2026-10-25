#pragma once

#include <functional>
#include <optional>
#include <string_view>

#include "access_path.hpp"
#include "token.hpp"
#include "type.hpp"

// the types of two walks shared by the checks, the checks are in
// 'ub_assigned.hpp', 'ub_order.hpp' and 'stmt_call.hpp':
//   the reads ('statement::visit_reads'): each read of a variable hands out
//   its bytes for the definite assignment check and its path for the
//   aliasing checks
//   the calls ('statement::visit_call_effects'): what the calls of a
//   statement read and write, see 'call_effect'

// the variable whose reads 'visit_reads' reports, none for every variable
using read_filter = std::optional<std::string_view>;

// receives each read of a variable: the token of its root name, where an
// error points, its text as written, the bytes of the variable that it
// reaches, the whole array after a run-time index, and its path
using read_visitor = std::function_ref<void(
    const token& src_loc_tk, std::string_view read_text,
    const byte_range& accessed_bytes, const access_path& accessed_path)>;

// what the body of a called function does to storage its caller names too: a
// global it reads or writes, or an argument it writes through a 'mut'
// parameter, e.g. the 'gx' that 'gw()' writes or the 'i' of 'h(i)'; the
// storage is resolved, a caller's local of the same name does not shadow it
struct call_effect {
    // the call, where an error points
    token call_tk;
    // e.g. 'gx', the global or the argument as written
    std::string_view name;
    storage_path storage;
    bool is_write{};
};

// receives each effect of a call, the calls in its arguments first
using call_effect_visitor = std::function_ref<void(const call_effect& effect)>;
