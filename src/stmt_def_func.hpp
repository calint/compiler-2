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
#include"stmt_def_func_param.hpp"
#include"stmt_block.hpp"
class stmt_def_func final:public statement{public:

	inline stmt_def_func(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk},
		ident_{t.next_token()}
	{
		if(!t.is_next_char('(')){
			no_args_=true;
//			throw compiler_error(*this,"expected '(' followed by function arguments",identifier->name_copy());
		}
		if(!no_args_){
			while(true){
				if(t.is_next_char(')'))break;
				unique_ptr<stmt_def_func_param>fp=make_unique<stmt_def_func_param>(*this,t);
				if(t.is_next_char(')')){
					params_.push_back(move(fp));
					break;
				}
				if(!t.is_next_char(','))
					throw compiler_error(*fp,"expected ',' after parameter at ",fp->tok().name_copy());
				params_.push_back(move(fp));
			}
		}
		if(t.is_next_char(':')){// returns
			while(true){
				returns_.push_back(t.next_token());
				if(t.is_next_char(','))continue;
				break;
			}
		}
		code_=make_unique<stmt_block>(parent,t);
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		ident_.source_to(os);
		if(!no_args_){
			os<<"(";
			const size_t nparam=params_.size()-1;
			size_t ii{0};
			for(auto&s:params_){
				s->source_to(os);
				if(ii++!=nparam)os<<",";
			}
			os<<")";
		}
		if(!returns_.empty()){
			os<<":";
			const size_t sz=returns_.size()-1;
			size_t i{0};
			for(auto&t:returns_){
				t.source_to(os);
				if(i++!=sz)os<<",";
			}
		}
		code_->source_to(os);
	}


	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		tc.add_func(*this,ident_.name(),this);//? in constructor for forward ref
		if(is_inline())
			return;

		throw"?";

		os<<ident_.name()<<":\n";
		for (size_t i=params_.size();i-->0;)
			os<<"  pop "<<params_[i]->tok().name()<<endl;

		code_->compile(tc,os,indent_level+1);
		os<<"  ret\n";
	}

//	inline void link(toc&tc,ostream&os)const override{code_->link(tc,os);}

	inline bool is_inline()const{return true;}

	inline const vector<token>&getreturns()const{return returns_;}

	inline const stmt_def_func_param&get_param(const size_t ix)const{return*params_[ix];}

	inline const stmt_block*code_block()const{return code_.get();}

private:
	token ident_;
	bool no_args_{false};
	vector<token>returns_;
	vector<unique_ptr<stmt_def_func_param>>params_;
	unique_ptr<stmt_block>code_;
};
