#pragma once
#include<memory>
using namespace std;
class tokenizer;
class token;
class statement;
class expression;
inline unique_ptr<statement>create_call_statement_from_tokenizer(statement const&parent,token const&tk,tokenizer&t);
inline unique_ptr<statement>create_statement_from_tokenizer(statement const&parent,tokenizer&t);
//inline unique_ptr<expression>create_expression_from_tokenizer(statement const&parent,tokenizer&t);
