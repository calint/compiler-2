#pragma once

#include"stmt_if_branch.hpp"

class stmt_if final:public statement{
public:
	inline stmt_if(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk}
	{
		while(true){
			branches_.push_back(stmt_if_branch{*this,t});

			token tkn=t.next_token();
			if(!tkn.is_name("else")){
				t.pushback_token(tkn);
				return;
			}

			token tkn2=t.next_token();
			if(!tkn2.is_name("if")){
				// if else ...
				t.pushback_token(tkn2);
				else_if_tokens_.push_back(tkn);
				else_code_=make_unique<stmt_block>(*this,t);
				return;
			}

			else_if_tokens_.push_back(tkn);
			else_if_tokens_.push_back(tkn2);
			// if else if ...
		}

	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		const stmt_if_branch&cond=branches_[0];
		cond.source_to(os);
		const size_t n=branches_.size();
		for(size_t i=1;i<n;i++){
			const stmt_if_branch&c=branches_[i];
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
//		indent(os,indent_level,true);tc.source_to_as_comment(os,*this);
		const string source_loc=tc.source_location(tok());
		string label_after_if="if_"+source_loc+"_end";
		string label_else_branch=else_code_?"if_else_"+source_loc:label_after_if;

		const size_t n=branches_.size();
		for(size_t i=0;i<n;i++){
			const auto&e=branches_[i];
			string jmp_if_false=label_else_branch;
			string jmp_after_if=label_after_if;
			if(i<n-1){ // if branch is false jump to next if
				jmp_if_false=branches_[i+1].if_bgn_label_source_location(tc);
			}else{
				// if last branch and no "else" then don't jump to "after_if", just continue
				if(!else_code_){
					jmp_after_if="";
				}
			}
			e.compile(tc,os,indent_level,jmp_if_false,jmp_after_if);
		}

		if(else_code_){
			indent(os,indent_level);os<<label_else_branch<<":\n";
			tc.push_if("else");
			else_code_->compile(tc,os,indent_level+1);
			tc.pop_if("else");
		}
		indent(os,indent_level);os<<label_after_if<<":\n";
	}

private:
	vector<stmt_if_branch>branches_;
	vector<token>else_if_tokens_;
	unique_ptr<stmt_block>else_code_;
};
