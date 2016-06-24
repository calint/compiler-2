#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

#include"compiler_error.hpp"
#include"decouple.hpp"
#include"statement.hpp"
#include"token.hpp"
#include"tokenizer.hpp"
#include"decouple.hpp"

class stmt_def_var;
class stmt_def_field;
class stmt_def_func;
class stmt_def_class;

class stmt_block:public statement{public:

	inline stmt_block(statement*parent,unique_ptr<class token>tkn,tokenizer&t):statement{parent,move(tkn)}{
		if(!t.is_next_char('{'))
			is_one_statement=true;

		while(true){
			if(t.is_eos())throw compiler_error(*this,"unexpected end of string",token().name_copy());
			if(not is_one_statement and t.is_next_char_block_close())break;
			unique_ptr<statement>stmt=read_next_statement(parent,t);
			statements.push_back(move(stmt));
			if(is_one_statement)
				break;
		}
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level)const override{
		tc.framestk().push_block("");
		for(auto&s:statements)
			s->compile(tc,os,indent_level+1);
		tc.framestk().pop_block("");
	}

	inline void link(toc&tc,ostream&os)const override{for(auto&s:statements)s->link(tc,os);}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if(!is_one_statement)os<<"{";
		for(auto&s:statements)
			s->source_to(os);
		if(!is_one_statement)os<<"}";
	}

	bool is_one_statement{false};
	vector<unique_ptr<statement>>statements;
	vector<stmt_def_var*>vars_;
	vector<stmt_def_field*>fields_;
	vector<stmt_def_func*>funcs_;
	vector<stmt_def_class*>classes_;
private:
};
