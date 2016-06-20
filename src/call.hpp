#pragma once
#include"expression.hpp"
using vup_statement=vector<up_statement>;
class call:public expression{
public:
	static up_statement read_expression(statement*parent,tokenizer&st){
		up_token t=st.next_token();
//		printf("[%s]\n",t->get_name());
		if(!st.is_next_char_expression_open()){
			return make_unique<expression>(parent,move(t));
		}
		st.unread();
		return create_call_func(t->name(),parent,move(t),st);
	}


	call(statement*parent,up_token t,tokenizer&st):expression{parent,move(t)}{
		assert(st.is_next_char_args_open());
		while(true){
			if(st.is_next_char_args_close())break;
			up_statement e=call::read_expression(this,st);
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
	vup_statement args;
};
using allocs=vector<const char*>;

