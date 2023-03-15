#pragma once

class call_asm_xor final:public call_asm{
public:
	inline call_asm_xor(const token&tkn,tokenizer&t):
		call_asm{tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		tc.source_comment(*this,os,indent_level);
		toc::indent(os,indent_level);
		const string&dest=tc.resolve_ident_to_nasm(arg(0));
		os<<"xor "<<dest<<",";
		if(arg_count()==1){
			os<<dest;
		}else{
			os<<tc.resolve_ident_to_nasm(arg(1));
		}
		os<<endl;
	}
};
