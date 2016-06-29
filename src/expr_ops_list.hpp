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
//  var c=a+b+b+c*(1+a*(2+b)/a)*(1+2)-4-a*sqrt(2)
//



class expr_ops_list:public expression{public:

	inline expr_ops_list(const statement&parent,tokenizer&t,const char list_append_op,size_t presedence,bool enclosed,unique_ptr<statement>first_expression,char first_op):
		expression{parent,t.next_whitespace_token()},
		enclosed_{enclosed},
		presedence_{presedence},
		list_append_op_{list_append_op}
	{

		if(first_expression){// entry list
			expressions_.push_back(move(first_expression));
		}
//		if(first_op){
//			ops_.push_back(first_op);
//		}

		while(true){// +a  +3
			if(t.is_next_char('(')){
//				expr_ops_list a(*this,t,'=',presedence,true,unique_ptr<statement>());
				unique_ptr<statement>nxtstmt=create_statement_from_tokenizer(*this,t);
				expressions_.push_back(make_unique<expr_ops_list>(*this,t,'=',presedence,true,move(nxtstmt),0));
				continue;
			}

			if(t.is_peek_char(';')){
				break;
			}

			if(t.is_next_char(')')){
				if(not enclosed_)
					throw compiler_error(*expressions_.back(),"unexpected ')'");
				break;
			}

			if(t.is_peek_char('+')){
				ops_.push_back('+');
			}else
			if(t.is_peek_char('-')){
				ops_.push_back('-');
			}else
			if(t.is_peek_char('*')){
				ops_.push_back('*');
			}else
				throw compiler_error(*expressions_.back(),"unknown operator",to_string(t.peek_char()));

			const size_t next_presedence=_presedence_for_op(t.peek_char());
			if(next_presedence>presedence_){
				presedence_=next_presedence;
				if(!ops_.empty()){
					char first_op=ops_.back();
					ops_.pop_back();
					char list_op=ops_.back();
//					ops_.pop_back();
					unique_ptr<statement>prev=move(expressions_.back());
					expressions_.erase(expressions_.end());
					expressions_.push_back(make_unique<expr_ops_list>(*this,t,list_op,presedence_,enclosed_,move(prev),first_op));
				}
			}else{
				presedence_=next_presedence;
				t.next_char();// read the peek operator
			}

			if(t.is_peek_char(';')){
				break;
			}

			if(t.is_next_char(')')){
				if(not enclosed_)
					throw compiler_error(*expressions_.back(),"unexpected ')'");
				break;
			}

			if(t.is_peek_char('('))
				continue;

			unique_ptr<statement>stmt=create_statement_from_tokenizer(*this,t);
			if(stmt->tok().is_blank())
				break;

			expressions_.push_back(move(stmt));
		}
	}

	inline void source_to(ostream&os)const override{
		expression::source_to(os);
//		if(list_append_op_=='=')
//			os<<list_append_op_;
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

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest)const override{
		indent(os,indent_level+1,true);
		source_to(os);
		os<<"      ";
		tc.source_location_to_stream(os,this->tok());
		os<<endl;

		if(expressions_.empty())
			return;

		statement*st=expressions_[0].get();
		string dest_resolved=tc.resolve_ident_to_nasm(*this,dest);
		_asm_op(tc,os,indent_level,st,'=',dest_resolved,true);

		auto len=ops_.size();
		for(size_t i{0};i<len;i++){
			auto op=ops_[i];
			auto st=expressions_[i+1].get();

			_asm_op(tc,os,indent_level,st,op,dest_resolved,false);

		}

	}

	inline bool is_expression()const override{
		if(expressions_.empty())
			return false;

		if(ops_.empty())
			return false;

		return true;
	}

	inline static size_t _presedence_for_op(const char ch){
		if(ch=='+'||ch=='-')
			return 1;
		if(ch=='*'||ch=='/')
			return 2;
//		if(ch=='=')
			return 1;
//		throw string(to_string(__LINE__)+" err");
	}

	inline void _asm_op(toc&tc,ostream&os,size_t indent_level,const statement*st,const char op,const string&dest_resolved,bool first_in_list)const{
		if(op=='+'){// order1op
			if(st->is_expression()){
				auto r=tc.alloc_scratch_register(st->tok());
				st->compile(tc,os,indent_level+1,r);
				_asm("add",*st,tc,os,indent_level+1,   dest_resolved,r);
				tc.free_scratch_reg(r);
				return;
			}
			_asm("add",*st,tc,os,indent_level+1,   dest_resolved,tc.resolve_ident_to_nasm(*st));
			return;
		}
		if(op=='-'){// order1op
			if(st->is_expression()){
				auto r=tc.alloc_scratch_register(*st);
				st->compile(tc,os,indent_level+1,r);
				_asm("sub",*st,tc,os,indent_level+1,    dest_resolved,r);
				tc.free_scratch_reg(r);
				return;
			}
			_asm("sub",*st,tc,os,indent_level+1,dest_resolved,tc.resolve_ident_to_nasm(*st));
			return;
		}
		if(op=='*'){// order2op
			if(st->is_expression()){
				auto r=tc.alloc_scratch_register(*st);
				st->compile(tc,os,indent_level+1,r);
				_asm("imul",*st,tc,os,indent_level+1,dest_resolved,r);
				tc.free_scratch_reg(r);
				return;
			}
			if(first_in_list){
				_asm("mov",*st,tc,os,indent_level+1,dest_resolved,tc.resolve_ident_to_nasm(*st));
			}else
				_asm("imul",*st,tc,os,indent_level+1,dest_resolved,tc.resolve_ident_to_nasm(*st));

			return;
		}
		if(op=='='){
			if(st->is_expression()){
				st->compile(tc,os,indent_level+1,dest_resolved);
			}else{
				_asm("mov",*st,tc,os,indent_level+1,dest_resolved,tc.resolve_ident_to_nasm(*st));
			}
			return;
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


	size_t enclosed_{0}; // ie   =(1+2)   vs  =1+2
	size_t presedence_{0};
	char list_append_op_{0};
	vector<unique_ptr<statement>>expressions_;
	vector<char>ops_;
private:
};
