#pragma once

#include <stddef.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "compiler_error.hpp"
#include "decouple.hpp"
#include "expression.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include <string>
#include <sstream>
#include "stmt_if_bool_op.hpp"

class stmt_if_branch final:public statement{public:

	inline stmt_if_branch(const statement&parent,tokenizer&t):
		statement{parent,t.next_whitespace_token()}
	{
		if(t.is_next_char('(')){
			enclosed_=true;
		}

		boolbinaryops_.push_back(stmt_if_bool_op{*this,t});

		if(t.is_peek_char(')')){
			if(enclosed_){
				t.next_char();
			}
			code_=make_unique<stmt_block>(*this,t);
			return;
		}
		code_=make_unique<stmt_block>(*this,t);
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);

		if(enclosed_)
			os<<"(";

		for(auto&e:boolbinaryops_){
			e.source_to(os);
		}

		if(enclosed_)
			os<<")";

		if(code_)
			code_->source_to(os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest)const override{
//		indent(os,indent_level+1,true);
//		source_to(os);
//		os<<"      ";
//		tc.source_location_to_stream(os,this->tok());
//		os<<endl;

		vector<string>jmplabels=split(dest,':');

		indent(os,indent_level,false);
		os<<"_if_bgn_"<<to_string(tok().char_index())<<":"<<endl;

		const stmt_if_bool_op&e=boolbinaryops_[0];
		_resolve("cmp",tc,os,indent_level,*e.lhs_,*e.rhs_);
		indent(os,indent_level,false);
		if(e.isnot_){
			if(e.op_=="="){
				os<<"je";
			}else if(e.op_=="<"){
				os<<"jl";
			}else if(e.op_=="<="){
				os<<"jle";
			}
		}else{
			if(e.op_=="="){
				os<<"jne";
			}else if(e.op_=="<"){
				os<<"jge";
			}else if(e.op_=="<="){
				os<<"jg";
			}
		}

		os<<" "<<jmplabels[0]<<endl;

		code_->compile(tc,os,indent_level+1);
		indent(os,indent_level+1,false);
		os<<"jmp "<<jmplabels[1]<<endl; // jump to code after if else block

		indent(os,indent_level,false);
		os<<"_if_end_"<<to_string(tok().char_index())<<":"<<endl;

	}

//	inline bool is_expression()const override{return true;}
//
//	inline bool is_ops_list()const override{return true;}
//
//	inline const string&identifier()const override{
//		if(expressions_[0]->is_ops_list())
//			return expressions_[0]->identifier();
//
//		return expressions_[0]->identifier();
//	}


private:
//	inline static size_t _presedence_for_op(const char ch){
//		if(ch=='=')return 3;
//		if(ch=='|')return 1;
//		if(ch=='&')return 2;
//		throw string(to_string(__LINE__)+" err");
//	}

	inline void _resolve(const string&op,toc&tc,ostream&os,size_t indent_level,const statement&sa,const statement&sb)const{
		string ra,rb;
		vector<string>allocated_registers;
		if(sa.is_expression()){
			ra=tc.alloc_scratch_register(sa);
			allocated_registers.push_back(ra);
			sa.compile(tc,os,indent_level+1,ra);
		}else{
			ra=tc.resolve_ident_to_nasm(sa);
		}
		if(sb.is_expression()){
			rb=tc.alloc_scratch_register(sb);
			allocated_registers.push_back(rb);
			sb.compile(tc,os,indent_level+1,rb);
		}else{
			rb=tc.resolve_ident_to_nasm(sb);
		}

		if(!ra.find("dword[") and !rb.find("dword[")){
			const string&r=tc.alloc_scratch_register(identifier());
			indent(os,indent_level,false);
			os<<"mov "<<r<<","<<rb<<"  ;  ";
			tc.source_location_to_stream(os,identifier());
			os<<endl;
			indent(os,indent_level,false);
			os<<op<<" "<<ra<<","<<r<<"  ;  ";
			tc.source_location_to_stream(os,identifier());
			os<<endl;
			tc.free_scratch_reg(r);
			return;
		}

		indent(os,indent_level,false);
		os<<op<<" "<<ra<<","<<rb<<"  ;  ";
		tc.source_location_to_stream(os,identifier());
		os<<endl;

		for(auto&r:allocated_registers)tc.free_scratch_reg(r);
	}

	bool enclosed_{false}; // ie   =(1+2)   vs  =1+2
	vector<stmt_if_bool_op>boolbinaryops_;
	unique_ptr<stmt_block>code_;
};
