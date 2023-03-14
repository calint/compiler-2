#pragma once

#include"token.hpp"

class toc;

class statement{
public:
	inline statement(const statement&parent,const token&tk):parent_{parent},token_{tk}{}

	inline statement(const statement&other):parent_{other.parent_},token_{other.token_}{}

	inline virtual~statement(){}

	inline virtual void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const{
		token_.compile_to(os);
	}

	inline virtual void source_to(ostream&os)const{token_.source_to(os);}

	inline const token&tok()const{return token_;}

	inline const statement&parent()const{return parent_;}

	inline virtual bool is_in_data_section()const{return false;}

	inline virtual bool is_expression()const{return false;}

	inline virtual const string&identifier()const{return tok().name();}

	inline virtual bool is_empty()const{return token_.is_blank();}


private:
	const statement&parent_;
	const token token_;
};
