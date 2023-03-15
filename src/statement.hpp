#pragma once

#include"token.hpp"

class toc;

class statement{
public:
	inline statement(token tk):token_{move(tk)}{}

	inline statement(const statement&other):token_{other.token_}{}

	inline virtual~statement(){}

	inline virtual void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const{
		token_.compile_to(os);
	}

	inline virtual void source_to(ostream&os)const{token_.source_to(os);}

	inline const token&tok()const{return token_;}

	inline virtual bool is_in_data_section()const{return false;}

	inline virtual bool is_expression()const{return false;}

	inline virtual const string&identifier()const{return tok().name();}

	inline virtual bool is_empty()const{return token_.is_blank();}


private:
	const token token_;
};
