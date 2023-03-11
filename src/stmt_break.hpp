#pragma once

class stmt_break final:public statement{
public:
	inline stmt_break(const statement&parent,const token&tk):
		statement{parent,tk}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.source_comment(os,*this);
		// get loop label
		const string&loop_label=tc.get_loop_label_or_break(*this);
		// jump out of the loop
		indent(os,indent_level);os<<"jmp "<<loop_label<<"_end\n";
	}
};
