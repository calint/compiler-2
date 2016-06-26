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

class stmt_def_var final:public statement{public:

	inline stmt_def_var(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk},
		ident_{t.next_token()},
		initial_expression_{t.is_next_char('=')?
				create_statement_from_tokenizer(*this,t)  :  make_unique<statement>(*this,token{})
		}
	{}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		ident_.source_to(os);
		if(not initial_expression_->is_empty()){
			os<<"=";
			return initial_expression_->source_to(os);
		}
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		indent(os,indent_level,true);
		os<<"var "<<ident_.name()<<"="<<endl;

		tc.framestk().add_var(ident_.name(),"");

		if(initial_expression_->is_empty())
			return;

		if(initial_expression_->is_expression()){
			initial_expression_->set_dest_nasm_ident(ident_.name());
			initial_expression_->compile(tc,os,indent_level+1);
			return;
		}

		const string&ra=tc.resolve_ident_to_nasm(*this,ident_.name());
		const string&rb=tc.resolve_ident_to_nasm(*initial_expression_,initial_expression_->tok().name());

		if(ra==rb)
			return;

		if(!ra.find("dword[") and !rb.find("dword[")){
			const string&r=tc.framestk().alloc_scratch_register(token());
			indent(os,indent_level,false);
			os<<"mov "<<r<<","<<rb<<"  ;  ";
			tc.source_location_to_stream(os,tok());
			os<<endl;
			indent(os,indent_level,false);
			os<<"mov "<<ra<<","<<r<<"  ;  ";
			tc.source_location_to_stream(os,tok());
			os<<endl;
			tc.framestk().free_scratch_reg(r);
			return;
		}

		indent(os,indent_level,false);
		os<<"mov "<<ra<<","<<rb<<endl;
	}

private:
	token ident_;
	up_statement initial_expression_;
};
