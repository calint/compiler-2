#pragma once
#include"call.hpp"
class call_read:public call{
public:
	call_read(statement*parent,up_token tkn,tokenizer&t):call{parent,move(tkn),t}{}
	void compile(toc&tc)override{
		allocs all{"eax","edi","esi","edx"};
		const statement&e=argument(0);
		printf("  xor eax,eax\n");// 0 (write syscall number)
		printf("  xor edi,edi\n");// 0 (stdin file descriptor)
		printf("  mov esi,%s\n",e.token().name());// address of the buffer
		printf("  mov edx,%s.len\n",e.token().name());// size of the buffer
		printf("  syscall\n");// call kernel
		// allocs.free
	}
	void link(toc&tc)override{
		const statement&e=argument(0);
		if(!tc.has_def(e.token().name())){
			printf("%zu:%zu identifier '%s' not found\n",e.token().token_start_char(),e.token().token_end_char(),e.token().name());
			tc.print_to_stdout();
			throw 1;
		}
	}
};
