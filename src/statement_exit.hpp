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
	void compiled_to_stdout(toc&tc)override{
		puts("    mov     eax,1");
		puts("    int     0x80");
	}
	void source_to_stdout()override{
		statement::source_to_stdout();
		printf("()");
	}

};
