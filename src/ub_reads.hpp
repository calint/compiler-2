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
// place of which only the indexes are read is 'index_only_place': the node
// writes it, e.g. the buffer of 'read' or the destination of 'array_copy',
// or needs only its length, e.g. the array of 'array_length'
struct node_operand {
    const statement* node{};
    // null when the operand is read
    const stmt_identifier* index_only_place{};

    //
    // statics
    //

    // an operand that is read, e.g. an operand of an arithmetic expression
    [[nodiscard]] static auto read(const statement& node) -> node_operand {
        return {
            .node{&node},
            .index_only_place{},
        };
    }
};

// receives each operand of a node in evaluation order
using operand_visitor = std::function_ref<void(const node_operand& operand)>;

// receives each read of a variable, the path that reads it: its root token,
// where an error points, and the storage it reaches, see
// 'stmt_identifier::may_reach' and 'assert_set_before_read'
using read_visitor = std::function_ref<void(const stmt_identifier& read)>;

// what the body of a called function does to storage its caller names too: a
// global or a part of an argument it reads or writes, e.g. the 'gx' that
// 'gw()' writes or the 'i' of 'h(i)'; the storage is resolved, a caller's
// local of the same name does not shadow it
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
