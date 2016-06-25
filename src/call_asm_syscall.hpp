#pragma once

#include <algorithm>
#include <iostream>

#include "statement.hpp"
#include "stmt_call.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_syscall final:public stmt_call{public:

	inline call_asm_syscall(const statement&parent,const token&tkn,tokenizer&t):
		stmt_call{parent,tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		indent(os,indent_level);
		os<<"syscall";
		os<<"  ;  ";
		tc.source_location_to_stream(os,tok());
		os<<endl;

	}

};
