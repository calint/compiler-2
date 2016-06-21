#pragma once

#include <algorithm>
#include <iostream>

#include "call.hpp"
#include "statement.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call_asm_syscall final:public call{public:
	inline call_asm_syscall(statement*parent,up_token tkn,tokenizer&t):call{parent,move(tkn),t}{}
	inline void compile(toc&tc,ostream&os)override{os<<"  syscall\n";}
};
