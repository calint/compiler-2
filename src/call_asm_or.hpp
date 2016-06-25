#pragma once

#include <algorithm>
#include <iostream>

#include "statement.hpp"
#include "stmt_call.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_or final:public stmt_call{public:

	inline call_asm_or(statement&parent,const token&tkn,tokenizer&t):
		stmt_call{parent,tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		indent(os,indent_level);
		os<<"or "<<tc.resolve_ident_to_nasm(argument(0),argument(0).tok().name())<<",";
		os<<tc.resolve_ident_to_nasm(argument(1),argument(1).tok().name());
		os<<endl;
	}

};
