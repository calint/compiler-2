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

	inline compiler_error(const statement&s,const char*message,unique_ptr<const char[]>identifier)
		:stmt{s},msg{message},ident{move(identifier)},
		 start_char{s.token().token_start_char()},end_char{s.token().token_end_char()}{}
};
