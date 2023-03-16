#pragma once

#include<iostream>
#include<regex>

class token final{
public:
	inline token():
		ws_left_{},
		start_ix_{},
		name_{},
		end_ix_{},
		ws_right_{},
		is_str_{}
	{}

	inline token(string wb,size_t start_ix,string tk,size_t end_ix,string wa,bool is_str=false):
		ws_left_{wb},
		start_ix_{start_ix},
		name_{tk},
		end_ix_{end_ix},
		ws_right_{wa},
		is_str_{is_str}
	{
		// cout<<"constructor\n";
	}

	inline token(const token&tk):
		ws_left_{tk.ws_left_},
		start_ix_{tk.start_ix_},
		name_{tk.name_},
		end_ix_{tk.end_ix_},
		ws_right_{tk.ws_right_},
		is_str_{tk.is_str_}
	{
		// cout<<"copy constructor\n";
	}

	token&operator=(const token&other){
		// cout<<"operator =\n";
		ws_left_=other.ws_left_;
		start_ix_=other.start_ix_;
		name_=other.name_;
		end_ix_=other.end_ix_;
		ws_right_=other.ws_right_;
		is_str_=other.is_str_;
		return*this;
	}

	inline token(token&&tk):
		ws_left_{move(tk.ws_left_)},
		start_ix_{tk.start_ix_},
		name_{move(tk.name_)},
		end_ix_{tk.end_ix_},
		ws_right_{move(tk.ws_right_)},
		is_str_{tk.is_str_}
	{
		// cout<<"move constructor\n";
	}

	inline void source_to(ostream&os)const{
		if(!is_str_){
			os<<ws_left_<<name_<<ws_right_;
			return;
		}
		os<<ws_left_<<'"'<<regex_replace(name_,regex("\n"),"\\n")<<'"'<<ws_right_;
	}

	inline void compile_to(ostream&os)const{
		if(!is_str_){
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

	inline bool is_blank()const{
		if(ws_left_.empty() and name_.empty() and ws_right_.empty())
			return true;
		return false;
	}

	inline size_t total_length_in_chars()const{
		return ws_left_.length()+name_.length()+ws_right_.length();
	}

	inline bool is_string()const{return is_str_;}

private:
	string ws_left_;
	size_t start_ix_{0};
	string name_;
	size_t end_ix_{0};
	string ws_right_;
	bool is_str_{false};
};
