#pragma once

#include <functional>
#include <string_view>

#include "access_path.hpp"
#include "token.hpp"

// the types of the walks shared by the checks, see the class comment of
// 'statement'

class statement;
class stmt_identifier;

// an operand of a node in evaluation order, e.g. an argument of a call; a
// place of which only the indexes are read is 'written_place': the node
// writes it, e.g. the buffer of 'read' or the destination of 'array_copy',
// or needs only its length, e.g. the array of 'array_length'
struct list_operand {
    const statement* node{};
    // null when the operand is read
    const stmt_identifier* written_place{};
};

// receives each operand of a node in evaluation order
using operand_visitor = std::function_ref<void(const list_operand& operand)>;

// receives each read of a variable, the path that reads it: its root token,
// where an error points, the bytes of the variable that it reaches, the whole
// array after a run-time index, and its path
using read_visitor = std::function_ref<void(const stmt_identifier& read)>;

// what the body of a called function does to storage its caller names too: a
// global it reads or writes, or an argument it writes through a 'mut'
// parameter, e.g. the 'gx' that 'gw()' writes or the 'i' of 'h(i)'; the
// storage is resolved, a caller's local of the same name does not shadow it
struct call_effect {
    // the call, where an error points
    token call_tk;
    // e.g. 'gx', the global or the argument as written
    std::string_view name;
    storage_path storage{};
    bool is_write{};
};

// receives each effect of a call, the calls in its arguments first
using call_effect_visitor = std::function_ref<void(const call_effect& effect)>;
