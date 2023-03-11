#pragma once

#include<cassert>
#include<sstream>
#include<regex>

#include"lut.hpp"
#include"compiler_error.hpp"

class token;
class stmt_def_func;
class stmt_def_field;
class stmt_def_table;

class allocated_var final{
public:
	inline allocated_var(const string&name,int stkix,const string&in_register,const string&asm_op_param,char bits):
		name_{name},
		stkix_{stkix},
		in_register_{in_register},
		nasm_ident_{asm_op_param},
		bits_{bits}
	{}

	inline bool is_name(const string&nm)const{return nm==name_;}

	inline bool is_const()const{return bits_&1;}

	inline const string&nasm_ident()const{return nasm_ident_;}

private:
	string name_;
	int stkix_{0};
	string in_register_;
	string nasm_ident_;
	char bits_{0}; // 1: const
};

class frame final{
public:
	inline frame(const string&name,char bits,const string&call_path,const string&ret,const bool is_inline):
		name_{name},
		call_path_{call_path},
		ret_{ret},
		is_inline_{is_inline},
		bits_{bits}
	{}

	inline void add_var(const string&nm,const int stkix,const string&flags){
		string str;
		if(stkix>0){
			// function argument
			str="qword[rbp+"+to_string(stkix<<3)+"]";
		}else if(stkix<0){
			// variable
			str="qword[rbp"+to_string(stkix<<3)+"]";
			allocated_stack_++;
		}else{
			throw "toc:fram:add_var";
		}
		vars_.put(nm,allocated_var{nm,stkix,"",str,0});
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

	inline const string&ret_label()const{return ret_;}

	inline const string&call_path()const{return call_path_;}

	inline bool is_func_inline()const{return is_inline_;}

private:
	string name_;
	string call_path_;
	size_t allocated_stack_{0};
	lut<allocated_var>vars_;
	lut<string>aliases_;
	string ret_;
	bool is_inline_{false};
	char bits_{0}; // 1 is func
};

struct field_meta{
	const stmt_def_field*def{nullptr};
	bool is_str{false};
};

class toc final{
public:
	inline toc(const string&source):
		all_registers_{"rax","rbx","rcx","rdx","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15"},
		free_registers_{all_registers_},
		source_str_{source}
	{}

	inline void add_field(const statement&s,const string&ident,const stmt_def_field*f,const bool is_str_field){
		if(fields_.has(ident)){
			throw compiler_error(s.tok(),"field '"+ident+"' already defined");
		}
		fields_.put(ident,{f,is_str_field});
	}

	inline void add_func(const statement&s,const string&ident,const stmt_def_func*ref){
		if(funcs_.has(ident))
			throw compiler_error(s.tok(),"function '"+ident+"' already defined");

		funcs_.put(ident,ref);
	}

	inline const stmt_def_func&get_func_or_break(const statement&s,const string&name)const{
		bool valid;
		const stmt_def_func*f=funcs_.get_valid(name,valid);
		if(!valid){
			throw compiler_error(s.tok(),"function '"+name+"' not found");
		}
		return*f;
	}

	inline void add_table(const statement&s,const string&ident,const stmt_def_table*f){
		if(tables_.has(ident))
			throw compiler_error(s.tok(),"table '"+ident +"' already defined");

		tables_.put(ident,f);
	}

	inline string source_location(const token&t)const{
		size_t char_in_line;
		const size_t n=line_number_for_char_index(t.char_index(),source_str_.c_str(),char_in_line);
		return to_string(n)+"_"+to_string(char_in_line);
	}

	inline static size_t line_number_for_char_index(const size_t char_index,const char*ptr,size_t&char_in_line){
		size_t ix{0};
		size_t lix{0};
		size_t lineno{1};
		while(true){
			if(ix==char_index)
				break;
			if(*ptr++=='\n'){
				lineno++;
				ix++;
				lix=ix;
				continue;
			}
			ix++;
		}
		char_in_line=ix-lix+1;
		return lineno;
	}

