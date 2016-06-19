#pragma once
#include"call.hpp"
using allocs=vector<const char*>;
class call_read:public call{
public:
	call_read(utokenp t,tokenizer&st):call(move(t),st){}
	void compile(toc&tc)override{
		allocs all{"eax","edi","esi","edx"};
		const expression&e=get_argument(0);
		printf("  xor eax,eax\n");// 0 (write syscall number)
		printf("  xor edi,edi\n");// 0 (stdin file descriptor)
		printf("  mov esi,%s\n",e.get_token().get_name());// address of the buffer
		printf("  mov edx,%s.len\n",e.get_token().get_name());// size of the buffer
		printf("  syscall\n");// call kernel
		// allocs.free
	}
	void link(toc&tc)override{
		const expression&e=get_argument(0);
		if(!tc.has_def(e.get_token().get_name())){
			printf("%zu:%zu identifier '%s' not found\n",e.get_token().get_nchar(),e.get_token().get_nchar_end(),e.get_token().get_name());
			tc.print_to_stdout();
			throw 1;
		}
	}
};
