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

class stmt_break final:public statement{public:

	inline stmt_break(statement*parent,up_token tkn,tokenizer&t):statement{parent,move(tkn)}{
		if(!t.is_next_char_expression_open())
			throw compiler_error(*this,"break expects '(' followed by function arguments",token().name_copy());
		while(!t.is_next_char_expression_close())params.push_back(t.next_token());
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		const char*loop_name=tc.framestk().find_parent_loop_name();
		indent(os,indent_level,false);os<<"jmp "<<loop_name<<"_end\n";
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		os<<"(";
		for(auto&s:params)
			s->source_to(os);
		os<<")";
	}


private:
	vup_tokens params;
};
