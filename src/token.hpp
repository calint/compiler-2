#pragma once
#include<memory>
using namespace std;
#include<string.h>
#include<cassert>
using ucharp=unique_ptr<const char[]>;
class token{
	ucharp ws_before;
	size_t token_start_nchar;
	ucharp name;
	size_t token_end_nchar;
	ucharp ws_after;
public:
	inline token(ucharp wb,size_t n,ucharp tk,size_t n2,ucharp wa)
		:ws_before(move(wb)),token_start_nchar(n),name(move(tk)),token_end_nchar(n2),ws_after(move(wa)){}
	inline void source_to_stdout()const{printf("%s%s%s",ws_before.get(),name.get(),ws_after.get());}
	inline void to_stdout2()const{printf("[%zu]%s[%zu]\n",token_start_nchar,name.get(),token_end_nchar);}
	inline void compiled_to_stdout()const{source_to_stdout();}
	inline bool is_name(const char*str)const{return !strcmp(name.get(),str);}
	inline const char*get_name()const{return name.get();}
};
