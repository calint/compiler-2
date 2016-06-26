#pragma once

#include <stddef.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "compiler_error.hpp"
#include "decouple.hpp"
#include "expression.hpp"
#include "statement.hpp"
#include "stmt_block.hpp"
#include "stmt_def_func.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class stmt_call:public expression{public:

	inline stmt_call(const statement&parent,const token&tk,tokenizer&t):
		expression{parent,tk}
	{
		if(!t.is_next_char('(')){
			no_args_=true;
			return;
		}
		while(true){
			if(t.is_next_char(')'))break;
			args_.push_back(create_statement_from_tokenizer(*this,t));
			if(t.is_next_char(')'))break;
			if(!t.is_next_char(','))throw compiler_error(*args_.back(),"expected ',' after argument at ",tok().name_copy());
		}
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if(no_args_)
			return;
		os<<"(";
		size_t i=args_.size()-1;
		for(auto&e:args_){
			e->source_to(os);
			if(i--)os<<",";
		}
		os<<")";
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		const string&nm=tok().name();


		//-- comment
		indent(os,indent_level,true);
		os<<nm<<"(";
		const size_t n=args_.size();
		const size_t nn=n-1;
		for(size_t i=0;i<n;i++){
			os<<args_[i].get()->tok().name();
			if(i<nn)os<<" ";
		}
		os<<")";
		const string&expr_dest=expression_dest_nasm_identifier();
		if(not expr_dest.empty())os<<":"<<expr_dest;
		os<<"  ";
		tc.source_location_to_stream(os,tok());
		os<<endl;
		//--- - - - -- - - - -

		if(!is_inline()){
			throw"?";
//			for(auto&a:args)os<<"  push "<<a.get()->token().name()<<endl;
//			os<<"  call "<<token().name()<<endl;
//			return;
		}
		const stmt_def_func*f=tc.get_func_or_break(*this,nm);
		framestack&fs=tc.framestk();
//		if(*token().name()=='_'){
//			tc.framestk().export_varspace_at_current_frame_in_subcalls(true);
//		}
		fs.push_func(nm);
		if(not expr_dest.empty()){
			if(f->getreturns().empty())
				throw compiler_error(*this,"cannot assign from call without return",tok().name_copy());
//			for(auto&e:f->getreturns()){
//				fs.add_alias(e->name(),expr_dest);
//			}
			fs.add_alias(f->getreturns()[0].name(),expr_dest);
		}
		vector<string>allocated_registers;
		size_t i=0;
		for(auto&a:args_){
			auto param=f->get_param(i);
			i++;
			if(a->is_expression()){
				const string reg=fs.alloc_scratch_register(param);
				allocated_registers.push_back(reg);
				a->set_dest_to_nasm_ident(reg);
				fs.add_alias(param.name(),reg);
				a->compile(tc,os,indent_level+1);
				continue;
			}
			fs.add_alias(param.name(),a->tok().name());
		}

		f->code_block()->compile(tc,os,indent_level+1);

		indent(os,indent_level,false);os<<"_end_"<<nm<<"_"<<tok().char_index_in_source()<<":";
//		os<<"  ;  ";
//		tc.source_location_to_stream(os,);
		os<<endl;


		for(auto r:allocated_registers)
			fs.free_scratch_reg(r);

		fs.pop_func(nm);
//		tc.framestk().export_varspace_at_current_frame_in_subcalls(false);

//		indent(os,indent_level,true);os<<"}\n";
	}

	inline statement&argument(size_t ix)const{return*(args_[ix].get());}

	inline size_t argument_count()const{return args_.size();}

	inline bool is_inline()const{return true;}


private:
	bool no_args_{false};
	vector<up_statement>args_;
};

