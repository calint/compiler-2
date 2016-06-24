#pragma once

#include <algorithm>
#include <memory>

#include "statement.hpp"
#include "token.hpp"

class expression:public statement{public:

	inline expression(toc&tc,statement*parent,unique_ptr<class token>tkn):statement{tc,parent,move(tkn)}{}

	inline bool is_expression()const override{return true;}

};
using up_expression=unique_ptr<expression>;
