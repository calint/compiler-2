#pragma once

#include <algorithm>
#include <iostream>

#include "statement.hpp"
#include "stmt_call.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_jmp final:public stmt_call{public:

	inline call_asm_jmp(statement*parent,unique_ptr<class token>tkn,tokenizer&t):stmt_call{parent,move(tkn),t}{}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		indent(os,indent_level);
		framestack&fs=tc.framestk();
		os<<"jmp "<<fs.resolve_func_arg(argument(0).token().name())<<endl;
	}

};
