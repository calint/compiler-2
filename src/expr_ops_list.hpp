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


class expr_ops_list final:public expression{public:

	inline expr_ops_list(const statement&parent,tokenizer&t,bool in_args=false,bool enclosed=false,const char list_append_op='=',size_t first_op_presedence=3,unique_ptr<statement>first_expression=unique_ptr<statement>()):
		expression{parent,t.next_whitespace_token()},
		enclosed_{enclosed},
		in_args_{in_args},
		presedence_{first_op_presedence},
		list_append_op_{list_append_op}
	{

		if(first_expression){
			expressions_.push_back(move(first_expression));
		}else{
			if(t.is_next_char('(')){
				expressions_.push_back(make_unique<expr_ops_list>(*this,t,in_args,true));
			}else{
				expressions_.push_back(create_statement_from_tokenizer(*this,t));
			}
		}

		while(true){// +a  +3
			if(t.is_peek_char(';')){
				break;
			}

			if(in_args){//? rewrite is_in_bool_expr
				if(t.is_peek_char('<'))
					break;
				if(t.is_peek_char('='))
					break;

			}

			if(in_args and (t.is_peek_char(',') or t.is_peek_char(')'))){
				break;
			}

			if(t.is_peek_char(')')){
				if(enclosed_){
					t.next_char();
				}
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
			if(t.is_peek_char('/')){
				ops_.push_back('/');
			}else
			if(t.is_peek_char('%')){
				ops_.push_back('%');
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
					expressions_.push_back(make_unique<expr_ops_list>(*this,t,in_args,false,list_op,first_op_presedence,move(prev)));
					continue;
				}
			}else{
				presedence_=next_presedence;
				t.next_char();// read the peeked operator
			}

			if(t.is_next_char('(')){
				expressions_.push_back(make_unique<expr_ops_list>(*this,t,in_args,true));
				continue;
			}

			unique_ptr<statement>stmt=create_statement_from_tokenizer(*this,t);
			if(stmt->tok().is_blank())
				throw compiler_error(*stmt,"unexpected blank token");

			expressions_.push_back(move(stmt));
		}
	}

	inline void source_to(ostream&os)const override{
		expression::source_to(os);//?

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

		if(expressions_.empty())
			return;

		statement*st=expressions_[0].get();
		string dest_resolved=tc.resolve_ident_to_nasm(*this,dest);
		_asm_op(tc,os,indent_level,st,'=',dest,dest_resolved,true);

		auto len=ops_.size();
		for(size_t i{0};i<len;i++){
			auto op=ops_[i];
			auto st=expressions_[i+1].get();

			_asm_op(tc,os,indent_level,st,op,dest,dest_resolved,false);

		}

	}

	inline bool is_expression()const override{
		if(expressions_.size()==1){
			return expressions_[0]->is_expression();
		}

		return true;
	}


	inline bool is_ops_list()const override{return true;}

	inline const string&identifier()const override{
		if(expressions_[0]->is_ops_list())
			return expressions_[0]->identifier();

		return expressions_[0]->identifier();
	}

	inline bool alloc_register()const{return!ops_.empty();}

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
	inline bool is_empty()const override{return expressions_.empty();}

private:
	inline static size_t _presedence_for_op(const char ch){
		if(ch=='+'||ch=='-')
			return 1;
		if(ch=='*'||ch=='/')
			return 2;
		throw string(to_string(__LINE__)+" err");
	}

	inline void _asm_op(toc&tc,ostream&os,size_t indent_level,const statement*st,const char op,const string&dest,const string&dest_resolved,bool first_in_list)const{
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
				st->compile(tc,os,indent_level+1,dest);
			}else{
				_asm("mov",*st,tc,os,indent_level+1,dest_resolved,tc.resolve_ident_to_nasm(*st,st->identifier()));
			}
			return;
		}
	}



	bool enclosed_{false}; // ie   =(1+2)   vs  =1+2
	bool in_args_{false}; // ie   =func(1+2)
	size_t presedence_{0};
	char list_append_op_{0};
	vector<unique_ptr<statement>>expressions_;
	vector<char>ops_;
};
