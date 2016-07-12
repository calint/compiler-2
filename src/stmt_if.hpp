#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "compiler_error.hpp"
#include "statement.hpp"
#include "stmt_block.hpp"
#include "stmt_if_branch.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class stmt_if final:public statement{public:

	inline stmt_if(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk},
		name_{"_if_"+to_string(tk.char_index())}
	{
		while(true){
			conds_.push_back(stmt_if_branch{*this,t});

			token tkn=t.next_token();
			if(!tkn.is_name("else")){
				t.pushback_token(tkn);
				return;
			}

			token tkn2=t.next_token();
			if(!tkn2.is_name("if")){
				t.pushback_token(tkn2);
				else_if_tokens_.push_back(tkn);
				else_code_=make_unique<stmt_block>(*this,t);
				return;
			}

			else_if_tokens_.push_back(tkn);
			else_if_tokens_.push_back(tkn2);
		}

	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		const stmt_if_branch&cond=conds_[0];
		cond.source_to(os);
		const size_t n=conds_.size();
		for(size_t i=1;i<n;i++){
			const stmt_if_branch&c=conds_[i];
			else_if_tokens_[((i-1)<<1)  ].source_to(os);
			else_if_tokens_[((i-1)<<1)+1].source_to(os);
			c.source_to(os);
		}
		if(else_code_){
			else_if_tokens_.back().source_to(os);
			else_code_->source_to(os);
		}
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);os<<"if ["<<tok().char_index()<<"]\n";

		string jump_to_end="_if_end_"+to_string(tok().char_index());
		string jump_to_else="_if_else_"+to_string(tok().char_index());

		const size_t n=conds_.size();
		for(size_t i=0;i<n;i++){
			const auto&e=conds_[i];
			string elsejmp=jump_to_else;
			if(i<(n-1)){
				elsejmp="_if_bgn_"+to_string(conds_[i+1].tok().char_index());
			}
			e.compile(tc,os,indent_level+1,elsejmp+":"+jump_to_end);
		}

		indent(os,indent_level,false);
		os<<jump_to_else<<":\n";
		tc.push_if("else");
		if(else_code_)else_code_->compile(tc,os,indent_level+1);
		tc.pop_if("else");
		indent(os,indent_level,false);os<<jump_to_end<<":\n";
	}

private:
	string name_;
	vector<stmt_if_branch>conds_;
	vector<token>else_if_tokens_;
	unique_ptr<stmt_block>else_code_;
};
