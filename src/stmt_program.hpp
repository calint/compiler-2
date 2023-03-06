#pragma once
#include<memory>
#include"stmt_call.hpp"
#include"decouple.hpp"
#include"toc.hpp"
#include"token.hpp"
#include"tokenizer.hpp"
#include"stmt_def_func.hpp"
#include"stmt_def_field.hpp"
#include"stmt_def_table.hpp"
#include"stmt_comment.hpp"



class stmt_program final:public statement{public:

	inline stmt_program(const string&source):
		statement{*this,token{}},
		tc_{source},
		t_{source}
	{
		vector<string>assem{"mov","int","xor","syscall"};
		for(auto&s:assem)
			tc_.add_func(*this,s,nullptr);

		while(true){
			if(t_.is_eos())
				break;

			const token tk=t_.next_token();

			if(tk.is_blank()){
				if(t_.is_eos())
					break;
				throw compiler_error(tk,"unexpected blank token");
			}
			if(tk.is_name("field")){
				stmts_.push_back(make_unique<stmt_def_field>(*this,tk,t_));
			}else
			if(tk.is_name("func")){
				stmts_.push_back(make_unique<stmt_def_func>(*this,tk,t_));
			}else
			if(tk.is_name("table")){
				stmts_.push_back(make_unique<stmt_def_table>(*this,tk,t_));
			}else
			if(tk.is_name("#")){
				stmts_.push_back(make_unique<stmt_comment>(*this,tk,t_));
			}else
			if(tk.is_name("")){// whitespace
				stmts_.push_back(make_unique<statement>(*this,tk));
			}else{
				throw compiler_error(tk,"unexpected keyword",tk.name());
			}
		}
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		for(auto&s:stmts_)
			s->source_to(os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		os<<"section .data\n";
		for(auto&s:stmts_)
			if(s->is_in_data_section())s->compile(tc,os,indent_level);

		os<<"\nsection .bss\nstk resd 256\nstk.end:\n";
		os<<"\nsection .text\nglobal _start\n_start:\n  mov ebp,stk\n  mov esp,stk.end\n";
		for(auto&s:stmts_)
			if(!s->is_in_data_section())
				s->compile(tc,os,indent_level);

		const stmt_def_func*main=tc.get_func_or_break(*this,"main");
		tc.push_func("main");
		main->code_block()->compile(tc,os,indent_level);
		tc.pop_func("main");
	}

	inline void build(ostream&os){
		compile(tc_,os,0);
		link(tc_,os);
		tc_.finish(tc_,os);
	}

private:
	vector<unique_ptr<statement>>stmts_;
	toc tc_;
	tokenizer t_;
};
