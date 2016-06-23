#pragma once

#include <algorithm>
#include <memory>

#include "statement.hpp"
#include "token.hpp"

class stmt_expr:public statement{public:

	inline stmt_expr(statement*parent,up_token tkn):statement{parent,move(tkn)}{}

	inline bool is_expression()const override{return true;}

};
using up_expression=unique_ptr<stmt_expr>;
