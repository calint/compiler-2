#pragma once

#include <stddef.h>

class statement;

class compiler_error final{public:
	statement&stmt;
	const char*msg{nullptr};
	size_t start_char{0};
	size_t end_char{0};

	compiler_error(statement&s,const char*str):stmt{s},msg{str},start_char{s.token().token_start_char()},end_char{s.token().token_end_char()}{}
};
