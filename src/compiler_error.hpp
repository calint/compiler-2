#pragma once

#include"token.hpp"
#include"statement.hpp"
#include"tokenizer.hpp"

class compiler_error final{
public:
	inline compiler_error(const token&tk,const string&message):
		msg{message},
		start_char{tk.char_index()},end_char{tk.char_index_end()}
	{}

	inline compiler_error(const statement&s,const string&message):
		compiler_error{s.tok(),message}
	{}

	inline compiler_error(const tokenizer&t,const string&message):
		msg{message},
		start_char{t.current_char_index_in_source()},
		end_char{t.current_char_index_in_source()}
	{}

	const string msg;
	size_t start_char{0};
	size_t end_char{0};
};
