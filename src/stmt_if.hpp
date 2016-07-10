#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "compiler_error.hpp"
#include "statement.hpp"
#include "stmt_block.hpp"
#include "stmt_if_branch.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class stmt_if final:public statement{public:

	inline stmt_if(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk},
		name{"_if_"+to_string(tk.char_index())},
		ch1{t.next_char()},
		if_branch{*this,t},
		if_true_code{*this,t}
	{}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if_branch.source_to(os);
		if_true_code.source_to(os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);os<<"if ["<<tok().char_index()<<"]\n";

		string jump_if_false="_end"+name;

		if_branch.compile(tc,os,indent_level,jump_if_false);

		tc.push_if(name.data());

		if_true_code.compile(tc,os,indent_level+1);

		indent(os,indent_level,false);os<<jump_if_false<<":\n";

		tc.pop_if(name.data());
	}

private:
	string name;
	char ch1{0};
	stmt_if_branch if_branch;
	stmt_block if_true_code;
	//elseifs
	//else
};
