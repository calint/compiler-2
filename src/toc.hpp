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
class stmt_def_type;

class allocated_var final{
public:
	inline allocated_var(const string&name,const size_t size,const int stkix,const string&nasm_ident,const char bits,const bool initiated):
		name_{name},
		size_{size},
		stkix_{stkix},
		nasm_ident_{nasm_ident},
		bits_{bits},
		initiated_{initiated}
	{}

	inline allocated_var()=default;
	inline allocated_var(const allocated_var&)=default;
	inline allocated_var(allocated_var&&)=default;
	inline allocated_var&operator=(const allocated_var&)=default;
	inline allocated_var&operator=(allocated_var&&)=default;

	inline bool is_name(const string&nm)const{return nm==name_;}

	inline bool is_const()const{return bits_&1;}

	inline const string&nasm_ident()const{return nasm_ident_;}

	inline size_t get_size()const{return size_;}

	inline const string&get_name()const{return name_;}

	inline bool is_initiated()const{return initiated_;}

	inline void set_initiated(const bool yes){initiated_=yes;}

private:
	string name_;
	size_t size_{};
	int stkix_{};
	string nasm_ident_;
	char bits_{}; // 1: const
	bool initiated_{};
};

class frame final{
public:
	enum class type{FUNC,BLOCK,LOOP};

	inline frame(const string&name,const type tpe,const string&call_path="",const string&ret_label="",const bool is_inline=false,const string&ret_var=""):
		name_{name},
		call_path_{call_path},
		ret_label_{ret_label},
		ret_var_{ret_var},
		is_inline_{is_inline},
		type_{tpe}
	{}

	inline void add_var(const string&nm,const size_t size,const int stkix,const bool initiated){
		string ident;
		if(stkix>0){
			// function argument
			ident="[rbp+"+to_string(stkix)+"]";
		}else if(stkix<0){
			// variable
			ident="[rbp"+to_string(stkix)+"]";
			allocated_stack_+=size;
		}else{
			throw "toc:fram:add_var";
		}
		if(size==8){
			ident="qword"+ident;
		// }else if(size==4){
		// 	ident="dword"+ident;
		// }else if(size==2){
		// 	ident="word"+ident;
		// }else if(size==1){
		// 	ident="byte"+ident;
		}else{
			throw"unexpected variable size: "+to_string(size);
		}
		vars_.put(nm,allocated_var{nm,size,stkix,ident,0,initiated});
	}

	inline size_t allocated_stack_size()const{return allocated_stack_;}

	inline bool has_var(const string&name)const{return vars_.has(name);}

	inline allocated_var get_var(const string&name)const{return vars_.get(name);}

	inline allocated_var&get_var_ref(const string&name){return vars_.get_ref(name);}

	inline void add_alias(const string&ident,const string&outside_ident){
		aliases_.put(ident,outside_ident);
	}

	inline bool is_func()const{return type_==type::FUNC;}

	inline bool is_block()const{return type_==type::BLOCK;}

	inline bool is_loop()const{return type_==type::LOOP;}

	inline bool is_name(const string&nm)const{return name_==nm;}

	inline bool has_alias(const string&name)const{return aliases_.has(name);}

	inline const string get_alias(const string&name)const{return aliases_.get(name);}

	inline const string&name()const{return name_;}

	inline const string&ret_label()const{return ret_label_;}

	inline const string&inline_call_path()const{return call_path_;}

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
	size_t last_added_var_size{0};
};

struct field_meta final{
	const stmt_def_field*def{nullptr};
	bool is_str{false};
};

struct call_meta final{
	size_t nregs_pushed{0};
	size_t alloc_reg_idx{0};
	size_t nbytes_of_vars{0};
};

struct ident_resolved final{
	enum class type{CONST,VAR,REGISTER,FIELD,IMPLIED};

	string id;
	type type{type::CONST};

	inline bool is_const()const{return type==type::CONST;}
	inline bool is_var()const{return type==type::VAR;}
	inline bool is_register()const{return type==type::REGISTER;}
	inline bool is_field()const{return type==type::FIELD;}
	inline bool is_implied()const{return type==type::IMPLIED;}
};

