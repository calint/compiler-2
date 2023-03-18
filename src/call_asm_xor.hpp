#pragma once

class call_asm_xor final:public call_asm{
public:
	inline call_asm_xor(token tkn,tokenizer&t):
		call_asm{move(tkn),t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		tc.source_comment(*this,os,indent_level);
		toc::indent(os,indent_level);
		const ident_resolved&dr{tc.resolve_ident_to_nasm(arg(0))};
		os<<"xor "<<dr.id<<",";
		if(arg_count()==1){
			os<<dr.id;
		}else{
			const ident_resolved&sr{tc.resolve_ident_to_nasm(arg(1))};
			// !! negation
			os<<sr.id;
		}
		os<<endl;
	}
};
