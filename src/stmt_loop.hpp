#pragma once

class stmt_loop final:public statement{
public:
	inline stmt_loop(toc&tc,token tk,tokenizer&t):
		statement{move(tk)}
	{
		set_type(tc.get_type(*this,toc::void_type_str));
		const string&lbl{"loop_"+tc.source_location_for_label(tok())};
		tc.enter_loop(lbl);
		code_={tc,t};
		tc.exit_loop(lbl);
	}

	inline stmt_loop()=default;
	inline stmt_loop(const stmt_loop&)=default;
	inline stmt_loop(stmt_loop&&)=default;
	inline stmt_loop&operator=(const stmt_loop&)=default;
	inline stmt_loop&operator=(stmt_loop&&)=default;

	inline void compile(toc&tc,ostream&os,size_t indent,const string&dst="")const override{
		toc::indent(os,indent,true);tc.token_comment(os,tok());
		// make unique label for this loop considering in-lined functions
		// current path of source locations where in-lined functions have been called
		const string&call_path{tc.get_inline_call_path(tok())};
		// current source location
		const string&src_loc{tc.source_location_for_label(tok())};
		// the loop label
		const string&lbl{"loop_"+(call_path.empty()?src_loc:(src_loc+"_"+call_path))};
		tc.asm_label(*this,os,indent,lbl);
		// enter loop scope
		tc.enter_loop(lbl);
		code_.compile(tc,os,indent);
		// jump to loop
		tc.asm_jmp(*this,os,indent,lbl);
		// exit loop label
		tc.asm_label(*this,os,indent,lbl+"_end");
		// exit loop scope
		tc.exit_loop(lbl);
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		code_.source_to(os);
	}

private:
	stmt_block code_;
};
