#pragma once

#include"expr_ops_list.hpp"

class stmt_assign_var final:public statement{
public:
	inline stmt_assign_var(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk},
		oplist_{expr_ops_list{*this,t}}
	{}


	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		os<<"=";
		oplist_.source_to(os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.source_comment(os,*this);
		// compare generated instructions with and without allocated scratch register
		// select the method that produces least instructions
		stringstream ss1;
		oplist_.compile(tc,ss1,indent_level,identifier());

		stringstream ss2;
		const string&reg=tc.alloc_scratch_register(tok());
		oplist_.compile(tc,ss2,indent_level,reg);
		const string&resolv=tc.resolve_ident_to_nasm(*this);
		expr_ops_list::asm_cmd("mov",*this,tc,ss2,indent_level,resolv,reg);
		tc.free_scratch_reg(reg);

		const size_t ss1_count=count_instructions(ss1);
		const size_t ss2_count=count_instructions(ss2);
		if(ss1_count<ss2_count){
			os<<ss1.str();
		}else{
			os<<ss2.str();
		}
	}

private:
	inline static size_t count_instructions(stringstream&ss){
		regex regex("^\\s*;.*$");
		string line;
		size_t n=0;
		while(getline(ss,line)){
			if(regex_search(line,regex))
				continue;
			n++;
		}
		return n;
	}

	expr_ops_list oplist_;
};
