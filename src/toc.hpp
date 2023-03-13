#pragma once

#include<cassert>
#include<sstream>
#include<regex>
#include<unordered_set>

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
	enum class type{FUNC,BLOCK,LOOP,IF};

	inline frame(const string&name,const type tpe,const string&call_path="",const string&ret_label="",const bool is_inline=false,const string&ret_var=""):
		name_{name},
		call_path_{call_path},
		ret_label_{ret_label},
		ret_var_{ret_var},
		is_inline_{is_inline},
		type_{tpe}
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

	inline bool is_func()const{return type_==type::FUNC;}

	inline bool is_block()const{return type_==type::BLOCK;}

	inline bool is_loop()const{return type_==type::LOOP;}

	inline bool is_if()const{return type_==type::IF;}

	inline bool is_name(const string&nm)const{return name_==nm;}

	inline bool has_alias(const string&name)const{return aliases_.has(name);}

	inline const string get_alias(const string&name)const{return aliases_.get(name);}

	inline const string&name()const{return name_;}

	inline const string&ret_label()const{return ret_label_;}

	inline const string&call_path()const{return call_path_;}

	inline bool is_func_inline()const{return is_inline_;}

	inline const string&ret_var()const{return ret_var_;}

private:
	string name_; // optional name
	string call_path_; // a unique path of source locations of the inlined call
	size_t allocated_stack_{0}; // number of slots used on the stack by this frame
	lut<allocated_var>vars_; // vars declared in this frame
	lut<string>aliases_; // aliases that refers to previous frame alias or var
	string ret_label_; // the label to jump to when exiting an inlined function
	string ret_var_; // the variable that contains the return value
	bool is_inline_{false};
	type type_{type::FUNC}; // frame type
};

struct field_meta{
	const stmt_def_field*def{nullptr};
	bool is_str{false};
};

class toc final{
public:
	inline toc(const string&source):
		all_registers_{"rax","rbx","rcx","rdx","rsi","rdi","rbp","rsp","r8","r9","r10","r11","r12","r13","r14","r15"},
		scratch_registers_{"r8","r9","r10","r11","r12","r13","r14","r15"},
		named_registers_{"rax","rbx","rcx","rdx","rsi","rdi"},
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
		if(!funcs_.has(name))
			throw compiler_error(s.tok(),"function '"+name+"' not found");
		const stmt_def_func*f=funcs_.get(name);
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
		assert(scratch_registers_.size()==8);
		assert(named_registers_.size()==6);
//		assert(stkix_==0);
//		assert(framestk_.import_frames_.size()==0);
		os<<"\n; max scratch registers in use: "<<tc.max_usage_scratch_regs_<<endl;
		os<<";            max frames in use: "<<tc.max_frame_count_<<endl;
//		os<<";          max stack in use: "<<tc.max_stack_usage_<<endl;
	}

	inline string resolve_ident_to_nasm(const statement&stmt)const{//? tidy duplicate code
		return resolve_ident_to_nasm(stmt,stmt.identifier());
	}

	inline string resolve_ident_to_nasm(const statement&stmt,const string&ident)const{//? tidy duplicate code
		string nasm_ident=resolve_ident_to_nasm_or_empty(stmt,ident);
		if(!nasm_ident.empty())
			return nasm_ident;

		throw compiler_error(stmt.tok(),"cannot resolve identifier '"+ident+"'");
	}

	inline void add_alias(const string&ident,const string&parent_frame_ident){
		frames_.back().add_alias(ident,parent_frame_ident);
	}

	inline void push_func(const string&name,const string&call_loc,const string&ret_jmp,const bool is_inline,const string&ret_var){
		frames_.emplace_back(name,frame::type::FUNC,call_loc,ret_jmp,is_inline,ret_var);
		check_usage();
	}

	inline void push_block(){
		frames_.emplace_back("",frame::type::BLOCK);
		check_usage();
	}

	inline void push_loop(const string&name){
		frames_.emplace_back(name,frame::type::LOOP);
		check_usage();
	}

