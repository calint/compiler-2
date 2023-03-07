#pragma once

#include"stmt_def_func_param.hpp"
#include"stmt_block.hpp"

class stmt_def_func final:public statement{
public:
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
				params_.emplace_back(*this,t);
				if(t.is_next_char(')'))break;
				if(!t.is_next_char(','))
					throw compiler_error(params_.back(),"expected ',' after parameter '"+params_.back().tok().name()+"'");
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
			size_t i{0};
			for(const auto&s:params_){
				s.source_to(os);
				if(i++!=nparam)os<<",";
			}
			os<<")";
		}
		if(!returns_.empty()){
			os<<":";
			const size_t sz=returns_.size()-1;
			size_t i{0};
			for(const auto&t:returns_){
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

		throw"this code should not be reached";
//		os<<ident_.name()<<":\n";
//		for (size_t i=params_.size();i-->0;)
//			os<<"  pop "<<params_[i].tok().name()<<endl;
//
//		code_->compile(tc,os,indent_level+1);
//		os<<"  ret\n";
	}

	inline bool is_inline()const{return true;}

	inline const vector<token>&returns()const{return returns_;}

	inline const stmt_def_func_param&param(const size_t ix)const{return params_[ix];}

	inline const stmt_block&code_block()const{return*code_.get();}

private:
	token ident_;
	vector<stmt_def_func_param>params_;
	vector<token>returns_;
	unique_ptr<stmt_block>code_;
	bool no_args_{false};
};
