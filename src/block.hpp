#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

#include "call.hpp"
#include "compiler_error.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include"var.hpp"

using vup_statement=vector<up_statement>;
class block final:public statement{public:
	inline block(statement*parent,tokenizer&t):statement{parent,t.next_token()}{
		assert(t.is_next_char_block_open());
		while(true){
			if(t.is_eos())throw compiler_error(*this,"unexpected end of string",parent->token().name_copy());
			if(t.is_next_char_block_close())break;
			up_token tkn=t.next_token();
			if(tkn->is_name("")){
				statements.push_back(make_unique<statement>(parent,move(tkn)));
				continue;
			}
			if(tkn->is_name("var")){
				statements.push_back(make_unique<var>(parent,move(tkn),t));
				continue;
			}
			statements.push_back(create_call(tkn->name(),parent,move(tkn),t));
		}
	}
	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		for(auto&s:statements)
			s->compile(tc,os,indent_level+1);
	}
	inline void link(toc&tc,ostream&os)const override{for(auto&s:statements)s->link(tc,os);}
	inline void source_to(ostream&os)const override{statement::source_to(os);os<<"{";for(auto&s:statements)s->source_to(os);os<<"}";}

private:
	vup_statement statements;
};
using up_block=unique_ptr<block>;
