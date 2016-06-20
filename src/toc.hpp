#pragma once

#include <cstring>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

using namespace std;
class toc{public:
	inline bool has_file(const char*identifier)const{
		for(auto&e:files)if(!strcmp(e,identifier))return true;
		return false;
	}
	inline void put_def(const char*identifier){
		if(has_file(identifier))throw"data already defined";
		files.insert(identifier);
	}
	inline bool has_func(const char*identifier)const{
		for(auto&e:funcs)if(!strcmp(e,identifier))return true;
		return false;
	}
	inline void put_func(const char*identifier){
		if(has_func(identifier))throw"function already defined";
		funcs.insert(identifier);
	}
	void print_to(ostream&os){for(auto&e:files)os<<e;}
private:
	unordered_set<const char*>files;
	unordered_set<const char*>funcs;
};
