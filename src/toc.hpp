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
class file;


using namespace std;

class avar{public:
	const char*name{""};
	size_t stkix{0};
	const char*reg{nullptr};
//				unique_ptr<char>resolv;
	const char*resolv;
};

class frame final{public:
	frame(const char*nm,bool isfunc):name{nm},bits{isfunc?1:0}{}
	inline void add_var(const char*nm,const size_t stkix,const char*flags){
		char buf[256];
		const int n=snprintf(buf,sizeof buf,"dword[ebp+%zu]",stkix<<2);
		if(n<0||n==sizeof buf)throw"??";
		const size_t len=size_t(n)+1;
		char*str=(char*)malloc(len);
		memcpy(str,buf,len);
//		avar*a=new avar{nm,stack_index,nullptr,str};
		vars_.put(nm, avar{nm,stkix,nullptr,str});//? str leak
		stkix_++;
	}
	inline size_t stack_index()const{return stkix_;}
	inline const lut<avar>&vars()const{return vars_;}
	inline const lut<const char*>&aliases()const{return aliases_;}
	inline void add_alias(const char*local_name,const char*outside_name){aliases_.put(local_name,outside_name);}
	inline bool is_func()const{return(bits&1)!=0;}
	inline bool is_name(const char*nm)const{return!strcmp(name,nm);}
private:
	const char*name{""};
	int bits{0}; // 1 is func
	size_t stkix_{0};
	lut<avar>vars_;
	lut<const char*>aliases_;
};

class framestack final{public:
	framestack(){
		free_registers.push_back("edi");
		free_registers.push_back("esi");
		free_registers.push_back("edx");
		free_registers.push_back("ecx");
		free_registers.push_back("ebx");
		free_registers.push_back("eax");
	}
	inline void push_func(const char*name){
		frames.push_back(frame{name,true});
	}
	inline void add_alias(const char*ident,const char*parent_frame_ident){
		frames.back().add_alias(ident,parent_frame_ident);
	}
	inline const char*alias_for_identifier(const char*ident)const{
		if(!frames.back().aliases().has(ident))
			return nullptr;
		return frames.back().aliases().get(ident);
	}
	inline void pop_func(const char*name){
		frame&f=frames.back();
		if(f.is_func())
			if(!f.is_name(name))
				throw __LINE__;
		stkix-=frames.back().stack_index();
		frames.pop_back();
	}
	inline void add_var(const char*name,const char*flags){
		frames.back().add_var(name,stkix++,flags);
	}
	inline const char*resolve_identifier(const char*name)const{
		if(!frames.back().aliases().has(name)){
			const char*alias=frames.back().aliases().get(name);
			if(alias){
				// recurse
				return alias;
			}
		}
		return frames.back().vars().get(name).resolv;
	}
	inline const char*resolve_argument(const char*ident)const{
		size_t i=frames.size()-1;// recurse until aliased var found
		const char*s=ident;
		while(true){
			if(!frames[i].aliases().has(s))
				break;
			s=frames[i].aliases().get(s);
			i--;
		}
		if(!frames[i].vars().has(s)){// assume constant
			return s;
		}
		const char*resolved=frames[i].vars().get(s).resolv;
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
	vector<frame>frames;
	size_t stkix{0};
};

class toc final{public:
	inline toc(){
	}
	inline bool has_file(const char*identifier)const{return files_.has(identifier);}
	inline void put_file(const statement&s,const char*identifier,file*f){
		if(has_file(identifier)){
			throw compiler_error(s,"file already defined at ...",s.token().name_copy());
		}
		files_.put(identifier,f);
	}
	inline bool has_func(const char*identifier)const{return funcs_.has(identifier);}
	inline void put_func(const char*identifier,func*ref){
		if(has_func(identifier))throw"function already defined";
		funcs_.put(identifier,ref);
	}
//	inline void print_to(ostream&os){for(auto&e:files)os<<e;}
	inline func*get_func(const char*name)const{
		if(!funcs_.has(name))
			return nullptr;
		return funcs_.get(name);
	}
	inline framestack&framestk(){return framestk_;}
private:
	lut<file*>files_;
	lut<func*>funcs_;
	framestack framestk_;
};
