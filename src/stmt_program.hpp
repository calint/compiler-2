#pragma once

#include"statement.hpp"
#include"toc.hpp"
#include"tokenizer.hpp"
#include"stmt_def_field.hpp"
#include"stmt_def_func.hpp"
#include"stmt_def_table.hpp"

class stmt_program final:public statement{
public:
	inline stmt_program(const string&source):
		statement{*this,token{}},
		tc_{source}
	{
		vector<string>assem{"mov","int","xor","syscall"};
		for(const auto&s:assem)
			tc_.add_func(*this,s,nullptr);
			
		tokenizer t{source};
		while(true){
			if(t.is_eos())
				break;

			const token tk=t.next_token();

			if(tk.is_blank()){
				if(t.is_eos())
					break;
				throw compiler_error(tk,"unexpected blank token");
			}
			if(tk.is_name("field")){
				stmts_.push_back(make_unique<stmt_def_field>(*this,tk,t));
			}else
			if(tk.is_name("func")){
				stmts_.push_back(make_unique<stmt_def_func>(*this,tk,t));
			}else
			if(tk.is_name("table")){
				stmts_.push_back(make_unique<stmt_def_table>(*this,tk,t));
			}else
			if(tk.is_name("#")){
				stmts_.push_back(make_unique<stmt_comment>(*this,tk,t));
			}else
			if(tk.is_name("")){// whitespace
				stmts_.push_back(make_unique<statement>(*this,tk));
			}else{
				throw compiler_error(tk,"unexpected keyword '"+tk.name()+"'");
			}
		}
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		for(const auto&s:stmts_)
			s->source_to(os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		os<<"section .data\n";
		for(const auto&s:stmts_)
			if(s->is_in_data_section())
				s->compile(tc,os,indent_level);

		os<<"\nsection .bss\nstk resd 256\nstk.end:\n";
		os<<"\nsection .text\nbits 32\nglobal _start\n_start:\nmov ebp,stk\nmov esp,stk.end\n";
		for(const auto&s:stmts_)
			if(!s->is_in_data_section())
				s->compile(tc,os,indent_level);

		const stmt_def_func&main=tc.get_func_or_break(*this,"main");
		tc.push_func("main");
		main.code_block().compile(tc,os,indent_level);
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
};
