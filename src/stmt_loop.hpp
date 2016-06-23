#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "block.hpp"
#include "compiler_error.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class stmt_loop final:public statement{public:

	inline stmt_loop(statement*parent,up_token tkn,tokenizer&t):statement{parent,move(tkn)}{
		if(!t.is_next_char_expression_open())
			throw compiler_error(*this,"loop expects '(' followed by function arguments",token().name_copy());
		while(!t.is_next_char_expression_close())params.push_back(t.next_token());
		code=make_unique<block>(parent,t);
		name="_loop_"+to_string(token().token_start_char());
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		indent(os,indent_level,false);
		os<<name<<":\n";

		tc.framestk().push_loop(name.data());

		code->compile(tc,os,indent_level+1);
		indent(os,indent_level,false);os<<"jmp _loop_"<<token().token_start_char()<<endl;
		indent(os,indent_level,false);os<<"_loop_"<<token().token_start_char()<<"_end:\n";

		tc.framestk().pop_loop(name.data());
	}

	inline void link(toc&tc,ostream&os)const override final{code->link(tc,os);}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		os<<"(";
		for(auto&s:params)
			s->source_to(os);
		os<<")";
		code->source_to(os);
	}


private:
	vup_tokens params;
	up_block code;
	string name;
};
