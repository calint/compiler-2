#pragma once

class stmt_return final:public statement{
public:
	inline stmt_return(const statement&parent,const token&tk):
		statement{parent,tk}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		toc::indent(os,indent_level,true);tc.source_comment(os,*this);
		const string&ret=tc.get_func_return_label_or_break(*this);
		if(ret.empty()){
			// not in-lined
			const string&ret_var=tc.get_func_return_var_name_or_break(*this);
			if(!ret_var.empty()){
				const string&src_resolved=tc.resolve_ident_to_nasm(*this,ret_var);
				tc.asm_cmd(*this,os,indent_level,"mov","rax",src_resolved);
			}
			tc.asm_pop(*this,os,indent_level,"rbp");
//			indent(os,indent_level);os<<"pop rbp\n";
			toc::indent(os,indent_level);os<<"ret\n";
			return;
		}
		// in-lined
		toc::indent(os,indent_level);os<<"jmp "<<ret<<endl;
	}
};
