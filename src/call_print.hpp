#pragma once

#include <algorithm>
#include <iostream>

#include "call.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include"compiler_error.hpp"
class call_print final:public call{public:
	inline call_print(statement*parent,up_token tkn,tokenizer&t):call{parent,move(tkn),t}{}
	inline void compile(toc&tc,ostream&os)override{
		const statement&e=argument(0);
		allocs regs{"edx","ecx","ebx","eax"};
		os<<"  mov edx,"<<e.token().name()<<".len\n";// length of text
		os<<"  mov ecx,"<<e.token().name()<<"\n";// pointer to text
		os<<"  mov ebx,1\n";// file descriptor (stdout)
		os<<"  mov eax,4\n";// system call number (sys_write)
		os<<"  int 0x80\n";// call kernel
		// allocs.free()
	}
	inline void link(toc&tc,ostream&os)override{
		const statement&e=argument(0);
		if(!tc.has_file(e.token().name()))throw compiler_error(e,"cannot find file");
	}
};
