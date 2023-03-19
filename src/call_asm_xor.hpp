#pragma once

class call_asm_xor final:public call_asm{
public:
	inline call_asm_xor(token tkn,tokenizer&t):
		call_asm{move(tkn),t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		tc.source_comment(*this,os,indent_level);
		toc::indent(os,indent_level);
		const ident_resolved&dst_r{tc.resolve_ident_to_nasm(arg(0))};
		if(!dst_r.is_register())
			throw compiler_error(arg(0),"argument must be a register");
		os<<"xor "<<dst_r.id<<","<<dst_r.id<<endl;
	}
};
