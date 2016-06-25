#pragma once

#include <stddef.h>
#include <algorithm>
#include <memory>

#include "statement.hpp"
#include "token.hpp"

class compiler_error final{public:

	inline compiler_error(const statement&s,const char*message,unique_ptr<const char[]>identifier=unique_ptr<const char[]>(new char[1]{0})):
		msg{message},ident{move(identifier)},
		start_char{s.tok().token_start_char()},end_char{s.tok().token_end_char()}
	{}

	inline compiler_error(const token*tk,const char*message,unique_ptr<const char[]>identifier=unique_ptr<const char[]>(new char[1]{0})):
		msg{message},ident{move(identifier)},
		start_char{tk->token_start_char()},end_char{tk->token_end_char()}
	{}

	const char* msg;
	unique_ptr<const char[]>ident;
	size_t start_char{0};
	size_t end_char{0};

};
