#pragma once
class expression:public statement{public:
	expression(statement*parent,up_token tkn):statement{parent,move(tkn)}{}
};
using up_expression=unique_ptr<expression>;
