#pragma once
#include<memory>
using namespace std;
#include<string.h>
#include<cassert>
#include"token.hpp"
class string_tokenizer{
public:
	inline string_tokenizer(const char*string):ptr(string){}
	inline bool is_eos()const{return !last_char;}
	inline unique_ptr<token>next_token(){
		assert(!is_eos());
		return make_unique<token>(next_whitespace(),nchar_bm,next_token_str(),nchar,next_whitespace());
	}
	inline bool is_next_char_expression_open(){
		if(*ptr=='('){
			next_char();
			return true;
		}
		return false;
	}
	inline bool is_next_char_expression_close(){
		if(*ptr==')'){
			next_char();
			return true;
		}
		return false;
	}
private:
	const char*ptr{nullptr};
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
	inline bool is_char_whitespace(const char ch){
		return ch==' '||ch=='\t'||ch=='\r'||ch=='\n';
	}
	inline unique_ptr<const char[]>next_whitespace(){
		if(is_eos())
			return unique_ptr<const char[]>(new char[1]{0});
		nchar_bm=nchar;
		while(true){
			const char ch=next_char();
			if(is_char_whitespace(ch))
				continue;
			unsafe_seek(-1);
			break;
		}
		const size_t len=nchar-nchar_bm;
		char*str=new char[len+1];
		if(len)memcpy(str,ptr-len,len);
		str[len]=0;
		return unique_ptr<const char[]>(str);
	}
	inline unique_ptr<const char[]>next_token_str(){
		if(is_eos())
			return unique_ptr<const char[]>(new char[1]{0});
		nchar_bm=nchar;
		while(true){
			const char ch=next_char();
			if(is_char_whitespace(ch)||ch==0||ch=='('||ch==')'){
				unsafe_seek(-1);
				break;
			}
			continue;
		}
		const size_t len=nchar-nchar_bm;
		char*str=new char[len+1];
		if(len)memcpy(str,ptr-len,len);
		str[len]=0;
		return unique_ptr<const char[]>(str);
	}
	inline void unsafe_seek(const int nbr_of_chars){
		ptr+=nbr_of_chars;
		nchar+=nbr_of_chars;
	}
};
