#pragma once

#include <cstring>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include<algorithm>
#include"lut.hpp"

class token;
class stmt_def_func;
class stmt_def_field;
class stmt_def_table;

using namespace std;

class allocated_var{public:

	inline allocated_var(const char*name,size_t stkix,const char*in_register,const char*asm_op_param,char bits):
		name_{name},stkix_{stkix},in_register_{in_register},nasm_ident_{asm_op_param},bits_{bits}{}

	inline bool is_const()const{return bits_&1;}

	inline const char*nasm_ident()const{return nasm_ident_;}

	inline bool is_name(const char*name)const{return!strcmp(name_,name);}

private:
	const char*name_{""};
	size_t stkix_{0};
	const char*in_register_{nullptr};
	const char*nasm_ident_{nullptr};
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
		to_be_deleted_.push_back(unique_ptr<const char[]>(str));
		vars_.put(nm, allocated_var{nm,stkix,nullptr,str,0});
		allocated_stack_++;
	}

	inline size_t allocated_stack_size()const{return allocated_stack_;}

	inline bool has_var(const char*name)const{return vars_.has(name);}

	inline const allocated_var get_var(const char*name)const{return vars_.get(name);}

	inline void add_alias(const char*ident,const char*outside_ident){aliases_.put(ident,outside_ident);}

	inline bool is_func()const{return bits_&1;}

//	inline bool is_block()const{return bits_&2;}

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
	vector<unique_ptr<const char[]>>to_be_deleted_;

};




class framestack final{public:

	framestack():
		all_registers_{"eax","ebx","ecx","edx","esi","edi"},
		free_registers_{all_registers_}
	{}

	inline void push_func(const char*name){frames_.push_back(frame{name,1});}

	inline void push_block(const char*name){frames_.push_back(frame{name,2});}

	inline void push_loop(const char*name){frames_.push_back(frame{name,4});}


	inline void add_alias(const char*ident,const char*parent_frame_ident){
//		cerr<<"   ++++++++++   "<<ident<<" -> "<<parent_frame_ident<<endl;
		frames_.back().add_alias(ident,parent_frame_ident);
	}

	inline void pop_func(const char*name){
		frame&f=frames_.back();
		if(f.is_func())
			if(!f.is_name(name))
				throw __LINE__;
		stkix_-=frames_.back().allocated_stack_size();
		frames_.pop_back();
	}

	inline void pop_loop(const char*name){
		frame&f=frames_.back();
		if(f.is_loop())
			if(!f.is_name(name))
				throw __LINE__;
		stkix_-=frames_.back().allocated_stack_size();
		frames_.pop_back();
	}

	inline void push_if(const char*name){
		exported_frame_ix_=frames_.size()-1;
//		export_varspace_at_current_frame_in_subcalls(true);
		frames_.push_back(frame{name,8});
	}

//	inline void export_varspace_at_current_frame_in_subcalls(bool b){
//		if(b){
//			exported_frame_ix=frames.size()-1;
//			return;
//		}
//		exported_frame_ix=0;
//	}

	inline void pop_if(const char*name){
		frame&f=frames_.back();
		if(not f.is_if() or not f.is_name(name))throw __LINE__;
		stkix_-=frames_.back().allocated_stack_size();
		frames_.pop_back();
		exported_frame_ix_=0;
//		export_varspace_at_current_frame_in_subcalls(false);
	}

	inline void add_var(const char*name,const char*flags){
		frames_.back().add_var(name,stkix_++,flags);
	}

//	inline const char*resolve_ident_to_nasm(const char*ident)const{
//		size_t i=frames_.size()-1;// recurse until aliased var found
//		const char*name=ident;
//		while(true){
//			if(!frames_[i].has_alias(name))break;
//			name=frames_[i].get_alias(name);
//			i--;
//		}
//
//		if(frames_[i].has_var(name))
//			return frames_[i].get_var(name).nasm_ident();
//
//		if(!exported_frame_ix_){// no additional frame to look from
//
//			for(auto e:all_registers_)
//				if(!strcmp(e,name))
//					return name;
//
//
//			return name;
//		}
//
//		// try to find from a different frame   i e   var a=1 var d=plus(plus(a b),(c d);
//		i=exported_frame_ix_;
//		while(true){
//			if(!frames_[i].has_alias(name))break;
//			name=frames_[i].get_alias(name);
//			i--;
//		}
//
//		if(frames_[i].has_var(name))
//			return frames_[i].get_var(name).nasm_ident();
//
//		for(auto e:all_registers_)
//			if(!strcmp(e,name))
//				return name;
//
//		//? throw if ident does not match a field or table or const or register
//		//? ie prompt.len -> len implicit for field, table
//
//		return name;
//	}

	inline const char*alloc_scratch_register(){
		const char*r=free_registers_[free_registers_.size()-1];
		free_registers_.pop_back();
		return r;
	}

