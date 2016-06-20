#pragma once

#include <algorithm>
#include <iostream>

#include "call.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_exit final:public call{public:
	inline call_exit(statement*parent,up_token tkn,tokenizer&t):call{parent,move(tkn),t}{}
	inline void compile(toc&tc,ostream&os)override{
//		assure_arg_count(0);
		os<<"  mov eax,1\n";
		os<<"  int 0x80\n";
	}
};
