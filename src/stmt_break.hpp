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

class stmt_break final:public stmt_call{public:

	inline stmt_break(const statement&parent,const token&tk,tokenizer&t):
		stmt_call{parent,tk,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		//? break(2)
		const string&loop_name=tc.find_parent_loop_name();
		indent(os,indent_level,false);os<<"jmp _end"<<loop_name<<"\n";
	}

};
