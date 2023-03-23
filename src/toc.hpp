#pragma once

#include<cassert>
#include<sstream>
#include<regex>
#include<unordered_set>

#include"lut.hpp"
#include"compiler_error.hpp"

class stmt_def_func;
class stmt_def_field;
class stmt_def_type;

struct var_meta final{
	string name;
	token declared_at;
	size_t size{}; // in bytes
	int stack_idx{}; // location relative to rbp
	string nasm_ident; // nasm usable literal
	bool initiated{}; // true if variable has been initiated
};

class frame final{
public:
	enum class type{FUNC,BLOCK,LOOP};

	inline frame(const string&name,const type type,const string&call_path="",const string&func_ret_label="",const bool func_is_inline=false,const string&func_ret_var=""):
		name_{name},
		call_path_{call_path},
		func_ret_label_{func_ret_label},
		func_ret_var_{func_ret_var},
		func_is_inline_{func_is_inline},
		type_{type}
	{}

	inline void add_var(const string&name,const token&declared_at,const size_t size,const int stack_idx,const bool initiated){
		string ident;
		if(stack_idx>0){
			// function argument
			ident="[rbp+"+to_string(stack_idx)+"]";
		}else if(stack_idx<0){
			// variable
			ident="[rbp"+to_string(stack_idx)+"]";
			allocated_stack_+=size;
		}else{
			throw"toc:fram:add_var";
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
		vars_.put(name,{name,declared_at,size,stack_idx,ident,initiated});
	}

	inline size_t allocated_stack_size()const{return allocated_stack_;}

	inline bool has_var(const string&name)const{return vars_.has(name);}

	inline var_meta&get_var_ref(const string&name){return vars_.get_ref(name);}

	inline const var_meta&get_var_const_ref(const string&name)const{return vars_.get_const_ref(name);}

	inline void add_alias(const string&ident,const string&outside_ident){aliases_.put(ident,outside_ident);}

	inline bool is_func()const{return type_==type::FUNC;}

	inline bool is_block()const{return type_==type::BLOCK;}

	inline bool is_loop()const{return type_==type::LOOP;}

	inline bool is_name(const string&nm)const{return name_==nm;}

	inline bool has_alias(const string&nm)const{return aliases_.has(nm);}

	inline const string get_alias(const string&nm)const{return aliases_.get(nm);}

	inline const string&name()const{return name_;}

	inline const string&func_ret_label()const{return func_ret_label_;}

	inline const string&inline_call_path()const{return call_path_;}

	inline bool func_is_inline()const{return func_is_inline_;}

	inline const string&func_ret_var()const{return func_ret_var_;}

private:
	string name_; // optional name
	string call_path_; // a unique path of source locations of the inlined call
	size_t allocated_stack_{}; // number of bytes used on the stack by this frame
	lut<var_meta>vars_; // variables declared in this frame
	lut<string>aliases_; // aliases that refers to previous frame(s) alias or variable
	string func_ret_label_; // the label to jump to when exiting an inlined function
	string func_ret_var_; // the variable that contains the return value
	bool func_is_inline_{};
	type type_{type::FUNC}; // frame type
};

struct field_meta final{
	const stmt_def_field*def{};
	const token declared_at;
	const bool is_str{};
};

struct func_meta final{
	const stmt_def_func*def{};
	const token declared_at;
};

struct call_meta final{
	size_t nregs_pushed{};
	size_t alloc_reg_idx{};
	size_t nbytes_of_vars{};
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

	inline void add_field(const statement&st,const string&ident,const stmt_def_field*f,const bool is_str_field){
		if(fields_.has(ident)){
			const field_meta&fld=fields_.get(ident);
			throw compiler_error(st.tok(),"field '"+ident+"' already defined at "+source_location_hr(fld.declared_at));
		}
		fields_.put(ident,{f,st.tok(),is_str_field});
	}

	inline void add_func(const statement&st,const string&ident,const stmt_def_func*ref){
		if(funcs_.has(ident)){
			const func_meta&func=funcs_.get(ident);
			throw compiler_error(st,"function '"+ident+"' already defined at "+source_location_hr(func.declared_at));
		}
		funcs_.put(ident,{ref,st.tok()});
	}

	inline const stmt_def_func&get_func_or_break(const statement&st,const string&name)const{
		if(not funcs_.has(name))
			throw compiler_error(st,"function '"+name+"' not found");

		return*funcs_.get_const_ref(name).def;
	}

	inline void add_type(const statement&st,const string&ident,const stmt_def_type*f){
		if(types_.has(ident))
			throw compiler_error(st,"type '"+ident +"' already defined");

		types_.put(ident,f);
	}

	inline string source_location_for_label(const token&tk)const{
		size_t char_in_line;
		const size_t n{line_number_for_char_index(tk.char_index(),source_str_.c_str(),char_in_line)};
		return to_string(n)+"_"+to_string(char_in_line);
	}