class toc final{
public:
	inline toc(const string&source):
		all_registers_{"rax","rbx","rcx","rdx","rsi","rdi","rbp","rsp","r8","r9","r10","r11","r12","r13","r14","r15"},
		scratch_registers_{"r8","r9","r10","r11","r12","r13","r14","r15"},
		named_registers_{"rax","rbx","rcx","rdx","rsi","rdi"},
		source_str_{source}
	{}

	inline toc()=default;
	inline toc(const toc&)=default;
	inline toc(toc&&)=default;
	inline toc&operator=(const toc&)=default;
	inline toc&operator=(toc&&)=default;

	inline void add_field(const statement&s,const string&ident,const stmt_def_field*f,const bool is_str_field){
		if(fields_.has(ident))
			throw compiler_error(s.tok(),"field '"+ident+"' already defined");

		fields_.put(ident,{f,is_str_field});
	}

	inline void add_func(const statement&s,const string&ident,const stmt_def_func*ref){
		if(funcs_.has(ident))
			throw compiler_error(s,"function '"+ident+"' already defined");

		funcs_.put(ident,ref);
	}

	inline const stmt_def_func&get_func_or_break(const statement&s,const string&name)const{
		if(not funcs_.has(name))
			throw compiler_error(s,"function '"+name+"' not found");

		return*funcs_.get(name);
	}

	inline void add_type(const statement&s,const string&ident,const stmt_def_type*f){
		if(types_.has(ident))
			throw compiler_error(s,"type '"+ident +"' already defined");

		types_.put(ident,f);
	}

