#pragma once
#include"call.hpp"
class call_exit:public call{
public:
	call_exit(statement*parent,up_token t,tokenizer&st):call{parent,move(t),st}{}
	void compile(toc&tc)override{
//		assure_arg_count(0);
		puts("  mov eax,1");
		puts("  int 0x80");
	}
};
