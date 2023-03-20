#pragma once

#include"token.hpp"
#include"unary_ops.hpp"

class toc;

class statement{
public:
	inline statement(token tk,unary_ops uops={}):
		token_{move(tk)},
		uops_{uops}
	{}
	inline virtual~statement()=default;
	
	inline statement()=default;
	inline statement(statement&&)=default;
	inline statement(const statement&)=default;
	inline statement&operator=(const statement&)=default;
	inline statement&operator=(statement&&)=default;

	inline virtual void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const{
		uops_.source_to(os);
		token_.compile_to(os);
	}

	inline virtual void source_to(ostream&os)const{
		uops_.source_to(os);
		token_.source_to(os);
	}

	inline const token&tok()const{return token_;}

	inline virtual bool is_negated()const{return uops_.is_negated();}

	inline virtual bool is_in_data_section()const{return false;}

	inline virtual bool is_expression()const{return false;}

	inline virtual const string&identifier()const{return tok().name();}

	inline virtual const unary_ops&get_unary_ops()const{return uops_;}

private:
	token token_;
	unary_ops uops_;
};
