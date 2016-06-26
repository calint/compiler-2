#pragma once

#include <algorithm>
#include <iostream>

#include "statement.hpp"
#include "stmt_call.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_xor final:public stmt_call{public:

	inline call_asm_xor(const statement&parent,const token&tkn,tokenizer&t):
		stmt_call{parent,tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level);
		os<<"xor "<<tc.resolve_ident_to_nasm(arg(0),arg(0).tok().name())<<",";
		if(arg_count()==1){
			os<<tc.resolve_ident_to_nasm(arg(0),arg(0).tok().name());
		}else{
			os<<tc.resolve_ident_to_nasm(arg(1),arg(1).tok().name());
		}
		os<<"  ;  ";
		tc.source_location_to_stream(os,tok());
		os<<endl;
	}

};
