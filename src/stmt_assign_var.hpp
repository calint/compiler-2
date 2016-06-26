#pragma once

#include <stddef.h>
#include <algorithm>
#include <iostream>
#include <memory>

#include "decouple.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"
#include"stmt_call.hpp"
#include"expr_ops_list.hpp"

class stmt_assign_var final:public statement{public:

	inline stmt_assign_var(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk},
		ex_{expr_ops_list{*this,t,'='}}
	{}


	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		ex_.source_to(os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);
		os<<tok().name()<<"="<<ex_.tok().name()<<"    ";
		tc.source_location_to_stream(os,tok());
		os<<endl;

		const string&reg=tc.alloc_scratch_register(tok()); //? overrallocates in simplests cases ie a=1

		ex_.compile(tc,os,indent_level,reg);

		const string&resolv=tc.resolve_ident_to_nasm(*this,tok().name());

		expr_ops_list::_asm("mov",*this,tc,os,indent_level,resolv,reg);

		tc.free_scratch_reg(reg);
	}

private:
	expr_ops_list ex_;
};
