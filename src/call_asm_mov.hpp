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

		if(!ra.find("dword[") and !rb.find("dword[")){
			const string&r=tc.framestk().alloc_scratch_register();
			indent(os,indent_level,false);
			os<<"mov "<<r<<","<<rb<<"  ;  ";
			tc.source_location_to_stream(os,tok());
			os<<endl;
			indent(os,indent_level,false);
			os<<"mov "<<ra<<","<<r<<"  ;  ";
			tc.source_location_to_stream(os,tok());
			os<<endl;
			tc.framestk().free_scratch_reg(r);
			return;
		}

		indent(os,indent_level);
		os<<"mov "<<ra<<","<<rb<<"  ;  ";
		tc.source_location_to_stream(os,tok());
		os<<endl;
	}

};
