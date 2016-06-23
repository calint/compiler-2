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

class call_asm_loop final:public statement{public:
	inline call_asm_loop(statement*parent,up_token tkn,tokenizer&t):statement{parent,move(tkn)}{
		if(!t.is_next_char_expression_open())
			throw compiler_error(*this,"loop expects '(' followed by function arguments",token().name_copy());
		while(!t.is_next_char_expression_close())params.push_back(t.next_token());
		code=make_unique<block>(parent,t);
	}
	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		indent(os,indent_level,false);os<<"_loop_"<<token().token_start_char()<<":\n";

		code->compile(tc,os,indent_level+1);

		indent(os,indent_level,false);os<<"jmp _loop_"<<token().token_start_char()<<endl;
	}
	inline void link(toc&tc,ostream&os)override final{code->link(tc,os);}
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
};
