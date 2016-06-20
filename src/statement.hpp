#pragma once
#include"token.hpp"
#include"toc.hpp"
class statement{
	up_token tk;
	statement*pt;
public:
	inline statement(statement*parent,up_token t):tk{move(t)},pt{parent}{}
	inline virtual~statement(){}
	inline virtual void compile(toc&tc){tk->compiled_to_stdout();}
	inline virtual void link(toc&tc){}
	inline virtual void source_to_stdout(){tk->source_to_stdout();}
	inline const token&token()const{return*tk;}
	inline statement*parent()const{return pt;}
};
using up_statement=unique_ptr<statement>;

