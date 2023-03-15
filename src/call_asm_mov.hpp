#pragma once

#include"call_asm.hpp"

class call_asm_mov final:public call_asm{
public:
	inline call_asm_mov(const token&tkn,tokenizer&t):
		call_asm{tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		tc.source_comment(*this,os,indent_level);

		const string&dest=tc.resolve_ident_to_nasm(arg(0));
		const statement&src_arg=arg(1);
		if(src_arg.is_expression()){ // ? the assembler commands might not need this
			src_arg.compile(tc,os,indent_level+1,dest);
			return;
		}
		const string&src=tc.resolve_ident_to_nasm(src_arg);
		tc.asm_cmd(*this,os,indent_level,"mov",dest,src);
	}
};
