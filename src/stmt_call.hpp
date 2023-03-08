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
			auto arg=make_unique<expr_ops_list>(*this,t,true);
			args_.push_back(move(arg));
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

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.source_comment(os,*this);

		const string&nm=tok().name();
		if(!is_inline())
			throw compiler_error(*this,"function not inlined");

		const stmt_def_func&f=tc.get_func_or_break(*this,nm);
		if(f.params().size()!=args_.size())
			throw compiler_error(*this,"function '"+f.name()+"' expects "+to_string(f.params().size())+" argument"+(f.params().size()==1?"":"s")+" but "+to_string(args_.size())+" are provided");

		vector<tuple<string,string>>aliases_to_add;
		if(not dest_ident.empty()){
			if(f.returns().empty())
				throw compiler_error(*this,"cannot assign from function without return");
			const string&from=f.returns()[0].name();
			const string&to=dest_ident;
			aliases_to_add.push_back(make_tuple(from,to));
		}

		vector<string>allocated_registers;
		size_t i=0;
		for(const auto&a:args_){
			auto&param=f.param(i);
			i++;
			if(a->is_expression()){
				// in reg
				string reg;
				for(const auto&kw:param.keywords()){
					if(kw.name().find("reg_")==0){
						string name=kw.name().substr(4,kw.name().size());
						reg=tc.alloc_scratch_register(param,name);
						break;
					}
				}
				if(reg=="")
					reg=tc.alloc_scratch_register(param);
				allocated_registers.push_back(reg);
				a->compile(tc,os,indent_level+1,reg);
				aliases_to_add.push_back(make_tuple(param.identifier(),reg));
				continue;
			}

			const string&id=a->identifier();
			aliases_to_add.push_back(make_tuple(param.identifier(),id));
		}

		tc.push_func(nm);

		for(const auto&e:aliases_to_add)
			tc.add_alias(get<0>(e),get<1>(e));

		f.code().compile(tc,os,indent_level);

		indent(os,indent_level);os<<nm<<"_"<<tc.source_location(tok())<<"_end:"<<endl;

		for(const auto&r:allocated_registers)
			tc.free_scratch_reg(r);

		tc.pop_func(nm);
	}

	inline statement&arg(size_t ix)const{return*(args_[ix].get());}

	inline size_t arg_count()const{return args_.size();}

	inline bool is_inline()const{return true;}

private:
	bool no_args_{false};
	vector<unique_ptr<statement>>args_;
};

