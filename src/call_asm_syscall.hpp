#pragma once

#include"call_asm.hpp"
#include"statement.hpp"
#include"toc.hpp"
#include"token.hpp"
#include"tokenizer.hpp"

class call_asm_syscall final:public call_asm{public:

	inline call_asm_syscall(const statement&parent,const token&tkn,tokenizer&t):
		call_asm{parent,tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.source_to_as_comment(os,*this);
		indent(os,indent_level);os<<"syscall"<<endl;
	}

};
