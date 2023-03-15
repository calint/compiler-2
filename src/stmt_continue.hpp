#pragma once

class stmt_continue final:public statement{
public:
	inline stmt_continue(const token&tk):
		statement{tk}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		tc.source_comment(*this,os,indent_level);
		// get current loop label
		const string&loop_label=tc.get_loop_label_or_break(*this);
		// jump to it
		tc.asm_jmp(*this,os,indent_level,loop_label);
	}
};
