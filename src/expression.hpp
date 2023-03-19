#pragma once

#include"statement.hpp"

class expression:public statement{
public:
	inline expression(token tk,const bool negated=false):
		statement{move(tk),negated}
	{}

	inline bool is_expression()const override{return true;}
};
