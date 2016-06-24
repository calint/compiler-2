#pragma once

#include <algorithm>
#include <memory>

#include "statement.hpp"
#include "stmt_def.hpp"
#include "token.hpp"
#include "tokenizer.hpp"


class def_func final:public statement{public:

	inline def_func(toc&tc,statement*parent,unique_ptr<class token>tkn,tokenizer&t)
		:statement{tc,parent,move(tkn)}
	{
		ident_=t.next_token();
		if(ident_->is_name(""))
			throw compiler_error(*this,"expected func name",copy_to_unique_pointer(""));

		if(t.is_next_char('(')){
			while(true){
				if(t.is_eos())throw compiler_error(*this,"unexpected end of string",token().name_copy());
				unique_ptr<token>param=t.next_token();
				params_.push_back(move(param));
				if(t.is_next_char(')'))
					break;

				if(!t.is_next_char(','))
					throw compiler_error(*this,"expected ',' or ')' in after function parameter",param->name_copy());
			}

		}else{
			is_no_params=true;
		}
		if(t.is_next_char(':')){
			while(true){
				if(t.is_eos())throw compiler_error(*this,"unexpected end of string",token().name_copy());
				unique_ptr<token>param=t.next_token();
				returns_.push_back(move(param));
				if(t.is_next_char('{')){
					t.unread();
					break;
				}

				if(!t.is_next_char(','))
					throw compiler_error(*this,"expected ',' or ')' in after function parameter",param->name_copy());
			}
		}
		code_=read_next_statement(tc,this,t);

		const statement*s=parent;
		while(true){
			if(s->is_class())
				break;
			s=s->parent();
			if(s==nullptr)
				break;
		}
		assert(parent);
		parent->ev(3,this);
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		ident_->source_to(os);
		if(!is_no_params){
			os<<"(";
			size_t i=params_.size()-1;
			for(auto&p:params_){
				p->source_to(os);
				if(i--)
					os<<",";
			}
			os<<")";
		}
		if(!returns_.empty()){
			os<<":";
			size_t i=returns_.size()-1;
			for(auto&p:returns_){
				p->source_to(os);
				if(i--)
					os<<",";
			}
		}
		code_->source_to(os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		framestack&fs=tc.framestk();

		const char*nm=ident_->name();

		fs.current_frame().add_func(*this,nm,this);

		fs.push_class(nm);

//		if(!is_inline()){
		os<<"_bgn_"<<nm<<":\n";
		code_->compile(tc,os,indent_level);
		os<<"_end_"<<nm<<":\n";
//		}

		fs.pop_class(nm);
	}

	inline bool is_func()const override{return true;}

	inline virtual const char*identifier()const{return ident_->name();}

	inline void ev(int type,const statement*s)override{
		if(type==1){
			fields_.push_back((def_field*)s);
		}else if(type==2){
			classes_.push_back((def_class*)s);
		}else if(type==3){
			funcs_.push_back((def_func*)s);
		}else if(pt)pt->ev(type,s);
	}


	bool is_no_params{false};
	unique_ptr<token>ident_;
	vector<unique_ptr<token>>params_;
	vector<unique_ptr<token>>returns_;
	unique_ptr<statement>code_;

	vector<def_field*>fields_;
	vector<def_func*>funcs_;
	vector<def_class*>classes_;

};
