#pragma once

#include <algorithm>
#include <memory>

#include "statement.hpp"
#include "token.hpp"

class expression:public statement{public:

	inline expression(const statement&parent,token tkn):
		statement{parent,tkn}
	{}

	inline bool is_expression()const override{return true;}

};
using up_expression=unique_ptr<expression>;
