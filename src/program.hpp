#pragma once
class program:public statement{
public:
	program(tokenizer&st):program{nullptr,nullptr,st}{}
	void compile(toc&tc)override{
		printf("section .text\nglobal _start\n_start:\n");
		for(auto&s:statements){
			s->compile(tc);
		}
	}
	void source_to_stdout()override{
		for(auto&s:statements)s->source_to_stdout();
	}

private:
	inline program(statement*parent,up_token t,tokenizer&st):statement{parent,move(t)}{
		while(true){
			if(st.is_eos())break;
			up_token tk=st.next_token();
			up_statement stmt;
			if(tk->is_name("data")){
				stmt=make_unique<data>(parent,move(tk),st);
			}else if(tk->is_name("func")){
				stmt=make_unique<func>(parent,move(tk),st);
			}else{
				stmt=create_call_func(tk->name(),parent,move(tk),st);
			}
			statements.push_back(move(stmt));
		}
	}
	vup_statement statements;
};
using up_program=unique_ptr<program>;
