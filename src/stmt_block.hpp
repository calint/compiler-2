#pragma once

#include"tokenizer.hpp"
#include"stmt_semicolon.hpp"
#include"stmt_def_var.hpp"
#include"stmt_comment.hpp"

class stmt_block final:public statement{
public:
	inline stmt_block(const statement&parent,tokenizer&t):
		statement{parent,token{}},
		is_one_statement_{not t.is_next_char('{')}
	{
		while(true){
			bool last_statement_considered_no_statment=false;
			if(t.is_eos())
				break;

			if(t.is_next_char('}')){
				if(not is_one_statement_)
					break;
				throw compiler_error(*this,"unexpected '}' in single statement block");
			}

			token tk=t.next_token();

			if(tk.is_blank()){
				if(t.is_next_char(';')){ // in-case ; is used
					stmts_.push_back(make_unique<stmt_semicolon>(*this,move(tk),t));
					last_statement_considered_no_statment=true;
					continue;
				}
				throw compiler_error(tk,"unexpected '"+string{t.peek_char()}+"'");
			}
			if(tk.is_name("var")){
				stmts_.push_back(make_unique<stmt_def_var>(*this,move(tk),t));
			}else if(t.is_next_char('=')){
				stmts_.push_back(make_unique<stmt_assign_var>(*this,move(tk),t));
			}else if(tk.is_name("#")){
				stmts_.push_back(make_unique<stmt_comment>(*this,move(tk),t));
				last_statement_considered_no_statment=true;
			}else if(tk.is_name("")){
				stmts_.push_back(make_unique<statement>(*this,move(tk)));
			}else{
				stmts_.push_back(create_call_statement_from_tokenizer(*this,move(tk),t));
			}

			if(is_one_statement_&&!last_statement_considered_no_statment)
				break;
		}
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if(!is_one_statement_)os<<"{";
		for(const auto&s:stmts_)
			s->source_to(os);
		if(!is_one_statement_)os<<"}";
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		for(const auto&s:stmts_)
			s->compile(tc,os,indent_level+1);
	}

private:
	bool is_one_statement_{false};
	vector<unique_ptr<statement>>stmts_;
};
