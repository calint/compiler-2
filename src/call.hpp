#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

#include "decouple.hpp"
#include "expression.hpp"
#include "statement.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include"compiler_error.hpp"

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
		if(!t.is_next_char_args_open())throw compiler_error(*this,"expected ( and arguments");
//		assert(t.is_next_char_args_open());
		while(!t.is_next_char_args_close()){
			args.push_back(call::read_statement(this,t));
		}
	}
	void source_to(ostream&os)override{
		statement::source_to(os);
		os<<"(";
		for(auto&e:args)e->source_to(os);
		os<<")";
	}
	inline const statement&argument(size_t ix)const{return*(args[ix].get());}

private:
	vup_statement args;
};
using allocs=vector<const char*>;

