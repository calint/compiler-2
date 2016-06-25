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

	inline stmt_def_var(statement*parent,up_token tkn,tokenizer&t):statement{parent,move(tkn)}{
		identifier=t.next_token();
		if(!t.is_next_char('='))
			return;
		initial_value=create_statement_from_tokenizer(this,t);
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		identifier->source_to(os);
		if(initial_value){
			os<<"=";
			return initial_value->source_to(os);
		}
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		indent(os,indent_level,true);os<<"var "<<identifier->name()<<"="<<endl;

		tc.framestk().add_var(identifier->name(),"");

		if(initial_value){
			if(initial_value->is_expression()){
				initial_value->set_expression_dest_nasm_identifier(identifier->name());
				initial_value->compile(tc,os,indent_level+1);
			}else{
				indent(os,indent_level,false);
				const char*ra=tc.resolve_ident_to_nasm(*this,identifier->name());
				const char*rb=tc.resolve_ident_to_nasm(*initial_value,initial_value->tok().name());
				os<<"mov "<<ra<<","<<rb<<endl;
			}
		}
	}

private:
	up_token identifier;
	up_statement initial_value;
};