//		inline void push_if(const char*name){
//			frames_.emplace_back(name,frame::type::IF);
//			check_usage();
//		}
//
//		inline void pop_if(const string&name){
//			frame&f=frames_.back();
//			assert(f.is_if() and f.is_name(name));
//	//		stkix_-=frames_.back().allocated_stack_size();
//			frames_.pop_back();
//		}

	inline void pop_func(const string&name){
		frame&f=frames_.back();
		assert(f.is_func() and f.is_name(name));
		frames_.pop_back();
	}

	inline void pop_loop(const string&name){
		frame&f=frames_.back();
		assert(f.is_loop() and f.is_name(name));
		frames_.pop_back();
	}

	inline void pop_block(){
		frame&f=frames_.back();
		assert(f.is_block());
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

	inline void add_var(const statement&st,const string&name,const string&flags=""){
		const size_t stkix=get_current_stack_size()+1;
		// offset by one since rsp points to most recently pushed value
		//   allocate next free slot
		if(frames_.back().has_var(name)){
			throw compiler_error(st,"variable '"+name+"' already declared");
		}
		frames_.back().add_var(name,-int(stkix),flags);
	}

	inline void add_func_arg(const string&name,const int stkix_delta,const string&flags=""){
		frames_.back().add_var(name,stkix_delta,flags);
	}

	inline const string&alloc_scratch_register(const statement&st,ostream&os,const size_t indent_level){
		if(scratch_registers_.empty()){
			throw compiler_error(st,"out of scratch registers. try to reduce expression complexity");
		}

		const string&r=scratch_registers_.back();
		scratch_registers_.pop_back();

		statement::indent(os,indent_level,true);os<<"alloc "<<r<<endl;

		const size_t n=8-scratch_registers_.size();
		if(n>max_usage_scratch_regs_) // ? stmt_assign_var tries 2 different methods making this metric inaccurate if a discarded method uses more registers than the selected method
			max_usage_scratch_regs_=n;

		allocated_registers_.push_back(r);
		return r;
	}

	inline void alloc_named_register_or_break(const statement&st,const string&reg,ostream&os,const size_t indent_level){
		auto r=find(named_registers_.begin(),named_registers_.end(),reg);
		if(r==named_registers_.end()){
			throw compiler_error(st,"named register '"+reg+"' cannot be allocated");
		}
		named_registers_.erase(r);
		allocated_registers_.push_back(reg);
		statement::indent(os,indent_level,true);os<<"alloc "<<reg<<endl;
	}

	inline void free_named_register(const string&reg,ostream&os,const size_t indent_level){
		statement::indent(os,indent_level,true);os<<"free "<<reg<<endl;
		assert(allocated_registers_.back()==reg);
		allocated_registers_.pop_back();
		named_registers_.push_back(reg);
		initiated_registers_.erase(reg);
	}

	inline void free_scratch_register(const string&reg,ostream&os,const size_t indent_level){
		statement::indent(os,indent_level,true);os<<"free "<<reg<<endl;
		assert(allocated_registers_.back()==reg);
		allocated_registers_.pop_back();
		scratch_registers_.push_back(reg);
		initiated_registers_.erase(reg);
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

	inline const string&get_func_return_var_name_or_break(const statement&st)const{
		size_t i=frames_.size()-1;
		while(true){
			if(frames_[i].is_func())
				return frames_[i].ret_var();
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

	inline bool enter_func_call(){
		const bool b=call_allocated_regs_idx_.empty();
		call_allocated_regs_idx_.push_back(allocated_registers_.size());
		return b;
	}

	inline bool exit_func_call(){
		call_allocated_regs_idx_.pop_back();
		return call_allocated_regs_idx_.empty();
	}

	inline size_t get_func_call_alloc_regs_idx()const{
		if(call_allocated_regs_idx_.size())
			return call_allocated_regs_idx_.back();
		return 0;
	}

	inline const vector<string>&get_allocated_registers()const{return allocated_registers_;}

	inline void asm_cmd(const statement&st,ostream&os,const size_t indent_level,const string&op,const string&dest_resolved,const string&src_resolved){
		if(op=="mov"){
			if(dest_resolved==src_resolved)
				return;

			if(is_identifier_register(dest_resolved)){
				initiated_registers_.insert(dest_resolved);
			}
		}
		// check if both source and destination are memory operations
		if(dest_resolved.find_first_of('[')!=string::npos and src_resolved.find_first_of('[')!=string::npos){
			const string&r=alloc_scratch_register(st,os,indent_level);
			statement::indent(os,indent_level);os<<"mov "<<r<<","<<src_resolved<<endl;
			statement::indent(os,indent_level);os<<op<<" "<<dest_resolved<<","<<r<<endl;
			free_scratch_register(r,os,indent_level);
			return;
		}
		statement::indent(os,indent_level);os<<op<<" "<<dest_resolved<<","<<src_resolved<<endl;
	}

	inline void asm_push(const statement&st,ostream&os,const size_t indent_level,const string&reg){
		statement::indent(os,indent_level);os<<"push "<<reg<<endl;
	}

	inline void asm_pop(const statement&st,ostream&os,const size_t indent_level,const string&reg){
		statement::indent(os,indent_level);os<<"pop "<<reg<<endl;
	}

	inline bool is_register_initiated(const string&reg)const{
		return initiated_registers_.contains(reg);
	}

private:
	inline const string resolve_ident_to_nasm_or_empty(const statement&stmt,const string&ident)const{
		string id=ident;
		// traverse the frames and resolve the id (which might be an alias) to
		// a variable, field, register or constant
		size_t frame_idx=frames_.size()-1;
		while(true){
			// is the frame a function?
			if(frames_[frame_idx].is_func()){
				// is it an alias defined by an argument in the function?
				if(!frames_[frame_idx].has_alias(id))
					break;
				// yes, continue resolving alias until it is
				// a variable, field, register or constant
				id=frames_[frame_idx].get_alias(id);
				if(frame_idx==0)
					break;
				frame_idx--;
				continue;
			}
			// does scope contain the variable
			if(frames_[frame_idx].has_var(id))
				break;
			// was this the last frame in the stack?
			if(frame_idx==0)
				break;
			// go 'up' in stack to next scope
			frame_idx--;
		}

		// is 'id' a variable?
		if(frames_[frame_idx].has_var(id))
			return frames_[frame_idx].get_var(id).nasm_ident();

		// is 'id' a register?
		for(const auto&e:all_registers_){
			if(e==id)
				return id;
		}

		// is 'id' a field?
		if(fields_.has(id)){
			const field_meta&fm=fields_.get(id);
			if(fm.is_str)
				return id;
			return"qword["+id+"]";
		}

		// is 'id' an implicit identifier?
		// i.e. 'prompt.len' of a string field 'prompt'
		string subid=id.substr(0,id.find('.'));
		if(fields_.has(subid)){ // ? tidy
			string after_dot=id.substr(subid.size()+1);
			if(after_dot=="len"){
				return id;
			}
			throw compiler_error(stmt.tok(),"unknown implicit field constant '"+id+"'");
		}

		// is it decimal number?
		size_t characters_read{0};
		stoi(id,&characters_read,10); // return ignored
		if(characters_read==id.size())
			return id;

		// is it hex number?
		if(!id.find("0x")){
			stoi(id.substr(2),&characters_read,16); // return ignored
			if(characters_read==id.size()-2)
				return id;
		}

		// is it binary number?
		if(!id.find("0b")){
			stoi(id.substr(2),&characters_read,2); // return ignored
			if(characters_read==id.size()-2)
				return id;
		}

		return"";
	}

	inline void check_usage(){
		if(frames_.size()>max_frame_count_)
			max_frame_count_=frames_.size();
	}

	vector<frame>frames_;
	vector<string>all_registers_;
	vector<string>scratch_registers_;
	vector<string>allocated_registers_;
	vector<string>named_registers_;
	size_t max_usage_scratch_regs_{0};
	size_t max_frame_count_{0};
	string source_str_;
	lut<field_meta>fields_;
	lut<const stmt_def_func*>funcs_;
	lut<const stmt_def_table*>tables_;
	vector<size_t>call_allocated_regs_idx_;
	unordered_set<string>initiated_registers_;
};
