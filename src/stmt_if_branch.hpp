#pragma once

#include"stmt_if_bool_op.hpp"
#include "stmt_if_bool_ops_list.hpp"

class stmt_if_branch final:public statement{
public:
	inline stmt_if_branch(const statement&parent,tokenizer&t):
		statement{parent,t.next_whitespace_token()},
		bol_{*this,t},
		code_{make_unique<stmt_block>(*this,t)}
	{}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		bol_.source_to(os);
		code_->source_to(os);
	}

	// returns the label where the if branch begins evaluating the boolean expression
	inline string if_bgn_label(const toc&tc)const{
		const string&call_path=tc.get_call_path(tok());
		return "if_"+tc.source_location(tok())+(call_path.empty()?"":"_"+call_path);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dst)const override{
		throw compiler_error(tok(),"this code should not be reached");
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&jmp_to_if_false_label,const string&jmp_to_after_code_label)const{
		const string if_bgn_lbl=if_bgn_label(tc);
		const string jmp_to_if_true_lbl=if_bgn_lbl+"_code";
		// the begining of this branch
		indent(os,indent_level);os<<if_bgn_lbl<<":"<<endl;
		// compile boolean ops list
		bol_.compile(tc,os,indent_level,jmp_to_if_false_label,jmp_to_if_true_lbl);
		// the label where to jump if evaluation of boolean ops is true
		indent(os,indent_level);os<<jmp_to_if_true_lbl<<":"<<endl;
		// the code of the branch
		code_->compile(tc,os,indent_level);
		// after the code of the branch is executed jump to the end of
		//   the 'if ... else if ... else ...'
		//   if label not provided then there is no 'else' and this is the last 'if'
		//   so just continue execution
		if(!jmp_to_after_code_label.empty()){
			indent(os,indent_level+1);os<<"jmp "<<jmp_to_after_code_label<<endl;
		}
	}

private:
	stmt_if_bool_ops_list bol_;
	unique_ptr<stmt_block>code_;
};
