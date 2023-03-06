#pragma once

#include"compiler_error.hpp"
#include"decouple.hpp"
#include"expression.hpp"
#include"toc.hpp"
#include"token.hpp"
#include"tokenizer.hpp"
#include"stmt_if_bool_op.hpp"
#include"stmt_if_bool_op_list.hpp"

class stmt_if_branch final:public statement{public:

	inline stmt_if_branch(const statement&parent,tokenizer&t):
		statement{parent,t.next_whitespace_token()},
		bol_{*this,t}
	{
		code_=make_unique<stmt_block>(*this,t);
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		bol_.source_to(os);
		code_->source_to(os);
	}

	inline string if_bgn_label_source_location(const toc&tc)const{
		return "if_"+tc.source_location(tok());
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dst)const override{
		throw compiler_error(tok(),"this code should not be reached");
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&jmp_to_if_false_label,const string&jmp_to_after_code_label)const{
		const string if_bgn_label=if_bgn_label_source_location(tc);
		const string jmp_to_if_true_label=if_bgn_label+"_code";

		indent(os,indent_level,false);os<<if_bgn_label<<":"<<endl;

		bol_.compile(tc, os, indent_level,jmp_to_if_false_label,jmp_to_if_true_label,true);

		indent(os,indent_level,false);os<<jmp_to_if_true_label<<":"<<endl;

		code_->compile(tc,os,indent_level);

		if(jmp_to_after_code_label!=""){
			// jump to code after if else block
			indent(os,indent_level+1,false);os<<"jmp "<<jmp_to_after_code_label<<endl;
		}
	}

private:
	stmt_if_bool_op_list bol_;
	unique_ptr<stmt_block>code_;
};
