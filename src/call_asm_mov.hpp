#pragma once

#include"call_asm.hpp"

class call_asm_mov final:public call_asm{
public:
	inline call_asm_mov(const statement&parent,const token&tkn,tokenizer&t):
		call_asm{parent,tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.source_comment(os,*this);

		const string&dest=tc.resolve_ident_to_nasm(arg(0));
		const statement&src_arg=arg(1);
		if(src_arg.is_expression()){ // ? the assembler commands might not need this
			src_arg.compile(tc,os,indent_level+1,arg(0).identifier());
			return;
		}
		const string&src=tc.resolve_ident_to_nasm(src_arg);

		if(dest==src)
			return;

		if(!dest.find("qword[") and !src.find("qword[")){
			const string&r=tc.alloc_scratch_register(tok());
			indent(os,indent_level);os<<"mov "<<r<<","<<src<<endl;
			indent(os,indent_level);os<<"mov "<<dest<<","<<r<<endl;
			tc.free_scratch_reg(r);
			return;
		}

		indent(os,indent_level);os<<"mov "<<dest<<","<<src<<endl;
	}
};
