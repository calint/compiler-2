#pragma once

class call_asm_int final:public call_asm{public:

	inline call_asm_int(const statement&parent,const token&tkn,tokenizer&t):
		call_asm{parent,tkn,t}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{// mov(eax 1)
		indent(os,indent_level,true);tc.source_to_as_comment(os,*this);

		indent(os,indent_level);
		os<<"int "<<tc.resolve_ident_to_nasm(arg(0),arg(0).identifier())<<endl;
	}

};
