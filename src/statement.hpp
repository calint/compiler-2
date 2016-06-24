#pragma once

#include <algorithm>
#include <iostream>
#include <memory>

#include "token.hpp"

class toc;

class statement{public:
	inline statement(toc&tc,statement*parent,unique_ptr<class token>t)
		:tk{move(t)},pt{parent}
	{
		token_str=tk->name();
	}

	inline virtual~statement(){}

	inline virtual void compile(toc&tc,ostream&os,size_t indent_level)const{tk->compile_to(os);}

	inline virtual void link(toc&tc,ostream&os)const{}

	inline virtual void source_to(ostream&os)const{tk->source_to(os);}

	inline const token&tok()const{return*tk;}

	inline const statement*parent()const{return pt;}

	inline virtual bool is_in_data_section()const{return false;}

	inline virtual bool is_expression()const{return false;}


	inline virtual bool is_func()const{return false;}
	inline virtual bool is_class()const{return false;}
	inline virtual bool is_block()const{return false;}

	inline virtual const char*identifier()const{return"";}



	inline void set_expression_dest_nasm_identifier(const char*destination){dest=destination;}

	inline const char*expression_dest_nasm_identifier()const{return dest;}

	inline virtual void ev(int type,const statement*s){if(pt)pt->ev(type,s);}


	inline static void indent(ostream&os,size_t level,bool comment=false){os<<(comment?"; ":"  ");for(size_t i=0;i<level;i++)os<<" ";}

	inline static unique_ptr<const char[]>copy_to_unique_pointer(const char*str){
		const size_t len=strlen(str)+1;
		char*ca=new char[len];
		memcpy(ca,str,len);
		return unique_ptr<const char[]>(ca);
	}

protected:
	statement*pt;

private:
	const char*dest{nullptr};
	unique_ptr<class token>tk;

	/// for debugger
	const char*token_str;
};

