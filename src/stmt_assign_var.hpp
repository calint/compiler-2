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

	inline stmt_assign_var(statement&parent,const token&tkn,tokenizer&t):
		statement{parent,tkn}
	{
		token tk=t.next_token();
		if(!t.is_peek_char('(')){
			expr=make_unique<statement>(parent,move(tk));// ie  0x80
			return;
		}
		expr=create_call_statement_from_tokenizer(tk.name(),*this,tk,t); // ie  f(...)
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		indent(os,indent_level,true);os<<tok().name()<<"="<<endl;
		if(expr->is_expression()){
			expr->set_expression_dest_nasm_identifier(tok().name());
			expr->compile(tc,os,indent_level+1);
			return;
		}
		indent(os,indent_level,false);
		const string&ra=tc.resolve_ident_to_nasm(*this,tok().name());
		const string&rb=tc.resolve_ident_to_nasm(*expr,expr->tok().name());
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
