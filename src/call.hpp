#pragma once
#include"expression.hpp"
using vup_statement=vector<up_statement>;
class call:public expression{public:
	static up_statement read_expression(statement*parent,tokenizer&t){
		up_token tkn=t.next_token();
//		printf("[%s]\n",t->get_name());
		if(!t.is_next_char_expression_open()){
			return make_unique<expression>(parent,move(tkn));
		}
		t.unread();
		return create_statement(tkn->name(),parent,move(tkn),t);
	}


	call(statement*parent,up_token tkn,tokenizer&t):expression{parent,move(tkn)}{
		assert(t.is_next_char_args_open());
		while(true){
			if(t.is_next_char_args_close())break;
			up_statement e=call::read_expression(this,t);
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

