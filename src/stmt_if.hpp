#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "compiler_error.hpp"
#include "def_block.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class stmt_if final:public statement{public:

	inline stmt_if(toc&tc,statement*parent,unique_ptr<class token>tkn,tokenizer&t)
		:statement{tc,parent,move(tkn)}
	{
		if(!t.is_next_char_expression_open())
			throw compiler_error(*this,"if expects '(' followed by boolean expression",tok().name_copy());

		bool_expr=read_next_statement(tc,this,t);

		if(!t.is_next_char_expression_close())
			throw compiler_error(*this,"if expects ')' after the boolean expression",tok().name_copy());

		code=read_next_statement(tc,parent,t);

		name="_if_"+to_string(tok().token_start_char());
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		indent(os,indent_level,true);os<<"if ["<<tok().token_start_char()<<"]\n";

		tc.framestk().push_if(name.data());

		const char*reg=tc.framestk().alloc_scratch_register();
		bool_expr->set_expression_dest_nasm_identifier(reg);
		bool_expr->compile(tc,os,indent_level);

		indent(os,indent_level,false);os<<"cmp "<<reg<<",1\n";
		indent(os,indent_level,false);os<<"jne _end"<<name<<"\n";


		code->compile(tc,os,indent_level+1);

		indent(os,indent_level,false);os<<"_end"<<name<<":\n";

		tc.framestk().pop_if(name.data());
	}

	inline void link(toc&tc,ostream&os)const override final{code->link(tc,os);}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		os<<"(";
			bool_expr->source_to(os);
		os<<")";
		code->source_to(os);
	}


private:
	string name;
	unique_ptr<statement>bool_expr;
	unique_ptr<statement>code;
};
