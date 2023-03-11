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

		vector<string>allocated_registers;

		if(!f.is_inline()){
			size_t nargs{args_.size()};
			const size_t rsp_delta{tc.get_current_stack_size()};
			if(tc.enter_func_call()){
				// if true then this is not a call within arguments of another call
				if(rsp_delta!=0){
					// adjust stack past the allocated vars
					expr_ops_list::asm_cmd("sub",*this,tc,os,indent_level,"rsp",to_string(rsp_delta<<3));
					// stack is now: <base>,.. vars ..,
				}
				// stack is now: <base>,
			}
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
					tc.alloc_scratch_register(param,arg_reg); // return ignored
					allocated_registers.push_back(arg_reg);
					argument_passed_in_register=true;
				}
				if(arg.is_expression()){
					// is the argument passed in through a register?
					if(!argument_passed_in_register){
						// no particular register requested for the argument
						arg_reg=tc.alloc_scratch_register(param);
						allocated_registers.push_back(arg_reg);
					}
					// compile expression with the result stored in arg_reg
					arg.compile(tc,os,indent_level+1,arg_reg);
					if(!argument_passed_in_register){
						// argument is passed to function through the stack
						indent(os,indent_level);os<<"push "<<arg_reg<<endl;
						// keep track of how many arguments are on the stack
						nargs_on_stack++;
					}
					continue;
				}
				// not an expression, resolve identifier to nasm
				const string&id=tc.resolve_ident_to_nasm(arg);
				if(argument_passed_in_register){
					// move the identifier to the requested register
					indent(os,indent_level);os<<"mov "<<arg_reg<<","<<id<<endl;
				}else{
					// push identifier on the stack
					indent(os,indent_level);os<<"push "<<id<<endl;
					nargs_on_stack++;
				}
			}
			//     stack is: <base>,.. vars ..,[arg n],[arg n-1],...,[arg 1],
			indent(os,indent_level);os<<"call "<<f.name()<<endl;
			//     stack is: <base>,.. vars ..,[arg n],[arg n-1],...,[arg 1],

			// restore stack pointer
			if(tc.exit_func_call()){
				// this is not a call within the arguments of another call
				// restore rsp to what it was before the call
				const size_t rsp_offset_to_base{rsp_delta+nargs_on_stack};
				if(rsp_offset_to_base){
					// if rsp needs to be moved to <base>
					indent(os,indent_level);os<<"add rsp,"<<(rsp_offset_to_base<<3)<<endl;
				}
				// stack is: <base>,
			}else{
				// this call is in the arguments of a different call
				// restore rsp past the arguments
				// stack is: <base>,.. other func args ..,[arg n],[arg n-1],...,[arg 1],
				if(!args_.empty()){
					indent(os,indent_level);os<<"add rsp,"<<(args_.size()<<3)<<endl;
					// stack is: <base>,.. other func args ..,
				}
			}

			if(not dest_ident.empty()){
				// function returns values in rax, copy return value to dest_ident
				indent(os,indent_level);os<<"mov "<<dest_ident<<",rax"<<endl;
			}

			// free allocated registers
			for(const string&r:allocated_registers){
				tc.free_scratch_reg(r);
			}
			return;
		}

		// inlined function
		vector<tuple<string,string>>aliases_to_add;
		if(not dest_ident.empty()){
			if(f.returns().empty())
				throw compiler_error(*this,"cannot assign from function without return");
			const string&from=f.returns()[0].name();
			const string&to=dest_ident;
			aliases_to_add.emplace_back(from,to);
		}

		size_t i=0;
		for(const auto&arg:args_){
			const auto&param=f.param(i);
			i++;
			string arg_reg=param.get_register_or_empty();
			if(!arg_reg.empty()){
				tc.alloc_scratch_register(*arg,arg_reg); // return ignored
				allocated_registers.push_back(arg_reg);
			}
			if(arg->is_expression()){
				// in reg
				if(arg_reg.empty()){ // no particular register requested for the argument
					arg_reg=tc.alloc_scratch_register(param);
					allocated_registers.push_back(arg_reg);
				}
				arg->compile(tc,os,indent_level+1,arg_reg);
				aliases_to_add.emplace_back(param.identifier(),arg_reg);
				continue;
			}
			if(arg_reg.empty()){
				const string&id=arg->identifier();
				aliases_to_add.emplace_back(param.identifier(),id);
			}else{
				const string&id=arg->identifier();
				aliases_to_add.emplace_back(param.identifier(),arg_reg);
				indent(os,indent_level+1);os<<"mov "<<arg_reg<<","<<tc.resolve_ident_to_nasm(param,id)<<endl;
			}
		}

		const string&call_path=tc.get_call_path(tok());
		const string&src_loc=tc.source_location(tok());
		const string&new_call_path=call_path.empty()?src_loc:(src_loc+"_"+call_path);
		const string&ret_jmp_label=nm+"_"+new_call_path+"_end";

		indent(os,indent_level+1,true);os<<"inline: "<<new_call_path<<endl;
		tc.push_func(nm,new_call_path,ret_jmp_label,true);
		for(const auto&e:aliases_to_add)
			tc.add_alias(get<0>(e),get<1>(e));
		f.code().compile(tc,os,indent_level);
		indent(os,indent_level);os<<ret_jmp_label<<":\n";
		for(const auto&r:allocated_registers)
			tc.free_scratch_reg(r);
		tc.pop_func(nm);
	}

	inline statement&arg(size_t ix)const{return*(args_[ix].get());}

	inline size_t arg_count()const{return args_.size();}

private:
	bool no_args_{false};
	vector<unique_ptr<statement>>args_;
};

