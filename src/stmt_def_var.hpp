#pragma once

#include"stmt_assign_var.hpp"

class stmt_def_var final:public statement{
public:
	inline stmt_def_var(token tk,tokenizer&t):
		statement{move(tk)},
		name_{t.next_token()},
		op_{t.next_char()},
		initial_value_{name_,t}
	{
		if(op_!='=')
			throw compiler_error(name_,"expected '=' and initializer");
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		initial_value_.source_to(os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		tc.source_comment(*this,os,indent_level);
		tc.add_var(*this,os,indent_level,name_.name(),8);
		initial_value_.compile(tc,os,indent_level,name_.name());
	}

private:
	token name_;
	char op_{};
	stmt_assign_var initial_value_;
};
