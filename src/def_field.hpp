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
#
class def_field final:public statement{public:

	inline def_field(toc&tc,statement*parent,unique_ptr<class token>tkn,tokenizer&t)
		:statement{tc,parent,move(tkn)}
	{
		ident_=t.next_token();
		if(ident_->is_name(""))
			throw compiler_error(*this,"expected field name",copy_to_unique_pointer(""));

		if(!t.is_next_char('{'))
			throw compiler_error(*this,"expected '{' and field default value",copy_to_unique_pointer(""));

		while(true){
			if(t.is_eos())
				throw compiler_error(*this,"unexpected end of stream",copy_to_unique_pointer(""));

			if(t.is_next_char('}'))
				break;

			tokens_.push_back(t.next_token());
		}

		const statement*s=parent;
		while(true){
			if(s->is_class())
				break;
			s=s->parent();
			if(s==nullptr)
				break;
		}
		assert(parent);
		parent->ev(1,this);// is def_class  (circular ref)
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		ident_->source_to(os);
		os<<"{";
		for(auto&s:tokens_)
			s->source_to(os);
		os<<"}";
	}

	inline bool is_in_data_section()const override{return true;}
//
	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
//		section .data
//		msg     db  'Hello, world!',0xa                 ;string
//		msg.len equ $ - msg                             ;length of string
//		section .text
//		os<<"section .data\n";

		vector<string>path;
		const statement*s=parent();
		while(true){
			if(s->is_class()||s->is_func()){
				path.push_back(s->identifier());
			}
			s=s->parent();
			if(s==nullptr)
				break;
		}
		string pth;
		size_t i=path.size();
		while(i--){
			pth.append(path[i]);
			pth.append(".");
		}


		for(size_t i=0;i<indent_level;i++)cout<<"  ";
		os<<pth<<ident_->name()<<"     db '";
		for(auto&s:tokens_)s->compile_to(os);
		os<<"'\n";

		for(size_t i=0;i<indent_level;i++)cout<<"  ";
		os<<pth<<ident_->name()<<".len equ $-"<<ident_->name()<<endl;
//		os<<"section .text\n";
		tc.framestk().current_frame().add_field(*this,ident_->name(),this);
	}
//
	unique_ptr<token>ident_;
	vector<unique_ptr<token>>tokens_;
};
