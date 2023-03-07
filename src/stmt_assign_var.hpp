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
		oplist_.compile(tc,os,indent_level,identifier());
	}

private:
	expr_ops_list oplist_;
};
