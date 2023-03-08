#pragma once

class stmt_break final:public stmt_call{
public:
	inline stmt_break(const statement&parent,const token&tk,tokenizer&t):
		stmt_call{parent,tk,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.source_comment(os,*this);
		const string&loop_name=tc.get_loop_or_break(*this);
		indent(os,indent_level);os<<"jmp "<<loop_name<<"_end\n";
	}
};
