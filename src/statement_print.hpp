#pragma once
#include"statement.hpp"
class statement_print:public statement{
public:
	inline statement_print(unique_ptr<token>t,string_tokenizer&st):statement(move(t)){
		if(!st.is_next_char_expression_open())
			throw 1;
		if(!st.is_next_char_expression_close())
			throw 1;
	}

	inline void to_asm()override{
		puts("    mov     edx,msg.len                         ;message length");
		puts("    mov     ecx,msg                             ;message to write");
		puts("    mov     ebx,1                               ;file descriptor (stdout)");
		puts("    mov     eax,4                               ;system call number (sys_write)");
		puts("    int     0x80                                ;call kernel");
		puts("section .data");
		puts("msg     db  'Hello, world!',0xa                 ;our dear string");
		puts("msg.len equ $ - msg                             ;length of our dear string");
		puts("section .text");
	}
};
