#pragma once

#include<variant>

#include"statement.hpp"

class stmt_if_bool_ops_list final:public statement{
public:
	inline stmt_if_bool_ops_list(const statement&parent,tokenizer&t,bool enclosed=false):
		statement(parent,t.next_whitespace_token()),
		enclosed_{enclosed}
	{
		while(true){
			if(t.is_next_char('(')){
				bools_.push_back(stmt_if_bool_ops_list{*this,t,true});
			}else{
				bools_.push_back(stmt_if_bool_op{*this,t});
			}
			if(t.is_next_char(')'))
				return;
			token tk=t.next_token();
			if(tk.is_name("or")){
				ops_.push_back(tk);
			}else if(tk.is_name("and")){
				ops_.push_back(tk);
			}else{
				t.pushback_token(tk);
				break;
			}
		}
		if(enclosed_){
			throw compfind_first_not_ofiler_error(tok(),"expected ')' to close expression");
		}
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if(enclosed_){
			os<<"(";
		}
		const size_t n=bools_.size();
		for(size_t i=0;i<n;i++){
			if(bools_[i].index()==0){
				get<stmt_if_bool_op>(bools_[i]).source_to(os);
			}else{
				get<stmt_if_bool_ops_list>(bools_[i]).source_to(os);
			}
			if(i<n-1){
				const token&t=ops_[i];
				t.source_to(os);
			}
		}
		if(enclosed_){
			os<<")";
		}
	}
	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dst="")const override{
		throw compiler_error(tok(),"this code should not be reached");
	}

	inline string cmp_bgn_label(const toc&tc)const{
		return "cmp_"+tc.source_location(tok());
	}

	inline void compile(toc&tc,ostream&os,const size_t indent_level,const string&jmp_to_if_false,const string&jmp_to_if_true)const{
		indent(os,indent_level,true);tc.source_comment(os,*this);

		const size_t n=bools_.size();
		for(size_t i=0;i<n;i++){
			if(bools_[i].index()==1){
				const stmt_if_bool_ops_list&el=get<stmt_if_bool_ops_list>(bools_[i]);
				string jmp_false=jmp_to_if_false;
				string jmp_true=jmp_to_if_true;
				if(i<n-1){
					// if not last element check if it is a 'or' or 'and' list
					if(ops_[i].is_name("or")){
						// if evaluation is false and next op is "or" then jump_false is next bool eval
						jmp_false=cmp_label_from_variant(tc,bools_[i+1]);
					}else if(ops_[i].is_name("and")){
						// if evaluation is true and next op is "and" then jump_true is next bool eval
						jmp_true=cmp_label_from_variant(tc,bools_[i+1]);
					}else{
						throw "expected 'or' or 'and'";
					}
					el.compile(tc,os,indent_level,jmp_false,jmp_true);
				}else{
					// if last in list jmp_false is next bool eval
					el.compile(tc,os,indent_level,jmp_false,jmp_true);
				}
				continue;
			}
			// a=1 and b=2   vs   a=1 or b=2
			const stmt_if_bool_op&e=get<stmt_if_bool_op>(bools_[i]);
			if(i<n-1){
				if(ops_[i].is_name("or")){
					e.compile_or(tc,os,indent_level,false,jmp_to_if_false,jmp_to_if_true);
				}else if(ops_[i].is_name("and")){
					e.compile_and(tc,os,indent_level,false,jmp_to_if_false,jmp_to_if_true);
				}else{
					throw "expected 'or' or 'and'";
				}
			}else{
				e.compile_or(tc,os,indent_level,true,jmp_to_if_false,jmp_to_if_true);
				// compile_or last_elem generates a jump in case of bool op is false
				indent(os,indent_level);os<<"jmp "<<jmp_to_if_true<<"\n";
			}
		}
	}

private:
	inline static string cmp_label_from_variant(const toc&tc,const variant<stmt_if_bool_op,stmt_if_bool_ops_list>&v){
		if(v.index()==1){
			return get<stmt_if_bool_ops_list>(v).cmp_bgn_label(tc);
		}
		return get<stmt_if_bool_op>(v).cmp_bgn_label(tc);
	}

	vector<variant<stmt_if_bool_op,stmt_if_bool_ops_list>>bools_;
	vector<token>ops_;
	bool enclosed_;  // (a=b and c=d) vs a=b and c=d
};
