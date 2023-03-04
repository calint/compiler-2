#pragma once
#include<memory>
class tokenizer;
class token;
class statement;
class expression;
inline unique_ptr<statement>create_call_statement_from_tokenizer(statement const&parent,token const&tk,tokenizer&t);
inline unique_ptr<statement>create_statement_from_tokenizer(statement const&parent,tokenizer&t);