	inline string source_location_hr(const token&tk){
		size_t char_in_line;
		const size_t n{line_number_for_char_index(tk.char_index(),source_str_.c_str(),char_in_line)};
		return to_string(n)+":"+to_string(char_in_line);
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

	inline ident_resolved resolve_ident_to_nasm(const statement&st,const bool must_be_initiated)const{
		const ident_resolved&ir{resolve_ident_to_nasm_or_empty(st,st.identifier(),must_be_initiated)};
		if(not ir.id.empty())
			return ir;

		throw compiler_error(st,"cannot resolve identifier '"+st.identifier()+"'");
	}

	inline ident_resolved resolve_ident_to_nasm(const statement&st,const string&ident,const bool must_be_initiated)const{
		const ident_resolved&ir{resolve_ident_to_nasm_or_empty(st,ident,must_be_initiated)};
		if(not ir.id.empty())
			return ir;

		throw compiler_error(st.tok(),"cannot resolve identifier '"+ident+"'");
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

	inline void add_var(const statement&st,ostream&os,size_t indnt,const string&name,const size_t size,const bool initiated){
		if(frames_.back().has_var(name)){
			const var_meta&var=frames_.back().get_var_const_ref(name);
			throw compiler_error(st,"variable '"+name+"' already declared at "+source_location_hr(var.declared_at));
		}
		pair<string,frame&>idfrm{get_id_and_frame_for_identifier(name)};
		const string&id=idfrm.first;
		if(idfrm.second.has_var(id)){
			const var_meta&var{idfrm.second.get_var_const_ref(id)};
			throw compiler_error(st,"variable '"+name+"' shadows variable declared at "+source_location_hr(var.declared_at));
		}

		// offset by 8 since if stack_idx is 0 then rsp points at return address
		//   or past the end of stack (if no function has been called)
		const size_t stack_idx{get_current_stack_size()+8};
		assert(size==8||size==4||size==2||size==1);
		frames_.back().add_var(name,st.tok(),size,-int(stack_idx),initiated);
		// comment the resolved name
		const ident_resolved&ir=resolve_ident_to_nasm(st,name,false);
		const string&dest_resolved{ir.id};
		indent(os,indnt,true);os<<name<<": "<<dest_resolved<<endl;
	}

	inline void add_func_arg(const statement&st,ostream&os,size_t indnt,const string&name,const size_t size,const int stack_idx){
		assert(frames_.back().is_func()&&not frames_.back().func_is_inline());
		frames_.back().add_var(name,st.tok(),size,stack_idx,true);
		// comment the resolved name
		const ident_resolved&ir{resolve_ident_to_nasm(st,name,false)};
		indent(os,indnt,true);os<<name<<": "<<ir.id<<endl;
	}

	inline const string&alloc_scratch_register(const statement&st,ostream&os,const size_t indnt){
		if(scratch_registers_.empty())
			throw compiler_error(st,"out of scratch registers. try to reduce expression complexity");

		const string&r{scratch_registers_.back()};
		scratch_registers_.pop_back();

		indent(os,indnt,true);os<<"alloc "<<r<<endl;

		const size_t n{8-scratch_registers_.size()};
		if(n>max_usage_scratch_regs_) // ? stmt_assign_var tries 2 different methods making this metric inaccurate if a discarded method uses more registers than the selected method
			max_usage_scratch_regs_=n;

		allocated_registers_.push_back(r);
		allocated_registers_loc_.push_back(source_location_hr(st.tok()));
		return r;
	}

	inline void alloc_named_register_or_break(const statement&st,ostream&os,const size_t indnt,const string&reg){
		indent(os,indnt,true);os<<"alloc "<<reg<<endl;
		auto r{find(named_registers_.begin(),named_registers_.end(),reg)};
		if(r==named_registers_.end()){
			const size_t n{allocated_registers_.size()};
			string loc;
			for(size_t i{0};i<n;i++){
				if(allocated_registers_[i]==reg){
					loc=allocated_registers_loc_[i];
				}
			}
			throw compiler_error(st,"cannot allocate register '"+reg+"' because it was allocated at "+loc);
		}
		named_registers_.erase(r);
		allocated_registers_.push_back(reg);
		allocated_registers_loc_.push_back(source_location_hr(st.tok()));
	}

	inline bool alloc_named_register(const statement&st,ostream&os,const size_t indnt,const string&reg){
		indent(os,indnt,true);os<<"alloc "<<reg;
		auto r{find(named_registers_.begin(),named_registers_.end(),reg)};
		if(r==named_registers_.end()){
			os<<": false"<<endl;
			return false;
		}
		named_registers_.erase(r);
		allocated_registers_.push_back(reg);
		allocated_registers_loc_.push_back(source_location_hr(st.tok()));
		os<<endl;
		return true;
	}

	inline void free_named_register(ostream&os,const size_t indnt,const string&reg){
		indent(os,indnt,true);os<<"free "<<reg<<endl;
		assert(allocated_registers_.back()==reg);
		allocated_registers_.pop_back();
		allocated_registers_loc_.pop_back();
		named_registers_.push_back(reg);
		initiated_registers_.erase(reg);
	}

	inline void free_scratch_register(ostream&os,const size_t indnt,const string&reg){
		indent(os,indnt,true);os<<"free "<<reg<<endl;
		assert(allocated_registers_.back()==reg);
		allocated_registers_.pop_back();
		allocated_registers_loc_.pop_back();
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
				return frames_[i].func_ret_label();
		}
		throw compiler_error(st,"not in a function");
	}

	inline const string&get_func_return_var_name_or_break(const statement&st)const{
		size_t i{frames_.size()};
		while(i--){
			if(frames_[i].is_func())
				return frames_[i].func_ret_var();
		}
		throw compiler_error(st,"not in a function");
	}

	inline void source_comment(const statement&st,ostream&os,const size_t indnt)const{
		size_t char_in_line;
		const size_t n{line_number_for_char_index(st.tok().char_index(),source_str_.c_str(),char_in_line)};

		indent(os,indnt,true);
		os<<"["<<to_string(n)<<":"<<char_in_line<<"]";

		stringstream ss;
		ss<<" ";
		st.source_to(ss);
		const string&s{ss.str()};
		const string&res{regex_replace(s,regex("\\s+")," ")};
		os<<res<<endl;
	}

	inline void source_comment(ostream&os,const string&dst,const string op,const statement&st)const{
		size_t char_in_line;
		const size_t n{line_number_for_char_index(st.tok().char_index(),source_str_.c_str(),char_in_line)};
		os<<"["<<to_string(n)<<":"<<char_in_line<<"]";

		stringstream ss;
		ss<<" "<<dst<<op;
		st.source_to(ss);
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

	inline void enter_call(const statement&st,ostream&os,const size_t indnt){
		const bool root_call{call_metas_.empty()};
		const size_t nbytes_of_vars_on_stack{root_call?get_current_stack_size():0};
		if(root_call){
			// this call is not nested within another call's arguments
			if(nbytes_of_vars_on_stack){
				// adjust stack past the allocated vars
				asm_cmd(st,os,indnt,"sub","rsp",to_string(nbytes_of_vars_on_stack));
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
				asm_push(st,os,indnt,reg);
				nregs_pushed_on_stack++;
			}
		}
		call_metas_.push_back(call_meta{nregs_pushed_on_stack,allocated_registers_.size(),nbytes_of_vars_on_stack});
	}

	inline void exit_call(const statement&st,ostream&os,const size_t indnt,const size_t nbytes_of_args_on_stack,const vector<string>&allocated_args_registers){
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
				asm_cmd(st,os,indnt,"add","rsp",offset);
				// stack is: <base>,
			}else{
				const string&offset=to_string(nbytes_of_args_on_stack);
				asm_cmd(st,os,indnt,"add","rsp",offset);
				// stack is: <base>,vars,
			}
			// free named registers
			size_t i{allocated_registers_.size()};
			while(true){
				if(i==alloc_reg_idx)
					break;
				i--;
				const string&reg{allocated_registers_[i]};
				// don't pop registers used to pass arguments
				if(find(allocated_args_registers.begin(),allocated_args_registers.end(),reg)!=allocated_args_registers.end()){
					free_named_register(os,indnt,reg);
				}
			}
		}else{
			// stack is: <base>,vars,regs,args,
			if(nbytes_of_args_on_stack){
				const string&offset=to_string(nbytes_of_args_on_stack);
				asm_cmd(st,os,indnt,"add","rsp",offset);
			}
			// stack is: <base>,vars,regs,

			// pop registers pushed prior to this call
			size_t i{allocated_registers_.size()};
			while(true){
				if(i==alloc_reg_idx)
					break;
				i--;
				const string&reg{allocated_registers_[i]};
				// don't pop registers used to pass arguments
				if(find(allocated_args_registers.begin(),allocated_args_registers.end(),reg)==allocated_args_registers.end()){
					if(is_register_initiated(reg)){
						// pop only registers that were pushed
						asm_pop(st,os,indnt,reg);
					}
				}else{
					free_named_register(os,indnt,reg);
				}
			}

			// stack is: <base>,vars,
			if(restore_rsp_to_base){
				// this was not a call within the arguments of another call
				// stack is: <base>,vars,
				if(nbytes_of_vars){
					const string&offset=to_string(nbytes_of_vars);
					asm_cmd(st,os,indnt,"add","rsp",offset);
				}
				// stack is: <base>,
			}
		}
	}

