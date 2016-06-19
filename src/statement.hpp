#pragma once
#include"token.hpp"
#include"toc.hpp"
class statement{
	utokenp tk;
	statement*pt;
public:
	inline statement(statement*parent,utokenp t):tk{move(t)},pt{parent}{}
	inline virtual~statement(){}
	inline virtual void compile(toc&tc){tk->compiled_to_stdout();}
	inline virtual void link(toc&tc){}
	inline virtual void source_to_stdout(){tk->source_to_stdout();}
	inline const token&token()const{return*tk;}
	inline statement*parent()const{return pt;}
};
using ustatementp=unique_ptr<statement>;
