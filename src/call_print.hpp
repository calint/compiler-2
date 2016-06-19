#pragma once
#include"call.hpp"
using allocs=std::vector<const char*>;
class call_print:public call{
public:
	call_print(statement*parent,utokenp t,tokenizer&st):call{parent,move(t),st}{}
	void compile(toc&tc)override{
		const expression&e=argument(0);
		allocs regs{"edx","ecx","ebx","eax"};
		printf("  mov edx,%s.len\n",e.token().name());// length of text
		printf("  mov ecx,%s\n",e.token().name());// pointer to text
		printf("  mov ebx,1\n");// file descriptor (stdout)
		printf("  mov eax,4\n");// system call number (sys_write)
		printf("  int 0x80\n");// call kernel
		// allocs.free()
	}
	void link(toc&tc)override{
		const expression&e=argument(0);
		if(!tc.has_def(e.token().name())){
			printf("%zu:%zu identifier '%s' not found\n",e.token().token_start_char(),e.token().token_end_char(),e.token().name());
			tc.print_to_stdout();
			throw 1;
		}
	}
};
