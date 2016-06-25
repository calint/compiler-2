#pragma once

#include <algorithm>
#include <iostream>

#include "statement.hpp"
#include "stmt_call.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_cmp final:public stmt_call{public:

	inline call_asm_cmp(statement*parent,const token&tkn,tokenizer&t):
		stmt_call{parent,tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		indent(os,indent_level);
		os<<"cmp "<<tc.resolve_ident_to_nasm(argument(0),argument(0).tok().name())<<","
				<<tc.resolve_ident_to_nasm(argument(1),argument(1).tok().name())<<endl;
	}

};
