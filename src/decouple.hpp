#pragma once
class tokenizer;
class statement;
class token;
inline up_statement create_call_statement_from_tokenizer(statement&parent,const token&tk,tokenizer&t);
inline up_statement create_statement_from_tokenizer(statement&parent,tokenizer&t);
