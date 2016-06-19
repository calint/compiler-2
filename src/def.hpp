#pragma once
#include"statement.hpp"
#include<vector>
using vutokenp=vector<unique_ptr<token>>;
class def:public statement{
public:
	def(statement*parent,utokenp t,tokenizer&st):statement{parent,move(t)}{
		identifier=st.next_token();
		if(!st.is_next_char_data_open())throw 1;
		while(true){
			if(st.is_next_char_data_close())break;
			tokens.push_back(st.next_token());
		}
	}
	void compile(toc&tc)override{
//		section .data
//		msg     db  'Hello, world!',0xa                 ;string
//		msg.len equ $ - msg                             ;length of string
//		section .text
		puts("section .data");
		printf("%s     db  '",identifier->name());
		for(auto&s:tokens){
			s->compiled_to_stdout();
		}
		printf("'\n");
		printf("%s.len equ $-%s\n",identifier->name(),identifier->name());
		puts("section .text");
		tc.put_def(identifier->name());
	}
	void source_to_stdout()override{
		statement::source_to_stdout();
		identifier->source_to_stdout();
		printf("{");
		for(auto&s:tokens)s->source_to_stdout();
		printf("}");
	}

private:
	utokenp identifier;
	vutokenp tokens;
};
