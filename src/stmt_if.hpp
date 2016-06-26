#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "compiler_error.hpp"
#include "statement.hpp"
#include "stmt_block.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class stmt_if final:public statement{public:

	inline stmt_if(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk}
	{
		if(!t.is_next_char('('))
			throw compiler_error(*this,"if expects '(' followed by boolean expression",tok().name_copy());

		bool_expr=create_statement_from_tokenizer(*this,t);

		if(!t.is_next_char(')'))
			throw compiler_error(*this,"if expects ')' after the boolean expression",tok().name_copy());

		code=make_unique<stmt_block>(parent,t);

		name="_if_"+to_string(tk.char_index_in_source());
	}



	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		indent(os,indent_level,true);os<<"if ["<<tok().char_index_in_source()<<"]\n";

		tc.framestk().push_if(name.data());

		const string&reg=tc.framestk().alloc_scratch_register(token());
		bool_expr->set_dest_nasm_ident(reg);
		bool_expr->compile(tc,os,indent_level);

		indent(os,indent_level,false);os<<"cmp "<<reg<<",1\n";
		indent(os,indent_level,false);os<<"jne _end_"<<name<<"\n";


		code->compile(tc,os,indent_level+1);

		indent(os,indent_level,false);os<<"_end_"<<name<<":\n";

		tc.framestk().pop_if(name.data());
	}



	inline void link(toc&tc,ostream&os)const override{
		code->link(tc,os);
	}



	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		os<<"(";
			bool_expr->source_to(os);
		os<<")";
		code->source_to(os);
	}


private:

	string name;

	up_statement bool_expr;

	unique_ptr<stmt_block>code;

};
