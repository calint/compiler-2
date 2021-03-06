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
#include "stmt_assign_var.hpp"

class stmt_def_var final:public statement{public:

	inline stmt_def_var(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk},
		ident_{t.next_token()},
		op_{t.next_char()},
		initial_value_{*this,ident_,t}
	{
		if(op_!='=')
			throw compiler_error(ident_,"expected '=' and initializer");
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		initial_value_.source_to(os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);
		os<<"var ";
		tc.source_location_to_stream(os,ident_);
		os<<endl;

		tc.add_var(ident_.name());

		initial_value_.compile(tc,os,indent_level+1,ident_.name());
	}

private:
	token ident_;
	char op_{0};
	stmt_assign_var initial_value_;
};
