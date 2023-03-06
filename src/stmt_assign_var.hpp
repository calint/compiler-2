#pragma once

#include"expr_ops_list.hpp"

class stmt_assign_var final:public statement{public:

	inline stmt_assign_var(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk},
		oplist_{expr_ops_list{*this,t}}
	{}


	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		os<<"=";
		oplist_.source_to(os);
//		os<<";";
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.source_to_as_comment(os,*this);

		if(!oplist_.is_expression()){
			oplist_.compile(tc,os,indent_level,identifier());
			return;
		}


		if(oplist_.alloc_register()){

			const string&reg=tc.alloc_scratch_register(tok()); //? overrallocates in simplests cases ie a=1

			oplist_.compile(tc,os,indent_level,reg);

			const string&resolv=tc.resolve_ident_to_nasm(*this,tok().name());

			expr_ops_list::_asm("mov",*this,tc,os,indent_level+1,resolv,reg);

			tc.free_scratch_reg(reg);

			return;
		}

		oplist_.compile(tc,os,indent_level,identifier());
}

private:
	expr_ops_list oplist_;
};
