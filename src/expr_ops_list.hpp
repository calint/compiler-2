#pragma once

#include"compiler_error.hpp"
#include"toc.hpp"
#include"expression.hpp"
#include"decouple.hpp"

class expr_ops_list final:public expression{
public:
	inline expr_ops_list(tokenizer&t,const bool in_args=false,const bool enclosed=false,const char list_op='=',const int first_op_precedence=3,unique_ptr<statement>first_expression=unique_ptr<statement>()):
		expression{t.next_whitespace_token()},
		enclosed_{enclosed},
		in_args_{in_args},
		list_op_{list_op}
	{
		int precedence=first_op_precedence;
		// read first expression
		// if called in a recursion with a first expression passed
		if(first_expression){
			// put in list
			exps_.push_back(move(first_expression));
		}else{
			// check for negated expression list. i.e. -(a+b)
			if(t.is_next_char('-')){
				token tk{t.next_token()};
				if(tk.is_name("")){
					if(t.is_next_char('(')){
						negated_=true;
						after_negation_ws_token_=tk;
						exps_.emplace_back(make_unique<expr_ops_list>(t,in_args,true));
					}else{
						throw compiler_error(t,"expected identifier or '(' after '-'");
					}
				}else{
					t.pushback_token(move(tk));
					t.pushback_char('-');
					exps_.emplace_back(create_statement_from_tokenizer(t));
				}
			}else{
				// if subexpression
				if(t.is_next_char('(')){
					// recurse
					exps_.emplace_back(make_unique<expr_ops_list>(t,in_args,true));
				}else{
					// add first expression
					exps_.emplace_back(create_statement_from_tokenizer(t));
				}
			}
		}

		while(true){ // +a  +3
			// if parsed in a function call argument list
			if(in_args){ // ? rewrite is_in_bool_expr
				// if in boolean expression exit when an operation is found
				if(t.is_peek_char('<'))break;
				if(t.is_peek_char('='))break;
				if(t.is_peek_char('>'))break;
			}
			// if in a function argument return when ',' or ')' found
			if(in_args and (t.is_peek_char(',') or t.is_peek_char(')')))
				break;

			// if end of subexpression
			if(t.is_peek_char(')')){
				if(enclosed_){
					t.next_char();
				}
				break;
			}

			// next operation
			if(t.is_peek_char('+')){
				ops_.push_back('+');
			}else if(t.is_peek_char('-')){
				ops_.push_back('-');
			}else if(t.is_peek_char('*')){
				ops_.push_back('*');
			}else if(t.is_peek_char('/')){
				ops_.push_back('/');
			}else if(t.is_peek_char('%')){
				ops_.push_back('%');
			}else{
				// no more operations
				break;
			}

			// check if next operation precedence is same or lower
			// if not then a new subexpression is added to the list with the last
			// expression in this list as first expression
			const int next_precedence{precedence_for_op(t.peek_char())};
			if(next_precedence>precedence){
				// i.e. =a+b*c+1 where the peeked char is '*'
				// next operation has higher precedence than current
				// list is now =[(=a)(+b)]
				// move last expression (+b) to subexpression
				//   =[(=a) +[(=b)(*c)(+1)]]
				precedence=next_precedence;
				if(!ops_.empty()){
					const int first_op_prec{precedence_for_op(ops_.back())};
					ops_.pop_back();
					const char lst_op{ops_.back()};
					unique_ptr<statement>prev{move(exps_.back())};
					exps_.pop_back();
					exps_.emplace_back(make_unique<expr_ops_list>(t,in_args,false,lst_op,first_op_prec,move(prev)));
					continue;
				}
			}else{
				precedence=next_precedence;
				t.next_char();// read the peeked operator
			}

			if(t.is_next_char('(')){
				exps_.emplace_back(make_unique<expr_ops_list>(t,in_args,true));
				continue;
			}

			unique_ptr<statement>stm{create_statement_from_tokenizer(t)};
			if(stm->tok().is_blank())
				throw compiler_error(*stm,"unexpected '"+string{t.peek_char()}+"'");

			exps_.push_back(move(stm));
		}
	}

	inline void source_to(ostream&os)const override{
		expression::source_to(os);//?
		if(negated_){
			os<<'-';
			after_negation_ws_token_.source_to(os);
		}

		if(enclosed_)
			os<<"(";

		if(not exps_.empty()){
			exps_[0]->source_to(os);
			const size_t len{exps_.size()};
			for(size_t i{1};i<len;i++){
				os<<ops_[i-1];
				exps_[i]->source_to(os);
			}
		}

		if(enclosed_)
			os<<")";
	}

	inline void compile(toc&tc,ostream&os,const size_t indent_level,const string&dest)const override{
		tc.source_comment(*this,os,indent_level);

		if(exps_.empty()) // ? can this happen?
			throw compiler_error(*this,"expressions is empty");

		// first element is assigned to destination
		const statement&st{*exps_[0].get()};
		const ident_resolved&ir{tc.resolve_ident_to_nasm(*this,dest,false)};
		asm_op(tc,os,indent_level,'=',dest,ir.id,st);

		// remaining elements are +,-,*,/
		const size_t n{ops_.size()};
		for(size_t i{0};i<n;i++){
			const char op=ops_[i];
			const statement&stm{*exps_[i+1].get()};
			asm_op(tc,os,indent_level,op,dest,ir.id,stm);
		}
		if(negated_){
			tc.asm_neg(*this,os,indent_level,ir.id);
		}
	}

