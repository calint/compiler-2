#pragma once

#include<iostream>
#include<regex>

class token final{
public:
	inline token(string wb,size_t start_ix,string tk,size_t end_ix,string wa,bool is_str=false):
		ws_left_{wb},
		start_ix_{start_ix},
		name_{tk},
		end_ix_{end_ix},
		ws_right_{wa},
		is_str_{is_str}
	{}

	inline token()=default;
	inline token(const token&)=default;
	inline token(token&&)=default;
	inline token&operator=(const token&)=default;
	inline token&operator=(token&&)=default;

	inline void source_to(ostream&os)const{
		if(not is_str_){
			os<<ws_left_<<name_<<ws_right_;
			return;
		}
		os<<ws_left_<<'"'<<regex_replace(name_,regex("\n"),"\\n")<<'"'<<ws_right_;
	}

	inline void compile_to(ostream&os)const{
		if(not is_str_){
			os<<name_;
			return;
		}
		// ? temporary fix
		os<<regex_replace(name_,regex("\\\\n"),"',10,'");
	}

	inline bool is_name(const string&s)const{return name_==s;}

	inline const string&name()const{return name_;}

	inline size_t char_index()const{return start_ix_;}

	inline size_t char_index_end()const{return end_ix_;}

	inline bool is_empty()const{
		return ws_left_.empty() and name_.empty() and ws_right_.empty();
	}

	// inline bool is_empty()const{return name_.empty();}

	inline size_t total_length_in_chars()const{
		return ws_left_.length()+name_.length()+ws_right_.length();
	}

	inline bool is_string()const{return is_str_;}

private:
	string ws_left_;
	size_t start_ix_{};
	string name_;
	size_t end_ix_{};
	string ws_right_;
	bool is_str_{};
};
