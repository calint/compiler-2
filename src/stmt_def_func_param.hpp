#pragma once

class stmt_def_func_param final:public statement{
public:
	inline stmt_def_func_param(tokenizer&t):
		statement{t.next_token()}
	{
		assert(!tok().is_name(""));

		if(!t.is_next_char(':'))
			return;

		while(true){
			keywords_.emplace_back(t.next_token());
			if(t.is_next_char(':'))
				continue;
			break;
		}
	}

	inline stmt_def_func_param()=default;
	inline stmt_def_func_param(const stmt_def_func_param&)=default;
	inline stmt_def_func_param(stmt_def_func_param&&)=default;
	inline stmt_def_func_param&operator=(const stmt_def_func_param&)=default;
	inline stmt_def_func_param&operator=(stmt_def_func_param&&)=default;

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if(!keywords_.empty()){
			os<<":";
			const size_t n=keywords_.size()-1;
			size_t i{0};
			for(const token&t:keywords_){
				t.source_to(os);
				if(i++!=n)
					os<<":";
			}
		}
	}

	inline const vector<token>&keywords()const{return keywords_;}

	inline const string&name()const{return tok().name();}

	inline const string get_register_or_empty()const{
		for(const token&kw:keywords()){
			if(kw.name().find("reg_"))
				continue;
			// requested register for this argument
			return kw.name().substr(4,kw.name().size());
		}
		return"";
	}

private:
	vector<token>keywords_;
};
