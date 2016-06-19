#pragma once
#include"statement.hpp"
class statement_def:public statement{
public:
	statement_def(unique_ptr<token>t,tokenizer&st):statement(move(t)){
		identifier=st.next_token();
		if(!st.is_next_char_data_open())
			throw 1;
		while(true){
			if(st.is_next_char_data_close())break;
			data_tokens.push_back(st.next_token());
		}
	}
	void compiled_to_stdout(toc&tc)override{
//		section .data
//		msg     db  'Hello, world!',0xa                 ;string
//		msg.len equ $ - msg                             ;length of string
//		section .text
		puts("section .data");
		printf("%s     db  '",identifier->get_name());
		for(auto&s:data_tokens){
			s->compiled_to_stdout();
		}
		printf("'\n");
		printf("%s.len equ $-%s\n",identifier->get_name(),identifier->get_name());
		puts("section .text");
		tc.put_def(identifier->get_name());
	}
	void source_to_stdout()override{
		statement::source_to_stdout();
		identifier->source_to_stdout();
		printf("{");
		for(auto&s:data_tokens){
			s->source_to_stdout();
		}
		printf("}");
	}

private:
	unique_ptr<token>identifier;
	vector<unique_ptr<token>>data_tokens;
};
