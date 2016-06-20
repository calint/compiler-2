#pragma once

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>

#include "call.hpp"
#include "decouple.hpp"
#include "file.hpp"
#include "func.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class program final:public statement{public:
	program(tokenizer&t):statement{nullptr,nullptr}{
		while(true){
			if(t.is_eos())break;
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
//		compile(tc);
//		link(tc);
	}
	void build(ostream&os){
		compile(tc,os);
		link(tc,os);
	}
	void compile(toc&tc,ostream&os)override{
		os<<"section .text\nglobal _start\n_start:\n";
		for(auto&s:statements)s->compile(tc,os);
	}
	void link(toc&tc,ostream&os)override{for(auto&s:statements)s->link(tc,os);}
	void source_to(ostream&os)override{for(auto&s:statements)s->source_to(os);}
	const toc&get_toc()const{return tc;}
private:
	vup_statement statements;
	toc tc;
};
using up_program=unique_ptr<program>;
