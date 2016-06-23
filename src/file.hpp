#pragma once

#include <algorithm>
#include <iostream>

#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class file final:public statement{public:
	inline file(statement*parent,up_token tkn,tokenizer&t):statement{parent,move(tkn)}{
		identifier=t.next_token();
		if(!t.is_next_char_data_open())throw compiler_error(*this,"expected { to open file",identifier->name_copy());
		while(true){
			if(t.is_next_char_data_close())break;
			tokens.push_back(t.next_token());
		}
	}
	inline bool is_in_data_section()const override{return true;}
	inline void compile(toc&tc,ostream&os,size_t indent_level)override{
//		section .data
//		msg     db  'Hello, world!',0xa                 ;string
//		msg.len equ $ - msg                             ;length of string
//		section .text
//		os<<"section .data\n";
		for(size_t i=0;i<indent_level;i++)cout<<"  ";
		os<<identifier->name()<<"     db '";
		for(auto&s:tokens)s->compile_to(os);
		os<<"'\n";

		for(size_t i=0;i<indent_level;i++)cout<<"  ";
		os<<identifier->name()<<".len equ $-"<<identifier->name()<<endl;
//		os<<"section .text\n";
		tc.put_file(*this,identifier->name(),this);
	}
	inline void source_to(ostream&os)const override{
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
