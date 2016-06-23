#pragma once

#include <algorithm>
#include <iostream>

#include "call.hpp"
#include "statement.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_int final:public call{public:
	inline call_asm_int(statement*parent,up_token tkn,tokenizer&t):call{parent,move(tkn),t}{}
	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{// mov(eax 1)
		indent(os,indent_level);
		framestack&fs=tc.framestk();
		os<<"int "<<fs.resolve_func_arg(argument(0).token().name())<<endl;
	}
};
