#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

#include"compiler_error.hpp"
#include"decouple.hpp"
#include"tokenizer.hpp"
#include"token.hpp"
#include"statement.hpp"
#include"stmt_call.hpp"
#include"stmt_def_var.hpp"
#include"stmt_assign_var.hpp"
#include"stmt_comment.hpp"
#include"stmt_semicolon.hpp"

class stmt_block final:public statement{public:

	inline stmt_block(const statement&parent,tokenizer&t):
		statement{parent,token{}},
		is_one_statement_{not t.is_next_char('{')}
	{
		while(true){
			if(t.is_eos())
				break;

			if(t.is_next_char('}')){
				if(not is_one_statement_)
					break;
				throw compiler_error(*this,"unexpected '}' in single statement block");
			}

			token tk=t.next_token();

			if(tk.is_blank()){
				if(t.is_next_char(';')){
					statements_.push_back(make_unique<stmt_semicolon>(*this,move(tk),t));
					continue;
				}
				throw compiler_error(tk,"unexpected end of string");
			}
			if(tk.is_name("var")){
				statements_.push_back(make_unique<stmt_def_var>(*this,move(tk),t));
			}else if(t.is_next_char('=')){
				statements_.push_back(make_unique<stmt_assign_var>(*this,move(tk),t));
			}else if(tk.is_name("#")){
				statements_.push_back(make_unique<stmt_comment>(*this,move(tk),t));
			}else if(tk.is_name("")){
				statements_.push_back(make_unique<statement>(*this,move(tk)));
			}else{
				statements_.push_back(create_call_statement_from_tokenizer(*this,move(tk),t));
			}

			if(is_one_statement_)
				break;
		}
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if(!is_one_statement_)os<<"{";
		for(auto&s:statements_)
			s->source_to(os);
		if(!is_one_statement_)os<<"}";
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		for(auto&s:statements_)
			s->compile(tc,os,indent_level+1);
	}

//	inline void link(toc&tc,ostream&os)const override{for(auto&s:statements_)s->link(tc,os);}

private:
	bool is_one_statement_{false};
	vector<up_statement>statements_;
};
