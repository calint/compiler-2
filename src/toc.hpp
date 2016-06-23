#pragma once

#include <cstring>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include"lut.hpp"

class token;
class stmt_def_func;
class stmt_def_field;


using namespace std;

class allocated_var{public:

	inline allocated_var(const char*name,size_t stkix,const char*in_register,const char*asm_op_param,char bits):
		name_{name},stkix_{stkix},in_register_{in_register},asm_op_param_{asm_op_param},bits_{bits}{}

	inline bool is_const()const{return bits_&1;}

	inline const char*asm_op_param()const{return asm_op_param_;}

	inline bool is_name(const char*name)const{return!strcmp(name_,name);}

private:
	const char*name_{""};
	size_t stkix_{0};
	const char*in_register_{nullptr};
	const char*asm_op_param_{nullptr};
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
//		char*str=(char*)malloc(len);
		memcpy(str,buf,len);
		to_be_deleted.push_back(unique_ptr<const char[]>(str));
		vars_.put(nm, allocated_var{nm,stkix,nullptr,str,0});
		allocated_stack_++;
	}

	inline size_t allocated_stack_size()const{return allocated_stack_;}

	inline bool has_var(const char*name)const{return vars_.has(name);}

	inline const allocated_var get_var(const char*name)const{return vars_.get(name);}

	inline void add_alias(const char*ident,const char*outside_ident){aliases_.put(ident,outside_ident);}

	inline bool is_func()const{return bits_&1;}

	inline bool is_block()const{return bits_&2;}

	inline bool is_loop()const{return bits_&4;}

	inline bool is_if()const{return bits_&8;}

	inline bool is_name(const char*nm)const{return!strcmp(name_,nm);}

	inline bool has_alias(const char*name)const{return aliases_.has(name);}

	inline const char*get_alias(const char*name)const{return aliases_.get(name);}

	inline const char*name()const{return name_;}

private:
	const char*name_{""};
	size_t allocated_stack_{0};
	char bits_{0}; // 1 is func
	lut<allocated_var>vars_;
	lut<const char*>aliases_;
	vector<unique_ptr<const char[]>>to_be_deleted;
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

	inline void pop_func(const char*name){
		frame&f=frames.back();
		if(f.is_func())
			if(!f.is_name(name))
				throw __LINE__;
		stkix-=frames.back().allocated_stack_size();
		frames.pop_back();
	}

	inline void push_block(const char*name){
		frames.push_back(frame{name,2});
	}

	inline void pop_block(const char*name){
		frame&f=frames.back();
		if(f.is_block())
			if(!f.is_name(name))
				throw __LINE__;
		stkix-=frames.back().allocated_stack_size();
		frames.pop_back();
	}

	inline void push_loop(const char*name){frames.push_back(frame{name,4});}

	inline void pop_loop(const char*name){
		frame&f=frames.back();
		if(f.is_loop())
			if(!f.is_name(name))
				throw __LINE__;
		stkix-=frames.back().allocated_stack_size();
		frames.pop_back();
	}

	inline void push_if(const char*name){
		exported_frame_ix=frames.size()-1;
		frames.push_back(frame{name,8});
	}

	inline void pop_if(const char*name){
		frame&f=frames.back();
		if(not f.is_if() or not f.is_name(name))throw __LINE__;
		stkix-=frames.back().allocated_stack_size();
		frames.pop_back();
		exported_frame_ix=0;
	}

	inline void add_alias(const char*ident,const char*parent_frame_ident){
		frames.back().add_alias(ident,parent_frame_ident);
	}

	inline void add_var(const char*name,const char*flags){
		frames.back().add_var(name,stkix++,flags);
	}

	inline const char*resolve_func_arg(const char*ident)const{
		size_t i=frames.size()-1;// recurse until aliased var found
		const char*name=ident;
		while(true){
			if(frames[i].has_var(name))
				return frames[i].get_var(name).asm_op_param();

			if(frames[i].has_alias(name)){
				name=frames[i].get_alias(name);
				i--;
				continue;
			}

			if(!frames[i].is_func()){
				i--;
				continue;
			}

			break;
		}

		if(!exported_frame_ix)// assume constant ie  0xb8000
			return name;

		// try in if context, cannot be first frame
		i=exported_frame_ix;
		while(true){
			if(frames[i].has_var(name))
				return frames[i].get_var(name).asm_op_param();

			if(frames[i].has_alias(name)){
				name=frames[i].get_alias(name);
				i--;
				continue;
			}

			if(!frames[i].is_func()){
				i--;
				continue;
			}

			break;
//
//			if(!frames[i].has_alias(name))
//				break;
//			name=frames[i].get_alias(name);
//			i--;
		}

		// assume constant ie  0xb8000
		return name;
	}

	inline const char*alloc_scratch_register(){
		const char*r=free_registers[free_registers.size()-1];
		free_registers.pop_back();
		return r;
	}

	inline void free_scratch_reg(const char*reg){free_registers.push_back(reg);}

	inline const char*find_parent_loop_name()const{
		size_t i=frames.size()-1;// recurse until aliased var found
		while(true){
			if(frames[i].is_loop())
				return frames[i].name();
			if(frames[i].is_func())
				throw"cannot find loop start";
			i--;
		}
	}

private:
	size_t stkix{0};
	size_t exported_frame_ix{0};
	vector<frame>frames;
	vector<const char*>free_registers;
};

class toc final{public:

	inline void add_file(const statement&s,const char*identifier,const stmt_def_field*f){
		if(has_file(identifier)){
			throw compiler_error(s,"file already defined at ...",copy_string_to_unique_pointer(identifier));
		}
		files_.put(identifier,f);
	}

	inline void add_func(const statement&s,const char*identifier,const stmt_def_func*ref){
		if(has_func(identifier)){
			throw compiler_error(s,"function already defined at ...",copy_string_to_unique_pointer(identifier));
		}
		funcs_.put(identifier,ref);
	}

	inline const stmt_def_func*get_func_or_break(const statement&stmt,const char*name)const{
		bool valid;
		const stmt_def_func*f=funcs_.get_valid(name,valid);
		if(!valid){
			throw compiler_error(stmt,"function not found",copy_string_to_unique_pointer(name));
		}
		return f;
	}

	inline framestack&framestk(){return framestk_;}



	inline static unique_ptr<const char[]>copy_string_to_unique_pointer(const char*str){
		const size_t len=strlen(str)+1;//? unsafe
		char*cpy=new char[len];
		memcpy(cpy,str,len);
		return unique_ptr<const char[]>(cpy);
	}


private:
	inline bool has_func(const char*name)const{return funcs_.has(name);}
	inline bool has_file(const char*name)const{return files_.has(name);}

	framestack framestk_;
	lut<const stmt_def_field*>files_;
	lut<const stmt_def_func*>funcs_;
};
