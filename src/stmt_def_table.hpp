#pragma once

class stmt_def_table final:public statement{
public:
	inline stmt_def_table(const statement&parent,const token&tk,tokenizer&t):
		statement{parent,tk},
		ident_{t.next_token()}
	{
		if(ident_.is_name(""))
			throw compiler_error(ident_,"expected table name");

		if(!t.is_next_char('(')){
			noargs_=true;
//			throw compiler_error(*this,"expected '(' followed by function arguments",identifier->name_copy());
		}
		if(!noargs_){
			while(true){
				if(t.is_next_char(')'))break;
				token ta=t.next_token();
				if(t.is_next_char(')')){
					params_.push_back(ta);
					break;
				}
				if(!t.is_next_char(','))
					throw compiler_error(ta,"expected ',' after parameter at ",tk.name());
				params_.push_back(ta);
			}
		}
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		ident_.source_to(os);
		if(!noargs_){
			os<<"(";
			const size_t nparam=params_.size()-1;
			size_t ii{0};
			for(auto&s:params_){
				s.source_to(os);
				if(ii++!=nparam)os<<",";
			}
			os<<")";
		}
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dest_ident="")const override{
		indent(os,indent_level,true);tc.source_to_as_comment(os,*this);
		os<<ident_.name()<<":\n";
		os<<ident_.name()<<".end:\n";
		os<<ident_.name()<<".len equ $-"<<ident_.name()<<"\n\n";
		tc.add_table(*this,ident_.name(),this);//? in constructor for forward ref
	}

	inline bool is_in_data_section()const override{return true;}

private:
	token ident_;
	bool noargs_{false};
	vector<token>params_;
};
