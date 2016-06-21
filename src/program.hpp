#pragma once
#include<memory>
using namespace std;
#include "call.hpp"
#include "decouple.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include"func.hpp"
#include"file.hpp"
class program final:public statement{public:
	inline program(tokenizer&t):statement{nullptr,make_unique<class token>()}{
		tc.put_func("mov",nullptr);
		tc.put_func("int",nullptr);
		tc.put_func("xor",nullptr);
		tc.put_func("syscall",nullptr);
		while(!t.is_eos()){
			up_token tk=t.next_token();
			up_statement stmt;
			if(tk->is_name("file")){stmt=make_unique<file>(this,move(tk),t);}
			else if(tk->is_name("func")){stmt=make_unique<func>(this,move(tk),t);}
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
	inline void build(ostream&os){compile(tc,os,0);link(tc,os);}
	inline void compile(toc&tc,ostream&os,size_t indent_level)override{
		os<<"section .text\nglobal _start:\n";
		for(auto&s:statements)
			if(!s->is_in_data_section())s->compile(tc,os,indent_level);
		os<<"\nsection .data\n";
		for(auto&s:statements)
			if(s->is_in_data_section())s->compile(tc,os,indent_level);
	}
	inline void link(toc&tc,ostream&os)override{for(auto&s:statements)s->link(tc,os);}
	inline void source_to(ostream&os)const override{statement::source_to(os);for(auto&s:statements)s->source_to(os);}
	inline const toc&get_toc()const{return tc;}
private:
	vup_statement statements;
	toc tc;
};
using up_program=unique_ptr<program>;
