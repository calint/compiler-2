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
		ws_left_{wb},
		start_char_{n},
		name_{tk},
		end_char_{n2},
		ws_right_{wa}
	{}

	inline token(const string&name):
		token{"",0,name,name.size()," "}
	{}

	inline token():
		token{"",0,"",0,""}
	{}

	// token{othertoken}
	inline token(const token&tk):
		ws_left_{tk.ws_left_},
		start_char_{tk.start_char_},
		name_{tk.name_},
		end_char_{tk.end_char_},
		ws_right_{tk.ws_right_}
	{}

	// return move(token{})
	inline token(const token&&tk):
		ws_left_{move(tk.ws_left_)},
		start_char_{tk.start_char_},
		name_{move(tk.name_)},
		end_char_{tk.end_char_},
		ws_right_{move(tk.ws_right_)}
	{}

//	inline token&operator=(const token&other){
//		ws_left_=other.ws_left_;
//		start_char_=other.start_char_;
//		name_=other.name_;
//		end_char_=other.end_char_;
//		ws_right_=other.ws_right_;
//		return*this;
//	}

	inline void source_to(ostream&os)const{os<<ws_left_<<name_<<ws_right_;}

	inline void compile_to(ostream&os)const{os<<name_;}

	inline bool is_name(const string&s)const{return!strcmp(name_.c_str(),s.c_str());}

	inline const string&name()const{return name_;}

	inline size_t token_start_char()const{return start_char_;}

	inline size_t token_end_char()const{return end_char_;}

	inline string name_copy()const{return name_;}

	inline bool is_blank()const{
		const char a=ws_left_[0];
		const char b=name_[0];
		const char c=ws_right_[0];
		if(a==0 and b==0 and c==0)
			return true;
		return false;
//		return!(ws_before[0]==name_[0]==ws_after[0]==0);
	}

private:
//	inline void to_stdout2()const{printf("[%zu]%s[%zu]\n",token_start_char_,name_.get(),token_end_char_);}
	string ws_left_;
	size_t start_char_;
	string name_;
	size_t end_char_;
	string ws_right_;
};
//using up_token=unique_ptr<token>;
//using vup_tokens=vector<up_token>;
