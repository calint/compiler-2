#pragma once

#include"call_asm.hpp"

class call_asm_mov final:public call_asm{
public:
	inline call_asm_mov(toc&tc,token tk,tokenizer&t):
		call_asm{tc,move(tk),t}
	{
		set_type(&tc.get_type(*this,toc::void_type_str));
	}

	inline call_asm_mov()=default;
	inline call_asm_mov(const call_asm_mov&)=default;
	inline call_asm_mov(call_asm_mov&&)=default;
	inline call_asm_mov&operator=(const call_asm_mov&)=default;
	inline call_asm_mov&operator=(call_asm_mov&&)=default;

	inline void compile(toc&tc,ostream&os,size_t indent,const string&dst="")const override{
		tc.source_comment(*this,os,indent);

		const ident_resolved&dst_r{tc.resolve_ident_to_nasm(arg(0),false)};
		const statement&src_arg{arg(1)};
		if(src_arg.is_expression()){ // ? the assembler commands might not need this
			src_arg.compile(tc,os,indent+1,dst_r.id);
			return;
		}
		// src is not an expression
		const ident_resolved&src{tc.resolve_ident_to_nasm(src_arg,true)};
		if(src.is_const()){
			// a constant
			tc.asm_cmd(*this,os,indent,"mov",dst_r.id,src_arg.get_unary_ops().get_ops_as_string()+src.id);
			return;
		}
		// variable, register or field
		tc.asm_cmd(*this,os,indent,"mov",dst_r.id,src.id);
		src_arg.get_unary_ops().compile(tc,os,indent,dst_r.id);
	}
};
