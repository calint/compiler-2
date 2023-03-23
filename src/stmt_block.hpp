#pragma once

#include"tokenizer.hpp"
#include"stmt_def_var.hpp"
#include"stmt_comment.hpp"
#include"stmt_break.hpp"
#include"stmt_return.hpp"
#include"stmt_continue.hpp"

class stmt_block final:public statement{
public:
	inline stmt_block(tokenizer&t):
		statement{t.next_whitespace_token()},
		is_one_statement_{not t.is_next_char('{')}
	{
		while(true){
			// comments, semi-colon not considered a statment
			bool last_statement_considered_no_statment{false};
			if(t.is_next_char('}')){
				if(not is_one_statement_)
					break;
				throw compiler_error(t,"unexpected '}' in single statement block");
			}

			token tk{t.next_token()};
			if(tk.is_empty())
				throw compiler_error(tk,"unexpected '"+string{t.peek_char()}+"'");
			
			if(tk.is_name("var")){
				stms_.emplace_back(make_unique<stmt_def_var>(move(tk),t));
			}else if(t.is_next_char('=')){
				stms_.emplace_back(make_unique<stmt_assign_var>(move(tk),t));
			}else if(tk.is_name("break")){
				stms_.emplace_back(make_unique<stmt_break>(move(tk)));
			}else if(tk.is_name("continue")){
				stms_.emplace_back(make_unique<stmt_continue>(move(tk)));
			}else if(tk.is_name("return")){
				stms_.emplace_back(make_unique<stmt_return>(move(tk)));
			}else if(tk.is_name("#")){
				stms_.emplace_back(make_unique<stmt_comment>(move(tk),t));
				last_statement_considered_no_statment=true;
			}else if(tk.is_name("")){ // white space
				stms_.emplace_back(make_unique<statement>(move(tk)));
			}else{ // circular reference resolver
				stms_.emplace_back(create_statement_from_tokenizer(move(tk),{},t));
			}

			if(is_one_statement_&&not last_statement_considered_no_statment)
				break;
		}
	}

	inline stmt_block()=default;
	inline stmt_block(const stmt_block&)=default;
	inline stmt_block(stmt_block&&)=default;
	inline stmt_block&operator=(const stmt_block&)=default;
	inline stmt_block&operator=(stmt_block&&)=default;

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if(not is_one_statement_)
			os<<"{";
		for(const auto&s:stms_)
			s->source_to(os);
		if(not is_one_statement_)
			os<<"}";
	}

	inline void compile(toc&tc,ostream&os,size_t indent,const string&dst="")const override{
		tc.enter_block();
		for(const auto&s:stms_)
			s->compile(tc,os,indent+1);
		tc.exit_block();
	}

	inline bool is_empty()const{
		return stms_.empty();
	}

private:
	bool is_one_statement_{};
	vector<unique_ptr<statement>>stms_;
};
