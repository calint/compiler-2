#pragma once

class stmt_def_type_field final:public statement{
public:
	inline stmt_def_type_field(toc&tc,token tk,tokenizer&t):
		statement{move(tk)}
	{
		set_type(&tc.get_type(*this,toc::void_type_str));
		if(not t.is_next_char(':')){
			return;
		}
		type_=t.next_token();
	}

	inline stmt_def_type_field()=default;
	inline stmt_def_type_field(const stmt_def_type_field&)=default;
	inline stmt_def_type_field(stmt_def_type_field&&)=default;
	inline stmt_def_type_field&operator=(const stmt_def_type_field&)=default;
	inline stmt_def_type_field&operator=(stmt_def_type_field&&)=default;

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if(type_.is_empty())
			return;
		os<<':';
		type_.source_to(os);
	}

	inline const string&name()const{return tok().name();}

	inline const string&type_str()const{return type_.name();}

private:
	token type_;
};

class stmt_def_type final:public statement{
public:
	inline stmt_def_type(toc&tc,token tk,tokenizer&t):
		statement{move(tk)},
		name_{t.next_token()}
	{
		if(not t.is_next_char('{'))
			throw compiler_error(name_,"expected '{'");

		while(true){
			token tknm{t.next_token()};
			if(tknm.is_name("")){
				ws_=tknm;
				if(not t.is_next_char('}'))
					throw compiler_error(t,"expected '}'");
				break;
			}
			fields_.emplace_back(stmt_def_type_field{tc,move(tknm),t});
			if(t.is_next_char('}'))
				break;
			if(not t.is_next_char(','))
				throw compiler_error(t,"expected ',' and more fields");
		}

		type_.set_name(name_.name());
		if(fields_.empty()){
			type_.set_size(toc::default_type_size);
		}else{
			for(const stmt_def_type_field&fld:fields_){
				const type&tp{tc.get_type(fld,fld.type_str().empty()?toc::default_type_str:fld.type_str())};
				type_.add_field(fld.tok(),fld.name(),tp);
			}
		}
		tc.add_type(*this,type_);
	}

	inline stmt_def_type()=default;
	inline stmt_def_type(const stmt_def_type&)=default;
	inline stmt_def_type(stmt_def_type&&)=default;
	inline stmt_def_type&operator=(const stmt_def_type&)=default;
	inline stmt_def_type&operator=(stmt_def_type&&)=default;

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		name_.source_to(os);
		os<<'{';
		ws_.source_to(os);
		size_t i{0};
		const size_t n{fields_.size()};
		for(const stmt_def_type_field&tf:fields_){
			tf.source_to(os);
			if(i<n-1){
				os<<',';
			}
			i++;
		}
		os<<'}';
	}

	inline void compile(toc&tc,ostream&os,size_t indent,const string&dst="")const override{}

private:
	token name_;
	token ws_;
	vector<stmt_def_type_field>fields_;
	size_t size_{};
	mutable type type_;
};
