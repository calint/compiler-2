#pragma once

#include <stddef.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "compiler_error.hpp"
#include "decouple.hpp"
#include "expression.hpp"
#include "statement.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "toc.hpp"

using vup_statement=vector<up_statement>;
class call:public expression{public:
	inline static up_statement read_statement(statement*parent,tokenizer&t){
		up_token tkn=t.next_token();
		if(!t.is_next_char_expression_open())
			return make_unique<expression>(parent,move(tkn));// ie  0x80
		t.unread();
		return create_call(tkn->name(),parent,move(tkn),t); // ie  f(...)
	}


	inline call(statement*parent,up_token tkn,tokenizer&t):expression{parent,move(tkn)}{
		if(!t.is_next_char_args_open())throw compiler_error(*this,"expected ( and arguments",token().copy_name());//? object invalid
		while(!t.is_next_char_args_close())args.push_back(call::read_statement(this,t));
	}
	inline void source_to(ostream&os)const override{statement::source_to(os);os<<"(";for(auto&e:args)e->source_to(os);os<<")";}
	inline void compile(toc&tc,ostream&os)override{
		for(auto&a:args)os<<"  push "<<a.get()->token().name()<<endl;
		os<<"  call "<<token().name()<<endl;
	}
	inline void link(toc&tc,ostream&os)override{
		if(!tc.has_func(token().name()))
			throw compiler_error(*this,"function not found",token().copy_name());
	}

	inline const statement&argument(size_t ix)const{return*(args[ix].get());}

private:
	vup_statement args;
};
using up_call=unique_ptr<call>;
using allocs=vector<const char*>;

