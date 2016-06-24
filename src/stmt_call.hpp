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
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include"decouple.hpp"
#include "def_block.hpp"
#include "def_func.hpp"
class stmt_call:public expression{public:

	inline stmt_call(toc&tc,statement*parent,unique_ptr<class token>tkn,tokenizer&t):expression{tc,parent,move(tkn)}{
		if(!t.is_next_char('(')){
			no_args_=true;
			return;
		}
		while(!t.is_next_char_args_close()){
			if(t.is_next_char(')'))break;
			args_.push_back(read_next_statement(tc,this,t));
			if(t.is_next_char(')'))break;
			if(!t.is_next_char(','))throw compiler_error(*this,"expected ',' after argument at ",tok().name_copy());
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
		const char*nm=tok().name();


		//-- comment
		indent(os,indent_level,true);
		os<<nm<<"(";
		const size_t n=args_.size();
		const size_t nn=n-1;
		for(size_t i=0;i<n;i++){
			const char*s=args_[i].get()->tok().name();
			os<<s;
			if(i<nn)os<<" ";
		}
		os<<")";
		const char*expr_dest=expression_dest_nasm_identifier();
		if(expr_dest)os<<":"<<expr_dest;
		os<<"{  ["<<tok().token_start_char()<<"]"<<endl;
		//--- - - - -- - - - -

		if(!is_inline()){
			for(auto&a:args_)os<<"  push "<<a.get()->tok().name()<<endl;
			os<<"  call "<<tok().name()<<endl;
			return;
		}

		const def_func*f=tc.framestk().current_frame().get_func_or_break(*this,nm);
		framestack&fs=tc.framestk();
//		if(*token().name()=='_'){
//			tc.framestk().export_varspace_at_current_frame_in_subcalls(true);
//		}
		fs.push_func(nm);
		if(expr_dest){
			if(f->returns_.empty())throw compiler_error(*this,"cannot assign from call without return",tok().name_copy());
//			for(auto&e:f->getreturns()){
//				fs.add_alias(e->name(),expr_dest);
//			}
			fs.add_alias(f->returns_[0]->name(),expr_dest);
		}
		vector<const char*>allocated_registers;
		size_t i=0;
		for(auto&a:args_){
			const char*param=f->params_[i]->name();
			i++;
			const char*reg{nullptr};
			if(a->is_expression()){
				reg=fs.alloc_scratch_register();
				allocated_registers.push_back(reg);
				a->set_expression_dest_nasm_identifier(reg);
				fs.add_alias(param,reg);
				a->compile(tc,os,indent_level);
				continue;
			}
			const char*tkn=a->tok().name();
			fs.add_alias(param,tkn);
		}

		f->code_->compile(tc,os,indent_level+1);

		indent(os,indent_level,false);os<<"_end_"<<nm<<"_"<<tok().token_start_char()<<":\n";

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
	vector<unique_ptr<statement>>args_;
	unique_ptr<token>ident_;
};

