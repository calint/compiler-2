#pragma once

#include <cstring>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include"lut.hpp"

class token;
class func;


using namespace std;
class toc final{
	class stackframe final{public:
		const char*name{""};
		lut<const char*>aliases;
		stackframe(const char*nm):name{nm}{}
	};

	class stack final{public:
		inline void push_func(const char*name){
			frames.push_back(stackframe{name});
		}
		inline void alias(const char*local_name,const char*outside_name){
			frames.back().aliases.put(local_name,outside_name);
		}
		inline const char*getalias(const char*local_name)const{
			return frames.back().aliases.get(local_name);
		}
	private:
		vector<stackframe>frames;
	};


public:
	inline bool has_file(const char*identifier)const{for(auto&e:files)if(!strcmp(e,identifier))return true;return false;}
	inline void put_def(const char*identifier){if(has_file(identifier))throw"data already defined";files.insert(identifier);}
	inline bool has_func(const char*identifier)const{for(auto&e:funcs)if(!strcmp(e,identifier))return true;return false;}
	inline void put_func(const char*identifier,func*ref){
		if(has_func(identifier))throw"function already defined";
		funcs.insert(identifier);
		funcs2.put(identifier,ref);
	}
	inline void print_to(ostream&os){for(auto&e:files)os<<e;}
	inline void stack_pushfunc(const char*name){stk.push_func(name);}
	inline const func*get_func(const char*name)const{
		const func*f=funcs2.get(name);
		return f;
//		throw"funcnotfound";
	}
	inline void stack_alias(const char*local_name,const char*outside_name){stk.alias(local_name,outside_name);}
	inline const char*stack_getalias(const char*local_name)const{return stk.getalias(local_name);}

private:
	unordered_set<const char*>files;
	unordered_set<const char*>funcs;
	lut<func*>funcs2;
	stack stk;
};
