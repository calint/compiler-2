#pragma once

#include"statement.hpp"

class expression:public statement{
public:
	inline expression(token tk):
		statement{move(tk)}
	{}

	inline bool is_expression()const override{return true;}
};
