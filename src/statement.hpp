#pragma once
#include<memory>
using namespace std;
#include<string.h>
#include<cassert>
#include"token.hpp"
class statement{
	unique_ptr<token>tk;
public:
	inline statement(unique_ptr<token>t):tk{move(t)}{}
	inline virtual~statement(){}
	inline virtual void to_asm(){
		tk->to_asm();
	}
	inline virtual void source_to_stdout(){
		tk->to_asm();
	}
};
