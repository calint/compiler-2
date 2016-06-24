#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "stmt_block.hpp"
#include "compiler_error.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class stmt_def_func final:public statement{public:

	inline stmt_def_func(statement*parent,unique_ptr<class token>tkn,tokenizer&t):statement{parent,move(tkn)}{
		identifier=t.next_token();
		if(!t.is_next_char_expression_open()){
			no_args=true;
//			throw compiler_error(*this,"expected '(' followed by function arguments",identifier->name_copy());
		}
		if(!no_args){
			while(true){
				if(t.is_next_char(')'))break;
				unique_ptr<class token>tk=t.next_token();
				if(t.is_next_char(')')){
					params.push_back(move(tk));
					break;
				}
				if(!t.is_next_char(','))throw compiler_error(*this,"expected ',' after parameter at ",tk->name_copy());
				params.push_back(move(tk));
			}
		}
		if(t.is_next_char(':')){// returns
			while(true){
				returns.push_back(t.next_token());
				if(t.is_next_char(','))continue;
				break;
			}
		}
		code=make_unique<stmt_block>(parent,unique_ptr<class token>(new class token()),t);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		tc.framestk().current_frame().add_func(*this,identifier->name(),this);//? in constructor for forward ref
		if(is_inline())
			return;

		os<<identifier->name()<<":\n";
		for (size_t i=params.size();i-->0;)
			os<<"  pop "<<params[i].get()->name()<<endl;

		code->compile(tc,os,indent_level+1);
		os<<"  ret\n";
	}

	inline void link(toc&tc,ostream&os)const override{
		code->link(tc,os);
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		identifier->source_to(os);
		if(!no_args){
			os<<"(";
			const size_t nparam=params.size()-1;
			size_t ii{0};
			for(auto&s:params){
				s->source_to(os);
				if(ii++!=nparam)os<<",";
			}
			os<<")";
		}
		if(!returns.empty()){
			os<<":";
			const size_t sz=returns.size()-1;
			size_t i{0};
			for(auto&t:returns){
				t->source_to(os);
				if(i++!=sz)os<<",";
			}
		}
		code->source_to(os);}

	inline bool is_inline()const{return true;}

	inline const vector<unique_ptr<class token>>&getreturns()const{return returns;}

	inline const class token&get_param(const size_t ix)const{return *(params[ix].get());}

	inline const stmt_block*code_block()const{return code.get();}

private:
	unique_ptr<class token>identifier;
	vector<unique_ptr<class token>>returns;
	vector<unique_ptr<class token>>params;
	unique_ptr<stmt_block>code;
	bool no_args{false};
};
