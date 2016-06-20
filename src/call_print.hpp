#pragma once

#include <algorithm>
#include <cstdio>
#include <iostream>

#include "call.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_print:public call{public:
	call_print(statement*parent,up_token tkn,tokenizer&t):call{parent,move(tkn),t}{}
	void compile(toc&tc,ostream&os)override{
		const statement&e=argument(0);
		allocs regs{"edx","ecx","ebx","eax"};
		os<<"  mov edx,"<<e.token().name()<<".len\n";// length of text
		os<<"  mov ecx,"<<e.token().name()<<"\n";// pointer to text
		os<<"  mov ebx,1\n";// file descriptor (stdout)
		os<<"  mov eax,4\n";// system call number (sys_write)
		os<<"  int 0x80\n";// call kernel
		// allocs.free()
	}
	void link(toc&tc,ostream&os)override{
		const statement&e=argument(0);
		if(!tc.has_file(e.token().name())){
			os<<e.token().token_start_char()<<":"<<e.token().token_end_char()<<"cannot find file '"<<e.token().name()<<"'\n";
			tc.print_to(os);
			throw 1;
		}
	}
};
