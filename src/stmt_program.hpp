#pragma once

#include"statement.hpp"
#include"toc.hpp"
#include"tokenizer.hpp"
#include"stmt_def_field.hpp"
#include"stmt_def_func.hpp"
#include"stmt_def_type.hpp"

class stmt_program final:public statement{
public:
	inline stmt_program(const string&source):
		statement{token{}},
		tc_{source}
	{
		// add built-in assembler calls
		vector<string>assem{"mov","int","xor","syscall"};
		for(const string&s:assem)
			tc_.add_func(*this,s,nullptr);

		tokenizer t{source};
		while(true){
			token tk{t.next_token()};
			if(tk.is_blank()){
				if(t.is_eos())
					break;
				throw compiler_error(tk,"unexpected blank token");
			}
			if(tk.is_name("field")){
				stms_.emplace_back(make_unique<stmt_def_field>(move(tk),t));
			}else if(tk.is_name("func")){
				stms_.emplace_back(make_unique<stmt_def_func>(move(tk),t));
			}else if(tk.is_name("type")){
				stms_.emplace_back(make_unique<stmt_def_type>(move(tk),t));
			}else if(tk.is_name("#")){
				stms_.emplace_back(make_unique<stmt_comment>(move(tk),t));
			}else if(tk.is_name("")){
				stms_.emplace_back(make_unique<stmt_whitespace>(move(tk)));
			}else{
				throw compiler_error(tk,"unexpected keyword '"+tk.name()+"'");
			}
		}
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		for(const auto&s:stms_)
			s->source_to(os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		os<<"; generated by baz\n\n";
		os<<"section .data\nalign 4\n";
		for(const auto&s:stms_)
			if(s->is_in_data_section())
				s->compile(tc,os,indent_level);

		os<<"\nsection .bss\nalign 4\nstk resd 256\nstk.end:\n";
		os<<"\nsection .text\nalign 4\nbits 64\nglobal _start\n_start:\nmov rsp,stk.end\nmov rbp,rsp\njmp main\n\n";
		for(const auto&s:stms_)
			if(!s->is_in_data_section())
				s->compile(tc,os,indent_level);

		// get the main function and compile
		const stmt_def_func&main=tc.get_func_or_break(*this,"main");
		if(!main.is_inline())
			throw compiler_error(main,"main function must be declared inline");
			
		os<<"main:"<<endl;
		tc.enter_func("main","","",true,"");
		main.code().compile(tc,os,indent_level);
		tc.exit_func("main");
	}

	inline void build(ostream&os){
		compile(tc_,os,0);
		tc_.finish(os);
	}

private:
	vector<unique_ptr<statement>>stms_;
	toc tc_;
};
