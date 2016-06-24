#pragma once

#include <stddef.h>
#include <algorithm>
#include <iostream>
#include <memory>

#include "compiler_error.hpp"
#include "stmt_def_func.hpp"

using namespace std;
#include "stmt_call.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include"stmt_block.hpp"
class stmt_def_class final:public stmt_block{public:

	inline stmt_def_class(statement*parent,unique_ptr<class token>tkn,tokenizer&t)
		:stmt_block{parent,move(tkn),t}{}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		framestack&fs=tc.framestk();

		const char*nm=token().name();

		fs.current_frame().add_class(*this,nm,this);

		fs.push_class(nm);

		stmt_block::compile(tc,os,indent_level);

		fs.pop_class(nm);
	}

	inline bool is_inline()const{return true;}

	inline const stmt_def_func*get_func_or_break(const statement&s,const char*nm)const{
	}

};
