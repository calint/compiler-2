#pragma once

class stmt_def_type final:public statement{
public:
	inline stmt_def_type(token tk,tokenizer&t):
		statement{move(tk)},
		name_{t.next_token()}
	{
		if(!t.is_next_char('{'))
			throw compiler_error(name_,"expected '{'");
		
		size_token_=t.next_token();

		char*ep;
		size_=size_t(strtol(size_token_.name().c_str(),&ep,10)); // return ignored
		if(*ep)
			throw compiler_error(size_token_,"expected size in bytes");

		if(!t.is_next_char('}'))
			throw compiler_error(name_,"expected '}'");
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		name_.source_to(os);
		os<<'{';
		size_token_.source_to(os);
		os<<'}';
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		tc.add_type(*this,name_.name(),this);
	}

private:
	token name_;
	token size_token_;
	size_t size_{0};
};
