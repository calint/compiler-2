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

	inline stmt_def_var(const statement&parent,const token&tkn,tokenizer&t):
		statement{parent,tkn},
		ident_{t.next_token()}
	{
		if(!t.is_next_char('='))
			return;
		initial_expression_=create_statement_from_tokenizer(*this,t);
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		ident_.source_to(os);
		if(initial_expression_){
			os<<"=";
			return initial_expression_->source_to(os);
		}
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		indent(os,indent_level,true);
		os<<"var "<<ident_.name()<<"="<<endl;

		tc.framestk().add_var(ident_.name(),"");

		if(initial_expression_){
			if(initial_expression_->is_expression()){
				initial_expression_->set_expression_dest_nasm_identifier(ident_.name());
				initial_expression_->compile(tc,os,indent_level+1);
			}else{
				indent(os,indent_level,false);
				const string&ra=tc.resolve_ident_to_nasm(*this,ident_.name());
				const string&rb=tc.resolve_ident_to_nasm(*initial_expression_,initial_expression_->tok().name());
				os<<"mov "<<ra<<","<<rb<<endl;
			}
		}
	}

private:
	token ident_;
	up_statement initial_expression_;
};
