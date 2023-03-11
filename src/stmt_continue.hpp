#pragma once

class stmt_continue final:public statement{
public:
	inline stmt_continue(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.source_comment(os,*this);
		// get current loop label
		const string&loop_label=tc.get_loop_label_or_break(*this);
		// jump to it
		indent(os,indent_level);os<<"jmp "<<loop_label<<"\n";
	}
};
