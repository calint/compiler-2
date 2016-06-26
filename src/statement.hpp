#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
using namespace std;

#include "token.hpp"

class toc;

class statement{public:
	inline statement(const statement&parent,const token&tk):
		token_{tk},
		parent_{parent}
	{}

	// statement a=b   a{b}
	inline statement(const statement&other):
		token_{other.token_},
		parent_{other.parent_}
	{}

	inline statement(const statement&&other):
		token_{move(other.token_)},
		parent_{other.parent_}
	{}

//	inline statement&operator=(const statement&other){
//		dest=other.dest;
//		tk=other.tk;
//		pt=other.pt;
//		return*this;
//	}

	inline virtual~statement(){}

	inline virtual void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const{
//		if(not dest_ident.empty()){
//			indent(os,indent_level,false);
//			os<<"mov "<<dest_ident<<",";
//			token_.compile_to(os);
//			os<<endl;
//			return;
//		}
		token_.compile_to(os);
	}

	inline virtual void link(toc&tc,ostream&os)const{}

	inline virtual void source_to(ostream&os)const{token_.source_to(os);}

	inline const token&tok()const{return token_;}

	inline const statement&parent()const{return parent_;}

	inline virtual bool is_in_data_section()const{return false;}

	inline virtual bool is_expression()const{return false;}

	inline virtual bool is_empty()const{return token_.is_blank();}

	inline static void indent(ostream&os,size_t indent_level,bool comment=false){os<<(comment?"; ":"  ");for(size_t i=0;i<indent_level;i++)os<<"  ";}

private:
	token token_;
	const statement&parent_;
};
using up_statement=unique_ptr<statement>;
