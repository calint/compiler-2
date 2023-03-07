#pragma once

class stmt_def_field final:public statement{
public:
	inline stmt_def_field(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk},
		ident_{t.next_token()}
	{
		if(ident_.is_name(""))
			throw compiler_error(ident_,"expected field name");

		if(!t.is_next_char('{'))
			throw compiler_error(ident_,"expected '{' initial value   then '}'");

		while(true){
			if(t.is_next_char('}'))break;
			tokens_.push_back(t.next_token());
		}
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		ident_.source_to(os);
		os<<"{";
		for(const auto&s:tokens_)
			s.source_to(os);
		os<<"}";
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.source_comment(os,*this);
		os<<ident_.name()<<" db '";
		for(const auto&s:tokens_)
			s.compile_to(os);
		os<<"'\n";

		for(size_t i=0;i<indent_level;i++)cout<<"  ";
		os<<ident_.name()<<".len equ $-"<<ident_.name()<<"\n\n";

		tc.add_field(*this,ident_.name(),this);
	}

	inline bool is_in_data_section()const override{return true;}

private:
	token ident_;
	vector<token>tokens_;
};