	inline void asm_cmd(const statement&st,ostream&os,const size_t indnt,const string&op,const string&dst_resolved,const string&src_resolved){
		if(op=="mov"){
			if(dst_resolved==src_resolved)
				return;

			if(is_identifier_register(dst_resolved)){
				// for optmiziation of push/pop when call
				initiated_registers_.insert(dst_resolved);
			}
		}
		// check if both source and destination are memory operations
		if(dst_resolved.find_first_of('[')!=string::npos and src_resolved.find_first_of('[')!=string::npos){
			const string&r=alloc_scratch_register(st,os,indnt);
			indent(os,indnt);os<<"mov "<<r<<","<<src_resolved<<endl;
			indent(os,indnt);os<<op<<" "<<dst_resolved<<","<<r<<endl;
			free_scratch_register(os,indnt,r);
			return;
		}
		indent(os,indnt);os<<op<<" "<<dst_resolved<<","<<src_resolved<<endl;
	}

	inline void asm_push(const statement&st,ostream&os,const size_t indnt,const string&reg){
		indent(os,indnt);os<<"push "<<reg<<endl;
	}

	inline void asm_pop(const statement&st,ostream&os,const size_t indnt,const string&reg){
		indent(os,indnt);os<<"pop "<<reg<<endl;
	}

