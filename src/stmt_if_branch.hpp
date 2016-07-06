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


class stmt_if_branch final:public statement{public:

	inline stmt_if_branch(const statement&parent,tokenizer&t,bool enclosed=true,const char list_append_op=' ',size_t first_op_presedence=3,unique_ptr<statement>first_expression=unique_ptr<statement>()):
		statement{parent,t.next_whitespace_token()},
		enclosed_{enclosed},
		presedence_{first_op_presedence},
		list_append_op_{list_append_op}
	{

		if(first_expression){
			expressions_.push_back(move(first_expression));
		}else{
			if(t.is_next_char('(')){
				expressions_.push_back(make_unique<stmt_if_branch>(*this,t));
			}else{
				expressions_.push_back(create_statement_from_tokenizer(*this,t));
			}
		}

		while(true){// +a  +3
			if(t.is_peek_char(')')){
				if(enclosed_){
					t.next_char();
				}
				break;
			}

			if(t.is_peek_char('=')){
				ops_.push_back('=');
//			}else
//			if(t.is_peek_char('&')){
//				ops_.push_back('&');
//			}else
//			if(t.is_peek_char('|')){
//				ops_.push_back('|');
//			}else
//			if(t.is_peek_char('^')){
//				ops_.push_back('^');
//			}else
//			if(t.is_peek_char('!')){
//				ops_.push_back('!');
			}else
				throw compiler_error(*expressions_.back(),"unknown operator",to_string(t.peek_char()));
//				break;

			const size_t next_presedence=_presedence_for_op(t.peek_char());
			if(next_presedence>presedence_){
				presedence_=next_presedence;
				if(!ops_.empty()){
					const size_t first_op_presedence=_presedence_for_op(ops_.back());
					ops_.pop_back();
					const char list_op=ops_.back();
					unique_ptr<statement>prev=move(expressions_.back());
					expressions_.erase(expressions_.end());
					expressions_.push_back(make_unique<stmt_if_branch>(*this,t,false,list_op,first_op_presedence,move(prev)));
					continue;
				}
			}else{
				presedence_=next_presedence;
				t.next_char();// read the peeked operator
			}

			if(t.is_next_char('(')){
				expressions_.push_back(make_unique<stmt_if_branch>(*this,t));
				continue;
			}

			unique_ptr<statement>stmt=create_statement_from_tokenizer(*this,t);
			if(stmt->tok().is_blank())
				throw compiler_error(*stmt,"unexpected blank token");

			expressions_.push_back(move(stmt));
		}
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);

		if(enclosed_)
			os<<"(";

		if(not expressions_.empty()){
			expressions_[0]->source_to(os);
			const size_t len=expressions_.size();
			for(size_t i{1};i<len;i++){
				os<<ops_[i-1];
				expressions_[i]->source_to(os);
			}
		}

		if(enclosed_)
			os<<")";
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest)const override{
		indent(os,indent_level+1,true);
		source_to(os);
		os<<"      ";
		tc.source_location_to_stream(os,this->tok());
		os<<endl;

		_resolve("cmp",tc,os,indent_level,*expressions_[0],*expressions_[1]);
		indent(os,indent_level,false);
		os<<"jne "<<dest<<endl;
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
	inline static size_t _presedence_for_op(const char ch){
		if(ch=='=')return 3;
		if(ch=='|')return 1;
		if(ch=='&')return 2;
		throw string(to_string(__LINE__)+" err");
	}

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
			sa.compile(tc,os,indent_level+1,rb);
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
	size_t presedence_{0};
	char list_append_op_{0};
	vector<unique_ptr<statement>>expressions_;
	vector<char>ops_;
};
