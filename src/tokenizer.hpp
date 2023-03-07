#pragma once

class tokenizer final{
public:
	inline tokenizer(const string&str):
		src_{str},
		ptr_{src_.c_str()}
	{}

	inline bool is_eos()const{return !last_char_;}

	inline token next_token(){
		auto wspre=next_whitespace();
		auto bgn=nchar_;
		if(is_next_char('"')){
			string s;
			while(true){
				if(is_next_char('"')){
					auto end=nchar_;
					auto wsaft=next_whitespace();
					return token{wspre,bgn,s,end,wsaft,true};
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
			auto end=nchar_;
			auto wsaft=next_whitespace();
			return token{wspre,bgn,tkn,end,wsaft,false};
		}
		auto tkn=next_token_str();
		auto end=nchar_;
		auto wsaft=next_whitespace();
		return token{wspre,bgn,tkn,end,wsaft};
	}

	inline void pushback_token(const token t){
		// ? validate token is same as source
		seek(-off_t(t.total_length_in_chars()));
	}

	inline token next_whitespace_token(){
		auto wspre=next_whitespace();
		auto bgn=nchar_;
		auto end=nchar_;
		return token{wspre,bgn,"",end,""};
	}

	inline bool is_next_char(const char ch){
		if(*ptr_!=ch)return false;
		next_char();
		return true;
	}

	inline bool is_peek_char(const char ch)const{return *ptr_==ch;}

	inline char peek_char()const{return *ptr_;}

	inline string read_rest_of_line(){
		const char*bgn=ptr_;
		while(true){
			if(*ptr_=='\n')break;
			if(*ptr_=='\0')break;
			ptr_++;
		}
		string s{bgn,size_t(ptr_-bgn)};
		ptr_++;
		nchar_+=size_t(ptr_-bgn);
		return s;
	}

	inline char next_char(){
		assert(last_char_);
		nchar_++;
		last_char_=*ptr_;
		ptr_++;
		return last_char_;
	}

	inline size_t current_char_index_in_source()const{return nchar_;}

private:
	inline string next_whitespace(){
		if(is_eos())return"";
		const size_t nchar_bm_=nchar_;
		while(true){
			const char ch=next_char();
			if(is_char_whitespace(ch))
				continue;
			seek(-1);
			break;
		}
		const size_t len=nchar_-nchar_bm_;
		return string{ptr_-len,len};
	}

	inline string next_token_str(){
		if(is_eos())return"";
		const size_t nchar_bm_=nchar_;
		while(true){
			const char ch=next_char();
			if(is_char_whitespace(ch)||ch==0||ch=='('||ch==')'||ch=='{'||ch=='}'||
					ch=='='||ch==','||ch==':'||
					ch=='+'||ch=='-'||ch=='*'||ch=='/'||ch==';'||
					ch=='<'||ch=='>'||ch=='.'
				){
				seek(-1);
				break;
			}
			continue;
		}
		const size_t len=nchar_-nchar_bm_;
		return string{ptr_-len,len};
	}

	inline void seek(const off_t nch){
		assert(ssize_t(src_.size())>=(ssize_t(nchar_)+nch) and (ssize_t(nchar_)+nch)>=0);
		ptr_+=nch;
		nchar_=size_t(ssize_t(nchar_)+nch);
	}

	inline static bool is_char_whitespace(const char ch){
		return ch==' '||ch=='\t'||ch=='\r'||ch=='\n';
	}

	string src_;
	const char*ptr_;
	size_t nchar_{0};
	char last_char_{-1};
};
