#pragma once
#include"expression.hpp"
#include"tokenizer.hpp"
#include<memory>
#include<vector>
#include<cassert>
using namespace std;
std::unique_ptr<statement>create_call_func(const char*,statement*,up_token,tokenizer&);
