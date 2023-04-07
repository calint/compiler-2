#pragma once

#include"stmt_def_func_param.hpp"
#include"stmt_block.hpp"

class stmt_def_func final:public statement{
public:
	inline stmt_def_func(toc&tc,token tk,tokenizer&t):
		statement{move(tk)},
		name_{t.next_token()}
	{
		if(name_.is_name("inline")){
			inline_tk_=name_;
			name_=t.next_token();
		}
		if(not t.is_next_char('('))
			no_args_=true;

		if(not no_args_){
			while(true){
				if(t.is_next_char(')'))
					break;
				params_.emplace_back(tc,t);
				if(t.is_next_char(')'))
					break;
				if(not t.is_next_char(','))
					throw compiler_error(params_.back(),"expected ',' after parameter '"+params_.back().tok().name()+"'");
			}
		}
		if(t.is_next_char(':')){// returns
			while(true){
				returns_.emplace_back(t.next_token());
				if(t.is_next_char(':'))
					continue;
				break;
			}
			if(returns_.size()>1){
				set_type(tc.get_type_or_throw(*this,returns_[1].name()));
//				return_type_str=returns_[1].name();
			}else{
				set_type(tc.get_type_default());
			}
		}else{
			set_type(tc.get_type_void());
		}
		tc.add_func(*this,name_.name(),get_type(),this);
		tc.enter_func(name(),"","",false,returns().empty()?"":returns()[0].name());
		vector<string>allocated_named_registers;
		null_stream ns{};
		init_variables(tc,ns,0,allocated_named_registers);
		code_={tc,t};
		free_allocated_register(tc,ns,0,allocated_named_registers);
		tc.exit_func(name());
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
		if(not no_args_){
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
		if(not returns_.empty()){
			os<<":";
			const size_t n{returns_.size()-1};
			size_t i{0};
			for(const token&t:returns_){
				t.source_to(os);
				if(i++!=n)
					os<<":";
			}
		}
		code_.source_to(os);
	}

	inline void source_def_comment_to(ostream&os)const{
		stringstream ss;
		name_.source_to(ss);
		if(not no_args_){
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
		if(not returns_.empty()){
			ss<<":";
			const size_t n{returns_.size()-1};
			size_t i{0};
			for(const token&t:returns_){
				t.source_to(ss);
				if(i++!=n)
					ss<<":";
			}
		}
		ss<<endl;

		string s{ss.str()};
		string res{regex_replace(s,regex("\\s+")," ")};
		os<<res<<endl;
	}

	inline void init_variables(toc&tc,ostream&os,size_t indent,vector<string>&allocated_named_registers)const{
		// return binding
		if(not returns().empty()){
			const string&nm{returns()[0].name()};
			tc.add_var(*this,os,indent+1,nm,get_type(),false);
		}

		// stack is now: ...,[prev rsp],...,[arg n],[arg n-1],...,[arg 1],[return address],[rbp]
		//   (some arguments might be passed through registers)

		// define variables in the called context by mapping arguments to stack
		const size_t n{params_.size()};
		size_t stk_ix{2<<3}; // skip [rbp] and [return address] on stack
		for(size_t i=0;i<n;i++){
			const stmt_def_func_param&pm{params_[i]};
			const type&arg_type{pm.get_type()};
			const string&pm_nm{pm.name()};
			const string&reg{pm.get_register_or_empty()};
			if(reg.empty()){
				// argument not passed as register
				tc.add_func_arg(*this,os,indent+1,pm_nm,arg_type,int(stk_ix));
				// only 64 bits values on the stack
				stk_ix+=tc.get_type_default().size();
				continue;
			}
			// argument passed as named register
			toc::indent(os,indent+1,true);os<<pm_nm<<": "<<reg<<endl;
			tc.alloc_named_register_or_break(pm,os,indent+1,reg);
			// ! check if arg_type fits in register
			allocated_named_registers.push_back(reg);
			tc.add_alias(pm_nm,reg);
		}
	}

	inline void free_allocated_register(toc&tc,ostream&os,size_t indent,const vector<string>&allocated_named_registers)const{
		size_t i{allocated_named_registers.size()};
		while(i--){
			tc.free_named_register(os,indent+1,allocated_named_registers[i]);
		}
	}

	inline void compile(toc&tc,ostream&os,size_t indent,const string&dst="")const override{
		if(is_inline())
			return;
		
		tc.asm_label(*this,os,indent,name());
		tc.indent(os,indent+1,true);source_def_comment_to(os);

		tc.enter_func(name(),"","",false,returns().empty()?"":returns()[0].name());

		tc.asm_push(*this,os,indent+1,"rbp");
		tc.asm_cmd(*this,os,indent+1,"mov","rbp","rsp");

		vector<string>allocated_named_registers;
		init_variables(tc,os,indent,allocated_named_registers);

		code_.compile(tc,os,indent,"");

		if(!returns().empty()){
			const string&ret_name{returns()[0].name()};
			const ident_resolved&ret_resolved{tc.resolve_identifier(*this,ret_name,true)};
			tc.asm_cmd(*this,os,indent+1,"mov","rax",ret_resolved.id_nasm);
		}

		tc.asm_pop(*this,os,indent+1,"rbp");
		tc.asm_ret(*this,os,indent+1);

		free_allocated_register(tc,os,indent,allocated_named_registers);

		os<<endl;
		tc.exit_func(name());
	}

	inline const vector<token>&returns()const{return returns_;}

	inline const stmt_def_func_param&param(const size_t ix)const{return params_[ix];}

	inline const vector<stmt_def_func_param>&params()const{return params_;}

	inline const stmt_block&code()const{return code_;}

	inline const string&name()const{return name_.name();}

	inline bool is_inline()const{return not inline_tk_.is_empty();}

private:
	token name_;
	vector<stmt_def_func_param>params_;
	vector<token>returns_;
	stmt_block code_;
	token inline_tk_;
	bool no_args_{};
};
