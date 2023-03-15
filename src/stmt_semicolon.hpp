#pragma once

// empty statement in case semi-colon is used
class stmt_semicolon final:public statement{
public:
	inline stmt_semicolon(token tk,tokenizer&t):
		statement{move(tk)}
	{}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		os<<';';
	}
};
