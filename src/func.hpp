#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "block.hpp"
#include "compiler_error.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class func final:public statement{public:
	inline func(statement*parent,up_token tkn,tokenizer&t):statement{parent,move(tkn)}{
		identifier=t.next_token();
		if(!t.is_next_char_expression_open())throw compiler_error(*this,"expected '(' followed by function arguments",identifier->name_copy());
		while(!t.is_next_char_expression_close())params.push_back(t.next_token());
		if(t.is_next_char(':')){// returns
			ret=t.next_token();
		}
		code=make_unique<block>(parent,t);
	}
	inline void compile(toc&tc,ostream&os,size_t indent_level)override{
		tc.put_func(identifier->name(),this);
		if(is_inline())
			return;

		os<<identifier->name()<<":\n";
		for (size_t i=params.size();i-->0;)
			os<<"  pop "<<params[i].get()->name()<<endl;

		code->compile(tc,os,indent_level+1);
		os<<"  ret\n";
	}
	inline void link(toc&tc,ostream&os)override final{code->link(tc,os);}
	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		identifier->source_to(os);
		os<<"(";
		for(auto&s:params)
			s->source_to(os);
		os<<")";
		if(ret)
			os<<":"<<ret->name();
		code->source_to(os);}
//	inline const vup_tokens&getparams()const{return params;}
	inline bool is_inline()const{return true;}
	inline const class token*getret()const{return ret.get();}

	vup_tokens params;
	up_block code;
private:
	up_token ret;
	up_token identifier;
};
