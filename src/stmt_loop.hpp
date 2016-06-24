#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "compiler_error.hpp"
#include "def_block.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class stmt_loop final:public stmt_call{public:

	inline stmt_loop(toc&tc,statement*parent,unique_ptr<class token>tkn,tokenizer&t)
		:stmt_call{tc,parent,move(tkn),t}
	{
		name="_loop_"+to_string(token().token_start_char());
		code=read_next_statement(tc,parent,t);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		indent(os,indent_level,false);
		os<<name<<":\n";

		tc.framestk().push_loop(name.data());

		code->compile(tc,os,indent_level+1);
		indent(os,indent_level,false);os<<"jmp _loop_"<<token().token_start_char()<<endl;
		indent(os,indent_level,false);os<<"_end_loop_"<<token().token_start_char()<<":\n";

		tc.framestk().pop_loop(name.data());
	}

	inline void link(toc&tc,ostream&os)const override final{code->link(tc,os);}

	inline void source_to(ostream&os)const override{
		stmt_call::source_to(os);
		code->source_to(os);
	}


private:
	string name;
	unique_ptr<statement>code;
};
