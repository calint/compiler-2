#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "compiler_error.hpp"
#include "statement.hpp"
#include "stmt_block.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

// empty statement in case semi-colon is used
class stmt_semicolon final:public statement{public:

	inline stmt_semicolon(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk}
	{}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		os<<';';
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
	}
};
