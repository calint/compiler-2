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

class stmt_def_table final:public statement{public:

	inline stmt_def_table(const statement&parent,const token&tkn,tokenizer&t):
		statement{parent,tkn},
		ident_{t.next_token()}
	{
		if(ident_.is_name(""))
			throw compiler_error(ident_,"expected table name");

		if(!t.is_next_char('(')){
			no_args_=true;
//			throw compiler_error(*this,"expected '(' followed by function arguments",identifier->name_copy());
		}
		if(!no_args_){
			while(true){
				if(t.is_next_char(')'))break;
				token tk=t.next_token();
				if(t.is_next_char(')')){
					params_.push_back(tk);
					break;
				}
				if(!t.is_next_char(','))
					throw compiler_error(tkn,"expected ',' after parameter at ",tkn.name());
				params_.push_back(tk);
			}
		}
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		ident_.source_to(os);
		if(!no_args_){
			os<<"(";
			const size_t nparam=params_.size()-1;
			size_t ii{0};
			for(auto&s:params_){
				s.source_to(os);
				if(ii++!=nparam)os<<",";
			}
			os<<")";
		}
	}

	inline bool is_in_data_section()const override{return true;}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		os<<"; --- table "<<ident_.name()<<" ";
		tc.source_location_to_stream(os,ident_);
		os<<endl;
//		os<<token().name()<<"_"<<identifier->name()<<":\n";
//		os<<token().name()<<"_"<<identifier->name()<<"_end:\n\n";
		os<<ident_.name()<<":\n";
		os<<ident_.name()<<".end:\n";
		os<<ident_.name()<<".len equ $-"<<ident_.name()<<"\n\n";
		tc.add_table(*this,ident_.name(),this);//? in constructor for forward ref
	}

private:
	token ident_;
	bool no_args_{false};
	vector<token>params_;
};
