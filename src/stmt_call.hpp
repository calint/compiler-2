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
			// stack is: <base>,

			const size_t nvars_on_stack{tc.get_current_stack_size()};
			const size_t alloc_regs_idx_cur_call_ctx=tc.get_func_call_alloc_reg_idx();
			if(tc.enter_func_call()){
				// this call is not nested within another call's arguments
				// adjust stack past the vars
				if(nvars_on_stack){
					// adjust stack past the allocated vars
					tc.asm_cmd(*this,os,indent_level,"sub","rsp",to_string(nvars_on_stack<<3));
					// stack: <base>,.. vars ..,
				}
			}

			// push allocated registers in this call context on the stack
			const vector<string>&alloc_regs=tc.get_allocated_registers();
			const size_t n=alloc_regs.size();
			size_t nregs_pushed_on_stack=0;
			for(size_t i=alloc_regs_idx_cur_call_ctx;i<n;i++){
				const string&reg=alloc_regs[i];
				if(tc.is_register_initiated(reg)){
					// push only registers that contain a valid value
					indent(os,indent_level);os<<"push "<<reg<<endl;
					nregs_pushed_on_stack++;
				}
			}

			// stack is now: <base>,.. vars ..,... allocated regs ...,

			// push arguments starting with the last
			//   some arguments might be passed through registers
			vector<string>allocated_args_registers;
			size_t nargs_on_stack=0;
			size_t nargs=args_.size();
			while(nargs--){
				const statement&arg=*args_[nargs];
				const stmt_def_func_param&param=f.param(nargs);
				// is the argument passed through a register?
				bool argument_passed_in_register=false;
				const string&arg_reg=param.get_register_or_empty();
				if(!arg_reg.empty()){
					argument_passed_in_register=true;
					tc.alloc_named_register_or_break(arg,arg_reg,os,indent_level);
					allocated_args_registers.push_back(arg_reg);
				}
				if(arg.is_expression()){
					if(!argument_passed_in_register){
						// no particular register requested for the argument
						const string&sr=tc.alloc_scratch_register(arg,os,indent_level);
						// compile expression with the result stored in arg_reg
						arg.compile(tc,os,indent_level+1,sr);
						// argument is passed to function through the stack
						indent(os,indent_level);os<<"push "<<sr<<endl;
						// free scratch register
						tc.free_scratch_register(sr,os,indent_level);
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
					tc.asm_cmd(arg,os,indent_level,"mov",arg_reg,id);
				}else{
					// push identifier on the stack
					indent(os,indent_level);os<<"push "<<id<<endl;
					nargs_on_stack++;
				}
			}

			// stack is: <base>,vars,regs,arguments,
			indent(os,indent_level);os<<"call "<<f.name()<<endl;

			const bool restore_rsp_to_base=tc.exit_func_call();

			// optimization to adjust rsp only once can be done if no registers need to be popped
			if(nregs_pushed_on_stack==0){
				// stack is: <base>,vars,args,
				if(restore_rsp_to_base){
					indent(os,indent_level);os<<"add rsp,"<<((nargs_on_stack+nvars_on_stack)<<3)<<endl;
					// stack is: <base>,
				}else{
					indent(os,indent_level);os<<"add rsp,"<<(nargs_on_stack<<3)<<endl;
					// stack is: <base>,vars,
				}
				// free named registers
				if(alloc_regs.size()!=0){
					const size_t alloc_regs_pop_idx=tc.get_func_call_alloc_reg_idx();
					size_t i=alloc_regs.size()-1;
					while(true){
						const string&reg=alloc_regs[i];
						// don't pop registers used to pass arguments
						if(find(allocated_args_registers.begin(),allocated_args_registers.end(),reg)!=allocated_args_registers.end()){
							tc.free_named_register(reg,os,indent_level);
						}
						if(i==alloc_regs_pop_idx)
							break;
						i--;
					}
				}
			}else{
				// stack is: <base>,vars,regs,args,
				if(nargs_on_stack){
					indent(os,indent_level);os<<"add rsp,"<<(nargs_on_stack<<3)<<endl;
				}
				// stack is: <base>,vars,regs,

				// pop registers pushed prior to this call
				const size_t alloc_regs_pop_idx=tc.get_func_call_alloc_reg_idx();
				size_t i=alloc_regs.size()-1;
				while(true){
					const string&reg=alloc_regs[i];
					// don't pop registers used to pass arguments
					if(find(allocated_args_registers.begin(),allocated_args_registers.end(),reg)==allocated_args_registers.end()){
						if(tc.is_register_initiated(reg)){
							// pop only registers that were pushed
							indent(os,indent_level);os<<"pop "<<reg<<endl;
						}
					}else{
						tc.free_named_register(reg,os,indent_level);
					}
					if(i==alloc_regs_pop_idx)
						break;
					i--;
				}
				// stack is: <base>,vars,
				if(restore_rsp_to_base){
					// this was not a call within the arguments of another call
					// stack is: <base>,vars,
					if(nvars_on_stack){
						indent(os,indent_level);os<<"add rsp,"<<(nvars_on_stack<<3)<<endl;
					}
					// stack is: <base>,
				}
			}

			// handle return value
			if(not dest_ident.empty()){
				// function returns value in rax, copy return value to dest_ident
				const string&dest_resolved=tc.resolve_ident_to_nasm(*this,dest_ident);
				tc.asm_cmd(*this,os,indent_level,"mov",dest_resolved,"rax");
			}

			return;
		}

		//
		// inlined function
		//

		vector<string>allocated_named_registers;
		vector<string>allocated_scratch_registers;
		vector<string>allocated_registers_in_order;

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
				allocated_registers_in_order.push_back(arg_reg);
			}
			if(arg->is_expression()){
				// argument is an expression, evaluate and store in arg_reg
				if(arg_reg.empty()){
					// no particular register requested for the argument
					arg_reg=tc.alloc_scratch_register(*arg,os,indent_level);
					allocated_scratch_registers.push_back(arg_reg);
					allocated_registers_in_order.push_back(arg_reg);
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
				tc.asm_cmd(param,os,indent_level+1,"mov",arg_reg,src);
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
		// free allocated registers in reverse order of allocation
		for(auto it=allocated_registers_in_order.rbegin();it!=allocated_registers_in_order.rend();++it) {
			const string&reg=*it;
			if(find(allocated_scratch_registers.begin(),allocated_scratch_registers.end(),reg)!=allocated_scratch_registers.end()){
				tc.free_scratch_register(reg,os,indent_level);
				continue;
			}
			if(find(allocated_named_registers.begin(),allocated_named_registers.end(),reg)!=allocated_named_registers.end()){
				tc.free_named_register(reg,os,indent_level);
				continue;
			}
			assert(false);
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

