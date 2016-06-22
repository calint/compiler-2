#pragma once

#include <stddef.h>
#include <algorithm>
#include <iostream>
#include <memory>

#include "call.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class var final:public statement{public:
	inline var(statement*parent,up_token tkn,tokenizer&t):statement{parent,move(tkn)}{
		identifier=t.next_token();
//		cout<<identifier->name()<<endl;
		if(t.is_next_char('=')){
			up_token fnm=t.next_token();
			initial_value=create_call(fnm->name(),this,move(fnm),t);
		}
		else throw"uninitialized var";
	}
	inline void compile(toc&tc,ostream&os,size_t indent_level)override{
		indent(os,indent_level,true);os<<"var "<<identifier->name()<<"="<<endl;

		tc.stack_add_var(identifier->name());
		initial_value->set_expression_dest_nasm_identifier(identifier->name());
		if(initial_value)initial_value->compile(tc,os,indent_level+1);
	}
	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		identifier->source_to(os);
		os<<"=";
		if(initial_value)initial_value->source_to(os);
	}

private:
	up_token identifier;
	up_statement initial_value;
};
