#pragma once
#include"statement.hpp"
class statement_exit:public statement{
public:
	statement_exit(unique_ptr<token>t,tokenizer&st):statement(move(t)){
		if(!st.is_next_char_expression_open())
			throw 1;
		if(!st.is_next_char_expression_close())
			throw 1;
	}
	void compile(toc&tc)override{
		puts("    mov     eax,1");
		puts("    int     0x80");
	}
	void print_source()override{
		statement::print_source();
		printf("()");
	}

};
