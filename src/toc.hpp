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
	lut<const char*>aliases;
	lut<avar>vars;
	frame(const char*nm):name{nm}{}
	inline void add_var(const char*nm,const size_t stkix,const char*flags){
		char buf[256];
		const int n=snprintf(buf,sizeof buf,"dword[ebp+%zu]",stkix<<2);
		if(n<0||n==sizeof buf)throw"??";
		const size_t len=size_t(n)+1;
		char*str=(char*)malloc(len);
		memcpy(str,buf,len);
//		avar*a=new avar{nm,stack_index,nullptr,str};
		vars.put(nm, avar{nm,stkix,nullptr,str});//? str leak
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
	inline void add_var(const char*name,const char*flags){
		frames.back().add_var(name,stkix++,flags);
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

	vector<frame>frames;
private:
	size_t stkix{0};
};

class toc final{public:
	inline toc(){
		free_registers.push_back("edi");
		free_registers.push_back("esi");
		free_registers.push_back("edx");
		free_registers.push_back("ecx");
		free_registers.push_back("ebx");
		free_registers.push_back("eax");
	}
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
	inline void stack_push_func(const char*name){framestk.push_func(name);}
	inline void stack_add_alias(const char*local_name,const char*outside_name){framestk.alias(local_name,outside_name);}
	inline const char*stack_get_alias(const char*local_name)const{return framestk.getalias(local_name);}
	inline void stack_add_var(const char*local_name){
//		cerr<<"add var "<<local_name<<endl;
		framestk.add_var(local_name,"");
	}
	inline void stack_pop(){framestk.pop();}
	inline const char*resolve_argument(const char*defval)const{
		size_t i=framestk.frames.size()-1;// recurse until aliased var found
		const char*als=defval;
		while(true){
			if(!framestk.frames[i].aliases.has(als))
				break;
			als=framestk.frames[i].aliases.get(als);
			i--;
		}
		if(!framestk.frames[i].vars.has(als)){// assume constant
			return als;
		}
		const char*resolved=framestk.frames[i].vars.get(als).resolv;
		return resolved;
	}
	inline const char*alloc_scratch_register(){
		const char*r=free_registers[free_registers.size()-1];
		free_registers.pop_back();
		return r;
	}
	inline void free_scratch_reg(const char*reg){
		free_registers.push_back(reg);
	}

private:
	vector<const char*>free_registers;
	unordered_set<const char*>files;
	unordered_set<const char*>funcs;
	lut<func*>funcs2;
	framestack framestk;
};
