#pragma once

#include <algorithm>
#include <iostream>

#include "call.hpp"
#include "statement.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_mov final:public call{public:
	inline call_asm_mov(statement*parent,up_token tkn,tokenizer&t):call{parent,move(tkn),t}{}
	inline void compile(toc&tc,ostream&os,size_t indent_level)override{// mov(eax 1)
//		indent(os,indent_level,true);source_to(os);os<<endl;
		indent(os,indent_level);
		framestack&fs=tc.framestk();
		const char*reg{nullptr};
		if(argument(1).is_expression()){
			reg=tc.framestk().alloc_scratch_register();
			argument(1).set_expression_dest_nasm_identifier(reg);
			argument(1).compile(tc,os,indent_level);
			os<<"mov "<<fs.resolve_argument(argument(0).token().name())<<","<<reg<<endl;
			tc.framestk().free_scratch_reg(reg);
			return;
		}
		os<<"mov "<<fs.resolve_argument(argument(0).token().name())<<","<<fs.resolve_argument(argument(1).token().name())<<endl;
	}
};
