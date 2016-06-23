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

class allocated_var{public:
	inline allocated_var(const char*name,size_t stkix,const char*in_register,const char*asm_op_param,char bits):
			name_{name},stkix_{stkix},in_register_{in_register},asm_op_param_{asm_op_param},bits_{bits}{}
	inline bool is_const()const{return bits_&1;}
//	inline const char*asm_op_param()const{return asm_op_param_.get();}
	inline const char*asm_op_param()const{return asm_op_param_;}
private:
	const char*name_{""};
	size_t stkix_{0};
	const char*in_register_{nullptr};
	const char*asm_op_param_{nullptr};
//	unique_ptr<const char>asm_op_param_;
	char bits_{0}; // 1: const     2: isloop
};

class frame final{public:
	frame(const char*name,char bits):name_{name},bits_{bits}{}
	inline void add_var(const char*nm,const size_t stkix,const char*flags){
		char buf[256];
		const int n=snprintf(buf,sizeof buf,"dword[ebp+%zu]",stkix<<2);
		if(n<0||n==sizeof buf)throw"??";
		const size_t len=size_t(n)+1;

		char*str=new char[len];
		to_be_deleted.push_back(unique_ptr<const char>(str));

		memcpy(str,buf,len);
		vars_.put(nm, allocated_var{nm,stkix,nullptr,str,0});
		stkix_++;
	}
	inline size_t stack_index()const{return stkix_;}
//	inline const lut<allocated_var>&vars()const{return vars_;}
	inline bool has_var(const char*name)const{return vars_.has(name);}
	inline const allocated_var vars_get(const char*name)const{return vars_.get(name);}
//	inline const lut<const char*>&aliases()const{return aliases_;}
	inline void add_alias(const char*ident,const char*outside_ident){aliases_.put(ident,outside_ident);}
	inline bool is_func()const{return bits_&1;}
	inline bool is_block()const{return bits_&2;}
	inline bool is_loop()const{return bits_&4;}
	inline bool is_name(const char*nm)const{return!strcmp(name_,nm);}
	inline bool aliases_has(const char*name)const{return aliases_.has(name);}
	inline const char*aliases_get(const char*name)const{return aliases_.get(name);}
private:
	vector<unique_ptr<const char>>to_be_deleted;
	const char*name_{""};
	size_t stkix_{0};
	lut<allocated_var>vars_;
	lut<const char*>aliases_;
	char bits_{0}; // 1 is func
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
	inline void push_func(const char*name){frames.push_back(frame{name,1});}
	inline void push_block(const char*name){frames.push_back(frame{name,2});}
	inline void push_loop(const char*name){frames.push_back(frame{name,4});}
	inline void add_alias(const char*ident,const char*parent_frame_ident){frames.back().add_alias(ident,parent_frame_ident);}
	inline const char*alias_for_identifier(const char*ident)const{
		if(!frames.back().aliases_has(ident))
			return nullptr;
		return frames.back().aliases_get(ident);
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
	inline const char*resolve_func_arg(const char*ident)const{
		size_t i=frames.size()-1;// recurse until aliased var found
		const char*name=ident;
		while(true){
			if(!frames[i].aliases_has(name))
				break;
			name=frames[i].aliases_get(name);
			i--;
		}
		if(!frames[i].has_var(name)){// assume constant ie  0xb8000
			return name;
		}
		const char*resolved=frames[i].vars_get(name).asm_op_param();
		return resolved;
	}
	inline const char*alloc_scratch_register(){
		const char*r=free_registers[free_registers.size()-1];
		free_registers.pop_back();
		return r;
	}
	inline void free_scratch_reg(const char*reg){free_registers.push_back(reg);}

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
