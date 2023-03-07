#pragma once

#include<iostream>
#include<regex>

class token final{
public:
	inline token(string wb,size_t n,string tk,size_t n2,string wa,bool is_string=false):
		ws_left_{wb},
		start_char_{n},
		name_{tk},
		end_char_{n2},
		ws_right_{wa},
		is_str{is_string}
	{}

	inline token(const token&tk):
		ws_left_{tk.ws_left_},
		start_char_{tk.start_char_},
		name_{tk.name_},
		end_char_{tk.end_char_},
		ws_right_{tk.ws_right_},
		is_str{tk.is_str}
	{}

	inline void source_to(ostream&os)const{
		if(!is_str){
			os<<ws_left_<<name_<<ws_right_;
			return;
		}
		os<<ws_left_<<'"'<<regex_replace(name_,regex("\n"),"\\n")<<'"'<<ws_right_;
	}

	inline void compile_to(ostream&os)const{
		if(!is_str){
			os<<name_;
			return;
		}
		os<<regex_replace(name_,regex("\n"),"',10,'");
	}

	inline bool is_name(const string&s)const{return!strcmp(name_.c_str(),s.c_str());}

	inline const string&name()const{return name_;}

	inline size_t char_index()const{return start_char_;}

	inline size_t char_index_end()const{return end_char_;}

	inline bool is_blank()const{
		const char a=ws_left_[0];
		const char b=name_[0];
		const char c=ws_right_[0];
		if(a==0 and b==0 and c==0)
			return true;
		return false;
//		return!(ws_before[0]==name_[0]==ws_after[0]==0);
	}

	inline size_t total_length_in_chars()const{
		return ws_left_.length()+name_.length()+ws_right_.length();
	}

private:
	string ws_left_;
	size_t start_char_{0};
	string name_;
	size_t end_char_{0};
	string ws_right_;
	bool is_str{false};
};
