#pragma once

#include"stmt_if_branch.hpp"

class stmt_if final:public statement{
public:
	inline stmt_if(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk}
	{
		// if a=b {x} else if c=d {y} else {z}
		// 'a=b {x}', 'c=d {y}' are 'branches'
		// 'if' token has been read
		while(true){
			// read branch i.e. a=b {x}
			branches_.emplace_back(*this,t);

			// check if it is a 'else if' or 'else' or a new statement
			token tkn=t.next_token();
			if(!tkn.is_name("else")){
				// not 'else', push token back instream and exit
				t.pushback_token(tkn);
				return;
			}
			// is 'else'
			// check if 'else if'
			token tkn2=t.next_token();
			if(!tkn2.is_name("if")){
				// not 'else if', push token back instream and exit
				t.pushback_token(tkn2);
				// 'else' branch
				// save tokens to be able to reproduce the source
				else_if_tokens_.push_back(tkn);
				// read the 'else' code
				else_code_=make_unique<stmt_block>(*this,t);
				return;
			}
			// 'else if': continue reading if branches
			// save tokens to be able to reproduce the source
			else_if_tokens_.push_back(tkn);
			else_if_tokens_.push_back(tkn2);
		}
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		// output first branch
		const stmt_if_branch&branch=branches_[0];
		branch.source_to(os);
		// output the remaining 'else if' branches
		const size_t n=branches_.size();
		for(size_t i=1;i<n;i++){
			const stmt_if_branch&br=branches_[i];
			// 'else if' tokens as read from source
			else_if_tokens_[((i-1)<<1)  ].source_to(os);
			else_if_tokens_[((i-1)<<1)+1].source_to(os);
			br.source_to(os);
		}
		// the 'else' code
		if(else_code_){
			else_if_tokens_.back().source_to(os);
			else_code_->source_to(os);
		}
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		// make unique labels considering in-lined functions
		const string call_path=tc.get_call_path(tok());
		const string src_loc=tc.source_location(tok());
		const string cp=call_path.empty()?"":"_"+call_path;

		string label_after_if="if_"+src_loc+cp+"_end";
		string label_else_branch=else_code_?"if_else_"+src_loc+cp:label_after_if;

		const size_t n=branches_.size();
		for(size_t i=0;i<n;i++){
			const auto&e=branches_[i];
			string jmp_if_false=label_else_branch;
			string jmp_after_if=label_after_if;
			if(i<n-1){
				// if branch is false jump to next if
				jmp_if_false=branches_[i+1].if_bgn_label(tc);
			}else{
				// if last branch and no 'else' then
				//   no need to jump to 'after_if' after the code for the branch
				//   has been executed. just continue
				if(!else_code_){
					jmp_after_if="";
				}
			}
			e.compile(tc,os,indent_level,jmp_if_false,jmp_after_if);
		}
		if(else_code_){
			tc.asm_label(*this,os,indent_level,label_else_branch);
//			toc::indent(os,indent_level);os<<label_else_branch<<":\n";
//			tc.push_if("else");
			else_code_->compile(tc,os,indent_level+1);
//			tc.pop_if("else");
		}
		tc.asm_label(*this,os,indent_level,label_after_if);
//		toc::indent(os,indent_level);os<<label_after_if<<":\n";
	}

private:
	vector<stmt_if_branch>branches_;
	vector<token>else_if_tokens_;
	unique_ptr<stmt_block>else_code_;
};
