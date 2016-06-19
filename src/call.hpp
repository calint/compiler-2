#pragma once
#include"decouple.hpp"
#include"expression.hpp"
#include"tokenizer.hpp"
using namespace std;
using allocs=vector<const char*>;
using vuexpressionp=vector<uexpressionp>;
using vustatementp=vector<ustatementp>;
class call:public expression{
public:
	static ustatementp read_expression(statement*parent,tokenizer&st){
		utokenp t=st.next_token();
//		printf("[%s]\n",t->get_name());
		if(!st.is_next_char_expression_open()){
			return make_unique<expression>(parent,move(t));
		}
		st.unread();
		return create_call_func(t->name(),parent,move(t),st);
	}


	call(statement*parent,utokenp t,tokenizer&st):expression{parent,move(t)}{
		assert(!st.is_next_char_expression_open());
		while(true){
			if(st.is_next_char_expression_close())break;
			ustatementp e=call::read_expression(this,st);
			args.push_back(move(e));
		}
	}
	void source_to_stdout()override{
		statement::source_to_stdout();
		printf("(");
		for(auto&e:args)e->source_to_stdout();
		printf(")");
	}
	inline const statement&argument(int ix)const{return*(args[ix].get());}

private:
	vustatementp args;
};
