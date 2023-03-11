#pragma once

class stmt_return final:public statement{
public:
	inline stmt_return(const statement&parent,const token&tk):
		statement{parent,tk}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.source_comment(os,*this);
		const string&ret=tc.get_func_return_label_or_break(*this);
		if(ret.empty()){
			// not in-lined
			indent(os,indent_level);os<<"pop rbp\n";
			indent(os,indent_level);os<<"ret\n";
			return;
		}
		// in-lined
		indent(os,indent_level);os<<"jmp "<<ret<<endl;
	}
};
