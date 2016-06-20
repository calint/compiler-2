#pragma once
class file:public statement{public:
	file(statement*parent,up_token tkn,tokenizer&t):statement{parent,move(tkn)}{
		identifier=t.next_token();
		if(!t.is_next_char_data_open())throw 1;
		while(true){
			if(t.is_next_char_data_close())break;
			tokens.push_back(t.next_token());
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
	up_token identifier;
	vup_tokens tokens;
};
