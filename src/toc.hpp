#pragma once
#include<unordered_set>
using namespace std;
class toc{public:
	inline bool has_def(const char*identifier)const{
		for(auto&e:defs)if(!strcmp(e,identifier))return true;
		return false;
	}
	inline void put_def(const char*identifier){
		if(has_def(identifier))throw"data already defined";
		defs.insert(identifier);
	}
	inline bool has_func(const char*identifier)const{
		for(auto&e:funcs)if(!strcmp(e,identifier))return true;
		return false;
	}
	inline void put_func(const char*identifier){
		if(has_func(identifier))throw"function already defined";
		funcs.insert(identifier);
	}
	void print_to_stdout(){
		for(auto&e:defs)
			puts(e);
	}
private:
	unordered_set<const char*>defs;
	unordered_set<const char*>funcs;
};
