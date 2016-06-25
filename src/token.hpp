#pragma once

#include <stddef.h>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>

using namespace std;
#include<string.h>
class token final{public:

	inline token(string wb,size_t n,string tk,size_t n2,string wa):
		ws_before{wb},
		token_start_char_{n},
		name_{tk},
		token_end_char_{n2},
		ws_after{wa}
	{}

	inline token():
			ws_before{""},
			token_start_char_{0},
			name_{""},
			token_end_char_{0},
			ws_after{""}
	{}

	inline void source_to(ostream&os)const{os<<ws_before<<name_<<ws_after;}

	inline void compile_to(ostream&os)const{os<<name_;}

	inline bool is_name(const string&s)const{return!strcmp(name_.c_str(),s.c_str());}

	inline const string&name()const{return name_;}

	inline size_t token_start_char()const{return token_start_char_;}

	inline size_t token_end_char()const{return token_end_char_;}

	inline string name_copy()const{return name_;}

	inline bool is_blank()const{
		const char a=ws_before[0];
		const char b=name_[0];
		const char c=ws_after[0];
		if(a==0 and b==0 and c==0)
			return true;
		return false;
//		return!(ws_before[0]==name_[0]==ws_after[0]==0);
	}

private:
//	inline void to_stdout2()const{printf("[%zu]%s[%zu]\n",token_start_char_,name_.get(),token_end_char_);}
	string ws_before;
	size_t token_start_char_;
	string name_;
	size_t token_end_char_;
	string ws_after;
};
//using up_token=unique_ptr<token>;
//using vup_tokens=vector<up_token>;
