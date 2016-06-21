#pragma once

#include <algorithm>
#include <iostream>

#include "call.hpp"
#include "statement.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_xor final:public call{public:
	inline call_asm_xor(statement*parent,up_token tkn,tokenizer&t):call{parent,move(tkn),t}{}
	inline void compile(toc&tc,ostream&os)override{// mov(eax 1)
		os<<"  xor "<<tc.alias_or(argument(0).token().name())<<","<<tc.alias_or(argument(1).token().name())<<endl;
	}
};
