#pragma once
#include"statement.hpp"
#include"tokenizer.hpp"
class expression:public statement{
public:
	expression(utokenp t):statement(move(t)){}
};
using uexpressionp=unique_ptr<expression>;
