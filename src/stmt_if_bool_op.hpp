#pragma once

#include <vector>

#include "statement.hpp"

vector<string> &split(const string &s, char delim, vector<string> &elems);
vector<string> &split(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

vector<string> split(const string &s, char delim);
vector<string> split(const string &s, char delim) {
    vector<string> elems;
    split(s, delim, elems);
    return elems;
}


class stmt_if_bool_op:public statement{public:
	inline stmt_if_bool_op(const statement&parent,tokenizer&t,bool is_list):
		statement(parent,t.next_whitespace_token()),
		is_list_{is_list}
	{
		if(is_list)
			return;
		// if not a=3
		bool isnot{false};
		while(true){
			token tk=t.next_token();
			if(not tk.is_name("not")){
				t.pushback_token(tk);
				break;
			}
			isnot=not isnot;
			nots_.push_back(tk);
		}
		isnot_=isnot;

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
	inline void compile_or(toc&tc,ostream&os,size_t indent_level,const bool last_elem,const string&jmp_to_if_false_label,const string&jmp_to_if_true_label)const{
		indent(os,indent_level,false);
		os<<cmp_bgn_label_source_location(tc);
		os<<":\n";
		_resolve("cmp",tc,os,indent_level,*lhs_,*rhs_);
		indent(os,indent_level,false);
		if(last_elem){
			// if last bool eval and false then jump to else label
			// else continue to if block code
			if(isnot_){
				if(op_=="="){
					os<<"je";
				}else if(op_=="<"){
					os<<"jl";
				}else if(op_=="<="){
					os<<"jle";
				}else if(op_==">"){
					os<<"jg";
				}else if(op_==">="){
					os<<"jge";
				}
			}else{
				if(op_=="="){
					os<<"jne";
				}else if(op_=="<"){
					os<<"jge";
				}else if(op_=="<="){
					os<<"jg";
				}else if(op_==">"){
					os<<"jle";
				}else if(op_==">="){
					os<<"jl";
				}
			}
			os<<" "<<jmp_to_if_false_label<<endl;
		}else{
			// if not last bool eval and true then jump to if block code
			// else continue to next bool eval
			if(isnot_){
				if(op_=="="){
					os<<"jne";
				}else if(op_=="<"){
					os<<"jge";
				}else if(op_=="<="){
					os<<"jg";
				}else if(op_==">"){
					os<<"jle";
				}else if(op_==">="){
					os<<"jl";
				}
			}else{
				if(op_=="="){
					os<<"je";
				}else if(op_=="<"){
					os<<"jl";
				}else if(op_=="<="){
					os<<"jle";
				}else if(op_==">"){
					os<<"jg";
				}else if(op_==">="){
					os<<"jge";
				}
			}
			os<<" "<<jmp_to_if_true_label<<endl;
		}
	}

	inline void compile_and(toc&tc,ostream&os,size_t indent_level,const bool last_elem,const string&jmp_to_if_false_label,const string&jmp_to_if_true_label)const{
		indent(os,indent_level,false);
		os<<cmp_bgn_label_source_location(tc);
		os<<":\n";
		_resolve("cmp",tc,os,indent_level,*lhs_,*rhs_);
		indent(os,indent_level,false);
		if(last_elem){
			// if last bool eval and false then jump to else label
			// else continue to if block code
			if(isnot_){
				if(op_=="="){
					os<<"je";
				}else if(op_=="<"){
					os<<"jl";
				}else if(op_=="<="){
					os<<"jle";
				}else if(op_==">"){
					os<<"jg";
				}else if(op_==">="){
					os<<"jge";
				}
			}else{
				if(op_=="="){
					os<<"jne";
				}else if(op_=="<"){
					os<<"jge";
				}else if(op_=="<="){
					os<<"jg";
				}else if(op_==">"){
					os<<"jle";
				}else if(op_==">="){
					os<<"jl";
				}
			}
			os<<" "<<jmp_to_if_false_label<<endl;
		}else{
			// if not last bool eval and false then jump to "else"
			// else continue to next bool eval
			if(isnot_){
				if(op_=="="){
					os<<"je";
				}else if(op_=="<"){
					os<<"jl";
				}else if(op_=="<="){
					os<<"jle";
				}else if(op_==">"){
					os<<"jg";
				}else if(op_==">="){
					os<<"jge";
				}
			}else{
				if(op_=="="){
					os<<"jne";
				}else if(op_=="<"){
					os<<"jge";
				}else if(op_=="<="){
					os<<"jg";
				}else if(op_==">"){
					os<<"jle";
				}else if(op_==">="){
					os<<"jl";
				}
			}
			os<<" "<<jmp_to_if_false_label<<endl;
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
	inline bool is_list()const{return is_list_;}
	inline string cmp_bgn_label_source_location(const toc&tc)const{
		return "cmp_"+tc.source_location(tok());
	}


	vector<token>nots_;
	bool isnot_;
	bool is_list_;
	unique_ptr<expr_ops_list>lhs_;
	string op_;
	unique_ptr<expr_ops_list>rhs_;
};
