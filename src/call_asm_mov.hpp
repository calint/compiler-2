#pragma once

#include <algorithm>
#include <iostream>

#include "call.hpp"
#include "statement.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_mov final:public call{public:
	inline call_asm_mov(statement*parent,up_token tkn,tokenizer&t):call{parent,move(tkn),t}{}
	inline void compile(toc&tc,ostream&os,size_t indent_level)override{// mov(eax 1)
		indent(os,indent_level);
		os<<"mov "<<tc.resolve_argument(argument(0).token().name())<<","<<tc.resolve_argument(argument(1).token().name())<<endl;
	}
};
