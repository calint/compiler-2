#pragma once
#include<memory>
using namespace std;
#include "stmt_call.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include"stmt_def_func.hpp"
#include"stmt_def_field.hpp"
class stmt_program final:public statement{public:

	inline stmt_program(tokenizer&t):statement{nullptr,make_unique<class token>()}{
		vector<const char*>assem{"mov","int","xor","syscall","cmp","je","tag","jmp","jne","if","cmove","cmovne","or","and"};
		for(auto s:assem)tc.add_func(*this,s,nullptr);
		while(!t.is_eos()){
			up_token tk=t.next_token();
			up_statement stmt;
			if(tk->is_name("field")){stmt=make_unique<stmt_def_field>(this,move(tk),t);}
			else if(tk->is_name("func")){stmt=make_unique<stmt_def_func>(this,move(tk),t);}
			else{
				if(tk->is_name("")){
					stmt=make_unique<statement>(this,move(tk));
				}else{
					stmt=create_call(tk->name(),this,move(tk),t);
				}
			}
			statements.push_back(move(stmt));
		}
	}

	inline void build(ostream&os){
		compile(tc,os,0);
		link(tc,os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		os<<"section .text\nglobal _start\n_start:\n  mov ebp,stk\n  mov esp,stkend\n";
		for(auto&s:statements)
			if(!s->is_in_data_section())s->compile(tc,os,indent_level);

		const stmt_def_func*main=tc.get_func_or_break(*this,"main");
		tc.framestk().push_func("main");
		indent(os,indent_level,true);os<<"main(){  ["<<token().token_start_char()<<"]"<<endl;

		main->code_block()->compile(tc,os,indent_level);

		indent(os,indent_level,true);os<<"}\n\n";

		os<<"\nsection .data\n";
		for(auto&s:statements)
			if(s->is_in_data_section())s->compile(tc,os,indent_level);

		os<<"mem1 dd 1\n";
		os<<"mem0 dd 0\n";

		os<<"\nsection .bss\nstk resd 256\nstkend:\n";
		tc.framestk().pop_func("main");
	}

	inline void link(toc&tc,ostream&os)const override{for(auto&s:statements)s->link(tc,os);}

	inline void source_to(ostream&os)const override{statement::source_to(os);for(auto&s:statements)s->source_to(os);}

	inline const toc&get_toc()const{return tc;}

	inline bool is_inline()const{return true;}


private:
	vup_statement statements;
	toc tc;
};
using up_program=unique_ptr<stmt_program>;
