#pragma once

#include <stddef.h>
#include <algorithm>
#include <iostream>
#include <memory>

#include "decouple.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include "stmt_assign_var.hpp"

class stmt_def_var final:public statement{public:

	inline stmt_def_var(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk},
		ident_{t.next_token()},
		op_{t.next_char()},
		assign_initial_value_{*this,ident_,t}
	{}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
//		ident_.source_to(os);
//		os<<"=";
		assign_initial_value_.source_to(os);
//		if(not initial_expression_->is_empty()){
//			os<<"=";
//			return initial_expression_->source_to(os);
//		}
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);
		os<<"var ";
		tc.source_location_to_stream(os,ident_);
		os<<endl;

		tc.add_var(ident_.name(),"");

		assign_initial_value_.compile(tc,os,indent_level+1,ident_.name());

//		if(initial_expression_->is_empty())
//			return;
//
//		if(initial_expression_->is_expression()){
//			initial_expression_->compile(tc,os,indent_level+1,ident_.name());
//			return;
//		}
//
//		const string&ra=tc.resolve_ident_to_nasm(*this,ident_.name());
//		const string&rb=tc.resolve_ident_to_nasm(*initial_expression_,initial_expression_->tok().name());
//
//		if(ra==rb)
//			return;
//
//		if(!ra.find("dword[") and !rb.find("dword[")){
//			const string&r=tc.alloc_scratch_register(token());
//			indent(os,indent_level,false);
//			os<<"mov "<<r<<","<<rb<<"  ;  ";
//			tc.source_location_to_stream(os,tok());
//			os<<endl;
//			indent(os,indent_level,false);
//			os<<"mov "<<ra<<","<<r<<"  ;  ";
//			tc.source_location_to_stream(os,tok());
//			os<<endl;
//			tc.free_scratch_reg(r);
//			return;
//		}
//
//		indent(os,indent_level,false);
//		os<<"mov "<<ra<<","<<rb<<endl;
	}

private:
	token ident_;
//	up_statement initial_expression_;
	char op_{0};
	stmt_assign_var assign_initial_value_;
};
