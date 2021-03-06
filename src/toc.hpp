#pragma once

#include<cstring>
#include<iostream>
#include<unordered_map>
#include<unordered_set>
#include<utility>
#include<vector>
#include<algorithm>
#include<cassert>
using namespace std;

#include"lut.hpp"

class token;
class stmt_def_func;
class stmt_def_field;
class stmt_def_table;

using namespace std;

class allocated_var final{public:

	inline allocated_var(const string&name,size_t stkix,const string&in_register,const string&asm_op_param,char bits):
		name_{name},
		stkix_{stkix},
		in_register_{in_register},
		nasm_ident_{asm_op_param},
		bits_{bits}
	{}

	inline bool is_name(const string&varname)const{return varname==name_;}

	inline bool is_const()const{return bits_&1;}

	inline const string&nasm_ident()const{return nasm_ident_;}

private:
	string name_;
	size_t stkix_{0};
	string in_register_;
	string nasm_ident_;
	char bits_{0}; // 1: const
};




class frame final{public:

	frame(const string&name,char bits):
		name_{name},
		bits_{bits}
	{}

	inline void add_var(const string&nm,const size_t stkix,const string&flags){
		string str="dword[ebp+"+to_string(stkix<<2)+"]";
		vars_.put(nm,allocated_var{nm,stkix,"",str,0});
		allocated_stack_++;
	}

	inline size_t allocated_stack_size()const{return allocated_stack_;}

	inline bool has_var(const string&name)const{return vars_.has(name);}

	inline const allocated_var get_var(const string&name)const{return vars_.get(name);}

	inline void add_alias(const string&ident,const string&outside_ident){
		aliases_.put(ident,outside_ident);
	}

	inline bool is_func()const{return bits_&1;}

//	inline bool is_block()const{return bits_&2;}

	inline bool is_loop()const{return bits_&4;}

	inline bool is_if()const{return bits_&8;}

	inline bool is_name(const string&nm)const{return name_==nm;}

	inline bool has_alias(const string&name)const{return aliases_.has(name);}

	inline const string get_alias(const string&name)const{return aliases_.get(name);}

	inline const string&name()const{return name_;}

private:
	string name_;
	size_t allocated_stack_{0};
	char bits_{0}; // 1 is func
	lut<allocated_var>vars_;
	lut<string>aliases_;

};



class toc final{public:

	inline toc(const string&source):
		all_registers_{"eax","ebx","ecx","edx","esi","edi"},
		free_registers_{all_registers_},
		source_str_(source)
	{}

	inline void add_field(const statement&s,const string&ident,const stmt_def_field*f){
		if(fields_.has(ident)){
			throw compiler_error(s.tok(),"field already defined at <line:col>",ident);
		}
		fields_.put(ident,f);
	}

	inline void add_func(const statement&s,const string&ident,const stmt_def_func*ref){
		if(funcs_.has(ident))
			throw compiler_error(s.tok(),"function already defined at <line:col>",ident);

		funcs_.put(ident,ref);
	}

	inline const stmt_def_func*get_func_or_break(const statement&s,const string&name)const{
		bool valid;
		const stmt_def_func*f=funcs_.get_valid(name,valid);
		if(!valid){
			throw compiler_error(s.tok(),"function not found",name);
		}
		return f;
	}

	inline void add_table(const statement&s,const string&ident,const stmt_def_table*f){
		if(tables_.has(ident))
			throw compiler_error(s.tok(),"table already defined at <line:col>",ident);

		tables_.put(ident,f);
	}

	inline void source_location_to_stream(ostream&os,const token&t){
		size_t char_in_line;
		const size_t n=line_number_for_char_index(t.char_index(),source_str_,char_in_line);
		os<<"["<<to_string(n)<<":"<<char_in_line<<"]";
	}

	inline void source_location_to_stream(ostream&os,size_t charix){
		size_t char_in_line;
		const size_t n=line_number_for_char_index(charix,source_str_,char_in_line);
		os<<"["<<to_string(n)<<":"<<char_in_line<<"]";
	}

	inline void source_location_for_identifier_to_stream(ostream&os,const token&t){
		size_t char_in_line;
		const size_t n=line_number_for_char_index(t.char_index(),source_str_,char_in_line);
		os<<"_"<<to_string(n)<<"_"<<char_in_line<<"_";
	}

	inline static size_t line_number_for_char_index(const size_t char_index,const string&str,size_t&char_in_line){
		size_t ix{0};
		size_t lix{0};
		size_t lineno{1};
		const char*p=str.c_str();
		while(true){
			if(ix==char_index)break;
			if(*p++=='\n'){
				lineno++;
				lix=ix;
			}
			ix++;
		}
		char_in_line=ix-lix;
		return lineno;
	}

	inline void finish(const toc&tc,ostream&os){
		assert(all_registers_.size()==free_registers_.size());
		assert(stkix_==0);
//		assert(framestk_.import_frames_.size()==0);
		os<<"\n;           max regs in use: "<<tc.max_usage_scratch_regs_<<endl;
		os<<";         max frames in use: "<<tc.max_frame_count_<<endl;
		os<<";          max stack in use: "<<tc.max_stack_usage_<<endl;
	}

	inline string resolve_ident_to_nasm(const statement&stmt)const{//? tidy duplicate code
		return resolve_ident_to_nasm(stmt,stmt.identifier());
	}
	inline string resolve_ident_to_nasm(const statement&stmt,const string&ident)const{//? tidy duplicate code
		const size_t frameix=frames_.size()-1;

		bool ok{false};
		string nasm_ident=_resolve_ident_to_nasm(stmt,ident,frameix,ok);
		if(ok)return nasm_ident;

		throw compiler_error(stmt.tok(),"cannot resolve identifier '"+ident+"'",nasm_ident);
	}

