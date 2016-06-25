#pragma once

#include <algorithm>
#include <iostream>

#include "statement.hpp"
#include "stmt_call.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_mov final:public stmt_call{public:

	inline call_asm_mov(const statement&parent,const token&tkn,tokenizer&t):
		stmt_call{parent,tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{// mov(eax 1)
		const string&ra=tc.resolve_ident_to_nasm(argument(0),argument(0).tok().name());
		const string&rb=tc.resolve_ident_to_nasm(argument(1),argument(1).tok().name());
		if(ra==rb){// ie  mov eax,eax
			return;
		}
		//? check illegal op comb


		indent(os,indent_level);
		os<<"mov "<<ra<<","<<rb<<"  ;  ";
		tc.source_location_to_stream(os,tok());
		os<<endl;
	}

};
