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
	program(tokenizer&t):statement{nullptr,make_unique<class token>()}{
		while(!t.is_eos()){
			up_token tk=t.next_token();
			up_statement stmt;
			if(tk->is_name("file")){
				stmt=make_unique<file>(this,move(tk),t);
			}else if(tk->is_name("func")){
				stmt=make_unique<func>(this,move(tk),t);
			}else{
				stmt=create_statement(tk->name(),this,move(tk),t);
			}
			statements.push_back(move(stmt));
		}
	}
	void build(ostream&os){compile(tc,os);link(tc,os);}
	void compile(toc&tc,ostream&os)override{os<<"section .text\nglobal _start\n_start:\n";for(auto&s:statements)s->compile(tc,os);}
	void link(toc&tc,ostream&os)override{for(auto&s:statements)s->link(tc,os);}
	void source_to(ostream&os)override{statement::source_to(os);for(auto&s:statements)s->source_to(os);}
	const toc&get_toc()const{return tc;}
private:
	vup_statement statements;
	toc tc;
};
using up_program=unique_ptr<program>;
