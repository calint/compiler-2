#pragma once
#include"expression.hpp"
#include"tokenizer.hpp"
using namespace std;
using allocs=vector<const char*>;
using vuexpressionp=vector<uexpressionp>;
class call:public expression{
public:
	static uexpressionp read_expression(statement*parent,tokenizer&st){
		utokenp t=st.next_token();
//		printf("[%s]\n",t->get_name());
		if(!st.is_next_char_expression_open()){
			return make_unique<expression>(parent,move(t));
		}
		st.unread();
		return make_unique<call>(parent,move(t),st);
	}


	call(statement*parent,utokenp t,tokenizer&st):expression{parent,move(t)}{
		if(!st.is_next_char_expression_open())throw 1;
		while(true){
			if(st.is_next_char_expression_close())break;
			uexpressionp e=call::read_expression(this,st);
			vexpr.push_back(move(e));
		}
	}
	void source_to_stdout()override{
		statement::source_to_stdout();
		printf("(");
		for(auto&e:vexpr)e->source_to_stdout();
		printf(")");
	}
	inline const expression&argument(int ix)const{return*(vexpr[ix].get());}

private:
	vuexpressionp vexpr;
};
