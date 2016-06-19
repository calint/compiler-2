#pragma once
#include"statement.hpp"
#include"tokenizer.hpp"
class expression:public statement{
public:
	expression(statement*parent,up_token t):statement{parent,move(t)}{}
};
using up_expression=unique_ptr<expression>;
