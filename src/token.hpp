#pragma once
#include<memory>
using namespace std;
#include<string.h>
#include<cassert>
class token{
public:
	unique_ptr<const char[]>ws_before;
	size_t token_start_nchar;
	unique_ptr<const char[]>token;
	size_t token_end_nchar;
	unique_ptr<const char[]>ws_after;

	inline void to_stdout(){
		printf("%s%s%s",ws_before.get(),token.get(),ws_after.get());
	}
	inline void to_stdout2(){
		printf("[%zu]%s[%zu]\n",token_start_nchar,token.get(),token_end_nchar);
	}
	inline void to_asm(){
//		printf("dd %s\n",token.get());
		to_stdout();
	}
};
