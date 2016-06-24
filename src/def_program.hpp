#pragma once
#include<memory>
using namespace std;
#include "stmt_call.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include"def_func.hpp"
#include"def_field.hpp"
#include"def_class.hpp"
class def_program final:public statement{public:

	inline def_program(tokenizer&t)
		:statement{tc,nullptr,make_unique<class token>()}
	{
		while(!t.is_eos()){
			statements_.push_back(read_next_statement(tc,this,t));
		}
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		for(auto&s:statements_)
			s->source_to(os);
	}


	inline void build(ostream&os){
		compile(tc,os,0);
//		link(tc,os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		framestack&fs=tc.framestk();

		fs.push_class("program");
		frame&f=fs.current_frame();

		vector<const char*>assem{"mov","int","xor","syscall","cmp","je","tag","jmp","jne","if","cmove","cmovne","or","and"};
		for(auto s:assem)f.add_func(*this,s,nullptr);

		os<<"\nsection .data\n";
		for(auto&s:statements_)
			if(s->is_in_data_section())
				s->compile(tc,os,indent_level);

		os<<"mem1 dd 1\n";
		os<<"mem0 dd 0\n";

		os<<"\nsection .bss\nstk resd 256\nstk.end:\n";

//		os<<"\nsection .text\nglobal _start\n_start:\n  mov ebp,stk\n  mov esp,stk.end\n";
//		for(auto&s:statements)
//			if(!s->is_in_data_section())
//				s->compile(tc,os,indent_level);
//
//		const def_class*cls=f.get_class_or_break(*this,"baz");
//		const def_func*main=cls->get_func_or_break(*this,"main");
//
//		fs.push_func("baz.main");
//
//		indent(os,indent_level,true);
//		os<<"baz.main{  ["<<token().token_start_char()<<"]"<<endl;
//
//		main->code->compile(tc,os,indent_level);
//
//		indent(os,indent_level,true);os<<"}\n\n";
//
//		fs.pop_func("baz.main");

		fs.pop_class("program");
	}

//	inline void link(toc&tc,ostream&os)const override{
//		for(auto&s:statements)
//			s->link(tc,os);
//	}


	inline const toc&get_toc()const{return tc;}

//	inline bool is_inline()const{return true;}


private:
	vector<unique_ptr<statement>>statements_;
	toc tc;
};
