#pragma once
#include<stddef.h>
#include<algorithm>
#include<cassert>
#include<cstring>
#include<memory>
using namespace std;
#include "token.hpp"

class tokenizer final{public:
	inline tokenizer(const char*string):ptr(string){}
	inline bool is_eos()const{return !last_char;}
	inline up_token next_token(){
		assert(!is_eos());
		auto wspre=next_whitespace();
		auto bgn=nchar;
		auto tkn=next_token_str();
		auto end=nchar;
		auto wsaft=next_whitespace();
		auto p=make_unique<token>(move(wspre),bgn,move(tkn),end,move(wsaft));
//		printf("%zu:%zu %s\n",a->get_nchar(),a->get_nchar_end(),a->get_name());
		return p;
	}
	inline bool is_next_char(const char ch){
		if(*ptr!=ch)return false;
		next_char();
		return true;
	}
//	inline bool is_next_char_expression_open(){
//		if(*ptr!='(')return false;
//		next_char();
//		return true;
//	}
//	inline bool is_next_char_expression_close(){
//		if(*ptr!=')')return false;
//		next_char();
//		return true;
//	}
//	inline bool is_next_char_data_open(){
//		if(*ptr!='{')return false;
//		next_char();
//		return true;
//	}
//	inline bool is_next_char_data_close(){
//		if(*ptr!='}')return false;
//		next_char();
//		return true;
//	}
//	inline bool is_next_char_args_open(){
//		if(*ptr!='(')return false;
//		next_char();
//		return true;
//	}
//	inline bool is_next_char_args_close(){
//		if(*ptr!=')')return false;
//		next_char();
//		return true;
//	}
//	inline bool is_next_char_block_open(){
//		if(*ptr!='{')return false;
//		next_char();
//		return true;
//	}
//	inline bool is_next_char_block_close(){
//		if(*ptr!='}')return false;
//		next_char();
//		return true;
//	}
	inline void unread(){
		unsafe_seek(-1);
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
	inline ua_char next_whitespace(){
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
	inline ua_char next_token_str(){
		if(is_eos())
			return unique_ptr<const char[]>(new char[1]{0});
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
		char*str=new char[len+1];
		if(len)memcpy(str,ptr-len,len);
		str[len]=0;
		return unique_ptr<const char[]>(str);
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
