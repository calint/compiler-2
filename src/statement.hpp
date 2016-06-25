#pragma once

#include <algorithm>
#include <iostream>
#include <memory>

#include "token.hpp"

class toc;

class statement{public:
	inline statement(statement*parent,const token&t):
		tk{t},
		pt{parent}
	{}

	inline virtual~statement(){}

	inline virtual void compile(toc&tc,ostream&os,size_t indent_level)const{tk.compile_to(os);}

	inline virtual void link(toc&tc,ostream&os)const{}

	inline virtual void source_to(ostream&os)const{tk.source_to(os);}

	inline const token&tok()const{return tk;}

	inline const statement*parent()const{return pt;}

	inline virtual bool is_in_data_section()const{return false;}

	inline virtual bool is_expression()const{return false;}

	inline void set_expression_dest_nasm_identifier(const string&destination){dest=destination;}

	inline const string&expression_dest_nasm_identifier()const{return dest;}



	inline static void indent(ostream&os,size_t indent_level,bool comment=false){os<<(comment?"; ":"  ");for(size_t i=0;i<indent_level;i++)os<<"  ";}

private:
	string dest;
	token tk;
	statement*pt;
};
using up_statement=unique_ptr<statement>;
