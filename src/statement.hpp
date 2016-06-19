#pragma once
#include"token.hpp"
#include"toc.hpp"
class statement{
	utokenp tk;
public:
	inline statement(utokenp t):tk{move(t)}{}
	inline virtual~statement(){}
	inline virtual void compile(toc&tc){tk->compiled_to_stdout();}
	inline virtual void link(toc&tc){}
	inline virtual void print_source(){tk->source_to_stdout();}
	inline const token&get_token()const{return*tk.get();}
};
