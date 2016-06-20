#pragma once
#include<memory>
#include<vector>
#include<cassert>
using namespace std;
#include"statement.hpp"
#include"tokenizer.hpp"
up_statement create_call_func(const char*,statement*,up_token,tokenizer&);
