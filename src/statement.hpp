#pragma once

#include"token.hpp"

class toc;

class statement{
public:
	inline statement(token tk,const bool negated=false):
		token_{move(tk)},
		negated_{negated}
	{}
	inline virtual~statement()=default;
	inline statement()=default;
	inline statement(statement&&)=default;
	inline statement(const statement&)=default;
	inline statement&operator=(const statement&)=default;
	inline statement&operator=(statement&&)=default;

	inline virtual void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const{
		if(negated_)
			os<<'-';
		token_.compile_to(os);
	}

	inline virtual void source_to(ostream&os)const{
		if(negated_)
			os<<'-';
		token_.source_to(os);
	}

	inline const token&tok()const{return token_;}

	inline virtual bool is_negated()const{return negated_;}

	inline virtual bool is_in_data_section()const{return false;}

	inline virtual bool is_expression()const{return false;}

	inline virtual const string&identifier()const{return tok().name();}

private:
	token token_;
	bool negated_{};
};
