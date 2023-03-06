#pragma once

class stmt_continue final:public stmt_call{
public:
	inline stmt_continue(const statement&parent,const token&tk,tokenizer&t):
		stmt_call{parent,tk,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.source_comment(os,*this);
		const string&loop_name=tc.find_parent_loop_name();
		indent(os,indent_level,false);os<<"jmp "<<loop_name<<"\n";
	}
};
