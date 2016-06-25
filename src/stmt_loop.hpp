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

	inline stmt_loop(statement*parent,const token&tkn,tokenizer&t):
		stmt_call{parent,tkn,t}
	{
		code=make_unique<stmt_block>(parent,t);
		name="_loop_"+to_string(tok().token_start_char());
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		indent(os,indent_level,false);
		os<<name<<":  ; ";
		tc.source_location_to_stream(os,tok());
		os<<endl;

		tc.framestk().push_loop(name.data());

		code->compile(tc,os,indent_level+1);
		indent(os,indent_level+1,false);
		os<<"jmp "<<name<<endl;
//		os<<"jmp _loop";
//		tc.source_location_for_identifier_to_stream(os,tok());
//		os<<token().token_start_char();
//		os<<endl;

		indent(os,indent_level,false);
		os<<"_end"<<name<<":  ; ";
		tc.source_location_to_stream(os,tok());
		os<<endl;


		tc.framestk().pop_loop(name.data());
	}

	inline void link(toc&tc,ostream&os)const override final{code->link(tc,os);}

	inline void source_to(ostream&os)const override{
		stmt_call::source_to(os);
		code->source_to(os);
	}


private:
	string name;
	up_block code;
};
