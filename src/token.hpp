#pragma once

#include <memory>
using namespace std;
#include<string.h>
using ua_char=unique_ptr<const char[]>;
class token final{public:
	inline token(ua_char wb,size_t n,ua_char tk,size_t n2,ua_char wa):ws_before{move(wb)},token_start_char_{n},name_{move(tk)},token_end_char_{n2},ws_after{move(wa)}{}
	inline token():ws_before{unique_ptr<const char[]>(new char[1]{0})},token_start_char_{0},name_{unique_ptr<const char[]>(new char[1]{0})},token_end_char_{0},ws_after{unique_ptr<const char[]>(new char[1]{0})}{}
	inline void source_to(ostream&os)const{os<<ws_before.get()<<name_.get()<<ws_after.get();}
	inline void compiled_to(ostream&os)const{source_to(os);}
	inline bool is_name(const char*str)const{return !strcmp(name_.get(),str);}
	inline const char*name()const{return name_.get();}
	inline size_t token_start_char()const{return token_start_char_;}
	inline size_t token_end_char()const{return token_end_char_;}

private:
	inline void to_stdout2()const{printf("[%zu]%s[%zu]\n",token_start_char_,name_.get(),token_end_char_);}
	ua_char ws_before;
	size_t token_start_char_;
	ua_char name_;
	size_t token_end_char_;
	ua_char ws_after;
};
using up_token=unique_ptr<token>;
using vup_tokens=vector<up_token>;
