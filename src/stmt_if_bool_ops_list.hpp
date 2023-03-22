#pragma once

#include<variant>

#include"statement.hpp"

class stmt_if_bool_ops_list final:public statement{
public:
	inline stmt_if_bool_ops_list(tokenizer&t,bool enclosed=false,token not_token={},
			bool is_sub_expr=false,
			variant<stmt_if_bool_op,stmt_if_bool_ops_list>first_bool_op={},
			token first_op={}
		):
		statement{t.next_whitespace_token()},
		not_token_{move(not_token)},
		enclosed_{enclosed}
	{
		token prv_op{first_op};
		if(not first_op.is_name("")){
			// sub-expression with first bool op provided
			bools_.push_back(move(first_bool_op));
			ops_.push_back(move(first_op));
		}
		// i.e. not (a=1 and b=1)
		// a=1 and b=1
		while(true){
			token tknot{t.next_token()};
			if(tknot.is_name("not")){
				if(t.is_next_char('(')){
					bools_.emplace_back(stmt_if_bool_ops_list{t,true,move(tknot)});
				}else{
					t.put_back_token(tknot);
					bools_.emplace_back(stmt_if_bool_op{t});
				}
			}else{
				t.put_back_token(tknot);
				if(t.is_next_char('(')){
					bools_.emplace_back(stmt_if_bool_ops_list{t,true});
				}else{
					bools_.emplace_back(stmt_if_bool_op{t});
				}
			}

			if(enclosed_ and t.is_next_char(')'))
				return;

			token tk{t.next_token()};
			if(not(tk.is_name("or") or tk.is_name("and"))){
				t.put_back_token(tk);
				break;
			}

			if(prv_op.is_blank()){
				prv_op=tk;
				if(tk.is_name("and")){
					stmt_if_bool_ops_list bol{t,false,{},true,move(bools_.back()),move(tk)};
					bools_.pop_back();
					bools_.push_back(move(bol));
					if(enclosed_ and t.is_next_char(')'))
						return;
					tk=t.next_token();
					if(not(tk.is_name("or") or tk.is_name("and"))){
						t.put_back_token(tk);
						break;
					}
					prv_op=tk;
					ops_.push_back(move(tk));
					continue;
				}
			}

			if(tk.is_name(prv_op.name())){
				ops_.push_back(move(tk));
				continue;
			}

			prv_op=tk;

			// previous op is not the same as next op
			//   either a new sub-expression or exit current sub-expression
			// a or b  or       c       and  d or e
			//       prv_tk ops.back()  tk
			if(is_sub_expr){
				// a or b and c or d
				//      ------- tk
				t.put_back_token(tk);
				return;
			}
			// a    or    b     and   c or d
			//    prv_op back()  tk
			if(tk.is_name("or")){
				// a and b or c or d
				//         tk
				ops_.push_back(move(tk));
				continue;
			}

			// a    or    (b     and   c) or d
			//    prv_op back()  tk
			stmt_if_bool_ops_list bol{t,false,{},true,move(bools_.back()),move(tk)};
			bools_.pop_back();
			bools_.push_back(move(bol));
			tk=t.next_token();
			if(not(tk.is_name("or") or tk.is_name("and"))){
				t.put_back_token(tk);
				break;
			}
			ops_.push_back(move(tk));
		}
		if(enclosed_)
			throw compiler_error(tok(),"expected ')' to close expression");
	}

