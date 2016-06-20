#pragma once

#include <stddef.h>

class statement;

class compiler_error final{public:
	const statement&stmt;
	const char*msg{nullptr};
	const char*ident{nullptr};
	size_t start_char{0};
	size_t end_char{0};

	compiler_error(const statement&s,const char*str,const char*id=nullptr)
		:stmt{s},msg{str},ident{id?id:s.token().name()},
		 start_char{s.token().token_start_char()},end_char{s.token().token_end_char()}{}
};
