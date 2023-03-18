#pragma once

#include"expression.hpp"
#include"tokenizer.hpp"
#include"expr_ops_list.hpp"
#include"stmt_def_func.hpp"

class stmt_call:public expression{
public:
	inline stmt_call(token tk,tokenizer&t):
		expression{move(tk)}
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
			args_.emplace_back(make_unique<expr_ops_list>(t,true));
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
		tc.source_comment(*this,os,indent_level);
		const string&nm=tok().name();
		const stmt_def_func&f=tc.get_func_or_break(*this,nm);
		
		if(f.params().size()!=args_.size())
			throw compiler_error(*this,"function '"+f.name()+"' expects "+to_string(f.params().size())+" argument"+(f.params().size()==1?"":"s")+" but "+to_string(args_.size())+" are provided");

		if(!f.is_inline()){
			// stack is: <base>,
			tc.enter_call(*this,os,indent_level);
			// stack is: <base>,vars,regs,

			// push arguments starting with the last
			// some arguments might be passed through registers
			vector<string>allocated_args_registers;
			size_t nbytes_of_args_on_stack=0;
			size_t i=args_.size();
			while(i--){
				const expr_ops_list&arg=*args_[i];
				const stmt_def_func_param&param=f.param(i);
				// is the argument passed through a register?
				const string&arg_reg=param.get_register_or_empty();
				bool argument_passed_in_register=false;
				if(!arg_reg.empty()){
					argument_passed_in_register=true;
					tc.alloc_named_register_or_break(arg,os,indent_level,arg_reg);
					allocated_args_registers.push_back(arg_reg);
				}
				if(arg.is_expression()){
					if(!argument_passed_in_register){
						// no particular register requested for the argument
						const string&sr=tc.alloc_scratch_register(arg,os,indent_level);
						// compile expression with the result stored in sr
						arg.compile(tc,os,indent_level+1,sr);
						// argument is passed to function through the stack
						tc.asm_push(arg,os,indent_level,sr);
						// free scratch register
						tc.free_scratch_register(os,indent_level,sr);
						// keep track of how many arguments are on the stack
						nbytes_of_args_on_stack+=8;
					}else{
						// compile expression with the result stored in arg_reg
						arg.compile(tc,os,indent_level+1,arg_reg);
					}
					continue;
				}
				// not an expression, resolve identifier to nasm
				const ident_resolved&ir{tc.resolve_ident_to_nasm(arg)};
				if(argument_passed_in_register){
					// move the identifier to the requested register
					if(ir.is_const()){
						tc.asm_cmd(arg,os,indent_level,"mov",arg_reg,ir.as_const());
					}else{
						tc.asm_cmd(arg,os,indent_level,"mov",arg_reg,ir.id);
						if(ir.negated){
							tc.asm_neg(arg,os,indent_level,arg_reg);
						}					}
				}else{
					// push identifier on the stack
					if(ir.is_const()){
						tc.asm_push(arg,os,indent_level,ir.as_const());
					}else{
						if(ir.negated){
							const string&sr=tc.alloc_scratch_register(arg,os,indent_level);
							tc.asm_cmd(arg,os,indent_level,"mov",sr,ir.id);
							tc.asm_neg(arg,os,indent_level,sr);
							tc.asm_push(arg,os,indent_level,sr);
							tc.free_scratch_register(os,indent_level,sr);
						}else{
							tc.asm_push(arg,os,indent_level,ir.id);
						}
					}
					nbytes_of_args_on_stack+=8;
				}
			}

			// stack is: <base>,vars,regs,args,
			tc.asm_call(*this,os,indent_level,f.name());

			tc.exit_call(*this,os,indent_level,nbytes_of_args_on_stack,allocated_args_registers);
			// stack is: <base>, (if this was not a call nested in another call's arguments)
			//       or: <base>,vars,regs,

			// handle return value
			if(not dest_ident.empty()){
				// function returns value in rax, copy return value to dest_ident
				const ident_resolved&ir{tc.resolve_ident_to_nasm(*this,dest_ident,false)};
				tc.asm_cmd(*this,os,indent_level,"mov",ir.id,"rax");
			}

			return;
		}

		//
		// inlined function
		//

		tc.indent(os,indent_level,true);
		f.source_def_comment_to(os);

		// create unique labels for in-lined functions based on the location the source
		// where the call occurred
		const string&call_path=tc.get_inline_call_path(tok());
		const string&src_loc=tc.source_location(tok());
		const string&new_call_path=call_path.empty()?src_loc:(src_loc+"_"+call_path);
		const string&ret_jmp_label=nm+"_"+new_call_path+"_end";

