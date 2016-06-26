#pragma once

#include <algorithm>
#include <iostream>

#include "statement.hpp"
#include "stmt_call.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_jmp final:public stmt_call{public:

	inline call_asm_jmp(const statement&parent,const token&tkn,tokenizer&t):
		stmt_call{parent,tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level);
		//? forward decl label or resolve labels after compiler done
		os<<"jmp "<<tc.resolve_ident_to_nasm(arg(0),arg(0).tok().name())<<endl;
	}

};
