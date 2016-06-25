#pragma once

#include <algorithm>
#include <iostream>

#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class stmt_def_field final:public statement{public:

	inline stmt_def_field(statement*parent,up_token tkn,tokenizer&t):statement{parent,move(tkn)}{
		identifier=t.next_token();
		if(identifier->is_name(""))
			throw compiler_error(*this,"expected field name");

		if(!t.is_next_char('{'))throw compiler_error(*this,"expected { to open file",identifier->name_copy());
		while(true){
			if(t.is_next_char('}'))break;
			tokens.push_back(t.next_token());
		}
	}

	inline bool is_in_data_section()const override{return true;}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		os<<"; --- field "<<identifier->name()<<"  ";
		tc.source_location_to_stream(os,*identifier);
		os<<endl;
		os<<identifier->name()<<" db '";
		for(auto&s:tokens)s->compile_to(os);
//		os<<"'";
		os<<"'\n";

		for(size_t i=0;i<indent_level;i++)cout<<"  ";
//		os<<" ";
		os<<identifier->name()<<".len equ $-"<<identifier->name()<<"\n\n";
//		os<<"section .text\n";
		tc.add_file(*this,identifier->name(),this);
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
