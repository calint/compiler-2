#pragma once

class statement;
class token;
class tokenizer;

inline unique_ptr<statement>create_statement_from_tokenizer(tokenizer&t);
inline unique_ptr<statement>create_statement_from_tokenizer(token tk,tokenizer&t);
