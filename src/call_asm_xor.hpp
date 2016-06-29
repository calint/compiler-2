#pragma once

#include <algorithm>
#include <iostream>

#include "statement.hpp"
#include "stmt_call.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "call_asm.hpp"
class call_asm_xor final:public call_asm{public:

	inline call_asm_xor(const statement&parent,const token&tkn,tokenizer&t):
		call_asm{parent,tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level);
		os<<"xor "<<tc.resolve_ident_to_nasm(arg(0),arg(0).identifier())<<",";
		if(arg_count()==1){
			os<<tc.resolve_ident_to_nasm(arg(0),arg(0).identifier());
		}else{
			os<<tc.resolve_ident_to_nasm(arg(1),arg(1).identifier());
		}
		os<<"  ;  ";
		tc.source_location_to_stream(os,tok());
		os<<endl;
	}

};