	inline string source_location(const token&t)const{
		size_t char_in_line;
		const size_t n{line_number_for_char_index(t.char_index(),source_str_.c_str(),char_in_line)};
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

	inline void finish(ostream&os){
		assert(scratch_registers_.size()==8);
		assert(named_registers_.size()==6);
		assert(frames_.empty());
		os<<"\n; max scratch registers in use: "<<max_usage_scratch_regs_<<endl;
		os<<";            max frames in use: "<<max_frame_count_<<endl;
//		os<<";          max stack in use: "<<tc.max_stack_usage_<<endl;
	}

	inline ident_resolved resolve_ident_to_nasm(const statement&stmt,const bool must_be_initiated)const{
		const ident_resolved&ir{resolve_ident_to_nasm_or_empty(stmt,stmt.identifier(),must_be_initiated)};
		if(not ir.id.empty())
			return ir;

		throw compiler_error(stmt,"cannot resolve identifier '"+stmt.identifier()+"'");
	}

	inline ident_resolved resolve_ident_to_nasm(const statement&stmt,const string&ident,const bool must_be_initiated)const{
		const ident_resolved&ir{resolve_ident_to_nasm_or_empty(stmt,ident,must_be_initiated)};
		if(not ir.id.empty())
			return ir;

		throw compiler_error(stmt.tok(),"cannot resolve identifier '"+ident+"'");
	}

	inline void add_alias(const string&ident,const string&parent_frame_ident){
		frames_.back().add_alias(ident,parent_frame_ident);
	}

	inline void enter_func(const string&name,const string&call_loc,const string&ret_jmp,const bool is_inline,const string&ret_var){
		frames_.emplace_back(name,frame::type::FUNC,call_loc,ret_jmp,is_inline,ret_var);
		check_usage();
	}

	inline void enter_block(){
		frames_.emplace_back("",frame::type::BLOCK);
		check_usage();
	}

	inline void enter_loop(const string&name){
		frames_.emplace_back(name,frame::type::LOOP);
		check_usage();
	}

	inline void exit_func(const string&name){
		const frame&f{frames_.back()};
		assert(f.is_func() and f.is_name(name));
		frames_.pop_back();
	}

	inline void exit_loop(const string&name){
		const frame&f{frames_.back()};
		assert(f.is_loop() and f.is_name(name));
		frames_.pop_back();
	}

	inline void exit_block(){
		const frame&f{frames_.back()};
		assert(f.is_block());
		frames_.pop_back();
	}

	inline void add_var(const statement&st,ostream&os,size_t indent_level,const string&name,const size_t size,const bool initiated){
		if(frames_.back().has_var(name))
			throw compiler_error(st,"variable '"+name+"' already declared");

		// offset by 8 since if stkix is 0 then rsp points at return address
		//   or past the end of stack (if no function has been called)
		const size_t stkix{get_current_stack_size()+8};
		assert(size==8||size==4||size==2||size==1);
		frames_.back().add_var(name,size,-int(stkix),initiated);
		// comment the resolved name
		const ident_resolved&ir=resolve_ident_to_nasm(st,name,false);
		const string&dest_resolved{ir.id};
		indent(os,indent_level,true);os<<name<<": "<<dest_resolved<<endl;
	}

	inline void add_func_arg(const statement&st,ostream&os,size_t indent_level,const string&name,const size_t size,const int stkix_delta){
		assert(frames_.back().is_func()&&not frames_.back().is_func_inline());
		frames_.back().add_var(name,size,stkix_delta,true);
		// comment the resolved name
		const ident_resolved&ir{resolve_ident_to_nasm(st,name,false)};
		indent(os,indent_level,true);os<<name<<": "<<ir.id<<endl;
	}

	inline const string&alloc_scratch_register(const statement&st,ostream&os,const size_t indent_level){
		if(scratch_registers_.empty())
			throw compiler_error(st,"out of scratch registers. try to reduce expression complexity");

		const string&r{scratch_registers_.back()};
		scratch_registers_.pop_back();

		indent(os,indent_level,true);os<<"alloc "<<r<<endl;

		const size_t n{8-scratch_registers_.size()};
		if(n>max_usage_scratch_regs_) // ? stmt_assign_var tries 2 different methods making this metric inaccurate if a discarded method uses more registers than the selected method
			max_usage_scratch_regs_=n;

		allocated_registers_.push_back(r);
		return r;
	}

	inline void alloc_named_register_or_break(const statement&st,ostream&os,const size_t indent_level,const string&reg){
		indent(os,indent_level,true);os<<"alloc "<<reg<<endl;
		auto r{find(named_registers_.begin(),named_registers_.end(),reg)};
		if(r==named_registers_.end()){
			throw compiler_error(st,"named register '"+reg+"' cannot be allocated");
		}
		named_registers_.erase(r);
		allocated_registers_.push_back(reg);
	}

	inline bool alloc_named_register(const statement&st,ostream&os,const size_t indent_level,const string&reg){
		indent(os,indent_level,true);os<<"alloc "<<reg;
		auto r{find(named_registers_.begin(),named_registers_.end(),reg)};
		if(r==named_registers_.end()){
			os<<" failed"<<endl;
			return false;
		}
		named_registers_.erase(r);
		allocated_registers_.push_back(reg);
		os<<endl;
		return true;
	}

	inline void free_named_register(ostream&os,const size_t indent_level,const string&reg){
		indent(os,indent_level,true);os<<"free "<<reg<<endl;
		assert(allocated_registers_.back()==reg);
		allocated_registers_.pop_back();
		named_registers_.push_back(reg);
		initiated_registers_.erase(reg);
	}

	inline void free_scratch_register(ostream&os,const size_t indent_level,const string&reg){
		indent(os,indent_level,true);os<<"free "<<reg<<endl;
		assert(allocated_registers_.back()==reg);
		allocated_registers_.pop_back();
		scratch_registers_.push_back(reg);
		initiated_registers_.erase(reg);
	}

	inline const string&get_loop_label_or_break(const statement&st)const{
		size_t i{frames_.size()};
		while(i--){
			if(frames_[i].is_loop())
				return frames_[i].name();
			if(frames_[i].is_func())
				throw compiler_error(st,"not in a loop");
		}
		throw compiler_error(st,"unexpected frames");
	}

	inline const string&get_inline_call_path(const token&tk)const{
		size_t i{frames_.size()};
		while(i--){
			if(frames_[i].is_func())
				return frames_[i].inline_call_path();
		}
		throw compiler_error(tk,"not in a function");
	}

	inline const string&get_func_return_label_or_break(const statement&st)const{
		size_t i{frames_.size()};
		while(i--){
			if(frames_[i].is_func())
				return frames_[i].ret_label();
		}
		throw compiler_error(st,"not in a function");
	}

	inline const string&get_func_return_var_name_or_break(const statement&st)const{
		size_t i{frames_.size()};
		while(i--){
			if(frames_[i].is_func())
				return frames_[i].ret_var();
		}
		throw compiler_error(st,"not in a function");
	}

	inline void source_comment(const statement&st,ostream&os,const size_t indent_level)const{
		size_t char_in_line;
		const size_t n{line_number_for_char_index(st.tok().char_index(),source_str_.c_str(),char_in_line)};

		indent(os,indent_level,true);
		os<<"["<<to_string(n)<<":"<<char_in_line<<"]";

		stringstream ss;
		ss<<" ";
		st.source_to(ss);
		const string&s{ss.str()};
		const string&res{regex_replace(s,regex("\\s+")," ")};
		os<<res<<endl;
	}

	inline void source_comment(ostream&os,const string&dest,const char op,const statement&stmt)const{
		size_t char_in_line;
		const size_t n{line_number_for_char_index(stmt.tok().char_index(),source_str_.c_str(),char_in_line)};
		os<<"["<<to_string(n)<<":"<<char_in_line<<"]";

		stringstream ss;
		ss<<" "<<dest<<op;
		stmt.source_to(ss);
		const string&s{ss.str()};
		const string&res{regex_replace(s,regex("\\s+")," ")};
		os<<res<<endl;
	}

	inline void token_comment(ostream&os,const token&tk)const{
		size_t char_in_line;
		const size_t n{line_number_for_char_index(tk.char_index(),source_str_.c_str(),char_in_line)};
		os<<"["<<to_string(n)<<":"<<char_in_line<<"]";
		os<<" "<<tk.name()<<endl;
	}

	inline bool is_identifier_register(const string&id)const{
		return find(all_registers_.begin(),all_registers_.end(),id)!=all_registers_.end();
	}

	inline void enter_call(const statement&st,ostream&os,const size_t indent_level){
		const bool root_call{call_metas_.empty()};
		const size_t nbytes_of_vars_on_stack{root_call?get_current_stack_size():0};
		if(root_call){
			// this call is not nested within another call's arguments
			if(nbytes_of_vars_on_stack){
				// adjust stack past the allocated vars
				asm_cmd(st,os,indent_level,"sub","rsp",to_string(nbytes_of_vars_on_stack));
				// stack: <base>,.. vars ..,
			}
		}
		// index in the allocated registers that have been allocated but not pushed
		// prior to this call (that might clobber them)
		const size_t alloc_regs_idx{root_call?0:call_metas_.back().alloc_reg_idx};

		// push registers allocated prior to this call
		const size_t n{allocated_registers_.size()};
		size_t nregs_pushed_on_stack{0};
		for(size_t i{alloc_regs_idx};i<n;i++){
			const string&reg{allocated_registers_[i]};
			if(is_register_initiated(reg)){
				// push only registers that contain a valid value
				asm_push(st,os,indent_level,reg);
				nregs_pushed_on_stack++;
			}
		}
		call_metas_.push_back(call_meta{nregs_pushed_on_stack,allocated_registers_.size(),nbytes_of_vars_on_stack});
	}

	inline void exit_call(const statement&st,ostream&os,const size_t indent_level,const size_t nbytes_of_args_on_stack,const vector<string>&allocated_args_registers){
		const size_t nregs_pushed{call_metas_.back().nregs_pushed};
		const size_t nbytes_of_vars{call_metas_.back().nbytes_of_vars};
		call_metas_.pop_back();
		const bool restore_rsp_to_base=call_metas_.empty();
		const size_t alloc_reg_idx{restore_rsp_to_base?0:call_metas_.back().alloc_reg_idx};

		// optimization can be done if no registers need to be popped
		//   rsp is adjusted once
		if(nregs_pushed==0){
			// stack is: <base>,vars,args,
			if(restore_rsp_to_base){
				const string&offset=to_string(nbytes_of_args_on_stack+nbytes_of_vars);
				asm_cmd(st,os,indent_level,"add","rsp",offset);
				// stack is: <base>,
			}else{
				const string&offset=to_string(nbytes_of_args_on_stack);
				asm_cmd(st,os,indent_level,"add","rsp",offset);
				// stack is: <base>,vars,
			}
			// free named registers
			size_t i=allocated_registers_.size();
			while(true){
				if(i==alloc_reg_idx)
					break;
				i--;
				const string&reg=allocated_registers_[i];
				// don't pop registers used to pass arguments
				if(find(allocated_args_registers.begin(),allocated_args_registers.end(),reg)!=allocated_args_registers.end()){
					free_named_register(os,indent_level,reg);
				}
			}
		}else{
			// stack is: <base>,vars,regs,args,
			if(nbytes_of_args_on_stack){
				const string&offset=to_string(nbytes_of_args_on_stack);
				asm_cmd(st,os,indent_level,"add","rsp",offset);
			}
			// stack is: <base>,vars,regs,

			// pop registers pushed prior to this call
			size_t i=allocated_registers_.size();
			while(true){
				if(i==alloc_reg_idx)
					break;
				i--;
				const string&reg=allocated_registers_[i];
				// don't pop registers used to pass arguments
				if(find(allocated_args_registers.begin(),allocated_args_registers.end(),reg)==allocated_args_registers.end()){
					if(is_register_initiated(reg)){
						// pop only registers that were pushed
						asm_pop(st,os,indent_level,reg);
					}
				}else{
					free_named_register(os,indent_level,reg);
				}
			}

			// stack is: <base>,vars,
			if(restore_rsp_to_base){
				// this was not a call within the arguments of another call
				// stack is: <base>,vars,
				if(nbytes_of_vars){
					const string&offset=to_string(nbytes_of_vars);
					asm_cmd(st,os,indent_level,"add","rsp",offset);
				}
				// stack is: <base>,
			}
		}
	}

	inline void asm_cmd(const statement&st,ostream&os,const size_t indent_level,const string&op,const string&dest_resolved,const string&src_resolved){
		if(op=="mov"){
			if(dest_resolved==src_resolved)
				return;

			if(is_identifier_register(dest_resolved)){
				// for optmiziation of push/pop when call
				initiated_registers_.insert(dest_resolved);
			}
		}
		// check if both source and destination are memory operations
		if(dest_resolved.find_first_of('[')!=string::npos and src_resolved.find_first_of('[')!=string::npos){
			const string&r=alloc_scratch_register(st,os,indent_level);
			indent(os,indent_level);os<<"mov "<<r<<","<<src_resolved<<endl;
			indent(os,indent_level);os<<op<<" "<<dest_resolved<<","<<r<<endl;
			free_scratch_register(os,indent_level,r);
			return;
		}
		indent(os,indent_level);os<<op<<" "<<dest_resolved<<","<<src_resolved<<endl;
	}

	inline void asm_push(const statement&st,ostream&os,const size_t indent_level,const string&reg){
		indent(os,indent_level);os<<"push "<<reg<<endl;
	}

	inline void asm_pop(const statement&st,ostream&os,const size_t indent_level,const string&reg){
		indent(os,indent_level);os<<"pop "<<reg<<endl;
	}

	inline void asm_ret(const statement&st,ostream&os,const size_t indent_level){
		indent(os,indent_level);os<<"ret\n";
	}

	inline void asm_jmp(const statement&st,ostream&os,const size_t indent_level,const string&label){
		indent(os,indent_level);os<<"jmp "<<label<<endl;
	}

	inline void asm_label(const statement&st,ostream&os,const size_t indent_level,const string&label){
		indent(os,indent_level);os<<label<<":"<<endl;
	}

	inline void asm_call(const statement&st,ostream&os,const size_t indent_level,const string&label){
		indent(os,indent_level);os<<"call "<<label<<endl;
	}

	inline void asm_neg(const statement&st,ostream&os,const size_t indent_level,const string&dest_resolved){
		indent(os,indent_level);os<<"neg "<<dest_resolved<<endl;
	}

	inline void var_has_been_initiated(const statement&st,const string&name){
		string id{name};
		// traverse the frames and resolve the id (which might be an alias) to
		// a variable, field, register or constant
		size_t i{frames_.size()};
		while(i--){
			// is the frame a function?
			if(frames_[i].is_func()){
				// is it an alias defined by an argument in the function?
				if(not frames_[i].has_alias(id))
					break;
				// yes, continue resolving alias until it is
				// a variable, field, register or constant
				id=frames_[i].get_alias(id);
			}else{
				// does scope contain the variable
				if(frames_[i].has_var(id))
					break;
			}
		}

		// is 'id' a variable?
		if(frames_[i].has_var(id)){
			frames_[i].get_var_ref(id).set_initiated(true);
			return;
		}
		
		// a register or field, assumed initiated
	} 

	inline static void indent(ostream&os,const size_t indent_level,const bool comment=false){
		if(indent_level==0){
			if(comment)
				os<<";";
			return;
		}
		os<<(comment?";":" ");
		for(size_t i{0};i<indent_level;i++)
			os<<"  ";
	}

private:
	inline bool is_register_initiated(const string&reg)const{
		return initiated_registers_.contains(reg);
	}

	inline size_t get_current_stack_size()const{
		assert(frames_.size());
		size_t n{0};
		size_t i{frames_.size()};
		while(i--){
			const frame&f{frames_[i]};
			n+=f.allocated_stack_size();
			if(f.is_func()&&not f.is_func_inline())
				break;
		}
		return n;
	}

	inline const ident_resolved resolve_ident_to_nasm_or_empty(const statement&stmt,const string&ident,const bool must_be_initiated=true)const{
		string id=ident;
		// traverse the frames and resolve the id (which might be an alias) to
		// a variable, field, register or constant
		size_t i{frames_.size()};
		while(i--){
			// is the frame a function?
			if(frames_[i].is_func()){
				// is it an alias defined by an argument in the function?
				if(not frames_[i].has_alias(id))
					break;
				// yes, continue resolving alias until it is
				// a variable, field, register or constant
				id=frames_[i].get_alias(id);
			}else{
				// does scope contain the variable
				if(frames_[i].has_var(id))
					break;
			}
		}

		// is 'id' a variable?
		if(frames_[i].has_var(id)){
			allocated_var var=frames_[i].get_var(id);
			if(must_be_initiated and not var.is_initiated())
				throw compiler_error(stmt,"variable '"+var.get_name()+"' is not initiated");
			return{var.nasm_ident(),ident_resolved::type::VAR};
		}

		// is 'id' a register?
		if(is_identifier_register(id)){
			if(must_be_initiated and not is_register_initiated(id))
				throw compiler_error(stmt,"register '"+id+"' is not initiated");

			return{id,ident_resolved::type::REGISTER}; // ? unary ops?
		}

		// is 'id' a field?
		if(fields_.has(id)){
			const field_meta&fm=fields_.get(id);
			if(fm.is_str)
				return{id,ident_resolved::type::FIELD};
			return{"qword["+id+"]",ident_resolved::type::FIELD};
		}

		// is 'id' an implicit identifier?
		// i.e. 'prompt.len' of a string field 'prompt'
		const string&subid=id.substr(0,id.find('.'));
		if(fields_.has(subid)){
			const string&after_dot=id.substr(subid.size()+1);
			if(after_dot=="len"){
				return{id,ident_resolved::type::IMPLIED};
			}
			throw compiler_error(stmt.tok(),"unknown implicit field constant '"+id+"'");
		}

		char*ep;
		strtol(id.c_str(),&ep,10); // return ignored
		if(!*ep)
			return{id,ident_resolved::type::CONST};

		if(id.find("0x")==0){ // hex
			strtol(id.c_str()+2,&ep,16); // return ignored
			if(!*ep)
				return{id,ident_resolved::type::CONST};
		}

		if(id.find("0b")==0){ // binary
			strtol(id.c_str()+2,&ep,2); // return ignored
			if(!*ep)
				return{id,ident_resolved::type::CONST};
		}

		// not resolved, return empty answer
		return{"",ident_resolved::type::CONST};
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
	lut<const stmt_def_type*>types_;
	vector<call_meta>call_metas_;
	unordered_set<string>initiated_registers_;
};
