#pragma once

#include <stddef.h>
#include <algorithm>
#include <memory>

#include "statement.hpp"
#include "token.hpp"

class compiler_error final{public:
	const statement&stmt;
	const char*msg{nullptr};
	unique_ptr<const char[]>ident;
	size_t start_char{0};
	size_t end_char{0};

	inline compiler_error(const statement&s,const char*str,unique_ptr<const char[]>token_name=unique_ptr<const char[]>(new char[1]{0}))
		:stmt{s},msg{str},ident{move(token_name)},
		 start_char{s.token().token_start_char()},end_char{s.token().token_end_char()}{}
};
