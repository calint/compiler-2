#pragma once
class tokenizer;
class statement;
class token;
inline up_statement create_call(const char*,statement*,unique_ptr<token>,tokenizer&);
