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

	inline stmt_def_table(statement*parent,const token&tkn,tokenizer&t):statement{parent,move(tkn)}{
		identifier=t.next_token();
		if(identifier.is_name(""))
			throw compiler_error(identifier,"expected table name");

		if(!t.is_next_char('(')){
			no_args=true;
//			throw compiler_error(*this,"expected '(' followed by function arguments",identifier->name_copy());
		}
		if(!no_args){
			while(true){
				if(t.is_next_char(')'))break;
				token tk=t.next_token();
				if(t.is_next_char(')')){
					params.push_back(tk);
					break;
				}
				if(!t.is_next_char(','))
					throw compiler_error(tkn,"expected ',' after parameter at ",tkn.name());
				params.push_back(tk);
			}
		}
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		identifier.source_to(os);
		if(!no_args){
			os<<"(";
			const size_t nparam=params.size()-1;
			size_t ii{0};
			for(auto&s:params){
				s.source_to(os);
				if(ii++!=nparam)os<<",";
			}
			os<<")";
		}
	}

	inline bool is_in_data_section()const override{return true;}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		os<<"; --- table "<<identifier.name()<<" ";
		tc.source_location_to_stream(os,identifier);
		os<<endl;
//		os<<token().name()<<"_"<<identifier->name()<<":\n";
//		os<<token().name()<<"_"<<identifier->name()<<"_end:\n\n";
		os<<identifier.name()<<":\n";
		os<<identifier.name()<<".end:\n";
		os<<identifier.name()<<".len equ $-"<<identifier.name()<<"\n\n";
		tc.add_table(*this,identifier.name(),this);//? in constructor for forward ref
	}

private:
	token identifier;
	bool no_args{false};
	vector<token>params;
};