	inline void finish(const toc&tc,ostream&os){
		assert(all_registers_.size()==free_registers_.size());
//		assert(stkix_==0);
//		assert(framestk_.import_frames_.size()==0);
		os<<"\n;      max registers in use: "<<tc.max_usage_scratch_regs_<<endl;
		os<<";         max frames in use: "<<tc.max_frame_count_<<endl;
//		os<<";          max stack in use: "<<tc.max_stack_usage_<<endl;
	}

	inline string resolve_ident_to_nasm(const statement&stmt)const{//? tidy duplicate code
		return resolve_ident_to_nasm(stmt,stmt.identifier());
	}

	inline string resolve_ident_to_nasm(const statement&stmt,const string&ident)const{//? tidy duplicate code
		const size_t frameix=frames_.size()-1;

		bool ok{false};
		string nasm_ident=_resolve_ident_to_nasm(stmt,ident,frameix,ok);
		if(ok)
			return nasm_ident;

		throw compiler_error(stmt.tok(),"cannot resolve identifier '"+ident+"'");
	}

	inline void push_func(const string&name,const string&call_loc,const string&ret_jmp,const bool is_inline){
		frames_.emplace_back(name,1,call_loc,ret_jmp,is_inline);
		check_usage();
	}

	inline void push_block(const string&name){
		frames_.emplace_back(name,2,"","",false);
		check_usage();
	}

	inline void push_loop(const string&name){
		frames_.emplace_back(name,4,"","",false);
		check_usage();
	}

	inline void add_alias(const string&ident,const string&parent_frame_ident){
		frames_.back().add_alias(ident,parent_frame_ident);
	}

	inline void pop_func(const string&name){
		frame&f=frames_.back();
		assert(f.is_func() and f.is_name(name));
//		stkix_-=frames_.back().allocated_stack_size();
		frames_.pop_back();
	}

	inline void pop_loop(const string&name){
		frame&f=frames_.back();
		assert(f.is_loop() and f.is_name(name));
//		stkix_-=frames_.back().allocated_stack_size();
		frames_.pop_back();
	}

	inline size_t get_current_stack_size()const{
		size_t delta{0};
		size_t frm_nbr=frames_.size()-1;
		while(true){
			const frame&frm=frames_[frm_nbr];
			delta+=frm.allocated_stack_size();
			if(frm.is_func()&&!frm.is_func_inline())
				break;
			if(frm_nbr==0)
				break;
			frm_nbr--;
		}
		return delta;
	}

	inline void push_if(const char*name){
		frames_.emplace_back(name,8,"","",false);
		check_usage();
	}

	inline void pop_if(const string&name){
		frame&f=frames_.back();
		assert(f.is_if() and f.is_name(name));
//		stkix_-=frames_.back().allocated_stack_size();
		frames_.pop_back();
	}

	inline void add_var(const string&name,const string&flags=""){
		const size_t stkix=get_current_stack_size()+1;
		// offset by one since rsp points to most recently pushed value
		//   allocate next free slot
		frames_.back().add_var(name,-int(stkix),flags);
	}

	inline void add_func_arg(const string&name,const int stkix_delta,const string&flags=""){
		frames_.back().add_var(name,stkix_delta,flags);
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
				throw compiler_error(source,"register '"+reg+"' cannot be allocated");//?
			}
			free_registers_.erase(r);
			return reg;
		}

		const string&r=free_registers_.back();
		free_registers_.pop_back();

		const size_t n=all_registers_.size()-free_registers_.size();
		if(n>max_usage_scratch_regs_)
			max_usage_scratch_regs_=n;

