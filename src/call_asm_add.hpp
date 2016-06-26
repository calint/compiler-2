#pragma once

#include <algorithm>
#include <iostream>

#include "statement.hpp"
#include "stmt_call.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_add final:public stmt_call{public:

	inline call_asm_add(const statement&parent,const token&tkn,tokenizer&t):
		stmt_call{parent,tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		const string&ra=tc.resolve_ident_to_nasm(arg(0),arg(0).tok().name());
		const string&rb=tc.resolve_ident_to_nasm(arg(1),arg(1).tok().name());

		if(!ra.find("dword[") and !rb.find("dword[")){
			const string&r=tc.alloc_scratch_register(token());
			indent(os,indent_level,false);
			os<<"mov "<<r<<","<<rb<<"  ;  ";
			tc.source_location_to_stream(os,tok());
			os<<endl;
			indent(os,indent_level,false);
			os<<"add "<<ra<<","<<r<<"  ;  ";
			tc.source_location_to_stream(os,tok());
			os<<endl;
			tc.free_scratch_reg(r);
			return;
		}


		indent(os,indent_level);
		os<<"add "<<ra<<","<<rb;
		os<<"  ;  ";
		tc.source_location_to_stream(os,tok());
		os<<endl;
	}

};
