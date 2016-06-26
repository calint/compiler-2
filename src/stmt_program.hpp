#pragma once
#include<memory>
using namespace std;
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
		tc{source},
		t{source}
	{
		vector<string>assem{"mov","int","xor","syscall","cmp","je","tag","jmp","jne","if","cmove","cmovne","or","and"};
		for(auto&s:assem)tc.add_func(*this,s,nullptr);
		while(true){
			token tk=t.next_token();

			if(tk.is_blank() and t.is_eos())
				break;

			if(tk.is_blank())
				throw compiler_error(tk,"unexpected character",to_string(t.peek_char()));

			if(tk.is_name("field")){
				statements.push_back(make_unique<stmt_def_field>(*this,tk,t));
			}else
			if(tk.is_name("func")){
				statements.push_back(make_unique<stmt_def_func>(*this,tk,t));
			}else
			if(tk.is_name("table")){
				statements.push_back(make_unique<stmt_def_table>(*this,tk,t));
			}else
			if(tk.is_name("//")){
				statements.push_back(make_unique<stmt_comment>(*this,tk,t));
			}else
			if(tk.is_name("")){// whitespace
				statements.push_back(make_unique<statement>(*this,tk));
			}else{
				throw compiler_error(tk,"unexpected keyword",tk.name());
			}
		}
	}

	inline void build(ostream&os){
		compile(tc,os,0);
		link(tc,os);
		tc.finish(tc,os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		os<<"section .data\n";
		for(auto&s:statements)
			if(s->is_in_data_section())s->compile(tc,os,indent_level);
		os<<"\ndd1 dd 1\ndd0 dd 0\n";
		os<<"\nsection .bss\nstk resd 256\nstk.end\n\n";

		os<<"\nsection .text\nglobal _start\n_start:\n  mov ebp,stk\n  mov esp,stk.end\n";
		for(auto&s:statements)
			if(!s->is_in_data_section())s->compile(tc,os,indent_level);

		const stmt_def_func*main=tc.get_func_or_break(*this,"main");
		tc.push_func("main");
//		indent(os,indent_level,true);os<<"main(){  ["<<token().token_start_char()<<"]"<<endl;

		main->code_block()->compile(tc,os,indent_level);

//		indent(os,indent_level,true);os<<"}\n\n";

		tc.pop_func("main");
	}

	inline void link(toc&tc,ostream&os)const override{for(auto&s:statements)s->link(tc,os);}

	inline void source_to(ostream&os)const override{statement::source_to(os);for(auto&s:statements)s->source_to(os);}

	inline const toc&get_toc()const{return tc;}

	inline bool is_inline()const{return true;}


private:
	vector<up_statement>statements;
	toc tc;
	tokenizer t;
};
