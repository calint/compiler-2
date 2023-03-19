#pragma once

#include"call_asm.hpp"

class call_asm_mov final:public call_asm{
public:
	inline call_asm_mov(token tkn,tokenizer&t):
		call_asm{move(tkn),t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		tc.source_comment(*this,os,indent_level);

		const ident_resolved&dst_r{tc.resolve_ident_to_nasm(arg(0))};
		const statement&src_arg{arg(1)};
		if(src_arg.is_expression()){ // ? the assembler commands might not need this
			src_arg.compile(tc,os,indent_level+1,dst_r.id);
			return;
		}
		// src is not an expression
		const ident_resolved&src{tc.resolve_ident_to_nasm(src_arg)};
		if(src.is_const()){
			// a constant
			tc.asm_cmd(*this,os,indent_level,"mov",dst_r.id,src.as_const());
			return;
		}
		// variable, register or field
		tc.asm_cmd(*this,os,indent_level,"mov",dst_r.id,src.id);
		if(src.negated){
			tc.asm_neg(*this,os,indent_level,dst_r.id);
		}
	}
};
