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
//			expr_ops_list(*this,t.next_whitespace_token(),t,'=',3,false,create_statement_from_tokenizer(*this,t),0);
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
			if(i--)os<<",";
		}
		os<<")";
	}

	static void replaceSubstring(string& original_string,const string& substring_to_replace,const string& replacement_string){
		size_t start_pos=original_string.find(substring_to_replace);
		if(start_pos!=string::npos){
			original_string.replace(start_pos,substring_to_replace.length(),replacement_string);
		}
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.source_comment(os,*this);

		const string&nm=tok().name();
		const stmt_def_func&f=tc.get_func_or_break(*this,nm);
		if(f.params().size()!=args_.size())
			throw compiler_error(*this,"function '"+f.name()+"' expects "+to_string(f.params().size())+" argument"+(f.params().size()==1?"":"s")+" but "+to_string(args_.size())+" are provided");

		vector<string>allocated_registers;

		if(!f.is_inline()){
			size_t nargs=args_.size();
			const size_t rsp_delta=tc.get_current_stack_base();
			size_t rsp_delta_with_args{rsp_delta+nargs};
			// initial stack pointer is saved in scratch register if there
			//   are arguments to be pushed on the stack
			string initial_rsp;
			if(rsp_delta!=0){
				// adjust stack past the allocated vars
				if(nargs){
					// if there are arguments then save the initial rsp
					//   because rsp will change while pushing arguments on the stack
					initial_rsp=tc.alloc_scratch_register(tok());
					allocated_registers.push_back(initial_rsp);
					expr_ops_list::asm_cmd("mov",*this,tc,os,indent_level,initial_rsp,"rsp");
				}
				expr_ops_list::asm_cmd("sub",*this,tc,os,indent_level,"rsp",to_string(rsp_delta<<3));
			}
			// stack is now: base,.. vars ..,
			// push arguments
			while(nargs--){
				const statement&arg=*args_[nargs];
				const stmt_def_func_param&param=f.param(nargs);
				if(arg.is_expression()){
					// in reg
					string reg;
					for(const auto&kw:param.keywords()){
						if(kw.name().find("reg_")==0){
							// requested register for this argument
							string reqreg=kw.name().substr(4,kw.name().size());
							reg=tc.alloc_scratch_register(param,reqreg);
							break;
						}
					}
					if(reg=="") // no particular register requested for the argument
						reg=tc.alloc_scratch_register(param);

					allocated_registers.push_back(reg);
					arg.compile(tc,os,indent_level+1,reg);
					indent(os,indent_level);os<<"push "<<reg<<endl;
					continue;
				}

				string id=tc.resolve_ident_to_nasm(arg);
				// id uses rsp as base but rsp is changing while pushing
				//   so use the saved inital rsp to refer arguments
				if(!initial_rsp.empty()){
					// replace rsp with register name of initial rsp
					replaceSubstring(id,string{"rsp"},initial_rsp);
				}
				indent(os,indent_level);os<<"push "<<id<<endl;
			}
			// free the allocated registers
			for(const string&r:allocated_registers)
				tc.free_scratch_reg(r);
			allocated_registers.clear();

			//     stack is: base,.. vars ..,[arg n],[arg n-1],...,[arg 1]
			indent(os,indent_level);os<<"call "<<f.name()<<endl;

			//     stack is: base,.. vars ..,[arg n],[arg n-1],...,[arg 1]
			if(rsp_delta_with_args!=0){
				// restore rsp to what it was before the call
				indent(os,indent_level);os<<"add rsp,"<<(rsp_delta_with_args<<3)<<endl;
			}
			// stack is: base
			assert(allocated_registers.size()==0);
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
			if(arg->is_expression()){
				// in reg
				string reg;
				for(const auto&kw:param.keywords()){
					if(kw.name().find("reg_")==0){
						// requested register for this argument
						string reqreg=kw.name().substr(4,kw.name().size());
						reg=tc.alloc_scratch_register(param,reqreg);
						break;
					}
				}
				if(reg=="") // no particular register requested for the argument
					reg=tc.alloc_scratch_register(param);

				allocated_registers.push_back(reg);
				arg->compile(tc,os,indent_level+1,reg);
				aliases_to_add.emplace_back(param.identifier(),reg);
				continue;
			}

			const string&id=arg->identifier();
			aliases_to_add.emplace_back(param.identifier(),id);
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

