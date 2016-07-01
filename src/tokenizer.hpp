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
		auto wspre=next_whitespace();
		auto bgn=nchar;
		if(is_next_char('"')){
			string s;
			while(true){
				if(is_next_char('"')){
					auto end=nchar;
					auto wsaft=next_whitespace();
					return move(token{wspre,bgn,s,end,wsaft,true});
					break;
				}
				const char ch=next_char();
				if(ch!='\\'){
					s.push_back(ch);
					continue;
				}
				if(is_next_char('\\')){//  \\  //
					s.push_back('\\');
					continue;
				}
				const char esc_ch=next_char();
				if(esc_ch=='n'){
					s.push_back('\n');
					continue;
				}
				throw string("unknown escaped character ["+to_string(esc_ch)+"]");
			}
			auto tkn=next_token_str();
			auto end=nchar;
			auto wsaft=next_whitespace();
			return move(token{wspre,bgn,tkn,end,wsaft,false});
		}
		auto tkn=next_token_str();
		auto end=nchar;
		auto wsaft=next_whitespace();
		return move(token{wspre,bgn,tkn,end,wsaft});
	}

	inline token next_whitespace_token(){
		auto wspre=next_whitespace();
		auto bgn=nchar;
		auto end=nchar;
		return move(token{wspre,bgn,"",end,""});
	}

	inline bool is_next_char(const char ch){
		if(*ptr!=ch)return false;
		next_char();
		return true;
	}

	inline bool is_peek_char(const char ch){return *ptr==ch;}

	inline char peek_char()const{return *ptr;}

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


	inline char next_char(){
		assert(last_char);
		nchar++;
		last_char=*ptr;
		ptr++;
		return last_char;
	}

private:
	string source;
	const char*ptr;
	size_t nchar_bm{0};
	size_t nchar{0};
	char last_char{-1};

	inline string next_whitespace(){
		if(is_eos())return"";
		nchar_bm=nchar;
		while(true){
			const char ch=next_char();
			if(is_char_whitespace(ch))
				continue;
			seek(-1);
			break;
		}
		const size_t len=nchar-nchar_bm;
		return string{ptr-len,len};
	}

	inline string next_token_str(){
		if(is_eos())return"";
		nchar_bm=nchar;
		while(true){
			const char ch=next_char();
			if(is_char_whitespace(ch)||ch==0||ch=='('||ch==')'||ch=='{'||ch=='}'||
					ch=='='||ch==','||ch==':'||
					ch=='+'||ch=='-'||ch=='*'||ch==';'){
				seek(-1);
				break;
			}
			continue;
		}
		const size_t len=nchar-nchar_bm;
		return string{ptr-len,len};
	}
	inline void seek(const off_t nch){
		assert(ssize_t(source.size())>=(ssize_t(nchar)+nch) and (ssize_t(nchar)+nch)>=0);
		ptr+=nch;
		nchar=size_t(ssize_t(nchar)+nch);
	}
	inline static bool is_char_whitespace(const char ch){
		return ch==' '||ch=='\t'||ch=='\r'||ch=='\n';
	}
};
