#pragma once

#include"stmt_assign_var.hpp"

class stmt_def_var final:public statement{
public:
	inline stmt_def_var(token tk,tokenizer&t):
		statement{move(tk)},
		name_{t.next_token()}
	{
		if(t.is_next_char(':')){
			type_=t.next_token();
		}
		if(not t.is_next_char('='))
			throw compiler_error(name_,"expected '=' and initializer");
		initial_value_={name_,type_,t};
	}

	inline stmt_def_var()=default;
	inline stmt_def_var(const stmt_def_var&)=default;
	inline stmt_def_var(stmt_def_var&&)=default;
	inline stmt_def_var&operator=(const stmt_def_var&)=default;
	inline stmt_def_var&operator=(stmt_def_var&&)=default;

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		initial_value_.source_def_to(os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent,const string&dst="")const override{
		tc.source_comment(*this,os,indent);
		const type&tp{tc.get_type(*this,type_.name().empty()?toc::default_type_str:type_.name())}; // ? magic word
		tc.add_var(*this,os,indent,name_.name(),tp,false);
		initial_value_.compile(tc,os,indent,name_.name());
	}

private:
	token name_;
	token type_;
	stmt_assign_var initial_value_;
};
