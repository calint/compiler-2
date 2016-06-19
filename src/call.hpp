#pragma once
#include"expression.hpp"
#include"tokenizer.hpp"
using namespace std;
using allocs=vector<const char*>;
using vuexpressionp=vector<uexpressionp>;
class call:public expression{
public:
	static uexpressionp read_expression(tokenizer&st){
		utokenp t=st.next_token();
//		printf("[%s]\n",t->get_name());
		if(!st.is_next_char_expression_open()){
			return make_unique<expression>(move(t));
		}
		st.unread();
		return make_unique<call>(move(t),st);
	}


	call(utokenp t,tokenizer&st):expression(move(t)){
		if(!st.is_next_char_expression_open())throw 1;
		while(true){
			if(st.is_next_char_expression_close())break;
			uexpressionp e=call::read_expression(st);
			vexpr.push_back(move(e));
		}
	}
	void print_source()override{
		statement::print_source();
		printf("(");
		for(auto&e:vexpr)e->print_source();
		printf(")");
	}
	inline const expression&get_argument(int ix)const{return*(vexpr[ix].get());}

private:
	vuexpressionp vexpr;
};