	inline bool is_expression()const override{
		if(exps_.size()==1){
			return exps_[0]->is_expression();
		}
		return true;
	}

	inline const string&identifier()const override{
		if(exps_.size()==1)
			return exps_[0]->identifier();
		throw"unexpected code path "+string{__FILE__}+":"+to_string(__LINE__);
	}

	inline bool is_negated()const override{
		if(exps_.size()==1){
			return exps_[0]->is_negated();
		}
		return negated_;
	}

	inline bool is_empty()const override{return exps_.empty();} // ? can be removed?

private:
	inline static int precedence_for_op(const char ch){
		if(ch=='+'||ch=='-')
			return 1;
		if(ch=='*'||ch=='/')
			return 2;
		throw string(to_string(__LINE__)+" err");
	}

	inline void asm_op(toc&tc,ostream&os,const size_t indent_level,const char op,const string&dest,const string&dest_resolved,const statement&src)const{
		toc::indent(os,indent_level,true);tc.source_comment(os,dest,op,src);
		if(op=='='){
			if(src.is_expression()){
				src.compile(tc,os,indent_level,dest);
				return;
			}
			const ident_resolved&ir{tc.resolve_ident_to_nasm(src)};
			if(ir.is_const()){
				tc.asm_cmd(src,os,indent_level,"mov",dest_resolved,ir.as_const());
			}else{
				tc.asm_cmd(src,os,indent_level,"mov",dest_resolved,ir.id);
				if(ir.negated){
					tc.asm_neg(src,os,indent_level,dest_resolved);
				}
			}
			return;
		}
		if(op=='+'){// order1op
			if(src.is_expression()){
				const string&r{tc.alloc_scratch_register(src,os,indent_level)};
				src.compile(tc,os,indent_level,r);
				tc.asm_cmd(src,os,indent_level,"add",dest_resolved,r);
				tc.free_scratch_register(os,indent_level,r);
				return;
			}
			const ident_resolved&ir{tc.resolve_ident_to_nasm(src)};
			if(ir.is_const()){
				tc.asm_cmd(src,os,indent_level,"add",dest_resolved,ir.as_const());
			}else{
				if(ir.negated){
					tc.asm_cmd(src,os,indent_level,"sub",dest_resolved,ir.id);
				}else{
					tc.asm_cmd(src,os,indent_level,"add",dest_resolved,ir.id);
				}
			}
			return;
		}
		if(op=='-'){// order1op
			if(src.is_expression()){
				const string&r{tc.alloc_scratch_register(src,os,indent_level)};
				src.compile(tc,os,indent_level,r);
				tc.asm_cmd(src,os,indent_level,"sub",dest_resolved,r);
				tc.free_scratch_register(os,indent_level,r);
				return;
			}
			const ident_resolved&ir{tc.resolve_ident_to_nasm(src)};
			if(ir.is_const()){
				tc.asm_cmd(src,os,indent_level,"sub",dest_resolved,ir.as_const());
			}else{
				if(ir.negated){
					tc.asm_cmd(src,os,indent_level,"add",dest_resolved,ir.id);
				}else{
					tc.asm_cmd(src,os,indent_level,"sub",dest_resolved,ir.id);
				}
			}
			return;
		}
		if(op=='*'){// order2op
			if(src.is_expression()){
				const string&r{tc.alloc_scratch_register(src,os,indent_level)};
				src.compile(tc,os,indent_level,r);
				// imul destination must be a register
				if(tc.is_identifier_register(dest_resolved)){
					tc.asm_cmd(src,os,indent_level,"imul",dest_resolved,r);
				}else{
					// imul destination is not a register
					tc.asm_cmd(src,os,indent_level,"imul",r,dest_resolved);
					tc.asm_cmd(src,os,indent_level,"mov",dest_resolved,r);
				}
				tc.free_scratch_register(os,indent_level,r);
				return;
			}
			// not an expression, either a register or memory location
			// imul destination operand must be register
			const ident_resolved&ir=tc.resolve_ident_to_nasm(src);
			if(tc.is_identifier_register(dest_resolved)){
				if(ir.is_const()){
					tc.asm_cmd(src,os,indent_level,"imul",dest_resolved,ir.as_const());
				}else{
					tc.asm_cmd(src,os,indent_level,"imul",dest_resolved,ir.id);
					if(ir.negated){ // ? correct?
						tc.asm_neg(src,os,indent_level,dest_resolved);
					}
				}
				return;
			}
			// imul destination is not a register
			const string&r{tc.alloc_scratch_register(src,os,indent_level)};
			tc.asm_cmd(src,os,indent_level,"mov",r,dest_resolved);
			if(ir.is_const()){
				tc.asm_cmd(src,os,indent_level,"imul",r,ir.as_const());
			}else{
				tc.asm_cmd(src,os,indent_level,"imul",r,ir.id);
				if(ir.negated){ // ? correct?
					tc.asm_neg(src,os,indent_level,r);
				}
			}
			tc.asm_cmd(src,os,indent_level,"mov",dest_resolved,r);
			tc.free_scratch_register(os,indent_level,r);
			return;
		}
	}

	bool enclosed_{false}; //  =(a+b) vs =a+b
	bool in_args_{false}; // foo(a+b)
	char list_op_{0}; // +[...]
	vector<unique_ptr<statement>>exps_;
	vector<char>ops_;
	bool negated_{false};
	token after_negation_ws_token_;
};
