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

	inline static up_statement read_statement(statement*parent,tokenizer&t){
		up_token tkn=t.next_token();
		if(!t.is_next_char('(')){
			return make_unique<statement>(parent,move(tkn));// ie  0x80
		}
		t.unread();
		return create_call(tkn->name(),parent,move(tkn),t); // ie  f(...)
	}


	inline stmt_call(statement*parent,up_token tkn,tokenizer&t):expression{parent,move(tkn)}{
		if(!t.is_next_char('(')){
			no_args=true;
			return;
		}
		while(true){
			if(t.is_next_char(')'))break;
			args.push_back(stmt_call::read_statement(this,t));
			if(t.is_next_char(')'))break;
			if(!t.is_next_char(','))throw compiler_error(*this,"expected ',' after argument at ",token().name_copy());
		}
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if(no_args)
			return;
		os<<"(";
		size_t i=args.size()-1;
		for(auto&e:args){
			e->source_to(os);
			if(i--)os<<",";
		}
		os<<")";
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		const char*nm=token().name();


		//-- comment
		indent(os,indent_level,true);
		os<<nm<<"(";
		const size_t n=args.size();
		const size_t nn=n-1;
		for(size_t i=0;i<n;i++){
			const char*s=args[i].get()->token().name();
			os<<s;
			if(i<nn)os<<" ";
		}
		os<<")";
		const char*expr_dest=expression_dest_nasm_identifier();
		if(expr_dest)os<<":"<<expr_dest;
		os<<"{  ["<<token().token_start_char()<<"]"<<endl;
		//--- - - - -- - - - -

		if(!is_inline()){
			for(auto&a:args)os<<"  push "<<a.get()->token().name()<<endl;
			os<<"  call "<<token().name()<<endl;
			return;
		}

		const stmt_def_func*f=tc.get_func_or_break(*this,nm);
		framestack&fs=tc.framestk();
//		if(*token().name()=='_'){
//			tc.framestk().export_varspace_at_current_frame_in_subcalls(true);
//		}
		fs.push_func(nm);
		if(expr_dest){
			if(f->getreturns().empty())throw compiler_error(*this,"cannot assign from call without return",token().name_copy());
//			for(auto&e:f->getreturns()){
//				fs.add_alias(e->name(),expr_dest);
//			}
			fs.add_alias(f->getreturns()[0]->name(),expr_dest);
		}
		vector<const char*>allocated_registers;
		size_t i=0;
		for(auto&a:args){
			const char*param=f->get_param(i)->name();
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
			const char*tkn=a->token().name();
			fs.add_alias(param,tkn);
		}

		f->code_block()->compile(tc,os,indent_level+1);

		indent(os,indent_level,false);os<<"_end_"<<nm<<"_"<<token().token_start_char()<<":\n";

		for(auto r:allocated_registers)
			fs.free_scratch_reg(r);

		fs.pop_func(nm);
//		tc.framestk().export_varspace_at_current_frame_in_subcalls(false);

//		indent(os,indent_level,true);os<<"}\n";
	}

	inline statement&argument(size_t ix)const{return*(args[ix].get());}

	inline size_t argument_count()const{return args.size();}

	inline bool is_inline()const{return true;}


private:
	vup_statement args;
	bool no_args{false};
};
using up_call=unique_ptr<stmt_call>;
using allocs=vector<const char*>;

