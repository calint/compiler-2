#pragma once
#include"expression.hpp"
using vup_statement=vector<up_statement>;
class call:public expression{public:
	static up_statement read_statement(statement*parent,tokenizer&t){
		up_token tkn=t.next_token();
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
			auto e=call::read_statement(this,t);
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

