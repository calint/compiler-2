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



//  var a=1
//  var b=2
//  var c=a+b+b+c*(1+a*(2+b)/a)*(1+2)-4-a
//



class expr_ops_list:public expression{public:

	inline expr_ops_list(const statement&parent,tokenizer&t,const char op):
		expression{parent,t.next_whitespace_token()},
		op_{op}
	{
		if(t.is_next_char('(')){
			enclosed_=true;
		}

		unique_ptr<statement>stmt=create_statement_from_tokenizer(*this,t);
		if(stmt->tok().is_blank())
			return;

		expressions_.push_back(move(stmt));


		while(true){// +a  +3
			if(t.is_peek_char('(')){
				expressions_.push_back(make_unique<expr_ops_list>(*this,t,ops_.back()));
				continue;
			}

			if(enclosed_ and t.is_next_char(')')){
				break;
			}
			if(t.is_next_char('+')){
				ops_.push_back('+');
				continue;
			}
			if(t.is_next_char('-')){
				ops_.push_back('-');
				continue;
			}
			if(t.is_next_char('*')){
				ops_.push_back('*');
				continue;
			}

			unique_ptr<statement>stmt=create_statement_from_tokenizer(*this,t);
			if(stmt->tok().is_blank())
				break;

			expressions_.push_back(move(stmt));
		}
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if(op_=='=')
			os<<op_;
		if(enclosed_)
			os<<"(";
		if(expressions_.size()>0){
			expressions_[0]->source_to(os);
			const size_t len=expressions_.size();
			for(size_t i{1};i<len;i++){
				os<<ops_[i-1];
				os.flush();
				expressions_[i]->source_to(os);
				os.flush();
			}
		}
		if(enclosed_)
			os<<")";
	}

	inline void _asm_op(toc&tc,ostream&os,size_t indent_level,const statement*st,const char op,const string&dest_resolved)const{
		if(op=='+'){// order1op
			if(st->is_expression()){
				if(presedence_==0){
					st->compile(tc,os,indent_level+1,dest_resolved);
					return;
				}
				auto r=tc.alloc_scratch_register(st->tok());
				st->compile(tc,os,indent_level+1,r);
				_asm("add",*st,tc,os,indent_level,dest_resolved,r);
				tc.free_scratch_reg(r);
				return;
			}
			_asm("add",*st,tc,os,indent_level+1,dest_resolved,tc.resolve_ident_to_nasm(*st));
			return;
		}
		if(op=='-'){// order1op
			if(st->is_expression()){
				if(presedence_==0){
					st->compile(tc,os,indent_level+1,dest_resolved);
					return;
				}
				auto r=tc.alloc_scratch_register(*st);
				st->compile(tc,os,indent_level+1,r);
				_asm("sub",*st,tc,os,indent_level,dest_resolved,r);
				tc.free_scratch_reg(r);
				return;
			}
			_asm("sub",*st,tc,os,indent_level+1,dest_resolved,tc.resolve_ident_to_nasm(*st));
			return;
		}
		if(op_=='='){
			if(st->is_expression()){
				st->compile(tc,os,indent_level+1,dest_resolved);
			}else{
				_asm("mov",*st,tc,os,indent_level+1,dest_resolved,tc.resolve_ident_to_nasm(*st));
			}
			return;
		}
//			if(op=='*'){// order2op
//				os<<"mul "<<reg<<","<<resolv<<endl;
//				continue;
//			}
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest)const override{
		indent(os,indent_level+1,true);
		source_to(os);
		os<<"      ";
		tc.source_location_to_stream(os,this->tok());
		os<<endl;

		if(expressions_.empty())
			return;

		string dest_resolved=tc.resolve_ident_to_nasm(*this,dest);
		if(op_=='='){
			auto st=expressions_[0].get();
			_asm_op(tc,os,indent_level,st,'=',dest_resolved);
		}else{
			auto st=expressions_[0].get();
			_asm_op(tc,os,indent_level,st,op_,dest_resolved);
		}

		auto len=ops_.size();
		for(size_t i{0};i<len;i++){
			auto op=ops_[i];
			auto st=expressions_[i+1].get();
			_asm_op(tc,os,indent_level,st,op,dest_resolved);
		}

	}
	inline static void _asm(const string&op,const statement&s,toc&tc,ostream&os,size_t indent_level,const string&ra,const string&rb){
		if(ra==rb){
			return;
		}

		if(!ra.find("dword[") and !rb.find("dword[")){
			const string&r=tc.alloc_scratch_register(s.tok());
			indent(os,indent_level,false);
			os<<"mov "<<r<<","<<rb<<"  ;  ";
			tc.source_location_to_stream(os,s.tok());
			os<<endl;
			indent(os,indent_level,false);
			os<<op<<" "<<ra<<","<<r<<"  ;  ";
			tc.source_location_to_stream(os,s.tok());
			os<<endl;
			tc.free_scratch_reg(r);
			return;
		}

		indent(os,indent_level,false);
		os<<op<<" "<<ra<<","<<rb<<"  ;  ";
		tc.source_location_to_stream(os,s.tok());
		os<<endl;
	}


private:
	bool enclosed_{false}; // ie   =(1+2)   vs  =1+2
	char presedence_{0};
	char op_{0};
	vector<unique_ptr<statement>>expressions_;
	vector<char>ops_;
};
