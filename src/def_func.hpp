#pragma once

#include <algorithm>
#include <memory>

#include "statement.hpp"
#include "stmt_def.hpp"
#include "token.hpp"
#include "tokenizer.hpp"


class def_func final:public stmt_def{public:

	inline def_func(toc&tc,statement*parent,unique_ptr<class token>tkn,tokenizer&t)
		:stmt_def{tc,parent,move(tkn),t}
	{
		parent->ev(2,this);
	}

	inline bool is_inline()const{return true;}

};
