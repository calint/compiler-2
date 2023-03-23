#pragma once

class stmt_def_field final:public statement{
public:
	inline stmt_def_field(token tk,tokenizer&t):
		statement{move(tk)},
		name_{t.next_token()}
	{
		if(name_.is_name(""))
			throw compiler_error(t,"expected field name");

		if(not t.is_next_char('='))
			throw compiler_error(t,"expected '=' and initial value");

		uops_={t};
		initial_value_=t.next_token();
	}

	inline stmt_def_field()=default;
	inline stmt_def_field(const stmt_def_field&)=default;
	inline stmt_def_field(stmt_def_field&&)=default;
	inline stmt_def_field&operator=(const stmt_def_field&)=default;
	inline stmt_def_field&operator=(stmt_def_field&&)=default;

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		name_.source_to(os);
		os<<"=";
		uops_.source_to(os);
		initial_value_.source_to(os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dst="")const override{
		tc.source_comment(*this,os,indent_level);
		os<<name_.name();
		if(initial_value_.is_string()){
			os<<" db '";
			initial_value_.compile_to(os);
			os<<"'\n";
			toc::indent(os,indent_level);
			os<<name_.name()<<".len equ $-"<<name_.name()<<"\n";
			tc.add_field(*this,name_.name(),this,true);
			return;
		}
		os<<" dq ";
		os<<uops_.get_ops_as_string();
		initial_value_.compile_to(os);
		os<<endl;
		tc.add_field(*this,name_.name(),this,false);
	}

	inline bool is_in_data_section()const override{return true;}

	inline bool is_string_field()const{return initial_value_.is_string();}

private:
	token name_;
	unary_ops uops_;
	token initial_value_;
};