	inline void free_scratch_reg(const char*reg){
		free_registers_.push_back(reg);
	}

	inline const char*find_parent_loop_name()const{
		size_t i=frames_.size()-1;// recurse until aliased var found
		while(true){
			if(frames_[i].is_loop())
				return frames_[i].name();
			if(frames_[i].is_func())
				throw"cannot find loop start";
			i--;
		}
	}

	vector<frame>frames_;
	size_t exported_frame_ix_{0};
	vector<const char*>all_registers_;
private:
	size_t stkix_{0};
	vector<const char*>free_registers_;
};




class toc final{public:

	inline toc(const char*source):source_str_(source){}

	inline void add_field(const statement&s,const char*identifier,const stmt_def_field*f){
		if(fields_.has(identifier)){
			throw compiler_error(s,"field already defined at <line:col>",copy_string_to_unique_pointer(identifier));
		}
		fields_.put(identifier,f);
	}

	inline void add_func(const statement&s,const char*identifier,const stmt_def_func*ref){
		if(funcs_.has(identifier))
			throw compiler_error(s,"function already defined at <line:col>",copy_string_to_unique_pointer(identifier));

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

	inline void add_table(const statement&s,const char*identifier,const stmt_def_table*f){
		if(tables_.has(identifier))
			throw compiler_error(s,"table already defined at <line:col>",copy_string_to_unique_pointer(identifier));

		tables_.put(identifier,f);
	}

	inline framestack&framestk(){return framestk_;}

	inline void source_location_to_stream(ostream&os,const token&t){
		size_t char_in_line;
		const size_t n=line_number_for_char_index(t.token_start_char(),source_str_,char_in_line);
		os<<"["<<to_string(n)<<":"<<char_in_line<<"]";
	}

	inline void source_location_for_identifier_to_stream(ostream&os,const token&t){
		size_t char_in_line;
		const size_t n=line_number_for_char_index(t.token_start_char(),source_str_,char_in_line);
		os<<"_"<<to_string(n)<<"_"<<char_in_line<<"_";
	}

	inline static unique_ptr<const char[]>copy_string_to_unique_pointer(const char*str){
		const size_t len=strlen(str)+1;//? unsafe
		char*cpy=new char[len];
		memcpy(cpy,str,len);
		return unique_ptr<const char[]>(cpy);
	}

	inline static size_t line_number_for_char_index(const size_t char_index,const char*str,size_t&char_in_line){
		size_t ix{0};
		size_t lix{0};
		size_t lineno{1};
		while(true){
			if(ix==char_index)break;
			if(*str++=='\n'){
				lineno++;
				lix=ix;
			}
			ix++;
		}
		char_in_line=ix-lix;
		return lineno;
	}
	inline const char*resolve_ident_to_nasm(const statement&stmt,const char*ident)const{//? tidy duplicate code
		const char*name=_resolve_ident_to_nasm(stmt,ident,framestk_.frames_.size()-1);
		if(name)return name;
		if(framestk_.exported_frame_ix_){
			name=_resolve_ident_to_nasm(stmt,name,framestk_.exported_frame_ix_);
			if(name)return name;
		}
		throw compiler_error(stmt,"cannot resolve identifier",copy_string_to_unique_pointer(ident));
	}


private:
	const char*source_str_{""};
	framestack framestk_;
	lut<const stmt_def_field*>fields_;
	lut<const stmt_def_func*>funcs_;
	lut<const stmt_def_table*>tables_;

	inline const char*_resolve_ident_to_nasm(const statement&stmt,const char*ident,size_t i)const{//? tidy duplicate code
//		size_t i=framestk_.frames_.size()-1;// recurse until aliased var found
		const char*name=ident;
		while(true){
			if(!framestk_.frames_[i].has_alias(name))break;
			name=framestk_.frames_[i].get_alias(name);
			i--;
		}

		if(framestk_.frames_[i].has_var(name))
			return framestk_.frames_[i].get_var(name).nasm_ident();

		for(auto e:framestk_.all_registers_)
			if(!strcmp(e,name))
				return name;

		if(fields_.has(name))
			return name;

		// ie  prompt.len
		const char*p=name;
		while(true){
			if(!*p)break;
			if(*p=='.')break;
			p++;
		}
		string s=string(name,size_t(p-name));
		if(fields_.has(s.c_str())){
			p++;
			const size_t ln=strlen(name)-size_t(p-name);
			string after_dot=string(p,ln);//? utf8
			if(!strcmp(after_dot.c_str(),"len")){
				return name;
			}
			throw compiler_error(stmt,"unknown implicit field constant",copy_string_to_unique_pointer(name));
		}

		char*ep;
		strtol(name,&ep,10);
		if(!*ep)// decimal number
			return name;

		strtol(name,&ep,16);
		if(!*ep)// hex number
			return name;

		return nullptr;
//		if(!framestk_.exported_frame_ix_)
//			throw "cannot resolve "+string(name);
	}
};
