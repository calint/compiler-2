#pragma once
#include "token.hpp"
using namespace std;
#include"statement.hpp"
#include"tokenizer.hpp"
up_statement create_statement(const char*,statement*,up_token,tokenizer&);
