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
#include"expr_ops_list_bool.hpp"

class stmt_if final:public statement{public:

	inline stmt_if(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk},
		name{"_if_"+to_string(tk.char_index())},
		ch1{t.next_char()},
		bool_expr{*this,t,false,ch1=='('},
		code{*this,t}
	{
//		if(!t.is_next_char('('))
//			throw compiler_error(*this,"if expects '(' followed by boolean expression",tok().name_copy());
//
//		bool_expr=create_statement_from_tokenizer(*this,t);
//
//		if(!t.is_next_char(')'))
//			throw compiler_error(*this,"if expects ')' after the boolean expression",tok().name_copy());
//
//		code=make_unique<stmt_block>(parent,t);

//		name="_if_"+to_string(tk.char_index());
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
//		os<<"(";
		bool_expr.source_to(os);
//		os<<")";
		code.source_to(os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);os<<"if ["<<tok().char_index()<<"]\n";


//		const string&reg=tc.alloc_scratch_register(token());
		string jump_if_false="_end"+name;
		bool_expr.compile(tc,os,indent_level,jump_if_false);
//
//		indent(os,indent_level,false);os<<"cmp "<<reg<<",1\n";
//		indent(os,indent_level,false);os<<"jne _end_"<<name<<"\n";

		tc.push_if(name.data());

		code.compile(tc,os,indent_level+1);

		indent(os,indent_level,false);os<<jump_if_false<<":\n";

		tc.pop_if(name.data());
	}


//
//	inline void link(toc&tc,ostream&os)const override{
//		code->link(tc,os);
//	}
//

private:

	string name;

	char ch1{0};

	expr_ops_list_bool bool_expr;

	stmt_block code;

};
