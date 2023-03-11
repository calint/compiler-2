#pragma once

class statement;
class token;
class tokenizer;

inline unique_ptr<statement>create_statement_from_tokenizer(statement const&parent,tokenizer&t);
inline unique_ptr<statement>create_statement_from_tokenizer(statement const&parent,token const&tk,tokenizer&t);
