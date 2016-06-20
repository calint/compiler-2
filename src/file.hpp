#pragma once

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>

#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class file:public statement{public:
	file(statement*parent,up_token tkn,tokenizer&t):statement{parent,move(tkn)}{
		identifier=t.next_token();
		if(!t.is_next_char_data_open())throw 1;
		while(true){
			if(t.is_next_char_data_close())break;
			tokens.push_back(t.next_token());
		}
	}
	void compile(toc&tc,ostream&os)override{
//		section .data
//		msg     db  'Hello, world!',0xa                 ;string
//		msg.len equ $ - msg                             ;length of string
//		section .text
		os<<"section .data\n";
		os<<identifier->name()<<"     db '";
		for(auto&s:tokens)s->compiled_to(os);
		os<<"'\n";
		os<<identifier->name()<<".len equ $-"<<identifier->name()<<endl;
		os<<"section .text\n";
		tc.put_def(identifier->name());
	}
	void source_to(ostream&os)override{
		statement::source_to(os);
		identifier->source_to(os);
		os<<"{";
		for(auto&s:tokens)s->source_to(os);
		os<<"}";
	}

private:
	up_token identifier;
	vup_tokens tokens;
};
