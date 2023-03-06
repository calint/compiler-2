#pragma once

#include"compiler_error.hpp"
#include"statement.hpp"
#include"stmt_block.hpp"
#include"toc.hpp"
#include"token.hpp"
#include"tokenizer.hpp"

class stmt_break final:public stmt_call{public:

	inline stmt_break(const statement&parent,const token&tk,tokenizer&t):
		stmt_call{parent,tk,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.source_to_as_comment(os,*this);
		const string&loop_name=tc.find_parent_loop_name();
		indent(os,indent_level,false);os<<"jmp "<<loop_name<<"_end\n";
	}
};
