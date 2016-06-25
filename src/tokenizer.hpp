#pragma once
#include<stddef.h>
#include<algorithm>
#include<cassert>
#include<cstring>
#include<memory>
using namespace std;
#include "token.hpp"

class tokenizer final{public:

	inline tokenizer(const string&str):
		source{str},
		ptr{source.c_str()}
	{}

	inline bool is_eos()const{return !last_char;}

	inline token next_token(){
		assert(!is_eos());
		auto wspre=next_whitespace();
		auto bgn=nchar;
		auto tkn=next_token_str();
		auto end=nchar;
		auto wsaft=next_whitespace();
		return token(wspre,bgn,tkn,end,wsaft);//? move
	}

	inline bool is_next_char(const char ch){
		if(*ptr!=ch)return false;
		next_char();
		return true;
	}

	inline bool is_peek_char(const char ch){return *ptr==ch;}

	inline char peek_char()const{return *ptr;}

	inline void unread(){
		unsafe_seek(-1);
	}

	inline string read_rest_of_line(){
		const char*bgn=ptr;
		while(true){
			if(*ptr=='\n')break;
			if(*ptr=='\0')break;
			ptr++;
		}
		string s{bgn,size_t(ptr-bgn)};
		ptr++;
		nchar+=size_t(ptr-bgn);
		return s;
	}


private:
	string source;
	const char*ptr;
	size_t nchar_bm{0};
	size_t nchar{0};
	char last_char{-1};
	inline char next_char(){
		assert(last_char);
		nchar++;
		last_char=*ptr;
		ptr++;
		return last_char;
	}
	inline string next_whitespace(){
		if(is_eos())return"";
		nchar_bm=nchar;
		while(true){
			const char ch=next_char();
			if(is_char_whitespace(ch))
				continue;
			unsafe_seek(-1);
			break;
		}
		const size_t len=nchar-nchar_bm;
		return string{ptr-len,len};
//		char*str=new char[len+1];
//		if(len)memcpy(str,ptr-len,len);
//		str[len]=0;
//		return unique_ptr<const char[]>(str);
//		return str;
	}
	inline string next_token_str(){
		if(is_eos())return"";
		nchar_bm=nchar;
		while(true){
			const char ch=next_char();
			if(is_char_whitespace(ch)||ch==0||ch=='('||ch==')'||ch=='{'||ch=='}'||ch=='['||ch==']'||ch=='='||ch==','||ch==':'){
				unsafe_seek(-1);
				break;
			}
			continue;
		}
		const size_t len=nchar-nchar_bm;
//		char*str=new char[len+1];
//		if(len)memcpy(str,ptr-len,len);
//		str[len]=0;
//		return unique_ptr<const char[]>(str);
		return string(ptr-len,len);
	}
	inline void unsafe_seek(const off_t nch){
//		if(nch>0){off_t n=nch;while(n--)assert(*ptr++);}
//		else{off_t n=-nch;while(n--)assert(*ptr--);}
		ptr+=nch;
		nchar=size_t(ssize_t(nchar)+nch);
	}
	inline static bool is_char_whitespace(const char ch){
		return ch==' '||ch=='\t'||ch=='\r'||ch=='\n';
	}
};
