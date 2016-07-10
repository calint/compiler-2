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
		name_{"_if_"+to_string(tk.char_index())},
		ch1_{t.next_char()},
		if_branch_{*this,t},
		if_true_code_{*this,t}
	{
		token tkn=t.next_token();
		if(!tkn.is_name("else")){
			t.pushback_token(tkn);
			return;
		}
		else_token_=tkn;

		token tkn2=t.next_token();
		if(!tkn2.is_name("if")){
			t.pushback_token(tkn2);
			else_=make_unique<stmt_block>(*this,t);
			return;
		}

//		else_token_=tkn;
		else_if_=make_unique<stmt_if>(*this,tkn2,t);

	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if_branch_.source_to(os);
		if_true_code_.source_to(os);
		if(else_if_){
			else_token_.source_to(os);
			else_if_->source_to(os);
			return;
		}
		if(else_){
			else_token_.source_to(os);
			else_->source_to(os);
		}
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);os<<"if ["<<tok().char_index()<<"]\n";

		string jump_to_end="_end"+name_;
		string jump_to_else="_else"+name_;

		if_branch_.compile(tc,os,indent_level,jump_to_else);

		tc.push_if(name_.c_str());

		if_true_code_.compile(tc,os,indent_level+1);

		indent(os,indent_level+1,false);os<<"jmp "<<jump_to_end<<endl;

		tc.pop_if(name_.c_str());

		indent(os,indent_level,false);os<<jump_to_else<<":\n";

		if(else_if_)else_if_->compile(tc,os,indent_level+1);

		if(else_)else_->compile(tc,os,indent_level+1);

		indent(os,indent_level,false);os<<jump_to_end<<":\n";
	}

private:
	string name_;
	char ch1_{0};
	stmt_if_branch if_branch_;
	stmt_block if_true_code_;
	unique_ptr<stmt_if>else_if_;
	unique_ptr<stmt_block>else_;
	token else_token_;
};
