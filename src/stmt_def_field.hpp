#pragma once

class stmt_def_field final:public statement{
public:
	inline stmt_def_field(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk},
		ident_{t.next_token()}
	{
		if(ident_.is_name(""))
			throw compiler_error(t,"expected field name");

		if(!t.is_next_char('='))
			throw compiler_error(t,"expected '=' and initial value");

		token_=make_unique<token>(t.next_token());
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		ident_.source_to(os);
		os<<"=";
		token_->source_to(os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.source_comment(os,*this);
		os<<ident_.name();
		if(token_->is_string()){
			os<<" db '";
			token_->compile_to(os);
			os<<"'\n";
			indent(os,indent_level);
			os<<ident_.name()<<".len equ $-"<<ident_.name()<<"\n";
			tc.add_field(*this,ident_.name(),this,true);
			return;
		}
		os<<" dd ";
		token_->compile_to(os);
		os<<endl;
		tc.add_field(*this,ident_.name(),this,false);
	}

	inline bool is_in_data_section()const override{return true;}

	inline bool is_string_field()const{return token_->is_string();}

private:
	token ident_;
	unique_ptr<token>token_;
};
