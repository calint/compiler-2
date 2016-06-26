#pragma once

#include <algorithm>
#include <iostream>

#include "statement.hpp"
#include "stmt_call.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_cmove final:public stmt_call{public:

	inline call_asm_cmove(const statement&parent,const token&tkn,tokenizer&t):
		stmt_call{parent,tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{// mov(eax 1)
//		indent(os,indent_level,true);source_to(os);os<<endl;
		indent(os,indent_level);
		if(arg(1).is_expression()){
			const string&reg=tc.alloc_scratch_register(token());
			arg(1).set_dest_nasm_ident(reg);
			arg(1).compile(tc,os,indent_level);
			os<<"cmove "<<tc.resolve_ident_to_nasm(arg(0),arg(0).tok().name())<<","<<reg<<endl;
			tc.free_scratch_reg(reg);
			return;
		}
		os<<"cmove "<<tc.resolve_ident_to_nasm(arg(0),arg(0).tok().name())<<",["<<tc.resolve_ident_to_nasm(arg(1),arg(1).tok().name())<<"]"<<endl;
	}

};
