#pragma once

#include <algorithm>
#include <iostream>

#include "call.hpp"
#include "statement.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_mov final:public call{public:
	inline call_asm_mov(statement*parent,up_token tkn,tokenizer&t):call{parent,move(tkn),t}{}
	inline void compile(toc&tc,ostream&os)override{// mov(eax 1)
		const statement&dst=argument(0);
		const statement&src=argument(1);

		const char*alias_dst=tc.stack_getalias(dst.token().name());
		const char*alias_src=tc.stack_getalias(src.token().name());

		os<<"  mov "<<(alias_dst?alias_dst:dst.token().name())<<","<<(alias_src?alias_src:src.token().name())<<endl;
	}
	inline void link(toc&tc,ostream&os)override{}
};
