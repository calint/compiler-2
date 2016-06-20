#pragma once

#include <algorithm>
#include <cstdio>
#include <iostream>

#include "call.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_read:public call{public:
	call_read(statement*parent,up_token tkn,tokenizer&t):call{parent,move(tkn),t}{}
	void compile(toc&tc,ostream&os)override{
		allocs all{"eax","edi","esi","edx"};
		const statement&e=argument(0);
		os<<"  xor eax,eax\n";// 0 (write syscall number)
		os<<"  xor edi,edi\n";// 0 (stdin file descriptor)
		os<<"  mov esi,"<<e.token().name()<<endl;// address of the buffer
		os<<"  mov edx,"<<e.token().name()<<".len\n";// size of the buffer
		os<<"  syscall\n";// call kernel
		// allocs.free
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
