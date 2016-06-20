#pragma once

#include <algorithm>
#include <iostream>

#include "call.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_exit:public call{public:
	call_exit(statement*parent,up_token tkn,tokenizer&t):call{parent,move(tkn),t}{}
	void compile(toc&tc,ostream&os)override{
//		assure_arg_count(0);
		os<<"  mov eax,1\n";
		os<<"  int 0x80\n";
	}
};
