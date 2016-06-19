#pragma once
#include"statement.hpp"
class statement_exit:public statement{
public:
	inline statement_exit(unique_ptr<token>t,string_tokenizer&st):statement(move(t)){
		if(!st.is_next_char_expression_open())
			throw 1;
		if(!st.is_next_char_expression_close())
			throw 1;
	}

	inline void to_asm()override{
		puts("    mov     eax,1");
		puts("    int     0x80");
	}
};
