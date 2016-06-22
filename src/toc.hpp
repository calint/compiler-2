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

class avar{public:
	const char*name{""};
	size_t stkix{0};
	const char*reg{nullptr};
//				unique_ptr<char>resolv;
	const char*resolv;
};

class frame final{public:
	const char*name{""};
	size_t stack_index{0};
	lut<const char*>aliases;
	lut<avar>vars;
	frame(const char*nm):name{nm}{}
	inline void add_var(const char*nm,const char*flags){
		char buf[256];
		const int n=snprintf(buf,sizeof buf,"ebp+%zu",stack_index<<2);
		if(n<0||n==sizeof buf)throw"??";
		const size_t len=size_t(n)+1;
		char*str=(char*)malloc(len);
		memcpy(str,buf,len);
//		avar*a=new avar{nm,stack_index,nullptr,str};
		vars.put(name, avar{nm,stack_index,nullptr,str});//? str leak
		stack_index++;
	}
};

class framestack final{public:
	inline void push_func(const char*name){frames.push_back(frame{name});}
	inline void alias(const char*local_name,const char*outside_name){
		frames.back().aliases.put(local_name,outside_name);
	}
	inline const char*getalias(const char*local_name)const{
		if(!frames.back().aliases.has(local_name))
			return nullptr;
		return frames.back().aliases.get(local_name);
	}
	inline void pop(){
		frames.pop_back();
	}
	inline void var(const char*name,const char*flags){
		frames.back().add_var(name,flags);
	}
	inline const char*resolve_identifier(const char*name)const{
		if(!frames.back().aliases.has(name)){
			const char*alias=frames.back().aliases.get(name);
			if(alias){
				// recurse
				return alias;
			}
		}
		return frames.back().vars.get(name).resolv;
	}

private:
	vector<frame>frames;
};

class toc final{public:
	inline bool has_file(const char*identifier)const{for(auto&e:files)if(!strcmp(e,identifier))return true;return false;}
	inline void put_def(const char*identifier){if(has_file(identifier))throw"data already defined";files.insert(identifier);}
	inline bool has_func(const char*identifier)const{for(auto&e:funcs)if(!strcmp(e,identifier))return true;return false;}
	inline void put_func(const char*identifier,func*ref){
		if(has_func(identifier))throw"function already defined";
		funcs.insert(identifier);
		funcs2.put(identifier,ref);
	}
	inline void print_to(ostream&os){for(auto&e:files)os<<e;}
	inline func*get_func(const char*name)const{
		if(!funcs2.has(name))
			return nullptr;
		return funcs2.get(name);
	}
	inline void stack_pushfunc(const char*name){framestk.push_func(name);}
	inline void stack_alias(const char*local_name,const char*outside_name){framestk.alias(local_name,outside_name);}
	inline const char*stack_getalias(const char*local_name)const{return framestk.getalias(local_name);}
	inline void stack_var(const char*local_name){framestk.var(local_name,"");}
	inline void stack_pop(){framestk.pop();}
	inline const char*resolve_argument(const char*defval)const{
		const char*alias=stack_getalias(defval);
		if(alias)return alias;
		return defval;
	}

private:
	unordered_set<const char*>files;
	unordered_set<const char*>funcs;
	lut<func*>funcs2;
	framestack framestk;
};
