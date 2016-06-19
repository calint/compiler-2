#pragma once
#include"call.hpp"
using allocs=std::vector<const char*>;
class call_print:public call{
public:
	call_print(utokenp t,tokenizer&st):call(move(t),st){}
	void compile(toc&tc)override{
		const expression&e=get_argument(0);
		allocs regs{"edx","ecx","ebx","eax"};
		printf("  mov edx,%s.len\n",e.get_token().get_name());// length of text
		printf("  mov ecx,%s\n",e.get_token().get_name());// pointer to text
		printf("  mov ebx,1\n");// file descriptor (stdout)
		printf("  mov eax,4\n");// system call number (sys_write)
		printf("  int 0x80\n");// call kernel
		// allocs.free()
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
