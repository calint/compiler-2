#pragma once

#include <stddef.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "block.hpp"
#include "compiler_error.hpp"
#include "decouple.hpp"
#include "expression.hpp"
#include "func.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class call:public expression{public:
	inline static up_statement read_statement(statement*parent,tokenizer&t){
		up_token tkn=t.next_token();
		if(!t.is_next_char_expression_open())
			return make_unique<statement>(parent,move(tkn));// ie  0x80
		t.unread();
		return create_call(tkn->name(),parent,move(tkn),t); // ie  f(...)
	}


	inline call(statement*parent,up_token tkn,tokenizer&t):expression{parent,move(tkn)}{
		if(!t.is_next_char_args_open())throw compiler_error(*this,"expected ( and arguments",token().name_copy());//? object invalid
		while(!t.is_next_char_args_close())args.push_back(call::read_statement(this,t));
	}
	inline void source_to(ostream&os)const override{statement::source_to(os);os<<"(";for(auto&e:args)e->source_to(os);os<<")";}
	inline void compile(toc&tc,ostream&os,size_t indent_level)override{
		//-- comment
		indent(os,indent_level,true);
		os<<token().name()<<"(";
		const size_t n=args.size();
		const size_t nn=n-1;
		for(size_t i=0;i<n;i++){
			const char*s=args[i].get()->token().name();
			os<<s;
			if(i<nn)os<<" ";
		}
		os<<")";
		if(dest)
			os<<":"<<dest;
		os<<"{  ["<<token().token_start_char()<<"]"<<endl;
		//--- - - - -- - - - -

		if(!is_inline()){
			for(auto&a:args)os<<"  push "<<a.get()->token().name()<<endl;
			os<<"  call "<<token().name()<<endl;
			return;
		}

		const char*nm=token().name();
		framestack&fs=tc.framestk();
		fs.push_func(nm);
		const func*f=tc.get_func_or_break(*this,nm);
		size_t i=0;
		if(dest){
			const class token*ret=f->getret();
			if(!ret)throw compiler_error(*this,"cannot assign from call without return",token().name_copy());
			fs.add_alias(ret->name(),dest);
		}
		vector<const char*>allocated_registers;
		for(auto&a:args){
			const char*param=f->params[i++]->name();
			const char*reg{nullptr};
			if(a->is_expression()){
				reg=fs.alloc_scratch_register();
				allocated_registers.push_back(reg);
				a->set_expression_dest_nasm_identifier(reg);
				fs.add_alias(param,reg);
				a->compile(tc,os,indent_level);
				continue;
			}
			const char*tkn=a->token().name();
			fs.add_alias(param,tkn);
		}
		f->code->compile(tc,os,indent_level+1);
		for(auto r:allocated_registers)
			fs.free_scratch_reg(r);
		fs.pop_func(nm);

//		indent(os,indent_level,true);os<<"}\n";
	}

	inline statement&argument(size_t ix)const{return*(args[ix].get());}
	inline bool is_inline()const{return true;}

private:
	vup_statement args;
};
using up_call=unique_ptr<call>;
using allocs=vector<const char*>;

