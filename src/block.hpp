#pragma once

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>

#include "call.hpp"
#include "compiler_error.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class block final:public statement{public:
	block(statement*parent,tokenizer&t):statement{parent,t.next_token()}{
		assert(t.is_next_char_block_open());
		while(true){
			if(t.is_eos())throw compiler_error(*this,"unexpected end of string",this->token().token_start_char(),this->token().token_end_char());
			if(t.is_next_char_block_close())break;
			up_token tkn=t.next_token();
			statements.push_back(create_statement(tkn->name(),parent,move(tkn),t));
		}
	}
	void compile(toc&tc,ostream&os)override{for(auto&s:statements)s->compile(tc,os);}
	void link(toc&tc,ostream&os)override{for(auto&s:statements)s->link(tc,os);}

	void source_to(ostream&os)override{
		statement::source_to(os);
		os<<"{";
		for(auto&s:statements)s->source_to(os);
		os<<"}";
	}

private:
	vup_statement statements;
};
using up_block=unique_ptr<block>;
