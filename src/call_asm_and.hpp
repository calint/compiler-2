#pragma once

#include <algorithm>
#include <iostream>

#include "statement.hpp"
#include "stmt_call.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_and final:public stmt_call{public:

	inline call_asm_and(const statement&parent,const token&tkn,tokenizer&t):
		stmt_call{parent,tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level);
		os<<"and "<<tc.resolve_ident_to_nasm(arg(0),arg(0).tok().name())
				<<","<<tc.resolve_ident_to_nasm(arg(1),arg(1).tok().name())<<endl;
	}

};