		toc::indent(os,indent_level+1,true);os<<"inline: "<<new_call_path<<endl;

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
			tc.indent(os,indent_level+1,true);os<<"alias "<<from<<" -> "<<to<<endl;
		}

		size_t i=0;
		for(const auto&arg:args_){
			const stmt_def_func_param&param=f.param(i);
			i++;
			// does the parameter want the value passed through a register?
			string arg_reg=param.get_register_or_empty();
			if(!arg_reg.empty()){
				// argument is passed through register
				tc.alloc_named_register_or_break(*arg,os,indent_level+1,arg_reg);
				allocated_named_registers.push_back(arg_reg);
				allocated_registers_in_order.push_back(arg_reg);
			}
			if(arg->is_expression()){
				// argument is an expression, evaluate and store in arg_reg
				if(arg_reg.empty()){
					// no particular register requested for the argument
					arg_reg=tc.alloc_scratch_register(*arg,os,indent_level+1);
					allocated_scratch_registers.push_back(arg_reg);
					allocated_registers_in_order.push_back(arg_reg);
				}
				tc.indent(os,indent_level+1,true);os<<"alias "<<param.identifier()<<" -> "<<arg_reg<<endl;
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
				if(arg->is_negated()){
					const ident_resolved&ir=tc.resolve_ident_to_nasm(*arg);
					const string&sr=tc.alloc_scratch_register(*arg,os,indent_level+1);
					allocated_registers_in_order.push_back(sr);
					allocated_scratch_registers.push_back(sr);
					tc.asm_cmd(param,os,indent_level+1,"mov",sr,ir.id);
					tc.asm_neg(param,os,indent_level+1,sr);
					aliases_to_add.emplace_back(param.identifier(),sr);
					tc.indent(os,indent_level+1,true);os<<"alias "<<param.identifier()<<" -> "<<sr<<endl;
				}else{
					const string&id=arg->identifier();
					aliases_to_add.emplace_back(param.identifier(),id);
					tc.indent(os,indent_level+1,true);os<<"alias "<<param.identifier()<<" -> "<<id<<endl;
				}
			}else{
				// alias parameter name to the register
				aliases_to_add.emplace_back(param.identifier(),arg_reg);
				tc.indent(os,indent_level+1,true);os<<"alias "<<param.identifier()<<" -> "<<arg_reg<<endl;
				// move argument to register
				const ident_resolved&ir{tc.resolve_ident_to_nasm(*arg)};
				if(ir.is_const()){
					tc.asm_cmd(param,os,indent_level+1,"mov",arg_reg,ir.as_const());
				}else{
					tc.asm_cmd(param,os,indent_level+1,"mov",arg_reg,ir.id);
					if(ir.negated){
						tc.asm_neg(param,os,indent_level+1,arg_reg);
					}
				}
			}
		}

		// enter function creating a new scope from which 
		//   prior variables are not visible
		if(f.returns().empty()){
			tc.enter_func(nm,new_call_path,ret_jmp_label,true,"");
		}else{
			tc.enter_func(nm,new_call_path,ret_jmp_label,true,f.returns()[0].name());
		}

		// add the aliases to the context of this scope
		for(const auto&e:aliases_to_add){
			const string&from=get<0>(e);
			const string&to=get<1>(e);
			tc.add_alias(from,to);
		}

		// compile in-lined code
		f.code().compile(tc,os,indent_level);

		// free allocated registers in reverse order of allocation
		for(auto it=allocated_registers_in_order.rbegin();it!=allocated_registers_in_order.rend();++it) {
			const string&reg=*it;
			if(find(allocated_scratch_registers.begin(),allocated_scratch_registers.end(),reg)!=allocated_scratch_registers.end()){
				tc.free_scratch_register(os,indent_level+1,reg);
				continue;
			}
			if(find(allocated_named_registers.begin(),allocated_named_registers.end(),reg)!=allocated_named_registers.end()){
				tc.free_named_register(os,indent_level+1,reg);
				continue;
			}
			assert(false);
		}

		// provide an exit label for 'return' to jump to instead of assembler 'ret'
		tc.asm_label(*this,os,indent_level,ret_jmp_label);

		// pop scope
		tc.exit_func(nm);
	}

	inline statement&arg(size_t ix)const{return*(args_[ix].get());}

	inline size_t arg_count()const{return args_.size();}

private:
	bool no_args_{false};
	vector<unique_ptr<expr_ops_list>>args_;
};

