#pragma once
#include"decouple.hpp"
#include"statement.hpp"
#include"func.hpp"
class program:public statement{
public:
	program(tokenizer&st):program{nullptr,nullptr,st,true}{}
	program(statement*parent,up_token t,tokenizer&st,bool is_root=false):statement{parent,move(t)},is_root_{is_root}{
		if(!is_root)assert(st.is_next_char_block_open());
		while(true){
			if(!is_root and st.is_eos())throw 1;
			if(!is_root and st.is_next_char_block_close())break;
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
	void compile(toc&tc)override{
		for(auto&s:statements){
			s->compile(tc);
		}
	}
	void source_to_stdout()override{
		if(!is_root_)printf("{");
		for(auto&s:statements){
			s->source_to_stdout();
		}
		if(!is_root_)printf("}");
	}

private:
	vup_statement statements;
	bool is_root_{false};
};
using up_program=unique_ptr<program>;
