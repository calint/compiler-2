#pragma once

#include"expr_ops_list.hpp"

class stmt_assign_var final:public statement{
public:
	inline stmt_assign_var(token tk,tokenizer&t):
		statement{move(tk)},
		eols_{t}
	{}

	inline stmt_assign_var()=default;
	inline stmt_assign_var(const stmt_assign_var&)=default;
	inline stmt_assign_var(stmt_assign_var&&)=default;
	inline stmt_assign_var&operator=(const stmt_assign_var&)=default;
	inline stmt_assign_var&operator=(stmt_assign_var&&)=default;

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		os<<"=";
		eols_.source_to(os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent,const string&dst="")const override{
		tc.source_comment(*this,os,indent);

		// for the sake of clearer error message make sure identifier can be resolved
		const ident_resolved&dest_resolved{tc.resolve_ident_to_nasm(*this,false)};

		// compare generated instructions with and without allocated scratch register
		// select the method that produces least instructions

		// try without scratch register
		stringstream ss1;
		eols_.compile(tc,ss1,indent,identifier());

		// try with scratch register
		stringstream ss2;
		const string&reg{tc.alloc_scratch_register(*this,ss2,indent)};
		eols_.compile(tc,ss2,indent,reg);
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
	}

private:
	inline static size_t count_instructions(stringstream&ss){
		const regex regex{R"(^\s*;.*$)"};
		string line;
		size_t n{0};
		while(getline(ss,line)){
			if(regex_search(line,regex))
				continue;
			n++;
		}
		return n;
	}

	expr_ops_list eols_;
};
