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

	inline stmt_assign_var(statement*parent,unique_ptr<class token>tkn,tokenizer&t):statement{parent,move(tkn)}{
		expr=read_next_statement(this,t);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		indent(os,indent_level,true);os<<token().name()<<"="<<endl;
		if(expr->is_expression()){
			expr->set_expression_dest_nasm_identifier(token().name());
			expr->compile(tc,os,indent_level+1);
			return;
		}
		indent(os,indent_level,false);
		const char*ra=tc.framestk().resolve_func_arg(token().name());
		const char*rb=tc.framestk().resolve_func_arg(expr->token().name());
		os<<"mov "<<ra<<","<<rb<<endl;
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		os<<"=";
		if(expr)expr->source_to(os);
	}

private:
	unique_ptr<statement>expr;
};
