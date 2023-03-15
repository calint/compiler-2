#pragma once

class stmt_break final:public statement{
public:
	inline stmt_break(token tk):
		statement{move(tk)}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		tc.source_comment(*this,os,indent_level);
		// get loop label
		const string&loop_label{tc.get_loop_label_or_break(*this)};
		// jump out of the loop
		tc.asm_jmp(*this,os,indent_level,loop_label+"_end");
	}
};
