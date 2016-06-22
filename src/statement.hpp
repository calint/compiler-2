#pragma once

#include <algorithm>
#include <iostream>
#include <memory>

#include "token.hpp"

class toc;

class statement{public:
	inline statement(statement*parent,up_token t):tk{move(t)},pt{parent}{}
	inline virtual~statement(){}
	inline virtual void compile(toc&tc,ostream&os,size_t indent_level){tk->compile_to(os);}
	inline virtual void link(toc&tc,ostream&os){}
	inline virtual void source_to(ostream&os)const{tk->source_to(os);}
	inline const token&token()const{return*tk;}
	inline statement*parent()const{return pt;}
	inline virtual bool is_in_data_section()const{return false;}

	inline static void indent(ostream&os,size_t level,bool comment=false){os<<(comment?"; ":"  ");for(size_t i=0;i<level;i++)os<<" ";}



	inline void set_expression_dest_nasm_identifier(const char*identifier){
		ident=identifier;
	}
private:
	up_token tk;
	statement*pt;
	const char*ident{nullptr};
};
using up_statement=unique_ptr<statement>;