		return r;
	}

	inline void free_scratch_reg(const string&reg){
		free_registers_.push_back(reg);
	}

	inline const string&get_loop_label_or_break(const statement&st)const{
		size_t i=frames_.size()-1;
		while(true){
			if(frames_[i].is_loop())
				return frames_[i].name();
			if(frames_[i].is_func())
				throw compiler_error(st,"not in a loop");
			i--;
		}
	}

	inline const string&get_call_path(const token&tk)const{
		size_t i=frames_.size()-1;
		while(true){
			if(frames_[i].is_func())
				return frames_[i].call_path();
			if(i==0) // ? can happen?
				break;
			i--;
		}
		throw compiler_error(tk,"not in a function");
	}

	inline const string&get_func_return_label_or_break(const statement&st)const{
		size_t i=frames_.size()-1;
		while(true){
			if(frames_[i].is_func())
				return frames_[i].ret_label();
			if(i==0) // ? can happen?
				break; 
			i--;
		}
		throw compiler_error(st,"not in a function");
	}

	inline void source_comment(ostream&os,const statement&stmt)const{
		size_t char_in_line;
		const size_t n=line_number_for_char_index(stmt.tok().char_index(),source_str_.c_str(),char_in_line);
		os<<"["<<to_string(n)<<":"<<char_in_line<<"]";

		stringstream ss;
		ss<<" ";
		stmt.source_to(ss);
		string s=ss.str();
		string res=regex_replace(s,regex("\\s+")," ");
		os<<res;
		os<<endl;
	}

	inline void source_comment(ostream&os,const string&dest,const char op,const statement&stmt)const{
		size_t char_in_line;
		const size_t n=line_number_for_char_index(stmt.tok().char_index(),source_str_.c_str(),char_in_line);
		os<<"["<<to_string(n)<<":"<<char_in_line<<"]";

		stringstream ss;
		ss<<" "<<dest<<op;
		stmt.source_to(ss);
		string s=ss.str();
		string res=regex_replace(s,regex("\\s+")," ");
		os<<res;
		os<<endl;
	}

	inline void token_comment(ostream&os,const token&tk)const{
		size_t char_in_line;
		const size_t n=line_number_for_char_index(tk.char_index(),source_str_.c_str(),char_in_line);
		os<<"["<<to_string(n)<<":"<<char_in_line<<"]";
		os<<" "<<tk.name()<<endl;
	}

	inline bool is_identifier_register(const string&id)const{
		return find(all_registers_.begin(),all_registers_.end(),id)!=all_registers_.end();
	}

	// returns true if stack pointer needs to be positioned for func call
	inline bool enter_func_call(){
		if(func_call_nested_==0){
			func_call_nested_++;
			return true;
		}
		func_call_nested_++;
		return false;
	}
	// returns true if stack pointer needs to be positioned at base
	inline bool exit_func_call(){
		func_call_nested_--;
		if(func_call_nested_==0)
			return true;
		return false;
	}

private:
	inline const string _resolve_ident_to_nasm(const statement&stmt,const string&ident,size_t i,bool&ok)const{//? tidy duplicate code
		string name=ident;
		while(true){
			if(frames_[i].is_func()){
				if(!frames_[i].has_alias(name))
					break;
				name=frames_[i].get_alias(name);
				if(i==0)
					break;
				i--;
				continue;
			}
			if(frames_[i].has_var(name))
				break;
			if(i==0)
				break;
			i--;
		}

		if(frames_[i].has_var(name)){
			ok=true;
			return frames_[i].get_var(name).nasm_ident();
		}

		for(const auto&e:all_registers_){
			if(e==name){
				ok=true;
				return name;
			}
		}

		const field_meta&fm=fields_.get_valid(name,ok);
		if(ok){
			if(fm.is_str)
				return name;
			return"qword["+name+"]";
		}

		// i.e.  prompt.len // ?
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
			throw compiler_error(stmt.tok(),"unknown implicit field constant '"+name+"'");
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
	}

	inline void check_usage(){
		if(frames_.size()>max_frame_count_)
			max_frame_count_=frames_.size();

//		if(stkix_>max_stack_usage_)
//			max_stack_usage_=stkix_;
	}

//	int stkix_{0};
	vector<frame>frames_;
	vector<string>all_registers_;
	vector<string>free_registers_;
	size_t max_usage_scratch_regs_{0};
	size_t max_frame_count_{0};
	int max_stack_usage_{0};
	string source_str_;
	lut<field_meta>fields_;
	lut<const stmt_def_func*>funcs_;
	lut<const stmt_def_table*>tables_;
	size_t func_call_nested_{0};
};
