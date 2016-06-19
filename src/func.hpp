#pragma once
#include"statement.hpp"
#include<vector>
using vutokenp=vector<unique_ptr<token>>;
class func:public statement{
public:
	func(statement*parent,utokenp t,tokenizer&st):statement{parent,move(t)}{
		identifier=st.next_token();
		if(!st.is_next_char_expression_open())throw 1;
		while(true){
			if(st.is_next_char_expression_close())break;
			params.push_back(st.next_token());
		}
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
	}

private:
	utokenp identifier;
	vutokenp params;
};
