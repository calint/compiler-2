#pragma once
#include"func_statements.hpp"
class func:public statement{
public:
	func(statement*parent,up_token t,tokenizer&st):statement{parent,move(t)}{
		identifier=st.next_token();
		if(!st.is_next_char_expression_open())throw 1;
		while(true){
			if(st.is_next_char_expression_close())break;
			params.push_back(st.next_token());
		}
		stmts=make_unique<func_statements>(parent,st);
	}
	void compile(toc&tc)override{
		tc.put_func(identifier->name());
	}
	void source_to_stdout()override{
		statement::source_to_stdout();
		identifier->source_to_stdout();
		printf("(");
		for(auto&s:params)s->source_to_stdout();
		printf(")");
		stmts->source_to_stdout();
	}

private:
	up_token identifier;
	vup_tokens params;
	up_func_statements stmts;
};
