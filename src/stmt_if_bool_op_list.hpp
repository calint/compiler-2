#pragma once

#include <vector>

#include "statement.hpp"

class stmt_if_bool_op_list:public stmt_if_bool_op{public:
	inline stmt_if_bool_op_list(const statement&parent,tokenizer&t,bool enclosed=false):
		stmt_if_bool_op(parent,t,true),
		enclosed_{enclosed}
	{
		while(true){
			if(t.is_next_char('(')){
				unique_ptr<stmt_if_bool_op>bols=make_unique<stmt_if_bool_op_list>(*this,t,true);
				bool_ops_.push_back(move(bols));
			}else{
				unique_ptr<stmt_if_bool_op>bool_op=make_unique<stmt_if_bool_op>(*this,t,false);
				bool_ops_.push_back(move(bool_op));
			}
			if(t.is_next_char(')')){
				return;
			}

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
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if(enclosed_){
			os<<"(";
		}
		const size_t n=bool_ops_.size();
		for(size_t i=0;i<n;i++){
			const stmt_if_bool_op&bo=*bool_ops_[i];
			bo.source_to(os);
			if(i<(n-1)){
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
	inline void compile(toc&tc,ostream&os,const size_t indent_level,const string&jmp_to_if_false_label,const string&jmp_to_if_true_label,const bool is_last)const{
		const size_t n=bool_ops_.size();
		for(size_t i=0;i<n;i++){
			stmt_if_bool_op*e=bool_ops_[i].get();
			if(e->is_list()){
				stmt_if_bool_op_list*el=dynamic_cast<stmt_if_bool_op_list*>(e);
				string jmp_false=jmp_to_if_false_label;
				string jmp_true=jmp_to_if_true_label;
				if(i<n-1){
					if(ops_[i].is_name("or")){
						// if evaluation is false and next op is "or" then jump_false is next bool eval
						jmp_false=bool_ops_[i+1]->cmp_bgn_label_source_location(tc);
					}else if(ops_[i].is_name("and")){
						// if evaluation is true and next op is "and" then jump_true is next bool eval
						jmp_true=bool_ops_[i+1]->cmp_bgn_label_source_location(tc);
					}else{
						throw "expected 'or' or 'and'";
					}
					el->compile(tc,os,indent_level,jmp_false,jmp_true,false);
				}else{
					// if last in list jmp_false is next bool eval
					el->compile(tc,os,indent_level,jmp_false,jmp_true,true);
				}
				continue;
			}
			// a=1 and b=2   vs   a=1 or b=2
			if(i<n-1){
				if(ops_[i].is_name("or")){
					e->compile_or(tc,os,indent_level,false,jmp_to_if_false_label,jmp_to_if_true_label);
				}else if(ops_[i].is_name("and")){
					e->compile_and(tc,os,indent_level,false,jmp_to_if_false_label,jmp_to_if_true_label);
				}else{
					throw "expected 'or' or 'and'";
				}
			}else{
				// if last eval in list
				e->compile_or(tc,os,indent_level,true,jmp_to_if_false_label,jmp_to_if_true_label);
				if(enclosed_&&!is_last){
//					indent(os,indent_level,false);
//					os<<"if NextInstruction != "<<jmp_to_if_true_label<<"\n";
//					indent(os,indent_level,false);
//					os<<"    jmp "<<jmp_to_if_true_label<<"\n";
//					indent(os,indent_level,false);
//					os<<"endif\n";

					indent(os,indent_level,false);
					os<<"jmp "<<jmp_to_if_true_label<<"\n";
				}
			}
		}
	}
	inline void source_to_as_comment(toc&tc,ostream&os)const{
		stringstream ss;
		statement::source_to(ss);
		const size_t n=bool_ops_.size();
		for(size_t i=0;i<n;i++){
			const stmt_if_bool_op&bo=*bool_ops_[i];
			bo.source_to(ss);
			if(i<(n-1)){
				const token&t=ops_[i];
				t.source_to(ss);
			}
		}

		ss<<"      ";
		tc.source_location_to_stream(ss,this->tok());
		string s=ss.str();
		string res=regex_replace(s,regex("\\s+")," ");
		os<<res;
		os<<endl;
	}

private:
	vector<unique_ptr<stmt_if_bool_op>>bool_ops_;
	vector<token>ops_;
	bool enclosed_;  // (a=b and c=d)  a=b and c=d
};
