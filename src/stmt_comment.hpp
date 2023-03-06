#pragma once

#include"compiler_error.hpp"
#include"statement.hpp"
#include"stmt_block.hpp"
#include"toc.hpp"
#include"token.hpp"
#include"tokenizer.hpp"

class stmt_comment final:public statement{public:

	inline stmt_comment(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk}
	{
		line_=t.read_rest_of_line();
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		os<<line_<<endl;
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.source_to_as_comment(os,*this);
	}

private:
	string line_;
};
