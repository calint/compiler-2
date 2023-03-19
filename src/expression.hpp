#pragma once

#include"statement.hpp"

class expression:public statement{
public:
	inline expression(token tk,const bool negated=false):
		statement{move(tk),negated}
	{}

	inline expression()=default;
	inline expression(const expression&)=default;
	inline expression(expression&&)=default;
	inline expression&operator=(const expression&)=default;
	inline expression&operator=(expression&&)=default;

	inline bool is_expression()const override{return true;}
};
