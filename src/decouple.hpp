#pragma once
#include <memory>
class tokenizer;
class statement;
inline unique_ptr<statement>read_next_statement(statement*parent,tokenizer&t);
