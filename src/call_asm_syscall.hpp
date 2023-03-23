#pragma once

class call_asm_syscall final:public call_asm{
public:
	inline call_asm_syscall(token tk,tokenizer&t):
		call_asm{move(tk),t}
	{}

	inline call_asm_syscall()=default;
	inline call_asm_syscall(const call_asm_syscall&)=default;
	inline call_asm_syscall(call_asm_syscall&&)=default;
	inline call_asm_syscall&operator=(const call_asm_syscall&)=default;
	inline call_asm_syscall&operator=(call_asm_syscall&&)=default;

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dst="")const override{
		tc.source_comment(*this,os,indent_level);
		toc::indent(os,indent_level);os<<"syscall"<<endl;
	}
};
