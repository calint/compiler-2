#pragma once

#include"expression.hpp"
#include"tokenizer.hpp"
#include"expr_ops_list.hpp"
#include"stmt_def_func.hpp"
#include"unary_ops.hpp"

class stmt_call:public expression{
public:
	inline stmt_call(token tk,unary_ops uops,tokenizer&t):
		expression{move(tk),move(uops)}
	{
		if(not t.is_next_char('(')){
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
			args_.emplace_back(t,true);
			if(t.is_next_char(')'))
				break;
			if(not t.is_next_char(','))
				throw compiler_error(args_.back(),"expected ',' after argument '"+args_.back().identifier()+"'");
			expect_arg=true;
		}
	}

	inline stmt_call()=default;
	inline stmt_call(const stmt_call&)=default;
	inline stmt_call(stmt_call&&)=default;
	inline stmt_call&operator=(const stmt_call&)=default;
	inline stmt_call&operator=(stmt_call&&)=default;


	inline void source_to(ostream&os)const override{
		expression::source_to(os);
		if(no_args_)
			return;
		os<<"(";
		size_t i=args_.size()-1;
		for(const auto&e:args_){
			e.source_to(os);
			if(i--){
				os<<",";
			}
		}
		os<<")";
	}

	inline void compile(toc&tc,ostream&os,size_t indent,const string&dst="")const override{
		tc.source_comment(*this,os,indent);
		const stmt_def_func&func{tc.get_func_or_break(*this,tok().name())};
		const string&func_nm{func.name()};
		
		if(func.params().size()!=args_.size())
			throw compiler_error(*this,"function '"+func_nm+"' expects "+
				to_string(func.params().size())+" argument"+(func.params().size()==1?"":"s")+
				" but "+to_string(args_.size())+" "+(args_.size()==1?"is":"are")+" provided");

		if(not func.is_inline()){
			// stack is: <base>,
			tc.enter_call(*this,os,indent);
			// stack is: <base>,vars,regs,

			// push arguments starting with the last
			// some arguments might be passed through registers
			vector<string>allocated_args_registers;
			size_t nbytes_of_args_on_stack{0};
			size_t i{args_.size()};
			while(i--){
				const expr_ops_list&arg{args_[i]};
				const stmt_def_func_param&param{func.param(i)};
				const type&arg_type=arg.get_type(tc);
				const type&param_type=param.get_type(tc);
				if(arg_type.name()!=param_type.name())
					throw compiler_error(arg,"argument "+to_string(i+1)+" of type '"+arg_type.name()+"' does not match parameter '"+param_type.name()+"'");
				// is the argument passed through a register?
				const string&arg_reg=param.get_register_or_empty();
				bool argument_passed_in_register{not arg_reg.empty()};
				if(argument_passed_in_register){
					tc.alloc_named_register_or_break(arg,os,indent,arg_reg);
					allocated_args_registers.push_back(arg_reg);
				}
				if(arg.is_expression()){
					if(argument_passed_in_register){
						// compile expression with the result stored in arg_reg
						arg.compile(tc,os,indent+1,arg_reg);
					}else{
						// argument passed through stack
						const string&sr{tc.alloc_scratch_register(arg,os,indent)};
						// compile expression with the result stored in sr
						arg.compile(tc,os,indent+1,sr);
						// argument is passed to function through the stack
						tc.asm_push(arg,os,indent,sr);
						// free scratch register
						tc.free_scratch_register(os,indent,sr);
						// keep track of how many arguments are on the stack
						nbytes_of_args_on_stack+=8;
					}
					continue;
				}
				// not an expression, resolve identifier to nasm
				const ident_resolved&ir{tc.resolve_ident_to_nasm(arg,true)};
				if(argument_passed_in_register){
					// move the identifier to the requested register
					if(ir.is_const()){
						tc.asm_cmd(arg,os,indent,"mov",arg_reg,arg.get_unary_ops().get_ops_as_string()+ir.id);
					}else{
						tc.asm_cmd(arg,os,indent,"mov",arg_reg,ir.id);
						arg.get_unary_ops().compile(tc,os,indent,arg_reg);
					}
				}else{
					// push identifier on the stack
					if(ir.is_const()){
						tc.asm_push(arg,os,indent,arg.get_unary_ops().get_ops_as_string()+ir.id);
					}else{
						if(arg.get_unary_ops().is_empty()){
							if(ir.tp.size()==toc::default_type_size){
								tc.asm_push(arg,os,indent,ir.id);
							}else{
								// value to be pushed is not a 64 bit value
								// push can only be done with 64 or 16 bits values
								const string&r{tc.alloc_scratch_register(arg,os,indent)};
								tc.asm_cmd(arg, os, indent,"mov",r,ir.id);
								tc.asm_push(arg,os,indent,r);
								tc.free_scratch_register(os,indent,r);
							}
						}else{
							const string&r{tc.alloc_scratch_register(arg,os,indent)};
							tc.asm_cmd(arg,os,indent,"mov",r,ir.id);
							arg.get_unary_ops().compile(tc,os,indent,r);
							tc.asm_push(arg,os,indent,r);
							tc.free_scratch_register(os,indent,r);
						}
					}
					nbytes_of_args_on_stack+=8;
				}
			}

			// stack is: <base>,vars,regs,args,
			tc.asm_call(*this,os,indent,func_nm);

			tc.exit_call(*this,os,indent,nbytes_of_args_on_stack,allocated_args_registers);
			// stack is: <base>, (if this was not a call nested in another call's arguments)
			//       or: <base>,vars,regs,

			// handle return value
			if(not dst.empty()){
				if(func.returns().empty())
					throw compiler_error(*this,"function call does not return a value");

				// function returns value in rax, copy return value to dst
				get_unary_ops().compile(tc,os,indent,"rax");
				const ident_resolved&ir{tc.resolve_ident_to_nasm(*this,dst,false)};
				const type&return_type{tc.get_type(*this,func.get_return_type_str())};
				if(return_type.size()>ir.tp.size())
					throw compiler_error(*this,"return type '"+func.get_return_type_str()+"' would be truncated when copied to '"+ir.tp.name()+"'");

				tc.asm_cmd(*this,os,indent,"mov",ir.id,"rax");
			}
			return;
		}

		//
		// inlined function
		//

		tc.indent(os,indent,true);
		func.source_def_comment_to(os);

		// create unique labels for in-lined functions based on the location the source
		// where the call occurred
		const string&call_path{tc.get_inline_call_path(tok())};
		const string&src_loc{tc.source_location_for_label(tok())};
		const string&new_call_path{call_path.empty()?src_loc:(src_loc+"_"+call_path)};
		const string&ret_jmp_label{func_nm+"_"+new_call_path+"_end"};

		toc::indent(os,indent+1,true);os<<"inline: "<<new_call_path<<endl;

		vector<string>allocated_named_registers;
		vector<string>allocated_scratch_registers;
		vector<string>allocated_registers_in_order;

		// buffer the aliases of arguments
		vector<tuple<string,string>>aliases_to_add;

		// if function returns value
		if(not dst.empty()){
			if(func.returns().empty())
				throw compiler_error(*this,"cannot assign from function without return");
			// alias 'from' identifier to 'dst' identifier
			const string&from{func.returns()[0].name()};
			const string&to{dst};
			aliases_to_add.emplace_back(from,to);
			tc.indent(os,indent+1,true);os<<"alias "<<from<<" -> "<<to<<endl;
		}

		size_t i=0;
		for(const auto&arg:args_){
			const stmt_def_func_param&param{func.param(i)};
			i++;
			const type&arg_type=arg.get_type(tc);
			const type&param_type=param.get_type(tc);
			if(arg_type.name()!=param_type.name())
				throw compiler_error(arg,"argument "+to_string(i)+" of type '"+arg_type.name()+"' does not match parameter '"+param_type.name()+"'");
			// does the parameter want the value passed through a register?
			string arg_reg{param.get_register_or_empty()};
			if(not arg_reg.empty()){
				// argument is passed through register
				tc.alloc_named_register_or_break(arg,os,indent+1,arg_reg);
				allocated_named_registers.push_back(arg_reg);
				allocated_registers_in_order.push_back(arg_reg);
			}
			if(arg.is_expression()){
				// argument is an expression, evaluate and store in arg_reg
				if(arg_reg.empty()){
					// no particular register requested for the argument
					arg_reg=tc.alloc_scratch_register(arg,os,indent+1);
					allocated_scratch_registers.push_back(arg_reg);
					allocated_registers_in_order.push_back(arg_reg);
				}
				tc.indent(os,indent+1,true);os<<"alias "<<param.identifier()<<" -> "<<arg_reg<<endl;
				// compile expression and store result in 'arg_reg'
				arg.compile(tc,os,indent+1,arg_reg);
				// alias parameter name to the register containing its value
				aliases_to_add.emplace_back(param.identifier(),arg_reg);
				continue;
			}

			// argument is not an expression

			if(arg_reg.empty()){
				// no register allocated for the argument
				// alias parameter name to the argument identifier
				if(not arg.get_unary_ops().is_empty()){
					const ident_resolved&ir{tc.resolve_ident_to_nasm(arg,true)};
					const string&sr{tc.alloc_scratch_register(arg,os,indent+1)};
					allocated_registers_in_order.push_back(sr);
					allocated_scratch_registers.push_back(sr);
					tc.asm_cmd(param,os,indent+1,"mov",sr,ir.id);
					arg.get_unary_ops().compile(tc,os,indent+1,sr);
					aliases_to_add.emplace_back(param.identifier(),sr);
					tc.indent(os,indent+1,true);os<<"alias "<<param.identifier()<<" -> "<<sr<<endl;
				}else{
					const string&id=arg.identifier();
					aliases_to_add.emplace_back(param.identifier(),id);
					tc.indent(os,indent+1,true);os<<"alias "<<param.identifier()<<" -> "<<id<<endl;
				}
			}else{
				// alias parameter name to the register
				aliases_to_add.emplace_back(param.identifier(),arg_reg);
				tc.indent(os,indent+1,true);os<<"alias "<<param.identifier()<<" -> "<<arg_reg<<endl;
				// move argument to register
				const ident_resolved&ir{tc.resolve_ident_to_nasm(arg,true)};
				if(ir.is_const()){
					const ident_resolved&arg_r{tc.resolve_ident_to_nasm(arg,true)};
					tc.asm_cmd(param,os,indent+1,"mov",arg_reg,arg.get_unary_ops().get_ops_as_string()+ir.id);
				}else{
					tc.asm_cmd(param,os,indent+1,"mov",arg_reg,ir.id);
					arg.get_unary_ops().compile(tc,os,indent+1,arg_reg);
				}
			}
		}

		// enter function creating a new scope from which 
		//   prior variables are not visible
		tc.enter_func(func_nm,new_call_path,ret_jmp_label,true,func.returns().empty()?"":func.returns()[0].name());

		// add the aliases to the context of this scope
		for(const auto&e:aliases_to_add){
			const string&from{get<0>(e)};
			const string&to{get<1>(e)};
			tc.add_alias(from,to);
		}

		// compile in-lined code
		func.code().compile(tc,os,indent);

		// free allocated registers in reverse order of allocation
		for(auto it=allocated_registers_in_order.rbegin();it!=allocated_registers_in_order.rend();++it) {
			const string&reg{*it};
			if(find(allocated_scratch_registers.begin(),allocated_scratch_registers.end(),reg)!=allocated_scratch_registers.end()){
				tc.free_scratch_register(os,indent+1,reg);
				continue;
			}
			if(find(allocated_named_registers.begin(),allocated_named_registers.end(),reg)!=allocated_named_registers.end()){
				tc.free_named_register(os,indent+1,reg);
				continue;
			}
			assert(false);
		}

		// provide an exit label for 'return' to jump to instead of assembler 'ret'
		tc.asm_label(*this,os,indent,ret_jmp_label);
		// if the result of the call has unary ops
		if(not get_unary_ops().is_empty()){
			if(func.returns().empty())
				throw compiler_error(*this,"function call has unary operations but it does not return a value");

			const ident_resolved&ir{tc.resolve_ident_to_nasm(*this,func.returns()[0].name(),true)};
			get_unary_ops().compile(tc,os,indent,ir.id);
		}
		// pop scope
		tc.exit_func(func_nm);
	}

	inline const statement&arg(size_t ix)const{return args_[ix];}

	inline size_t arg_count()const{return args_.size();}

private:
	vector<expr_ops_list>args_;
	bool no_args_{};
};

