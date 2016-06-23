#pragma once

#include <algorithm>
#include <iostream>

#include "statement.hpp"
#include "stmt_call.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_mov final:public stmt_call{public:
	inline call_asm_mov(statement*parent,up_token tkn,tokenizer&t):stmt_call{parent,move(tkn),t}{}
	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{// mov(eax 1)
		framestack&fs=tc.framestk();
		const char*reg{nullptr};
		if(argument(1).is_expression()){
			reg=tc.framestk().alloc_scratch_register();
			argument(1).set_expression_dest_nasm_identifier(reg);
			argument(1).compile(tc,os,indent_level);
			const char*ra=fs.resolve_func_arg(argument(0).token().name());
			if(!strcmp(ra,reg)){// ie  mov eax,eax
				return;
			}
			indent(os,indent_level);
			os<<"mov "<<ra<<","<<reg<<endl;
			tc.framestk().free_scratch_reg(reg);
			return;
		}
		const char*ra=fs.resolve_func_arg(argument(0).token().name());
		const char*rb=fs.resolve_func_arg(argument(1).token().name());
		if(!strcmp(ra,rb)){// ie  mov eax,eax
			return;
		}
		indent(os,indent_level);
		os<<"mov "<<ra<<","<<rb<<endl;
	}
};
