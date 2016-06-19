#pragma once
#include"token.hpp"
#include"toc.hpp"
class statement{
	unique_ptr<token>tk;
public:
	inline statement(unique_ptr<token>t):tk{move(t)}{}
	inline virtual~statement(){}
	inline virtual void compile(toc&tc){tk->compiled_to_stdout();}
	inline virtual void link(toc&tc){}
	inline virtual void print_source(){tk->source_to_stdout();}
};
