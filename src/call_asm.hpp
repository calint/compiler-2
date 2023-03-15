#pragma once

#include"stmt_call.hpp"

class call_asm:public stmt_call{
public:
	inline call_asm(const token&tkn,tokenizer&t):
		stmt_call{tkn,t}
	{}
};
