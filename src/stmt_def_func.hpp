#pragma once

#include"stmt_def_func_param.hpp"
#include"stmt_block.hpp"

class stmt_def_func final:public statement{
public:
	inline stmt_def_func(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk}
	{
		name_=t.next_token();
		if(name_.is_name("inline")){
			inline_tk_=name_;
			name_=t.next_token();
		}
		if(!t.is_next_char('(')){
			no_args_=true;
//			throw compiler_error(*this,"expected '(' followed by function arguments",identifier->name_copy());
		}
		if(!no_args_){
			while(true){
				if(t.is_next_char(')'))break;
				params_.emplace_back(*this,t);
				if(t.is_next_char(')'))break;
				if(!t.is_next_char(','))
					throw compiler_error(params_.back(),"expected ',' after parameter '"+params_.back().tok().name()+"'");
			}
		}
		if(t.is_next_char(':')){// returns
			while(true){
				returns_.emplace_back(t.next_token());
				if(t.is_next_char(','))continue;
				break;
			}
		}
		code_=make_unique<stmt_block>(parent,t);
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		inline_tk_.source_to(os);
		name_.source_to(os);
		if(!no_args_){
			os<<"(";
			const size_t nparam=params_.size()-1;
			size_t i{0};
			for(const auto&s:params_){
				s.source_to(os);
				if(i++!=nparam)os<<",";
			}
			os<<")";
		}
		if(!returns_.empty()){
			os<<":";
			const size_t sz=returns_.size()-1;
			size_t i{0};
			for(const auto&t:returns_){
				t.source_to(os);
				if(i++!=sz)os<<",";
			}
		}
		code_->source_to(os);
	}


	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		tc.add_func(*this,name_.name(),this);//? in constructor for forward ref
		if(is_inline())
			return;

		os<<endl;
		indent(os,indent_level);os<<name()<<":\n";
		tc.push_func(name(),"","",false);

		// return binding
		if(!returns().empty()){
			const string&from=returns()[0].name();
			tc.add_alias(from,"rax");
		}

		// stack is now: ...,[prev sp],[arg n],[arg n-1],...,[arg 1],[return address]
		// define variables in the called context by binding arguments to stack
		//    x=[rsp+argnum<<3+8] (const 8 skips return address)
		const int n=int(params_.size());
		for(int i=0;i<n;i++){
			const stmt_def_func_param&pm=params_[unsigned(i)];
			const string&pm_nm=pm.name();
			// (i+2)<<3 ?
			// stack after push rbp is ...,[arg n],...[arg 1],[return address],[rbp],
			indent(os,indent_level+1,true);os<<pm_nm<<": rsp+"<<((i+2)<<3)<<endl;
			tc.add_func_arg(pm_nm,i+2);
		}
		indent(os,indent_level+1);os<<"push rbp\n";
		indent(os,indent_level+1);os<<"mov rbp,rsp\n";
		code_->compile(tc,os,indent_level,"");
		indent(os,indent_level+1);os<<"pop rbp\n";
		indent(os,indent_level+1);os<<"ret\n";
		os<<endl;
		tc.pop_func(name());
	}

	inline const vector<token>&returns()const{return returns_;}

	inline const stmt_def_func_param&param(const size_t ix)const{return params_[ix];}

	inline const vector<stmt_def_func_param>&params()const{return params_;}

	inline const stmt_block&code()const{return*code_.get();}

	inline const string&name()const{return name_.name();}

	inline bool is_inline()const{
		const bool b=!inline_tk_.is_blank();
		return b;
	}

private:
	token name_;
	vector<stmt_def_func_param>params_;
	vector<token>returns_;
	unique_ptr<stmt_block>code_;
	token inline_tk_;
	bool no_args_{false};
};
