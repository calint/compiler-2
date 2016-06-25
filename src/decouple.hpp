#pragma once
class tokenizer;
class statement;
class token;
inline up_statement create_call_statement_from_tokenizer(const char*,statement*,unique_ptr<token>,tokenizer&);
inline up_statement create_statement_from_tokenizer(statement*parent,tokenizer&t);
