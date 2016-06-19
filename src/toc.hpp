#pragma once
#include<unordered_set>
using namespace std;
class toc{
	unordered_set<const char*>defs;
public:
	inline bool has_def(const char*identifier)const{
		for(auto&e:defs){
			if(!strcmp(e,identifier))return true;
		}
		return false;
	}
	inline void put_def(const char*identifier){
		if(has_def(identifier))throw"identifier already defined";
		defs.insert(identifier);
	}
	void print_to_stdout(){
		for(auto&e:defs)
			puts(e);
	}
};
