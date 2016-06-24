#pragma once

#include <algorithm>
#include <memory>

#include "stmt_def.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class toc;

class def_var;
class def_field;
class def_func;
class def_class;


class def_block:public stmt_def{public:

	inline def_block(toc&tc,statement*parent,unique_ptr<class token>tkn,tokenizer&t)
		:stmt_def{tc,parent,move(tkn),t}
	{
//		if(!t.is_next_char('{'))
//			is_one_statement=true;
//
//		while(true){
//			if(t.is_eos())throw compiler_error(*this,"unexpected end of string",token().name_copy());
//			if(not is_one_statement and t.is_next_char_block_close())break;
//			unique_ptr<statement>stmt=read_next_statement(tc,this,t);
//			statements.push_back(move(stmt));
//			if(is_one_statement)
//				break;
//		}
	}

//	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
//		tc.framestk().push_block("");
//		for(auto&s:statements)
//			s->compile(tc,os,indent_level+1);
//		tc.framestk().pop_block("");
//	}
//
//	inline void link(toc&tc,ostream&os)const override{for(auto&s:statements)s->link(tc,os);}
//
//	inline void source_to(ostream&os)const override{
//		statement::source_to(os);
//		if(!is_one_statement)os<<"{";
//		for(auto&s:statements)
//			s->source_to(os);
//		if(!is_one_statement)os<<"}";
//	}

//protected:
//	vector<unique_ptr<statement>>statements;
//	vector<def_var*>vars_;
//	vector<def_field*>fields_;
//	vector<def_func*>funcs_;
//	vector<def_class*>classes_;
//
//private:
//	bool is_one_statement{false};

};
