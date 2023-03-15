#pragma once

// empty statement in case semi-colon is used
class stmt_semicolon final:public statement{
public:
	inline stmt_semicolon(const token&tk,tokenizer&t):
		statement{tk}
	{}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		os<<';';
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
	}
};
