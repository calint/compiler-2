#pragma once
#include"statement.hpp"
#include"tokenizer.hpp"
class expression:public statement{
public:
	expression(statement*parent,utokenp t):statement{parent,move(t)}{}
};
using uexpressionp=unique_ptr<expression>;
