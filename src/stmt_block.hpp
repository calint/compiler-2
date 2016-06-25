#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

#include "compiler_error.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "stmt_call.hpp"
#include "stmt_def_var.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include"stmt_assign_var.hpp"
#include"stmt_comment.hpp"

using vup_statement=vector<up_statement>;
class stmt_block final:public statement{public:

	inline stmt_block(statement*parent,tokenizer&t):
		statement{parent,token{}}
	{
		if(!t.is_next_char('{'))
			is_one_statement_=true;

		while(true){
			if(t.is_eos())
				break;

			if(not is_one_statement_ and t.is_next_char('}'))
				break;

			token tkn=t.next_token();

			if(tkn.is_blank())
				throw compiler_error(tkn,"unexpected end of string");

			if(tkn.is_name("var")){
				statements_.push_back(make_unique<stmt_def_var>(this,tkn,t));
			}else
			if(t.is_next_char('=')){// assign  ie   a=0x80
				statements_.push_back(make_unique<stmt_assign_var>(parent,tkn,t));
			}else
			if(tkn.is_name("//")){
				statements_.push_back(make_unique<stmt_comment>(parent,tkn,t));
			}else
			if(tkn.is_name("")){
				statements_.push_back(make_unique<statement>(parent,tkn));
			}else{
				statements_.push_back(create_call_statement_from_tokenizer(tkn.name(),this,tkn,t));
			}

			if(is_one_statement_)
				break;
		}
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		for(auto&s:statements_)
			s->compile(tc,os,indent_level+1);
	}

	inline void link(toc&tc,ostream&os)const override{for(auto&s:statements_)s->link(tc,os);}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if(!is_one_statement_)os<<"{";
		for(auto&s:statements_)
			s->source_to(os);
		if(!is_one_statement_)os<<"}";
	}

private:
	vup_statement statements_;
	bool is_one_statement_{false};
};
using up_stmt_block=unique_ptr<stmt_block>;
