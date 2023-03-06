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
#include<sstream>

class stmt_loop final:public stmt_call{public:

	inline stmt_loop(const statement&parent,const token&tk,tokenizer&t):
		stmt_call{parent,tk,t},
		code_{stmt_block{parent,t}}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.token_to_as_comment(os,this->tok());
		string lbl="loop_"+tc.source_location(tok());
		indent(os,indent_level,false);os<<lbl<<":"<<endl;
		tc.push_loop(lbl);
		code_.compile(tc,os,indent_level+1);
		indent(os,indent_level,false);os<<"jmp "<<lbl<<endl;
		indent(os,indent_level,false);os<<lbl<<"_end:"<<endl;
		tc.pop_loop(lbl);
	}

	inline void source_to(ostream&os)const override{
		stmt_call::source_to(os);
		code_.source_to(os);
	}

private:
	stmt_block code_;
};
