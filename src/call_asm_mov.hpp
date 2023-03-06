#pragma once

#include"call_asm.hpp"
#include"statement.hpp"
#include"stmt_call.hpp"
#include"toc.hpp"
#include"token.hpp"
#include"tokenizer.hpp"

class call_asm_mov final:public call_asm{public:

	inline call_asm_mov(const statement&parent,const token&tkn,tokenizer&t):
		call_asm{parent,tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{// mov(eax 1)
		indent(os,indent_level,true);tc.source_to_as_comment(os,*this);

		const string&ra=tc.resolve_ident_to_nasm(arg(0),arg(0).identifier());
		const string&rb=tc.resolve_ident_to_nasm(arg(1),arg(1).identifier());

		if(ra==rb)
			return;

		if(!ra.find("dword[") and !rb.find("dword[")){
			const string&r=tc.alloc_scratch_register(token());
			indent(os,indent_level,false);os<<"mov "<<r<<","<<rb<<endl;
			indent(os,indent_level,false);os<<"mov "<<ra<<","<<r<<endl;
			tc.free_scratch_reg(r);
			return;
		}

		indent(os,indent_level);os<<"mov "<<ra<<","<<rb<<endl;
	}

};