	inline void push_func(const string&name){frames_.push_back(frame{name,1});check_usage();}

	inline void push_block(const string&name){frames_.push_back(frame{name,2});check_usage();}

	inline void push_loop(const string&name){frames_.push_back(frame{name,4});check_usage();}

	inline void add_alias(const string&ident,const string&parent_frame_ident){
		frames_.back().add_alias(ident,parent_frame_ident);
	}

	inline void pop_func(const string&name){
		frame&f=frames_.back();
		assert(f.is_func() and f.is_name(name));
		stkix_-=frames_.back().allocated_stack_size();
		frames_.pop_back();
	}

	inline void pop_loop(const string&name){
		frame&f=frames_.back();
		assert(f.is_loop() and f.is_name(name));
		stkix_-=frames_.back().allocated_stack_size();
		frames_.pop_back();
	}

	inline void push_if(const char*name){
		frames_.push_back(frame{name,8});
		check_usage();
	}

	inline void pop_if(const string&name){
		frame&f=frames_.back();
		assert(f.is_if() and f.is_name(name));
		stkix_-=frames_.back().allocated_stack_size();
		frames_.pop_back();
	}

	inline void add_var(const string&name,const string&flags=""){
		frames_.back().add_var(name,stkix_++,flags);
	}
	inline const string alloc_scratch_register(const statement&st,const string&reg=""){
		return alloc_scratch_register(st.tok(),reg);
	}

	inline const string alloc_scratch_register(const token&source,const string&reg=""){
		if(free_registers_.empty()){
			throw compiler_error(source,"out of scratch registers  reduce expression complexity");
		}
		if(reg!=""){
			auto r=find(free_registers_.begin(),free_registers_.end(),reg);
			if(r==free_registers_.end()){
				throw compiler_error(source,"register "+reg+" cannot be allocated");//?
			}
			free_registers_.erase(r);
			return reg;
		}

		const string&r=free_registers_[free_registers_.size()-1];
		free_registers_.pop_back();

		const size_t n=all_registers_.size()-free_registers_.size();
		if(n>max_usage_scratch_regs_)
			max_usage_scratch_regs_=n;

		return r;
	}

	inline void free_scratch_reg(const string&reg){
		free_registers_.push_back(reg);
	}

	inline const string&find_parent_loop_name()const{
		size_t i=frames_.size()-1;
		while(true){
			if(frames_[i].is_loop())
				return frames_[i].name();
			if(frames_[i].is_func())
				throw"cannot find loop start";
			i--;
		}
	}
//
//	inline void set_nasm_dest(const string&dst){
//		dst_=dst;
//	}
//	string dst_;
//
//	inline const string&get_evaluation_destination_nasm_identifier()const{
//		return dst_;
//	}
private:
	inline const string _resolve_ident_to_nasm(const statement&stmt,const string&ident,size_t i,bool&ok)const{//? tidy duplicate code
		string name=ident;
		while(true){
			if(frames_[i].is_func()){
				if(!frames_[i].has_alias(name))
					break;
				name=frames_[i].get_alias(name);
				i--;
				continue;
			}
			if(frames_[i].has_var(name))
				break;
			i--;
		}
		if(frames_[i].has_var(name)){
			ok=true;
//			is_const_litteral=false;
			return frames_[i].get_var(name).nasm_ident();
		}

		for(auto e:all_registers_)if(e==name){
			ok=true;
//			is_const_litteral=false;
			return name;
		}

		if(fields_.has(name)){
			ok=true;
//			is_const_litteral=false;
			return name;
		}

		// ie  prompt.len //?
		const char*p=name.c_str();
		while(true){
			if(!*p)break;
			if(*p=='.')break;
			p++;
		}
		string s=string(name.c_str(),size_t(p-name.c_str()));
		if(fields_.has(s)){//? tidy
			p++;
			const size_t ln=name.size()-size_t(p-name.c_str());
			string after_dot=string(p,ln);//? utf8
			if(after_dot=="len"){
//				is_const_litteral=false;
				ok=true;
				return name;
			}
			throw compiler_error(stmt.tok(),"unknown implicit field constant",string(name));
		}

		char*ep;
		strtol(name.c_str(),&ep,10);
		if(!*ep){// decimal number
			ok=true;
//			is_const_litteral=true;
			return name;
		}

		if(!name.find("0x")){
			strtol(name.c_str()+2,&ep,16);
			if(!*ep){// hex number{
				ok=true;
//				is_const_litteral=true;
				return name;
			}
		}

		ok=false;
		return name;
//		if(!framestk_.exported_frame_ix_)
//			throw "cannot resolve "+string(name);
	}

	inline void check_usage(){
		if(frames_.size()>max_frame_count_)
			max_frame_count_=frames_.size();

		if(stkix_>max_stack_usage_)
			max_stack_usage_=stkix_;
	}

	size_t stkix_{0};
	vector<frame>frames_;
	vector<string>all_registers_;
	vector<string>free_registers_;
	size_t max_usage_scratch_regs_{0};
	size_t max_frame_count_{0};
	size_t max_stack_usage_{0};
	string source_str_;
	lut<const stmt_def_field*>fields_;
	lut<const stmt_def_func*>funcs_;
	lut<const stmt_def_table*>tables_;

};
