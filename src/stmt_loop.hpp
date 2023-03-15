#pragma once

class stmt_loop final:public statement{
public:
	inline stmt_loop(const token&tk,tokenizer&t):
		statement{tk},
		code_{stmt_block{t}}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		toc::indent(os,indent_level,true);tc.token_comment(os,this->tok());
		// make unique label for this loop considering in-lined functions
		// current path of source locations where in-lined functions have been called
		const string&call_path=tc.get_call_path(tok());
		// current source location
		const string&src_loc=tc.source_location(tok());
		// the loop label
		const string lbl="loop_"+(call_path.empty()?src_loc:(src_loc+"_"+call_path));
		tc.asm_label(*this,os,indent_level,lbl);
//		toc::indent(os,indent_level);os<<lbl<<":"<<endl;
		// enter loop scope
		tc.push_loop(lbl);
		code_.compile(tc,os,indent_level);
		// jump to loop
		tc.asm_jmp(*this,os,indent_level,lbl);
//		toc::indent(os,indent_level);os<<"jmp "<<lbl<<endl;
		// exit loop label
		tc.asm_label(*this,os,indent_level,lbl+"_end");
//		toc::indent(os,indent_level);os<<lbl<<"_end:"<<endl;
		// exit loop scope
		tc.pop_loop(lbl);
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		code_.source_to(os);
	}

private:
	stmt_block code_;
};
