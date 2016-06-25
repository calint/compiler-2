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
#include"stmt_call.hpp"

class stmt_assign_var final:public statement{public:

	inline stmt_assign_var(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk},
		expr{create_statement_from_tokenizer(*this,t)}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		indent(os,indent_level,true);
		os<<tok().name()<<"="<<expr->tok().name()<<endl;

		if(expr->is_expression()){
			expr->set_dest_to_nasm_ident(tok().name());
			expr->compile(tc,os,indent_level+1);
			return;
		}

		const string&ra=tc.resolve_ident_to_nasm(*this,tok().name());
		const string&rb=tc.resolve_ident_to_nasm(*expr,expr->tok().name());

		if(!ra.find("dword[") and !rb.find("dword[")){
			const string&r=tc.framestk().alloc_scratch_register();
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

//			const string&r=tc.framestk().alloc_scratch_register();
//			indent(os,indent_level,false);
//			os<<"mov "<<r<<","<<rb<<endl;
//			indent(os,indent_level,false);
//			os<<"mov "<<ra<<","<<r<<endl;
//			tc.framestk().free_scratch_reg(r);
//			return;
		}

		indent(os,indent_level,false);
		os<<"mov "<<ra<<","<<rb<<endl;
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		os<<"=";
		if(expr)expr->source_to(os);
	}

private:
	up_statement expr;
};
