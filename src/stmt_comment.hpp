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

class stmt_comment final:public statement{public:

	inline stmt_comment(const statement&parent,const token&tkn,tokenizer&t):
		statement{parent,tkn}
	{
		comment_line_=t.read_rest_of_line();
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		os<<comment_line_<<endl;
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
//		indent(os,indent_level,true);
//		tc.source_location_to_stream(os,tok());
//		os<<comment_line<<endl;
	}

private:
	string comment_line_;
};
