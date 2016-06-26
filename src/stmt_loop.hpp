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
		name{"_loop_"+to_string(tk.char_index_in_source())},
		code{stmt_block{parent,t}}
	{}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		indent(os,indent_level,false);
		os<<name<<":  ; ";
		tc.source_location_to_stream(os,tok());
		os<<endl;

		tc.push_loop(name.data());

		code.compile(tc,os,indent_level+1);
		indent(os,indent_level+1,false);
		os<<"jmp "<<name<<endl;

		indent(os,indent_level,false);
		os<<"_end"<<name<<":  ; ";
		tc.source_location_to_stream(os,tok());
		os<<endl;

		tc.pop_loop(name.data());
	}

	inline void link(toc&tc,ostream&os)const override final{
		code.link(tc,os);
	}

	inline void source_to(ostream&os)const override{
		stmt_call::source_to(os);
		code.source_to(os);
	}


private:
	string name;
	stmt_block code;
};
