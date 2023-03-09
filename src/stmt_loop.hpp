#pragma once

class stmt_loop final:public stmt_call{
public:
	inline stmt_loop(const statement&parent,const token&tk,tokenizer&t):
		stmt_call{parent,tk,t},
		code_{stmt_block{parent,t}}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.token_comment(os,this->tok());
		const string&call_path=tc.get_call_path(tok());
		const string&src_loc=tc.source_location(tok());
		const string lbl="loop_"+(call_path.empty()?src_loc:(src_loc+"_"+call_path));
		indent(os,indent_level);os<<lbl<<":"<<endl;
		tc.push_loop(lbl);
		code_.compile(tc,os,indent_level);
		indent(os,indent_level);os<<"jmp "<<lbl<<endl;
		indent(os,indent_level);os<<lbl<<"_end:"<<endl;
		tc.pop_loop(lbl);
	}

	inline void source_to(ostream&os)const override{
		stmt_call::source_to(os);
		code_.source_to(os);
	}

private:
	stmt_block code_;
};
