#pragma once

#include"call_asm.hpp"

class call_asm_mov final:public call_asm{
public:
	inline call_asm_mov(const statement&parent,const token&tkn,tokenizer&t):
		call_asm{parent,tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{// mov(eax 1)
		indent(os,indent_level,true);tc.source_comment(os,*this);

		const string&rai=arg(0).identifier();
		const string&ra=tc.resolve_ident_to_nasm(arg(0),rai);
		const statement&rbs=arg(1);
		if(rbs.is_expression()){ // ? the assembler commands might not need this
			rbs.compile(tc,os,indent_level+1,rai);
			return;
		}
		const string&rb=tc.resolve_ident_to_nasm(rbs,rbs.identifier());

		if(ra==rb)
			return;

		if(!ra.find("dword[") and !rb.find("dword[")){
			const string&r=tc.alloc_scratch_register(token());
			indent(os,indent_level);os<<"mov "<<r<<","<<rb<<endl;
			indent(os,indent_level);os<<"mov "<<ra<<","<<r<<endl;
			tc.free_scratch_reg(r);
			return;
		}

		indent(os,indent_level);os<<"mov "<<ra<<","<<rb<<endl;
	}
};
