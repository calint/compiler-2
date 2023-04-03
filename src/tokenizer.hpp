#pragma once

class tokenizer final{
public:
	inline tokenizer(const string&str):
		src_{str},
		ptr_{src_.c_str()}
	{}

	inline tokenizer()=default;
	inline tokenizer(const tokenizer&)=default;
	inline tokenizer(tokenizer&&)=default;
	inline tokenizer&operator=(const tokenizer&)=default;
	inline tokenizer&operator=(tokenizer&&)=default;

	inline bool is_eos()const{return!last_char_;}

	inline token next_token(){
		const string&wspre{next_whitespace()};
		const size_t bgn{nchar_};
		if(is_next_char('"')){
			string s;
			while(true){
				if(is_next_char('"')){
					const size_t end{nchar_};
					const string&wsaft{next_whitespace()};
					return token{wspre,bgn,s,end,wsaft,true};
				}
				s.push_back(next_char());
			}
		}
		const string&tkn{next_token_str()};
		const size_t end{nchar_};
		const string&wsaft{next_whitespace()};
		return{wspre,bgn,tkn,end,wsaft};
	}

	inline void put_back_token(const token&t){
		// ? validate token is same as source
		seek(-off_t(t.total_length_in_chars()));
	}

	inline void put_back_char(const char ch){
		// ? validate char is same as source
		seek(-off_t(1));
	}

	inline token next_whitespace_token(){
		return{next_whitespace(),nchar_,"",nchar_,""};
	}

	inline bool is_next_char(const char ch){
		if(*ptr_!=ch)
			return false;
		next_char(); // return ignored
		return true;
	}

	inline bool is_peek_char(const char ch)const{return *ptr_==ch;}

	inline bool is_peek_char2(const char ch)const{
		return *ptr_==0?false:*(ptr_+1)==ch;
	}

	inline char peek_char()const{return *ptr_;}

	inline string read_rest_of_line(){
		const char*bgn{ptr_};
		while(true){
			if(*ptr_=='\n')break;
			if(*ptr_=='\0')break;
			ptr_++;
		}
		const string&s{bgn,size_t(ptr_-bgn)};
		ptr_++;
		nchar_+=size_t(ptr_-bgn);
		return s;
	}

	inline char next_char(){
		assert(last_char_);
		last_char_=*ptr_;
		ptr_++;
		nchar_++;
		return last_char_;
	}

	inline size_t current_char_index_in_source()const{return nchar_;}

private:
	inline string next_whitespace(){
		if(is_eos())
			return"";
		const size_t nchar_bm_{nchar_};
		while(true){
			const char ch{next_char()};
			if(is_char_whitespace(ch))
				continue;
			seek(-1);
			break;
		}
		const size_t len{nchar_-nchar_bm_};
		return{ptr_-len,len};
	}

	inline string next_token_str(){
		if(is_eos())
			return"";
		const size_t nchar_bm_{nchar_};
		while(true){
			const char ch{next_char()};
			if(is_char_whitespace(ch)||
					ch==0||ch=='('||ch==')'||ch=='{'||ch=='}'||
					ch=='='||ch==','||ch==':'||ch==';'||
					ch=='+'||ch=='-'||ch=='*'||ch=='/'||ch=='%'||
					ch=='&'||ch=='|'||ch=='^'||
					ch=='<'||ch=='>'||ch=='!'
				){
				seek(-1);
				break;
			}
			continue;
		}
		const size_t len{nchar_-nchar_bm_};
		return{ptr_-len,len};
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
	size_t nchar_{};
	char last_char_{-1};
};
