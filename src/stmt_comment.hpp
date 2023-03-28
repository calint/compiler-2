#pragma once

class stmt_comment final:public statement{
public:
	inline stmt_comment(toc&tc,token tk,tokenizer&t):
		statement{move(tk)},
		line_{t.read_rest_of_line()}
	{
		set_type(tc.get_type(*this,toc::void_type_str));
	}

	inline stmt_comment()=default;
	inline stmt_comment(const stmt_comment&)=default;
	inline stmt_comment(stmt_comment&&)=default;
	inline stmt_comment&operator=(const stmt_comment&)=default;
	inline stmt_comment&operator=(stmt_comment&&)=default;

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		os<<line_<<endl;
	}

	inline void compile(toc&tc,ostream&os,size_t indent,const string&dst="")const override{
		tc.source_comment(*this,os,indent);
	}

private:
	string line_;
};
