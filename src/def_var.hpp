#pragma once

#include <stddef.h>
#include <algorithm>
#include <iostream>
#include <memory>

#include "decouple.hpp"
#include "statement.hpp"
#include "stmt_def.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class def_var final:public stmt_def{public:

	inline def_var(toc&tc,statement*parent,unique_ptr<class token>tkn,tokenizer&t)
		:stmt_def{tc,parent,move(tkn),t}
	{
		tc.framestk().current_frame().add_var(*this,identifier->name(),this);

//		if(!t.is_next_char('='))
//			return;
//		initial_value=read_next_statement(tc,this,t);
	}

//	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
//		indent(os,indent_level,true);os<<"var "<<identifier->name()<<"="<<endl;
//
//		tc.framestk().add_alloc_var(identifier->name(),"");
//
//		if(initial_value){
//			if(initial_value->is_expression()){
//				const char*resolv=tc.framestk().resolve_func_arg(identifier->name());
//				initial_value->set_expression_dest_nasm_identifier(resolv);
//				initial_value->compile(tc,os,indent_level+1);
//			}else{
//				indent(os,indent_level,false);
//				const char*ra=tc.framestk().resolve_func_arg(identifier->name());
//				const char*rb=tc.framestk().resolve_func_arg(initial_value->token().name());
//				os<<"mov "<<ra<<","<<rb<<endl;
//			}
//		}
//	}
//
//	inline void source_to(ostream&os)const override{
//		statement::source_to(os);
//		identifier->source_to(os);
//		if(initial_value){
//			os<<"=";
//			return initial_value->source_to(os);
//		}
//	}

//private:
//	unique_ptr<class token>identifier;
//	unique_ptr<statement>initial_value;
};
