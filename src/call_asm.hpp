#pragma once

#include"statement.hpp"
#include"stmt_call.hpp"
#include"token.hpp"
#include"tokenizer.hpp"

class call_asm:public stmt_call{public:

	inline call_asm(const statement&parent,const token&tkn,tokenizer&t):
		stmt_call{parent,tkn,t}
	{}

};
