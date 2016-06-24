#pragma once

#include <stddef.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "compiler_error.hpp"
#include "statement.hpp"
#include "stmt_def.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class def_field final:public stmt_def{public:

	inline def_field(toc&tc,statement*parent,unique_ptr<class token>tkn,tokenizer&t)
		:stmt_def{tc,parent,move(tkn),t}
	{
		parent->ev(1,this);
	}

	inline bool is_in_data_section()const override{return true;}
//
//	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
////		section .data
////		msg     db  'Hello, world!',0xa                 ;string
////		msg.len equ $ - msg                             ;length of string
////		section .text
////		os<<"section .data\n";
//		for(size_t i=0;i<indent_level;i++)cout<<"  ";
//		os<<identifier->name()<<"     db '";
//		for(auto&s:tokens)s->compile_to(os);
//		os<<"'\n";
//
//		for(size_t i=0;i<indent_level;i++)cout<<"  ";
//		os<<identifier->name()<<".len equ $-"<<identifier->name()<<endl;
////		os<<"section .text\n";
//		tc.framestk().current_frame().add_field(*this,identifier->name(),this);
//	}
//
//	inline void source_to(ostream&os)const override{
//		statement::source_to(os);
//		identifier->source_to(os);
//		os<<"{";
//		for(auto&s:tokens)s->source_to(os);
//		os<<"}";
//	}

};
