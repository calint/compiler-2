#pragma once

#include"token.hpp"
#include"unary_ops.hpp"

class toc;
class type;

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

	inline virtual void compile(toc&tc,ostream&os,size_t indent,const string&dst="")const{
		uops_.source_to(os);
		token_.compile_to(os);
	}

	inline virtual void source_to(ostream&os)const{
		uops_.source_to(os);
		token_.source_to(os);
	}

	inline const token&tok()const{return token_;}

	inline virtual bool is_in_data_section()const{return false;}

	inline virtual bool is_expression()const{return false;}

	inline virtual const string&identifier()const{return token_.name();}

	inline virtual const unary_ops&get_unary_ops()const{return uops_;}

//	inline virtual const string&get_type_str()const{return type_str_;}

	inline virtual const type&get_type(toc&tc)const;

private:
	token token_;
	unary_ops uops_;
//	string type_str_;
};
