#pragma once

#include"stmt_def_func_param.hpp"
#include"stmt_block.hpp"

class stmt_def_func final:public statement{
public:
	inline stmt_def_func(token tk,tokenizer&t):
		statement{move(tk)},
		name_{t.next_token()}
	{
		if(name_.is_name("inline")){
			inline_tk_=name_;
			name_=t.next_token();
		}
		if(!t.is_next_char('('))
			no_args_=true;

		if(!no_args_){
			while(true){
				if(t.is_next_char(')'))
					break;
				params_.emplace_back(t);
				if(t.is_next_char(')'))
					break;
				if(!t.is_next_char(','))
					throw compiler_error(params_.back(),"expected ',' after parameter '"+params_.back().tok().name()+"'");
			}
		}
		if(t.is_next_char(':')){// returns
			while(true){
				returns_.emplace_back(t.next_token());
				if(t.is_next_char(','))
					continue;
				break;
			}
		}
		code_={t};
	}

	inline stmt_def_func()=default;
	inline stmt_def_func(const stmt_def_func&)=default;
	inline stmt_def_func(stmt_def_func&&)=default;
	inline stmt_def_func&operator=(const stmt_def_func&)=default;
	inline stmt_def_func&operator=(stmt_def_func&&)=default;

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		inline_tk_.source_to(os);
		name_.source_to(os);
		if(!no_args_){
			os<<"(";
			const size_t n{params_.size()-1};
			size_t i{0};
			for(const stmt_def_func_param&p:params_){
				p.source_to(os);
				if(i++!=n)
					os<<",";
			}
			os<<")";
		}
		if(!returns_.empty()){
			os<<":";
			const size_t n{returns_.size()-1};
			size_t i{0};
			for(const token&t:returns_){
				t.source_to(os);
				if(i++!=n)
					os<<",";
			}
		}
		code_.source_to(os);
	}

	inline void source_def_comment_to(ostream&os)const{
		stringstream ss;
		name_.source_to(ss);
		if(!no_args_){
			ss<<"(";
			const size_t n{params_.size()-1};
			size_t i{0};
			for(const stmt_def_func_param&s:params_){
				s.source_to(ss);
				if(i++!=n)
					ss<<",";
			}
			ss<<")";
		}
		if(!returns_.empty()){
			ss<<":";
			const size_t n{returns_.size()-1};
			size_t i{0};
			for(const token&t:returns_){
				t.source_to(ss);
				if(i++!=n)
					ss<<",";
			}
		}
		ss<<endl;

		string s{ss.str()};
		string res{regex_replace(s,regex("\\s+")," ")};
		os<<res<<endl;
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		tc.add_func(*this,name_.name(),this);//? in constructor for forward ref
		if(is_inline())
			return;
		
		tc.asm_label(*this,os,indent_level,name());
		tc.indent(os,indent_level+1,true);source_def_comment_to(os);

		tc.enter_func(name(),"","",false,returns().empty()?"":returns()[0].name());

		// return binding
		if(!returns().empty()){
			const string&nm{returns()[0].name()};
			tc.add_var(*this,os,indent_level+1,nm,8);
//			tc.add_alias(from,"rax");
		}

		// stack is now: ...,[prev sp],[arg n],[arg n-1],...,[arg 1],[return address]
		// define variables in the called context by binding arguments to stack
		//    x=[rsp+argnum<<3+8] (const 8 skips return address)
		vector<string>allocated_names_registers;
		const size_t n{params_.size()};
		size_t stk_ix{2<<3}; // skip [rbp] and [return address] on stack
		for(size_t i=0;i<n;i++){
			const stmt_def_func_param&pm{params_[i]};
			const string&pm_nm{pm.name()};
			// (i+2)<<3 ?
			// stack after push rbp is ...,[arg n],...[arg 1],[return address],[rbp],
			const string&reg{pm.get_register_or_empty()};
			if(reg.empty()){
//				toc::indent(os,indent_level+1,true);os<<pm_nm<<": rsp+"<<(stk_ix<<3)<<endl;
				tc.add_func_arg(*this,os,indent_level+1,pm_nm,8,int(stk_ix));
				stk_ix+=8;
			}else{
				toc::indent(os,indent_level+1,true);os<<pm_nm<<": "<<reg<<endl;
				tc.alloc_named_register_or_break(pm,os,indent_level+1,reg);
				allocated_names_registers.push_back(reg);
				tc.add_alias(pm_nm,reg);
			}
		}
		tc.asm_push(*this,os,indent_level+1,"rbp");
		tc.asm_cmd(*this,os,indent_level+1,"mov","rbp","rsp");
		code_.compile(tc,os,indent_level,"");
		if(!returns().empty()){
			const string&ret_name{returns()[0].name()};
			const ident_resolved&ir{tc.resolve_ident_to_nasm(*this,ret_name,false)};
			tc.asm_cmd(*this,os,indent_level+1,"mov","rax",ir.id);
		}
		tc.asm_pop(*this,os,indent_level+1,"rbp");
		tc.asm_ret(*this,os,indent_level+1);
		size_t i{allocated_names_registers.size()};
		while(i--){
			tc.free_named_register(os,indent_level+1,allocated_names_registers[i]);
		}
		os<<endl;
		tc.exit_func(name());
	}

	inline const vector<token>&returns()const{return returns_;}

	inline const stmt_def_func_param&param(const size_t ix)const{return params_[ix];}

	inline const vector<stmt_def_func_param>&params()const{return params_;}

	inline const stmt_block&code()const{return code_;}

	inline const string&name()const{return name_.name();}

	inline bool is_inline()const{return!inline_tk_.is_blank();}

private:
	token name_;
	vector<stmt_def_func_param>params_;
	vector<token>returns_;
	stmt_block code_;
	token inline_tk_;
	bool no_args_{};
};
