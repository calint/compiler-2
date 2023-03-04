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
		while(true){
			bool_ops_.push_back(stmt_if_bool_op{*this,t});
			token tk=t.next_token();
			if(tk.is_name("or")){
				ops_.push_back(tk);
			}else if(tk.is_name("and")){
				ops_.push_back(tk);
			}else{
				t.pushback_token(tk);
				break;
			}
		}
		code_=make_unique<stmt_block>(*this,t);
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		const size_t n=bool_ops_.size();
		for(size_t i=0;i<n;i++){
			const stmt_if_bool_op&bo=bool_ops_[i];
			bo.source_to(os);
			if(i<(n-1)){
				const token&t=ops_[i];
				t.source_to(os);
			}
		}
		code_->source_to(os);
	}

	inline string if_bgn_label_source_location(const toc&tc)const{
		return "if_"+tc.source_location(tok());
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dst)const override{
		throw compiler_error(tok(),"this code should not be reached");
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&jmp_to_if_false_label,const string&jmp_to_after_code_label)const{
		indent(os,indent_level,true);
		os<<"if ";
		bool_ops_source_to(tc,os);

		const string if_bgn_label=if_bgn_label_source_location(tc);
		const string jmp_to_code_label=if_bgn_label+"_code";

		indent(os,indent_level,false);
		os<<if_bgn_label<<":"<<endl;

		const size_t n=bool_ops_.size();
		for(size_t i=0;i<n;i++){
			const stmt_if_bool_op&e=bool_ops_[i];
			_resolve("cmp",tc,os,indent_level,*e.lhs_,*e.rhs_);
			indent(os,indent_level,false);
			if(i==n-1){
				// if last bool eval and false then jump to else label
				// else continue to if block code
				if(e.isnot_){
					if(e.op_=="="){
						os<<"je";
					}else if(e.op_=="<"){
						os<<"jl";
					}else if(e.op_=="<="){
						os<<"jle";
					}else if(e.op_==">"){
						os<<"jg";
					}else if(e.op_==">="){
						os<<"jge";
					}
				}else{
					if(e.op_=="="){
						os<<"jne";
					}else if(e.op_=="<"){
						os<<"jge";
					}else if(e.op_=="<="){
						os<<"jg";
					}else if(e.op_==">"){
						os<<"jle";
					}else if(e.op_==">="){
						os<<"jl";
					}
				}
				os<<" "<<jmp_to_if_false_label<<endl;
			}else{
				// if not last bool eval and true then jump to if block code
				// else continue to next bool eval
				if(e.isnot_){
					if(e.op_=="="){
						os<<"jne";
					}else if(e.op_=="<"){
						os<<"jge";
					}else if(e.op_=="<="){
						os<<"jg";
					}else if(e.op_==">"){
						os<<"jle";
					}else if(e.op_==">="){
						os<<"jl";
					}
				}else{
					if(e.op_=="="){
						os<<"je";
					}else if(e.op_=="<"){
						os<<"jl";
					}else if(e.op_=="<="){
						os<<"jle";
					}else if(e.op_==">"){
						os<<"jg";
					}else if(e.op_==">="){
						os<<"jge";
					}
				}
				os<<" "<<jmp_to_code_label<<endl;
			}
		}

		indent(os,indent_level,false);
		os<<jmp_to_code_label<<":"<<endl;

		code_->compile(tc,os,indent_level);

		if(jmp_to_after_code_label!=""){
			indent(os,indent_level,false);
			os<<"jmp "<<jmp_to_after_code_label<<endl; // jump to code after if else block
		}
	}

private:
	inline void bool_ops_source_to(toc&tc,ostream&os)const{
		stringstream ss;
		statement::source_to(ss);
		const size_t n=bool_ops_.size();
		for(size_t i=0;i<n;i++){
			const stmt_if_bool_op&bo=bool_ops_[i];
			bo.source_to(ss);
			if(i<(n-1)){
				const token&t=ops_[i];
				t.source_to(ss);
			}
		}

		ss<<"      ";
		tc.source_location_to_stream(ss,this->tok());
		string s=ss.str();
		string res=regex_replace(s,regex("\\s+")," ");
		os<<res;
		os<<endl;
	}
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
		tc.source_location_to_stream(os,sa.tok());
		os<<endl;

		for(auto&r:allocated_registers)tc.free_scratch_reg(r);
	}

	vector<stmt_if_bool_op>bool_ops_;
	vector<token>ops_;
	unique_ptr<stmt_block>code_;
};
