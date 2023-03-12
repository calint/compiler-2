#pragma once

#include"expression.hpp"
#include"tokenizer.hpp"
#include"expr_ops_list.hpp"
#include"stmt_def_func.hpp"

class stmt_call:public expression{
public:
	inline stmt_call(const statement&parent,const token&tk,tokenizer&t):
		expression{parent,tk}
	{
		if(!t.is_next_char('(')){
			no_args_=true;
			return;
		}
		bool expect_arg{false};
		while(true){
			if(t.is_peek_char(')')){ // foo()
				if(expect_arg)
					throw compiler_error(t,"expected argument after ','");
				t.next_char(); // consume the peeked char
				break;
			}
			args_.emplace_back(make_unique<expr_ops_list>(*this,t,true));
			if(t.is_next_char(')'))
				break;
			if(!t.is_next_char(','))
				throw compiler_error(*args_.back(),"expected ',' after argument '"+args_.back()->identifier()+"'");
			expect_arg=true;
		}
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if(no_args_)
			return;
		os<<"(";
		size_t i=args_.size()-1;
		for(const auto&e:args_){
			e->source_to(os);
			if(i--){
				os<<",";
			}
		}
		os<<")";
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.source_comment(os,*this);

		const string&nm=tok().name();
		const stmt_def_func&f=tc.get_func_or_break(*this,nm);
		if(f.params().size()!=args_.size())
			throw compiler_error(*this,"function '"+f.name()+"' expects "+to_string(f.params().size())+" argument"+(f.params().size()==1?"":"s")+" but "+to_string(args_.size())+" are provided");

		if(!f.is_inline()){
			size_t nargs{args_.size()};
			const size_t nvars_on_stack{tc.get_current_stack_size()};
			// stack is: <base>,
			if(tc.enter_func_call()){
				// if true then this is not a call within arguments of another call
				if(nvars_on_stack){
					// adjust stack past the allocated vars
					expr_ops_list::asm_cmd("sub",*this,tc,os,indent_level,"rsp",to_string(nvars_on_stack<<3));
					// stack: <base>,.. vars ..,
				}
			}

			// push allocated registers on the stack
			const vector<string>&alloc_regs=tc.allocated_registers();
			const size_t n=alloc_regs.size();
			for(size_t i=0;i<n;i++){
				const string&reg=alloc_regs[i];
				indent(os,indent_level);os<<"push "<<reg<<endl;
			}
			// stack is now: <base>,.. vars ..,... allocated regs ...,

			vector<string>allocated_args_registers;
			// push arguments starting with the last
			size_t nargs_on_stack{0};
			while(nargs--){
				const statement&arg=*args_[nargs];
				const stmt_def_func_param&param=f.param(nargs);
				// is the argument passed through a register?
				bool argument_passed_in_register{false};
				string arg_reg=param.get_register_or_empty();
				if(!arg_reg.empty()){
					// argument requests to be passed as a register
					tc.alloc_named_register_or_break(arg,arg_reg,os,indent_level);
					allocated_args_registers.push_back(arg_reg);
					argument_passed_in_register=true;
				}
				if(arg.is_expression()){
					// is the argument passed in through a register?
					if(!argument_passed_in_register){
						// no particular register requested for the argument
						arg_reg=tc.alloc_scratch_register(arg,os,indent_level);
						// compile expression with the result stored in arg_reg
						arg.compile(tc,os,indent_level+1,arg_reg);
						// argument is passed to function through the stack
						indent(os,indent_level);os<<"push "<<arg_reg<<endl;
						// free scratch register
						tc.free_scratch_register(arg_reg,os,indent_level);
						// keep track of how many arguments are on the stack
						nargs_on_stack++;
					}else{
						// compile expression with the result stored in arg_reg
						arg.compile(tc,os,indent_level+1,arg_reg);
					}
					continue;
				}
				// not an expression, resolve identifier to nasm
				const string&id=tc.resolve_ident_to_nasm(arg);
				if(argument_passed_in_register){
					// move the identifier to the requested register
					expr_ops_list::asm_cmd("mov",arg,tc,os,indent_level,arg_reg,id);
				}else{
					// push identifier on the stack
					indent(os,indent_level);os<<"push "<<id<<endl;
					nargs_on_stack++;
				}
			}

			// stack is: <base>,..vars..,...allocated regs...,[arg n],[arg n-1],...,[arg 1],
			indent(os,indent_level);os<<"call "<<f.name()<<endl;

			// stack is: <base>,..vars..,...allocated regs...,[arg n],[arg n-1],...,[arg 1],
			if(nargs_on_stack){
				indent(os,indent_level);os<<"add rsp,"<<(nargs_on_stack<<3)<<endl;
			}
			// stack is: <base>,..vars..,...allocated regs...,

			// pop allocated registers from the stack if any
			size_t nregs_on_stack=alloc_regs.size();
			// stack is: <base>,..vars..,...allocated regs...,
			while(nregs_on_stack--){
				const string&reg=alloc_regs[nregs_on_stack];
				// don't pop registers used to pass params
				if(find(allocated_args_registers.begin(),allocated_args_registers.end(),reg)==allocated_args_registers.end()){
					indent(os,indent_level);os<<"pop "<<reg<<endl;
				}
			}
			// stack is: <base>,..vars..,

			// restore stack pointer
			if(tc.exit_func_call()){
				// this is not a call within the arguments of another call
				// restore rsp to what it was before the call
				// stack is: <base>,..vars..,
				if(nvars_on_stack){
					indent(os,indent_level);os<<"add rsp,"<<(nvars_on_stack<<3)<<endl;
				}
				// stack is: <base>,
			}

			if(not dest_ident.empty()){
				// function returns values in rax, copy return value to dest_ident
				const string&dest_resolved=tc.resolve_ident_to_nasm(*this,dest_ident);
				expr_ops_list::asm_cmd("mov",*this,tc,os,indent_level,dest_resolved,"rax");
			}

			// free allocated registers
			for(const string&r:allocated_args_registers){
				tc.free_named_register(r,os,indent_level);
			}
			return;
		}

		//
		// inlined function
		//

		vector<string>allocated_named_registers;
		vector<string>allocated_scratch_registers;

		// buffer the aliases of arguments
		vector<tuple<string,string>>aliases_to_add;

		// if function returns value
		if(not dest_ident.empty()){
			if(f.returns().empty())
				throw compiler_error(*this,"cannot assign from function without return");
			// alias 'from' identifier to 'dest_ident' identifier
			const string&from=f.returns()[0].name();
			const string&to=dest_ident;
			aliases_to_add.emplace_back(from,to);
		}

		size_t i=0;
		for(const auto&arg:args_){
			const stmt_def_func_param&param=f.param(i);
			i++;
			// does the parameter want the value passed through a register?
			string arg_reg=param.get_register_or_empty();
			if(!arg_reg.empty()){
				// argument is passed through register
				tc.alloc_named_register_or_break(*arg,arg_reg,os,indent_level);
				allocated_named_registers.push_back(arg_reg);
			}
			if(arg->is_expression()){
				// argument is an expression, evaluate and store in arg_reg
				if(arg_reg.empty()){
					// no particular register requested for the argument
					arg_reg=tc.alloc_scratch_register(*arg,os,indent_level);
					allocated_scratch_registers.push_back(arg_reg);
				}
				// compile expression and store result in 'arg_reg'
				arg->compile(tc,os,indent_level+1,arg_reg);
				// alias parameter name to the register containing its value
				aliases_to_add.emplace_back(param.identifier(),arg_reg);
				continue;
			}
			// argument is not an expression
			if(arg_reg.empty()){
				// no register allocated for the argument
				// alias parameter name to the argument identifier
				const string&id=arg->identifier();
				aliases_to_add.emplace_back(param.identifier(),id);
			}else{
				// register allocated for the argument
				const string&id=arg->identifier();
				// alias parameter name to the register
				aliases_to_add.emplace_back(param.identifier(),arg_reg);
				// move argument to register
				const string&src=tc.resolve_ident_to_nasm(param,id);
				expr_ops_list::asm_cmd("mov",param,tc,os,indent_level+1,arg_reg,src);
			}
		}

		// create unique labels for in-lined functions based on the location the source
		// where the call occurred
		const string&call_path=tc.get_call_path(tok());
		const string&src_loc=tc.source_location(tok());
		const string&new_call_path=call_path.empty()?src_loc:(src_loc+"_"+call_path);
		const string&ret_jmp_label=nm+"_"+new_call_path+"_end";

		indent(os,indent_level+1,true);os<<"inline: "<<new_call_path<<endl;

		// enter function
		if(f.returns().empty()){
			tc.push_func(nm,new_call_path,ret_jmp_label,true,"");
		}else{
			tc.push_func(nm,new_call_path,ret_jmp_label,true,f.returns()[0].name());
		}
		// add the aliases to the context of this scope
		for(const auto&e:aliases_to_add){
			tc.add_alias(get<0>(e),get<1>(e));
		}
		// compile in-lined code
		f.code().compile(tc,os,indent_level);
		// provide an exit label for 'return' to use instead of assembler 'ret'
		indent(os,indent_level);os<<ret_jmp_label<<":\n";
		// free allocated registers
		for(const auto&r:allocated_scratch_registers){
			tc.free_scratch_register(r,os,indent_level);
		}
		for(const auto&r:allocated_named_registers){
			tc.free_named_register(r,os,indent_level);
		}
		// pop scope
		tc.pop_func(nm);
	}

	inline statement&arg(size_t ix)const{return*(args_[ix].get());}

	inline size_t arg_count()const{return args_.size();}

private:
	bool no_args_{false};
	vector<unique_ptr<statement>>args_;
};

