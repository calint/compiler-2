#pragma once

#include <stddef.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "compiler_error.hpp"
#include "decouple.hpp"
class def_field;
class def_func;
class def_class;

using namespace std;
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
class stmt_def:public statement{public:

	inline stmt_def(toc&tc,statement*parent,unique_ptr<class token>tkn,tokenizer&t)
		:statement{tc,parent,move(tkn)}
	{
		identifier=t.next_token();
		identifier_str=identifier->name();

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
		code=read_next_statement(tc,parent,t);
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
		code->source_to(os);
	}


	inline void ev(int type,const statement*s)override{
		if(type==1){
			fields_.push_back((def_field*)s);
			return;
		}
		if(type==2){
			funcs_.push_back((def_func*)s);
			return;
		}
		if(type==3){
			classes_.push_back((def_class*)s);
			return;
		}
	}

	unique_ptr<class token>identifier;
	const char*identifier_str{nullptr};
	bool no_args{false};
	vector<unique_ptr<class token>>params;
	vector<unique_ptr<class token>>returns;
	unique_ptr<statement>code;

	vector<def_field*>fields_;
	vector<def_func*>funcs_;
	vector<def_class*>classes_;
};
