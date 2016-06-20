#pragma once

#include <algorithm>
#include <iostream>
#include <memory>

#include "block.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class func final:public statement{public:
	func(statement*parent,up_token tkn,tokenizer&t):statement{parent,move(tkn)}{
		identifier=t.next_token();
		if(!t.is_next_char_expression_open())throw 1;
		while(!t.is_next_char_expression_close())params.push_back(t.next_token());
		code=make_unique<block>(parent,t);
	}
	void compile(toc&tc,ostream&os)override{tc.put_func(identifier->name());}
	void link(toc&tc,ostream&os)override final{code->link(tc,os);}
	void source_to(ostream&os)override{
		statement::source_to(os);
		identifier->source_to(os);
		os<<"(";
		for(auto&s:params)s->source_to(os);
		os<<")";
		code->source_to(os);
	}
private:
	up_token identifier;
	vup_tokens params;
	up_block code;
};
