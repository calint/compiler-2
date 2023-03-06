#pragma once

#include"statement.hpp"

class expression:public statement{public:

	inline expression(const statement&parent,token tk):
		statement{parent,tk}
	{}

	inline bool is_expression()const override{return true;}

};
