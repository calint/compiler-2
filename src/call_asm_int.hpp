#pragma once

#include <algorithm>
#include <iostream>

#include "call.hpp"
#include "statement.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_int final:public call{public:
	inline call_asm_int(statement*parent,up_token tkn,tokenizer&t):call{parent,move(tkn),t}{}
	inline void compile(toc&tc,ostream&os,size_t indent_level)override{// mov(eax 1)
		for(size_t i=0;i<indent_level;i++)cout<<"  ";
		os<<"int "<<tc.alias_or(argument(0).token().name())<<endl;
	}
};
