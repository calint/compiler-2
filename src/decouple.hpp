#pragma once
#include "token.hpp"
using namespace std;
#include"statement.hpp"
#include"tokenizer.hpp"
class call;
inline unique_ptr<call>create_call(const char*,statement*,up_token,tokenizer&);
