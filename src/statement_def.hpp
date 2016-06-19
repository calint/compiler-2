#pragma once
#include"statement.hpp"
class statement_def:public statement{
public:
	inline statement_def(unique_ptr<token>t,string_tokenizer&st):statement(move(t)){
		identifier=st.next_token();
		if(!st.is_next_char_data_open())
			throw 1;
		data=st.next_token();
		if(!st.is_next_char_data_close())
			throw 1;
	}
	inline void compiled_to_stdout()override{
//		section .data
//		msg     db  'Hello, world!',0xa                 ;our dear string
//		msg.len equ $ - msg                             ;length of our dear string
//		section .text
		puts("section .data");
		printf("%s     db  %s\n",identifier->get_name(),data->get_name());
		printf("%s.len equ $-%s\n",identifier->get_name(),identifier->get_name());
		puts("section .text");
	}
	inline void source_to_stdout()override{
		statement::source_to_stdout();
		identifier->to_stdout();
		printf("{");
		data->to_stdout();
		printf("}");
	}

private:
	unique_ptr<token>identifier;
	unique_ptr<token>data;
};
