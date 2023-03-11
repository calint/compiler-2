#pragma once

class call_asm_xor final:public call_asm{
public:
	inline call_asm_xor(const statement&parent,const token&tkn,tokenizer&t):
		call_asm{parent,tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.source_comment(os,*this);
		indent(os,indent_level);
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
