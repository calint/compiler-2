#pragma once

#include<memory>
using namespace std;
class tokenizer;
class statement;
class toc;
inline unique_ptr<statement>read_next_statement(toc&tc,statement*parent,tokenizer&t);
