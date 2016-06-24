#pragma once

#include <stddef.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "compiler_error.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "stmt_def.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class def_func;

using namespace std;
class def_class final:public stmt_def{public:

	inline def_class(toc&tc,statement*parent,unique_ptr<class token>tkn,tokenizer&t)
		:stmt_def{tc,parent,move(tkn),t}
	{
		parent->ev(3,this);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		framestack&fs=tc.framestk();

		const char*nm=token().name();

		fs.current_frame().add_class(*this,nm,this);

		fs.push_class(nm);

		code->compile(tc,os,indent_level);

		fs.pop_class(nm);
	}

	inline bool is_inline()const{return true;}

	inline const def_func*get_func_or_break(const statement&s,const char*nm)const{
//		for(auto e:funcs_){
//			if(e->is_identifier(nm))
//					return e;
//		}
		throw compiler_error(s,"cannot find function in class",copy_to_unique_pointer(nm));
	}
};
