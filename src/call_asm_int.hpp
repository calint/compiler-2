#pragma once

class call_asm_int final:public call_asm{
public:
	inline call_asm_int(token tkn,tokenizer&t):
		call_asm{move(tkn),t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		tc.source_comment(*this,os,indent_level);

		toc::indent(os,indent_level);
		os<<"int "<<tc.resolve_ident_to_nasm(arg(0))<<endl;
	}
};