	inline stmt_if_bool_ops_list()=default;
	inline stmt_if_bool_ops_list(const stmt_if_bool_ops_list&)=default;
	inline stmt_if_bool_ops_list(stmt_if_bool_ops_list&&)=default;
	inline stmt_if_bool_ops_list&operator=(const stmt_if_bool_ops_list&)=default;
	inline stmt_if_bool_ops_list&operator=(stmt_if_bool_ops_list&&)=default;

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		not_token_.source_to(os);
		if(enclosed_){
			os<<"(";
		}
		const size_t n{bools_.size()};
		for(size_t i{0};i<n;i++){
			if(bools_[i].index()==0){
				get<stmt_if_bool_op>(bools_[i]).source_to(os);
			}else{
				get<stmt_if_bool_ops_list>(bools_[i]).source_to(os);
			}
			if(i<n-1){
				const token&t{ops_[i]};
				t.source_to(os);
			}
		}
		if(enclosed_){
			os<<")";
		}
	}
	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dst="")const override{
		throw compiler_error(tok(),"this code should not be reached: "+string{__FILE__}+":"+to_string(__LINE__));
	}

	inline string cmp_bgn_label(const toc&tc)const{
		const string&call_path{tc.get_inline_call_path(tok())};
		return "cmp_"+tc.source_location(tok())+(call_path.empty()?"":"_"+call_path);
	}

	inline void compile(toc&tc,ostream&os,const size_t indent_level,const string&jmp_to_if_false,const string&jmp_to_if_true,const bool inverted)const{
		const size_t n{bools_.size()};
		if(n>1){
			// avoid repeated comment
			toc::indent(os,indent_level,true);tc.source_comment(os,"?",' ',*this);
		}
		if(inverted){
			toc::indent(os,indent_level,true);os<<"inverted\n";
		}
		// invert according to De Morgan's laws
		bool invert{inverted?not not_token_.is_name("not"):not_token_.is_name("not")};
		for(size_t i=0;i<n;i++){
			if(bools_[i].index()==1){
				//
				// stmt_if_bool_ops_list
				//
				const stmt_if_bool_ops_list&el{get<stmt_if_bool_ops_list>(bools_[i])};
				string jmp_false{jmp_to_if_false};
				string jmp_true{jmp_to_if_true};
				if(i<n-1){
					if(!invert){
						// if not last element check if it is a 'or' or 'and' list
						if(ops_[i].is_name("or")){
							// if evaluation is false and next op is "or" then jump_false is next bool eval
							jmp_false=cmp_label_from_variant(tc,bools_[i+1]);
						}else if(ops_[i].is_name("and")){
							// if evaluation is true and next op is "and" then jump_true is next bool eval
							jmp_true=cmp_label_from_variant(tc,bools_[i+1]);
						}else{
							throw"expected 'or' or 'and'";
						}
						el.compile(tc,os,indent_level,jmp_false,jmp_true,invert);
					}else{
						// invert according to De Morgan's laws
						// if not last element check if it is a 'or' or 'and' list
						if(ops_[i].is_name("and")){
							// if evaluation is false and next op is "or" (inverted) then jump_false is next bool eval
							jmp_false=cmp_label_from_variant(tc,bools_[i+1]);
						}else if(ops_[i].is_name("or")){
							// if evaluation is true and next op is "and" (inverted) then jump_true is next bool eval
							jmp_true=cmp_label_from_variant(tc,bools_[i+1]);
						}else{
							throw"expected 'or' or 'and'";
						}
						el.compile(tc,os,indent_level,jmp_false,jmp_true,invert);
					}
				}else{
					// if last in list jmp_false is next bool eval
					el.compile(tc,os,indent_level,jmp_false,jmp_true,invert);
				}
				continue;
			}

			//
			// stmt_if_bool_op
			//
			if(!invert){
				// a=1 and b=2   vs   a=1 or b=2
				const stmt_if_bool_op&e{get<stmt_if_bool_op>(bools_[i])};
				if(i<n-1){
					if(ops_[i].is_name("or")){
						e.compile_or(tc,os,indent_level,jmp_to_if_true,invert);
					}else if(ops_[i].is_name("and")){
						e.compile_and(tc,os,indent_level,jmp_to_if_false,invert);
					}else{
						throw"expected 'or' or 'and'";
					}
				}else{
					e.compile_and(tc,os,indent_level,jmp_to_if_false,invert);
					// if last element and not yet jumped to false then jump to true
					tc.asm_jmp(*this,os,indent_level,jmp_to_if_true);
				}
			}else{
				// inverted according to De Morgan's laws
				// a=1 and b=2   vs   a=1 or b=2
				const stmt_if_bool_op&e{get<stmt_if_bool_op>(bools_[i])};
				if(i<n-1){
					if(ops_[i].is_name("and")){
						e.compile_or(tc,os,indent_level,jmp_to_if_true,invert);
					}else if(ops_[i].is_name("or")){
						e.compile_and(tc,os,indent_level,jmp_to_if_false,invert);
					}else{
						throw"expected 'or' or 'and'";
					}
				}else{
					e.compile_and(tc,os,indent_level,jmp_to_if_false,invert);
					// if last element and not yet jumped to false then jump to true
					tc.asm_jmp(*this,os,indent_level,jmp_to_if_true);
				}
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
	vector<token>ops_; // 'and' or 'or'
	token not_token_;
	bool enclosed_{}; // (a=b and c=d) vs a=b and c=d
};
