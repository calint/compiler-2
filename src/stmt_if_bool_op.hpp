#pragma once
#include <vector>
#include "statement.hpp"

class stmt_if_bool_op:public statement{public:
	inline stmt_if_bool_op(const statement&parent,tokenizer&t):
		statement(parent,t.next_whitespace_token())
	{
		// if not a=3
		bool is_not{false};
		while(true){
			token tk=t.next_token();
			if(not tk.is_name("not")){
				t.pushback_token(tk);
				break;
			}
			is_not=not is_not;
			nots_.push_back(tk);
		}
		is_not_=is_not;

		lhs_=make_unique<expr_ops_list>(*this,t,true);
		if(lhs_->is_empty())
			throw compiler_error(parent,"expected left hand side of boolean operation");

		if(t.is_next_char('=')){
			op_="=";
		}else if(t.is_next_char('<')){
			if(t.is_next_char('=')){
				op_="<=";
			}else{
				op_="<";
			}
		}else if(t.is_next_char('>')){
			if(t.is_next_char('=')){
				op_=">=";
			}else{
				op_=">";
			}
		}else{
			throw compiler_error(parent,"expected boolean op");
		}
		rhs_=make_unique<expr_ops_list>(*this,t,true);
		if(rhs_->is_empty())// unary
			throw compiler_error(*lhs_,"expected right hand side of boolean operation");
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);

		for(auto&e:nots_){
			e.source_to(os);
		}

		lhs_->source_to(os);
		os<<op_;
		rhs_->source_to(os);
	}
	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dst="")const override{
		throw compiler_error(tok(),"this code should not be reached");
	}
	inline static string asm_jxx_for_op(const string&op){
		if(op=="="){
			return "je";
		}else if(op=="<"){
			return "jl";
		}else if(op=="<="){
			return "jle";
		}else if(op==">"){
			return "jg";
		}else if(op==">="){
			return "jge";
		}else{
			throw "unknown op "+op;
		}
	}
	inline static string asm_jxx_for_op_inv(const string&op){
		if(op=="="){
			return "jne";
		}else if(op=="<"){
			return "jge";
		}else if(op=="<="){
			return "jg";
		}else if(op==">"){
			return "jle";
		}else if(op==">="){
			return "jl";
		}else{
			throw "unknown op "+op;
		}
	}
	inline void compile_or(toc&tc,ostream&os,size_t indent_level,const bool last_elem,const string&jmp_to_if_false,const string&jmp_to_if_true)const{
		indent(os,indent_level,true);tc.source_to_as_comment(os,*this);
		indent(os,indent_level,false);os<<cmp_bgn_label(tc)<<":\n";
		_resolve("cmp",tc,os,indent_level,*lhs_,*rhs_);
		indent(os,indent_level,false);
		if(last_elem){
			// if last bool in list and false then jump to 'false' branch
			// else continue to 'true' branch
			os<<(is_not_?asm_jxx_for_op(op_):asm_jxx_for_op_inv(op_));
			os<<" "<<jmp_to_if_false<<endl;
		}else{
			// if not last bool in list and true then jump to 'true' branch
			// else continue to next bool
			os<<(is_not_?asm_jxx_for_op_inv(op_):asm_jxx_for_op(op_));
			os<<" "<<jmp_to_if_true<<endl;
		}
	}

	inline void compile_and(toc&tc,ostream&os,size_t indent_level,const bool last_elem,const string&jmp_to_if_false,const string&jmp_to_if_true_label)const{
		indent(os,indent_level,true);tc.source_to_as_comment(os,*this);
		indent(os,indent_level,false);os<<cmp_bgn_label(tc);os<<":\n";
		_resolve("cmp",tc,os,indent_level,*lhs_,*rhs_);
		indent(os,indent_level,false);
		if(last_elem){
			// if last bool and false then jump to 'false' branch
			// else continue to 'true' branch
			os<<(is_not_?asm_jxx_for_op(op_):asm_jxx_for_op_inv(op_));
			os<<" "<<jmp_to_if_false<<endl;
		}else{
			// if not last bool and false then jump to 'false' branch
			// else continue to next bool
			os<<(is_not_?asm_jxx_for_op(op_):asm_jxx_for_op_inv(op_));
			os<<" "<<jmp_to_if_false<<endl;
		}
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
			sb.compile(tc,os,indent_level+1,rb);
		}else{
			rb=tc.resolve_ident_to_nasm(sb);
		}

		if(!ra.find("dword[") and !rb.find("dword[")){
			const string&r=tc.alloc_scratch_register(identifier());
			indent(os,indent_level,false);
			os<<"mov "<<r<<","<<rb<<endl;
			indent(os,indent_level,false);
			os<<op<<" "<<ra<<","<<r<<endl;
			tc.free_scratch_reg(r);
			return;
		}

		indent(os,indent_level,false);
		os<<op<<" "<<ra<<","<<rb<<endl;

		for(auto&r:allocated_registers)tc.free_scratch_reg(r);
	}
	inline string cmp_bgn_label(const toc&tc)const{
		return "cmp_"+tc.source_location(tok());
	}

	vector<token>nots_;
	bool is_not_;
	unique_ptr<expr_ops_list>lhs_;
	string op_;
	unique_ptr<expr_ops_list>rhs_;
};
