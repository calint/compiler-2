#pragma once

#include <algorithm>
#include <iostream>

#include "statement.hpp"
#include "stmt_call.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_cmove final:public stmt_call{public:

	inline call_asm_cmove(statement*parent,up_token tkn,tokenizer&t):stmt_call{parent,move(tkn),t}{}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{// mov(eax 1)
//		indent(os,indent_level,true);source_to(os);os<<endl;
		indent(os,indent_level);
		framestack&fs=tc.framestk();
		const char*reg{nullptr};
		if(argument(1).is_expression()){
			reg=tc.framestk().alloc_scratch_register();
			argument(1).set_expression_dest_nasm_identifier(reg);
			argument(1).compile(tc,os,indent_level);
			os<<"cmove "<<fs.resolve_func_arg(argument(0).token().name())<<","<<reg<<endl;
			tc.framestk().free_scratch_reg(reg);
			return;
		}
		os<<"cmove "<<fs.resolve_func_arg(argument(0).token().name())<<",["<<fs.resolve_func_arg(argument(1).token().name())<<"]"<<endl;
	}

};
