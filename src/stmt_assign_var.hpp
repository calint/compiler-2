#pragma once

#include "bool_ops_list.hpp"
#include"expr_ops_list.hpp"

class stmt_assign_var final:public statement{
public:
	inline stmt_assign_var(toc&tc,token name,token type,tokenizer&t):
		statement{move(name)},
		type_{move(type)}
	{
		const ident_resolved&ir{tc.resolve_ident_to_nasm(*this,false)};
		if(ir.tp.name()==toc::bool_type_str){
			eols_=bool_ops_list{tc,t};
			return;
		}
		eols_=expr_ops_list{tc,t};
		set_type(ir.tp);
	}

	inline stmt_assign_var()=default;
	inline stmt_assign_var(const stmt_assign_var&)=default;
	inline stmt_assign_var(stmt_assign_var&&)=default;
	inline stmt_assign_var&operator=(const stmt_assign_var&)=default;
	inline stmt_assign_var&operator=(stmt_assign_var&&)=default;

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if(not type_.is_empty()){
			os<<':';
			type_.source_to(os);
		}
		os<<"=";
		if(eols_.index()==0){
			get<expr_ops_list>(eols_).source_to(os);
		}else{
			get<bool_ops_list>(eols_).source_to(os);
		}
	}

//	inline void source_def_to(ostream&os)const{
//		statement::source_to(os);
//		if(not type_.is_empty()){
//			os<<':';
//			type_.source_to(os);
//		}
//		os<<"=";
//		if(eols_.index()==0){
//			get<expr_ops_list>(eols_).source_to(os);
//		}else{
//			get<bool_ops_list>(eols_).source_to(os);
//		}
//	}

	inline void compile(toc&tc,ostream&os,size_t indent,const string&dst="")const override{
		tc.source_comment(*this,os,indent);

		// for the sake of clearer error message make sure identifier can be resolved
		const ident_resolved&dest_resolved{tc.resolve_ident_to_nasm(*this,false)};

		if(eols_.index()==0){
			const expr_ops_list&eol{get<expr_ops_list>(eols_)};

			// compare generated instructions with and without allocated scratch register
			// select the method that produces least instructions

			// try without scratch register
			stringstream ss1;
			eol.compile(tc,ss1,indent,identifier());

			// try with scratch register
			stringstream ss2;
			const string&reg{tc.alloc_scratch_register(*this,ss2,indent)};
			eol.compile(tc,ss2,indent,reg);
			tc.asm_cmd(*this,ss2,indent,"mov",dest_resolved.id,reg);
			tc.free_scratch_register(ss2,indent,reg);

			// compare instruction count
			const size_t ss1_count{count_instructions(ss1)};
			const size_t ss2_count{count_instructions(ss2)};

			// select version with least instructions
			if(ss1_count<=ss2_count){
				os<<ss1.str();
			}else{
				os<<ss2.str();
			}
			tc.set_var_is_initiated(tok().name());
			return;
		}

		// bool expression
		const bool_ops_list&eol{get<bool_ops_list>(eols_)};
		const string&call_path{tc.get_inline_call_path(tok())};
		const string&src_loc{tc.source_location_for_label(tok())};
		const string&postfix{src_loc+(call_path.empty()?"":("_"+call_path))};
		const string&jmp_to_if_true{"true_"+postfix};
		const string&jmp_to_if_false{"false_"+postfix};
		const string&jmp_to_end{"end_"+postfix};
		eol.compile(tc,os,indent,jmp_to_if_false,jmp_to_if_true,false);
		tc.asm_label(*this,os,indent,jmp_to_if_true);
		tc.asm_cmd(*this,os,indent,"mov",dest_resolved.id,"1");
		tc.asm_jmp(*this,os,indent,jmp_to_end);
		tc.asm_label(*this,os,indent,jmp_to_if_false);
		tc.asm_cmd(*this,os,indent,"mov",dest_resolved.id,"0");
		tc.asm_label(*this,os,indent,jmp_to_end);
		tc.set_var_is_initiated(identifier());
	}

private:
	inline static size_t count_instructions(stringstream&ss){
		const regex rxcomment{R"(^\s*;.*$)"};
		string line;
		size_t n{0};
		while(getline(ss,line)){
			if(regex_search(line,rxcomment))
				continue;
			n++;
		}
		return n;
	}

	token type_;
	variant<expr_ops_list,bool_ops_list>eols_;
};
