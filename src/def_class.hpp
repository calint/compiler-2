#pragma once

#include <stddef.h>
#include <algorithm>
#include <iostream>
#include <memory>

#include "compiler_error.hpp"
#include "statement.hpp"
#include "stmt_def.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class def_func;

using namespace std;
class def_class final:public statement{public:

	inline def_class(toc&tc,statement*parent,unique_ptr<class token>tkn,tokenizer&t)
		:statement{tc,parent,move(tkn)}
	{
		ident_=t.next_token();
		if(t.is_next_char(':')){
			while(true){
				if(t.is_eos())throw compiler_error(*this,"unexpected end of string",token().name_copy());
				unique_ptr<token>tk=t.next_token();
				extends_.push_back(move(tk));
				if(t.is_next_char('{')){
					t.unread();
					break;
				}
				if(!t.is_next_char(','))
					throw compiler_error(*this,"expected ',' or '{' in list of extensions",tk->name_copy());
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
		parent->ev(2,this);
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		ident_->source_to(os);
		if(!extends_.empty()){
			os<<":";
			size_t i=extends_.size()-1;
			for(auto&a:extends_){
				a->source_to(os);
				if(i-->0)
					os<<",";
			}
		}
		code_->source_to(os);
	}

	inline void compile_fields_recurse(toc&tc,ostream&os,size_t indent_level)const{
		for(auto f:fields_){
			f->compile(tc,os,indent_level);
		}

		for(auto f:classes_){
			f->compile_fields_recurse(tc,os,indent_level);
		}
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		framestack&fs=tc.framestk();

		const char*nm=ident_->name();

		fs.current_frame().add_class(*this,nm,this);

		fs.push_class(nm);

		indent(os,indent_level,false);
		os<<"_bgn_"<<ident_->name()<<":"<<endl;

		compile_fields_recurse(tc,os,indent_level);

		indent(os,indent_level,false);
		os<<"_end_"<<ident_->name()<<":"<<endl;

//		code_->compile(tc,os,indent_level);

		fs.pop_class(nm);
	}

	inline bool is_in_data_section()const override{return true;}

	inline bool is_class()const override{return true;}
	inline const char*identifier()const override{return ident_->name();}

	inline const def_func*get_func_or_break(const statement&s,const char*nm)const{
//		for(auto e:funcs_){
//			if(e->is_identifier(nm))
//					return e;
//		}
		throw compiler_error(s,"cannot find function in class",copy_to_unique_pointer(nm));
	}

	inline void ev(int type,const statement*s)override{
		if(type==1){
			fields_.push_back((def_field*)s);
		}else if(type==2){
			classes_.push_back((def_class*)s);
		}else if(type==3){
			funcs_.push_back((def_func*)s);
		}else if(pt)pt->ev(type,s);
	}

	unique_ptr<class token>ident_;
	vector<unique_ptr<token>>extends_;
	unique_ptr<statement>code_;

	vector<def_field*>fields_;
	vector<def_func*>funcs_;
	vector<def_class*>classes_;
};
