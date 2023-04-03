#pragma once

#include "bool_ops_list.hpp"
#include"expr_ops_list.hpp"

class expr_any final:public statement{
public:
	inline expr_any(toc&tc,const type&tp,tokenizer&t,bool in_args):
		statement{t.next_whitespace_token()}
	{
		if(tp.name()==tc.get_type_bool().name()){
			eols_=bool_ops_list{tc,t};
			set_type(tp);
			return;
		}
		expr_ops_list e{tc,t,in_args};
		set_type(e.get_type());
		eols_=move(e);
	}

	inline expr_any()=default;
	inline expr_any(const expr_any&)=default;
	inline expr_any(expr_any&&)=default;
	inline expr_any&operator=(const expr_any&)=default;
	inline expr_any&operator=(expr_any&&)=default;

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if(eols_.index()==0){
			get<expr_ops_list>(eols_).source_to(os);
		}else{
			get<bool_ops_list>(eols_).source_to(os);
		}
	}

	inline void compile(toc&tc,ostream&os,size_t indent,const string&dst="")const override{
		tc.source_comment(*this,os,indent);

		if(eols_.index()==0){
			const expr_ops_list&eol{get<expr_ops_list>(eols_)};
			eol.compile(tc,os,indent,dst);
			return;
		}

		// bool expression
		const bool_ops_list&eol{get<bool_ops_list>(eols_)};
		const string&call_path{tc.get_inline_call_path(tok())};
		const string&src_loc{tc.source_location_for_label(tok())};
		const string&postfix{src_loc+(call_path.empty()?"":("_"+call_path))};
		const string&jmp_to_if_true{"true_"+postfix};
		const string&jmp_to_if_false{"false_"+postfix};
		const string&jmp_to_end{"end_"+postfix};
		eol.compile(tc,os,indent,jmp_to_if_false,jmp_to_if_true,false);
		tc.asm_label(*this,os,indent,jmp_to_if_true);
		const ident_resolved&dst_resolved{tc.resolve_identifier(*this,dst,false)};
		tc.asm_cmd(*this,os,indent,"mov",dst_resolved.id,"1");
		tc.asm_jmp(*this,os,indent,jmp_to_end);
		tc.asm_label(*this,os,indent,jmp_to_if_false);
		tc.asm_cmd(*this,os,indent,"mov",dst_resolved.id,"0");
		tc.asm_label(*this,os,indent,jmp_to_end);
	}

	inline bool is_expression()const override{
		if(eols_.index()==0){
			const expr_ops_list&eol{get<expr_ops_list>(eols_)};
			return eol.is_expression();
		}

		// bool expression
		const bool_ops_list&eol{get<bool_ops_list>(eols_)};
		return eol.is_expression();
	}

	inline const string&identifier()const override{
		if(eols_.index()==0){
			const expr_ops_list&eol{get<expr_ops_list>(eols_)};
			return eol.identifier();
		}

		// bool expression
		const bool_ops_list&eol{get<bool_ops_list>(eols_)};
		return eol.identifier();
	}

	inline const unary_ops&get_unary_ops()const override{
		if(eols_.index()==0){
			const expr_ops_list&eol{get<expr_ops_list>(eols_)};
			return eol.get_unary_ops();
		}

		// bool expression
		const bool_ops_list&eol{get<bool_ops_list>(eols_)};
		return eol.get_unary_ops(); // ? can there be unary ops on bool
	}

private:
	inline static size_t count_instructions(stringstream&ss){
		const regex rxcomment{R"(^\s*;.*$)"};
		string line;
		size_t n{0};
		while(getline(ss,line)){
			if(regex_search(line,rxcomment))
				continue;
			n++;
		}
		return n;
	}

	token type_;
	variant<expr_ops_list,bool_ops_list>eols_;
};