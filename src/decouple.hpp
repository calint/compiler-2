#pragma once
#include"expression.hpp"
#include"tokenizer.hpp"
#include<memory>
std::unique_ptr<statement>create_call_func(const char*,statement*,utokenp,tokenizer&);
