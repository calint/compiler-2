#pragma once

#include <stddef.h>
#include <iostream>
#include <string>

#include "call_asm.hpp"
#include "statement.hpp"
#include "stmt_call.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_cmovne final:public call_asm{public:

	inline call_asm_cmovne(const statement&parent,const token&tkn,tokenizer&t):
		call_asm{parent,tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{// mov(eax 1)
//		indent(os,indent_level,true);source_to(os);os<<endl;
		indent(os,indent_level);
		if(arg(1).is_expression()){
			const string&reg=tc.alloc_scratch_register(token());
			arg(1).compile(tc,os,indent_level,reg);
			os<<"cmovne "<<tc.resolve_ident_to_nasm(arg(0),arg(0).tok().name())<<","<<reg<<endl;
			tc.free_scratch_reg(reg);
			return;
		}
		os<<"cmovne "<<tc.resolve_ident_to_nasm(arg(0),arg(0).tok().name())<<",["<<tc.resolve_ident_to_nasm(arg(1),arg(1).tok().name())<<"]"<<endl;
	}

};