	inline void asm_ret(const statement&st,ostream&os,const size_t indnt){
		indent(os,indnt);os<<"ret\n";
	}

	inline void asm_jmp(const statement&st,ostream&os,const size_t indnt,const string&label){
		indent(os,indnt);os<<"jmp "<<label<<endl;
	}

	inline void asm_label(const statement&st,ostream&os,const size_t indnt,const string&label){
		indent(os,indnt);os<<label<<":"<<endl;
	}

	inline void asm_call(const statement&st,ostream&os,const size_t indnt,const string&label){
		indent(os,indnt);os<<"call "<<label<<endl;
	}

	inline void asm_neg(const statement&st,ostream&os,const size_t indnt,const string&dst_resolved){
		indent(os,indnt);os<<"neg "<<dst_resolved<<endl;
	}

	inline void set_var_is_initiated(const string&name){
		pair<string,frame&>idfrm{get_id_and_frame_for_identifier(name)};
		const string&id=idfrm.first;
		// is 'id' a variable?
		if(idfrm.second.has_var(id)){
			idfrm.second.get_var_ref(id).initiated=true;
			return;
		}

		if(fields_.has(id))
			return; // a field, it is initiated

		if(is_identifier_register(id)){
			initiated_registers_.insert(id); // ? this might not be necessary since it is updated at asm_cmd(...,"mov",...)
			return;
		}

		throw"should not be reached: "+string{__FILE__}+":"+to_string(__LINE__);
	}

	inline static void indent(ostream&os,const size_t indnt,const bool comment=false){
		if(indnt==0){
			if(comment)
				os<<";";
			return;
		}
		os<<(comment?";":" ");
		for(size_t i{0};i<indnt;i++)
			os<<"  ";
	}

private:
	inline pair<string,frame&>get_id_and_frame_for_identifier(const string&name){
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
				continue;
			}
			// does scope contain the variable
			if(frames_[i].has_var(id))
				break;
		}
		return{move(id),frames_[i]};		
	}

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
			if(f.is_func()&&not f.func_is_inline())
				break;
		}
		return n;
	}

	inline const ident_resolved resolve_ident_to_nasm_or_empty(const statement&st,const string&ident,const bool must_be_initiated)const{
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
			const var_meta&var=frames_[i].get_var_const_ref(id);
			if(must_be_initiated and not var.initiated)
				throw compiler_error(st,"variable '"+var.name+"' is not initiated");
			return{var.nasm_ident,ident_resolved::type::VAR};
		}

		// is 'id' a register?
		if(is_identifier_register(id)){
			if(must_be_initiated and not is_register_initiated(id))
				throw compiler_error(st,"register '"+id+"' is not initiated");

			return{id,ident_resolved::type::REGISTER}; // ? unary ops?
		}

		// is 'id' a field?
		if(fields_.has(id)){
			const field_meta&fm=fields_.get_const_ref(id);
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
			throw compiler_error(st.tok(),"unknown implicit field constant '"+id+"'");
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
	vector<string>allocated_registers_loc_; // source locations of allocated_registers_
	vector<string>named_registers_;
	size_t max_usage_scratch_regs_{0};
	size_t max_frame_count_{0};
	string source_str_;
	lut<field_meta>fields_;
	lut<func_meta>funcs_;
	lut<const stmt_def_type*>types_;
	vector<call_meta>call_metas_;
	unordered_set<string>initiated_registers_;
};
