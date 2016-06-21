#pragma once

#include <stddef.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

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
			return make_unique<expression>(parent,move(tkn));// ie  0x80
		t.unread();
		return create_call(tkn->name(),parent,move(tkn),t); // ie  f(...)
	}


	inline call(statement*parent,up_token tkn,tokenizer&t):expression{parent,move(tkn)}{
		if(!t.is_next_char_args_open())throw compiler_error(*this,"expected ( and arguments",token().copy_name());//? object invalid
		while(!t.is_next_char_args_close())args.push_back(call::read_statement(this,t));
	}
	inline void source_to(ostream&os)const override{statement::source_to(os);os<<"(";for(auto&e:args)e->source_to(os);os<<")";}
	inline void compile(toc&tc,ostream&os)override{
		os<<"; "<<token().name()<<"(";
		const size_t n=args.size();
		const size_t nn=n-1;
		for(size_t i=0;i<n;i++){
			const char*s=args[i].get()->token().name();
			os<<s;
			if(i<nn)os<<" ";
		}
		os<<")\n";
		if(!is_inline()){
			for(auto&a:args)os<<"  push "<<a.get()->token().name()<<endl;
			os<<"  call "<<token().name()<<endl;
			return;
		}

		const char*nm=token().name();
		tc.stack_pushfunc(nm);
		size_t i=0;
		const func*f=tc.get_func(nm);
		if(!f)throw compiler_error(*this,"function not found",token().copy_name());
		for(auto&a:args){
			const char*tkn=a->token().name();
			const char*param=f->params[i++]->name();
			tc.stack_alias(param,tkn);
		}
		f->code->compile(tc,os);
	}
	inline void link(toc&tc,ostream&os)override{
	}

	inline const statement&argument(size_t ix)const{return*(args[ix].get());}
	inline bool is_inline()const{return true;}

private:
	vup_statement args;
};
using up_call=unique_ptr<call>;
using allocs=vector<const char*>;

