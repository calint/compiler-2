#pragma once

class call_asm_syscall final:public call_asm{
public:
	inline call_asm_syscall(const token&tkn,tokenizer&t):
		call_asm{tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		tc.source_comment(*this,os,indent_level);
		toc::indent(os,indent_level);os<<"syscall"<<endl;
	}
};
