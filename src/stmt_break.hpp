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

	inline stmt_break(statement*parent,unique_ptr<class token>tkn,tokenizer&t):stmt_call{parent,move(tkn),t}{}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		//? break(2)
		const char*loop_name=tc.framestk().find_parent_loop_name();
		indent(os,indent_level,false);os<<"jmp _end"<<loop_name<<"\n";
	}

};
