#pragma once
#include"decouple.hpp"
#include<string.h>
using ua_char=unique_ptr<const char[]>;
class token{
	ua_char ws_before;
	size_t token_start_char_;
	ua_char name_;
	size_t token_end_char_;
	ua_char ws_after;
public:
	inline token(ua_char wb,size_t n,ua_char tk,size_t n2,ua_char wa)
		:ws_before(move(wb)),token_start_char_(n),name_(move(tk)),token_end_char_(n2),ws_after(move(wa)){}
	inline void source_to_stdout()const{printf("%s%s%s",ws_before.get(),name_.get(),ws_after.get());}
	inline void to_stdout2()const{printf("[%zu]%s[%zu]\n",token_start_char_,name_.get(),token_end_char_);}
	inline void compiled_to_stdout()const{source_to_stdout();}
	inline bool is_name(const char*str)const{return !strcmp(name_.get(),str);}
	inline const char*name()const{return name_.get();}
	inline size_t token_start_char()const{return token_start_char_;}
	inline size_t token_end_char()const{return token_end_char_;}
};
using up_token=unique_ptr<token>;
using vup_tokens=vector<up_token>;
