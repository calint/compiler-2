#pragma once

#include <algorithm>
#include <iostream>

#include "statement.hpp"
#include "stmt_call.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_int final:public stmt_call{public:
	inline call_asm_int(statement*parent,up_token tkn,tokenizer&t):stmt_call{parent,move(tkn),t}{}
	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{// mov(eax 1)
		indent(os,indent_level);
		framestack&fs=tc.framestk();
		os<<"int "<<fs.resolve_func_arg(argument(0).tok().name());
		os<<"  ;  ";
		tc.source_location_to_stream(os,tok());
		os<<endl;
	}
};
