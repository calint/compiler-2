#pragma once

#include <algorithm>
#include <memory>

#include"statement.hpp"

class expression:public statement{public:

	inline expression(const statement&parent,token tk):
		statement{parent,tk}
	{}

	inline bool is_expression()const override{return true;}

};
