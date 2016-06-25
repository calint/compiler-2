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

	inline stmt_block(statement*parent,tokenizer&t):statement{parent,unique_ptr<class token>(new class token())}{
		if(!t.is_next_char('{'))
			is_one_statement=true;

		while(true){
			if(not is_one_statement and t.is_next_char('}'))
				break;

			up_token tkn=t.next_token();

			if(tkn->is_blank() and t.is_eos())
				break;

			if(tkn->is_blank())
				throw compiler_error(*this,"unexpected end of string",parent->tok().name_copy());

			if(tkn->is_name("var")){
				statements.push_back(make_unique<stmt_def_var>(parent,move(tkn),t));
				if(is_one_statement)break;
				continue;
			}

			if(t.is_next_char('=')){// assign  ie   a=0x80
				statements.push_back(make_unique<stmt_assign_var>(parent,move(tkn),t));
				if(is_one_statement)break;
				continue;
			}

			if(tkn->is_name("//")){
				statements.push_back(make_unique<stmt_comment>(parent,move(tkn),t));
				if(is_one_statement)break;
				continue;
			}

			if(tkn->is_blank())
				throw compiler_error(*tkn,"unexpected character",ua_char(new char[2]{t.peek_char(),0}));

			if(tkn->is_name("")){
				statements.push_back(make_unique<statement>(parent,move(tkn)));
				if(is_one_statement)break;//? throw
				continue;
			}

			// assume call
			statements.push_back(create_call_statement_from_tokenizer(tkn->name(),parent,move(tkn),t));
			if(is_one_statement)break;
		}
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		for(auto&s:statements)
			s->compile(tc,os,indent_level+1);
	}

	inline void link(toc&tc,ostream&os)const override{for(auto&s:statements)s->link(tc,os);}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if(!is_one_statement)os<<"{";
		for(auto&s:statements)
			s->source_to(os);
		if(!is_one_statement)os<<"}";
	}

private:
	vup_statement statements;
	bool is_one_statement{false};
};
using up_block=unique_ptr<stmt_block>;
