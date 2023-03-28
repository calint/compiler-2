#pragma once

class stmt_continue final:public statement{
public:
	inline stmt_continue(toc&tc,token tk):
		statement{move(tk)}
	{
		set_type(tc.get_type(*this,toc::void_type_str));
	}

	inline stmt_continue()=default;
	inline stmt_continue(const stmt_continue&)=default;
	inline stmt_continue(stmt_continue&&)=default;
	inline stmt_continue&operator=(const stmt_continue&)=default;
	inline stmt_continue&operator=(stmt_continue&&)=default;

	inline void compile(toc&tc,ostream&os,size_t indent,const string&dst="")const override{
		tc.source_comment(*this,os,indent);
		// get current loop label
		const string&loop_label=tc.get_loop_label_or_break(*this);
		// jump to it
		tc.asm_jmp(*this,os,indent,loop_label);
	}
};
